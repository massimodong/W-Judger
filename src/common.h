/*
 * common.h
 *
 * Copyright (C) 2021 - 
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _COMMON_H_
#define _COMMON_H_

#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/mount.h>

#include <glog/logging.h>
#include <queue>
#include <set>
#include <iostream>
#include <fstream>

#include <thread>
#include <mutex>
#include <condition_variable>

extern bool OJ_SHUTDOWN;
void oj_wait_shutdown();

extern int OJ_SLEEP_TIME;
extern int OJ_CNT_WORKERS;
extern const char *OJ_URL;
extern const char *OJ_TOKEN;

const uint64_t STD_MB = 1048576;
const int OJ_LANGUAGE_C = 0;
const int OJ_LANGUAGE_CPP = 1;
const int OJ_LANGUAGE_PASCAL = 2;
const int OJ_LANGUAGE_JAVA = 3;
const int OJ_LANGUAGE_PYTHON = 4;
const int OJ_PROBLEM_TYPE_INTERACT = 2;
const int OJ_PROBLEM_TYPE_SUBMIT_SOLUTION = 3;

const int OJ_SMALL_BUFFER_SIZE = 512;

#define IDENTITY_MACRO(x) x
#define MAKEARRAY(items) {items(IDENTITY_MACRO)}

#define safecall_err(err, fun, ...) if(fun(__VA_ARGS__) == err)\
  LOG(FATAL)<<#fun<<" "<<#__VA_ARGS__<<": "<<strerror(errno);

#define safecall(fun, ...) safecall_err(-1, fun, __VA_ARGS__)

static std::string readFile(const std::string &fileName)
{
    std::ifstream ifs(fileName.c_str(),
                      std::ios::in | std::ios::binary | std::ios::ate);

    std::ifstream::pos_type fileSize = ifs.tellg();
    ifs.seekg(0, std::ios::beg);

    std::vector<char> bytes(fileSize);
    ifs.read(&bytes[0], fileSize);

    return std::string(&bytes[0], fileSize);
}

static void writeFile(const std::string &fileName, const std::string &str)
{
    std::ofstream ofs(fileName.c_str(), std::ios::out);
	ofs<<str;
}

static void dpause(){
  return;
	int t;
	std::cout<<"pause"<<std::endl;
	std::cin>>t;
}

#endif // _COMMON_H_
