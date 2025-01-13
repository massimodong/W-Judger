/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*-  */
/*
 * sandbox.cc
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

#include "sandbox.h"
#include "jail.h"
#include <filesystem>
#include <sys/resource.h>
#include <sys/mman.h>

const static uint64_t COMPILE_TIME = 60;
const static uint64_t COMPILE_FILE_SIZE = 10 * STD_MB;
const static uint64_t COMPILE_MEMORY = 256 * STD_MB;

const static uint64_t SIMPLE_TIME = 1;
const static uint64_t SIMPLE_MEMORY = 1024 * STD_MB;

const static uint64_t RUN_FILE_SIZE = 2048 * STD_MB;

#define LIB_WJUDGER_SECCOMP_LOADER_PATH "/home/massimo/W/proj/wzoj-judger2/Debug/src/libwjudger_seccomp_loader.a"
#define WLAUNCHER_PATH "/home/massimo/W/proj/wzoj-judger2/Debug/src/wzoj_judger2_launcher"

#define BINDDIRS(ACTION)\
	ACTION("bin");\
	ACTION("usr");\
	ACTION("lib");\
	ACTION("lib64");

#define BIND(s)\
	safecall(mkdir, "compile/" s, S_IRWXU);\
	safecall(mount, "/" s, "compile/" s, "auto", MS_BIND, NULL);

#define UBIND(s)\
	safecall(umount, "compile/" s);\
	safecall(rmdir, "compile/" s);

Sandbox::Sandbox(int id):name(std::to_string(id)){
	LOG(INFO)<<"creating sandbox "<<name;
	safecall(mkdir, name.c_str(), S_IRWXU);
	safecall(chdir, name.c_str());
	safecall(mkdir, "compile", S_IRWXU);
	safecall(chown, "compile", JUDGER_UID, JUDGER_UID);
	safecall(mkdir, "run", S_IRWXU);
	safecall(chown, "run", JUDGER_UID, JUDGER_UID);
	BINDDIRS(BIND);
	std::filesystem::copy_file(LIB_WJUDGER_SECCOMP_LOADER_PATH, "compile/libwjudger_seccomp_loader.a");
	safecall(chdir, "..");
}

Sandbox::~Sandbox(){
	LOG(INFO)<<"removing sandbox "<<name;
	safecall(chdir, name.c_str());
	BINDDIRS(UBIND);
	safecall(unlink, "compile/libwjudger_seccomp_loader.a");
	safecall(rmdir, "compile");
	safecall(rmdir, "run");
	safecall(chdir, "..");
	safecall(rmdir, name.c_str());
}

static const char *LANGUAGE_FILE_NAMES[] = {
	"compile/Main.c",
	"compile/Main.cc",
	"pascal",
	"java",
	"python",
};

void Sandbox::ready(){
	safecall(chdir, name.c_str());
}

void Sandbox::clean(){
	for(auto exe_name: executable_files){
		safecall(unlink, ("run/" + exe_name).c_str());
	}
	executable_files.clear();

	for(auto file: normal_files){
		safecall(close, file.first);
		safecall(unlink, file.second.c_str());
	}
	normal_files.clear();

	for(auto fd: ram_files){
		safecall(close, fd);
	}
	ram_files.clear();

	safecall(chdir, "..");
}

int Sandbox::compile(int language, std::string code, int fd_ce){
	assert(language < 5);
	writeFile(LANGUAGE_FILE_NAMES[language], code);
	return raw_compile(language, fd_ce);
}

int Sandbox::open_file(const char *file_name){
	if(file_name == NULL){
		LOG(FATAL)<<"file name should be set now";
	}

	int fd = open(file_name, O_RDWR | O_CREAT, S_IRWXU);
	if(fd == -1){
		LOG(FATAL)<<"failed to open file "<<file_name<<strerror(errno);
	}

	normal_files.emplace_back(std::make_pair(fd, file_name));

	return fd;
}

int Sandbox::open_ram_file(){
	int fd = memfd_create("wjudger", 0);
	if(fd == -1){
		LOG(FATAL)<<"failed to create ram file :"<<strerror(errno);
	}
	ram_files.push_back(fd);
	return fd;
}

static void setlimits(uint64_t time, uint64_t memory, uint64_t file_size){
	struct rlimit LIM;
	int ret = 0;
	LIM.rlim_max = LIM.rlim_cur = time;
	ret |= setrlimit(RLIMIT_CPU, &LIM);
	
	LIM.rlim_max = LIM.rlim_cur = memory;
	ret |= setrlimit(RLIMIT_AS, &LIM);
	
	LIM.rlim_max = LIM.rlim_cur = file_size;
	ret |= setrlimit(RLIMIT_FSIZE, &LIM);

	LIM.rlim_max = LIM.rlim_cur = RLIM_INFINITY;
	ret |= setrlimit(RLIMIT_STACK, &LIM);

	if(ret){
		LOG(FATAL)<<"Failed setting limits";
	}

	//safecall(alarm, time); //TODO: enable alarm
}

[[ noreturn ]] static void executeCompile(int language, int fd_ce){
	const char * CP_C[] = { "gcc", "Main.c", "-o", "Main", "-O2", "-fno-asm", "-Wall",
			"-lm", "--static", "-std=c99", "-DONLINE_JUDGE", NULL };
	const char * CP_X[] = { "g++", "-Wl,--whole-archive", "libwjudger_seccomp_loader.a", "-Wl,--no-whole-archive", // Load seccomp by a static library
			"-Dconstructor=constructor_wjudger", "-D__constructor__=__constructor_wjudger__", // Disallow constructor attribute in source code
			"Main.cc", "-o", "Main", "-O2", "-fno-asm", "-Wall",
			"-lm", "--static", "-std=c++14", "-DONLINE_JUDGE", NULL };
	const char * CP_P[] =
			{ "fpc", "Main.pas","-Cs32000000","-Sh", "-O2", "-Co",
				"-Ct", "-Ci", NULL };
	const char * CP_PY[] = {"/bin/cp", "Main.py", "Main", NULL};
	char * const * CP[] = {(char * const *)CP_C,
		                   (char * const *)CP_X,
		                   (char * const *)CP_P,
	                       NULL,
	                       (char * const *)CP_PY};
	
	safecall(chdir, "./compile");
	if(language == OJ_LANGUAGE_PASCAL){
		dup2(fd_ce, 1);
	}else{
		dup2(fd_ce, 2);
	}
	setlimits(COMPILE_TIME, COMPILE_MEMORY, COMPILE_FILE_SIZE);
	jail();
	safecall(execvp, CP[language][0], CP[language]);
	LOG(FATAL)<<"should not reach here";
}

static pid_t fork_safe(){
	pid_t ret = fork();
	if(ret == -1){
		LOG(FATAL)<<"fork failed";
	}
	return ret;
}

int Sandbox::raw_compile(int language, int fd_ce){
	assert(language < 5);

	pid_t pid = fork_safe();
	if(pid == 0){
		executeCompile(language, fd_ce);
	}else{
		int status = 0;
		safecall(waitpid, pid, &status, 0);
		DLOG(INFO)<<"compile status: "<<status;
		safecall(unlink, LANGUAGE_FILE_NAMES[language]);
		if(status){
			return -1;
		}
	}

	int id = executable_files.size();
	auto exe_name = std::to_string(id);
	executable_files.emplace_back(exe_name);

	std::filesystem::copy_file("compile/Main", ("run/" + exe_name).c_str());
	safecall(unlink, "compile/Main");
	return id;
}



[[ noreturn ]] static void spawnedProcess(const char *exe, std::vector<std::pair<int, int>> mappings, int status_fd){
	char fd_str[30];
	const char *Main[] = { WLAUNCHER_PATH, exe, fd_str, NULL };

	int snret = snprintf(fd_str, sizeof(fd_str), "%d", status_fd);
	if(snret >= sizeof(fd_str)){
		LOG(FATAL)<<"error converting integer to string";
	}

	safecall(chdir, "./run");

	for(auto p: mappings){
		safecall(dup2, p.second, p.first);
		safecall(close, p.second);
	}

	setlimits(SIMPLE_TIME, SIMPLE_MEMORY, RUN_FILE_SIZE);
	//jail();
	//apply_seccomp();
	safecall(execvp, Main[0], (char * const *)Main);
	LOG(FATAL)<<"Should not reach here";
}

ExecuteData Sandbox::execute_program(int exe_id, std::vector<std::pair<int, int>> mappings){
	ExecuteData data;
	int status_pipefd[2];
	safecall(pipe, status_pipefd);

	pid_t pid = fork_safe();
	if(pid == 0){
		safecall(close, status_pipefd[0]);
		spawnedProcess(("./" + executable_files[exe_id]).c_str(), mappings, status_pipefd[1]);
		LOG(FATAL)<<"Should not reach here";
	}

	int status = 0;
	struct rusage usage;

	safecall(close, status_pipefd[1]);
	ssize_t read_ret = read(status_pipefd[0], &status, sizeof(status));
	if(read_ret != sizeof(status)){
		LOG(FATAL)<<"read execute status error!";
	}
	read_ret = read(status_pipefd[0], &usage, sizeof(usage));
	if(read_ret != sizeof(usage)){
		LOG(FATAL)<<"read execute usage error!";
	}
	safecall(close, status_pipefd[0]);

	int launcher_status = 0;
	safecall(waitpid, pid, &launcher_status, 0);
	//TODO: check launcher status

	data.ifexited = WIFEXITED(status);
	data.ifsignaled = WIFSIGNALED(status);
	if(data.ifexited) data.status = WEXITSTATUS(status);
	if(data.ifsignaled) data.signal = WTERMSIG(status);
	data.time_used = usage.ru_utime.tv_sec * (1000ll) + usage.ru_utime.tv_usec/1000;
	data.memory_used = usage.ru_maxrss;
	return data;
}