/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*-  */
/*
 * main.cc
 * Copyright (C) 2021 Massimo Dong <ms@maxmute.com>
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

#include "common.h"
#include "w-server.h"
#include "judger.h"

#include <getopt.h>
#include <signal.h>
#include <libconfig.h++>

bool OJ_SHUTDOWN = false;
std::condition_variable SHUTDOWN_CV;
std::mutex SHUTDOWN_MUTEX;

void oj_wait_shutdown(){
	std::unique_lock<std::mutex> lk(SHUTDOWN_MUTEX);
	SHUTDOWN_CV.wait(lk, []{return OJ_SHUTDOWN;});
}

const char *OJ_CONFIG_DIR = "/etc/wjudger";
const char *OJ_HOME = "/home/judger";
int OJ_SLEEP_TIME = 1;
int OJ_CNT_WORKERS = 1;
const char *OJ_URL = "localhost:8080/";
const char *OJ_TOKEN = "123456";

static void print_help(const char *name){
	printf("Usage: %s [options]\n", name);
	printf("Options:\n");
	printf("-h, --help\t\tDisplay help message.\n");
	printf("-v, --version\t\tDisplay version message.\n");
	printf("-i, --once\t\tRun once: judge one solution and exit.\n");
}

static void print_version(const char *name){
}

void call_for_exit(int s){
	DLOG(WARNING)<<"Shutting down...";
	{
		std::lock_guard<std::mutex> lk(SHUTDOWN_MUTEX);
		OJ_SHUTDOWN = true;
	}
	SHUTDOWN_CV.notify_all();
}

static std::unique_ptr<std::vector<Judger>> readConfig(){
	std::unique_ptr<std::vector<Judger>> judgers = std::make_unique<std::vector<Judger>>();
	libconfig::Config cfg;
	cfg.setIncludeDir(OJ_CONFIG_DIR);
	cfg.readFile((OJ_CONFIG_DIR + std::string("/wjudger.cfg")).c_str());
	const libconfig::Setting& root = cfg.getRoot();

	if(!root["judgers"].isArray()){
		LOG(FATAL)<<"judgers should be an array!";
	}

	int judgers_length = root["judgers"].getLength();

	for(int i=0;i<judgers_length;++i){
		std::string name = std::string(root["judgers"][i]);
		judgers->emplace_back(name, root[name]);
	}
	return judgers;
}

int main(int argc, char* argv[])
{
	//google::InitGoogleLogging(argv[0]);
	bool run_once = false;

	static const struct option longopts[] = {
		{"help", no_argument, NULL, 'h'},
		{"version", no_argument, NULL, 'v'},
		{"once", no_argument, NULL, 'i'},
		{NULL, 0, NULL, 0},
	};
	int optc;
	while((optc = getopt_long(argc, argv, ":hvw:i", longopts, NULL)) != -1){
		switch(optc){
			case 'h':
				print_help(argv[0]);
				return 0;
			case 'v':
				print_version(argv[0]);
				return 0;
			case 'i':
				run_once = true;
				break;
			default:
				fprintf(stderr,"Try `%s --help` for more information.\n", argv[0]);
				return 0;
		}
	}

	struct sigaction new_action;
	new_action.sa_handler = call_for_exit;
	sigemptyset (&new_action.sa_mask);
	new_action.sa_flags = 0;
	safecall(sigaction, SIGQUIT, &new_action, NULL);
	safecall(sigaction, SIGTERM, &new_action, NULL);
	safecall(sigaction, SIGINT, &new_action, NULL);


	safecall(chdir, OJ_HOME);
	safecall(unshare, CLONE_FS);

	auto judgers = readConfig();

	//CpuSetManager::getInstance(); //initialize

	WServer wserver;
	wserver.Run(std::move(judgers));
	return 0;
}

