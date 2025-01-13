/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*-  */
/*
 * judger.cc
 * Copyright (C) 2022 Massimo Dong <ms@maxmute.com>
 *
 * wzoj-judger2 is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * wzoj-judger2 is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "judger.h"

#include <algorithm>

static std::string getFdContent(int fd, size_t length = 4096){
  std::string ret;
  char buffer[OJ_SMALL_BUFFER_SIZE + 1];
  lseek(fd, 0, SEEK_SET);
  while(ret.length() < length){
    ssize_t count = read(fd, buffer, std::min(length, sizeof(buffer) - 1));
    if(count == -1){
      LOG(FATAL)<<"Error reading from file";
    }else if(count == 0){
      break;
    }else{
      buffer[count] = '\0';
      ret += buffer;
    }
  }
  return ret;
}


Judger::Judger(const libconfig::Setting &setting):
  sandbox(std::make_unique<Sandbox>()),
  token(std::string(setting["token"])),
  address(std::string(setting["address"])),
  dataroot(std::string(setting["dataroot"])){

    std::lock_guard<std::mutex> lk(mutex);
    is_working = false;
}

Judger::~Judger(){
  LOG(INFO)<<"destroying judger";
}

bool Judger::start_working(){
    std::lock_guard<std::mutex> lk(mutex);
    if(is_working){
      return false;
    }else{
      is_working = true;
      return true;
    }
}

void Judger::finish_working(){
    std::lock_guard<std::mutex> lk(mutex);
    is_working = false;
}

static std::vector<fs::directory_entry> get_testcases(std::string path){
  std::vector<fs::directory_entry> ret;
  for(const auto& entry: fs::directory_iterator(path)){
    if(entry.path().extension() == "in") ret.push_back(entry);
  }
  std::sort(ret.begin(), ret.end());
  return ret;
}

void Judger::judge(const JudgeTask &task){
  task.set_status(STATUS_OK);
  if(!task.check_token(token)){
    return;
  }

  if(!start_working()){
    task.set_status(STATUS_BUSY);
    return;
  }
  sandbox->ready();
  
  int fd_ce = sandbox->open_ram_file();
  int exe_id = sandbox->compile(task.language(), task.code(), fd_ce);

  if(exe_id == -1){
    LOG(INFO)<<"ce";
    task.set_compileerror(getFdContent(fd_ce));
  }else{
    auto testcases = get_testcases(dataroot + "/" + task.datapath());
    //TODO: report compile success
    //TODO: read from config file the judging procedure
  
    int fd_out = sandbox->open_file("user.out");
    //TODO: something like
    // usage = sandbox->execute_program(id, mappings)
    // where `id` is returned from compile
    // and `mappings` is std::vector<std::pair<int, int>> where we redirect `first` to `second` using dup2(2) (here, 1 -> fd_out)
    // and `usage` contains time and memory usage of this program
  }
  
  dpause();
  
  sandbox->clean();
  finish_working();
}


void Judger::simple(const SimpleTask &task){
  task.set_status(STATUS_OK);
  if(!task.check_token(token)){
    return;
  }

  if(!start_working()){
    task.set_status(STATUS_BUSY);
    return;
  }

  sandbox->ready();
  dpause();

  int fd_ce = sandbox->open_ram_file();
  int exe_id = sandbox->compile(task.language(), task.code(), fd_ce);

  if(exe_id == -1){
    LOG(INFO)<<"ce";
    task.set_compileerror(getFdContent(fd_ce));
  }else{
    writeFile("user.in", task.input()); //TODO: this can be a ramfile too
    int fd_in = sandbox->open_file("user.in");
    int fd_out = sandbox->open_ram_file();
    int fd_err = sandbox->open_ram_file();
    dpause();
    std::vector<std::pair<int, int>> mappings;
    mappings.push_back(std::make_pair(0, fd_in));
    mappings.push_back(std::make_pair(1, fd_out));
    mappings.push_back(std::make_pair(2, fd_err));
    auto data = sandbox->execute_program(exe_id, mappings);
    LOG(INFO)<<"RE: "<<data.re();
    LOG(INFO)<<"time: "<<data.time_used;
    LOG(INFO)<<"memory: "<<data.memory_used;
    dpause();
  
    if(data.re()) task.set_runtimeerror(getFdContent(fd_err));
    task.set_timeused(data.time_used);
    task.set_memoryused(data.memory_used);
    task.set_output(getFdContent(fd_out));
  }

  sandbox->clean();
  finish_working();
}
