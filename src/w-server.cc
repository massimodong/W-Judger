/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*-  */
/*
 * w-server.cc
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

#include "w-server.h"
#include "judge-task.h"

using namespace grpc;
using namespace WJudger;

class GrpcTask final: public JudgeTask{
	public:
	GrpcTask(const JudgeArgs * args, ServerWriter<JudgeReply> *writer):args(args), writer(writer){
	}

	int language() const override{
		return args->language();
	}

	std::string code() const override{
		return args->code();
	}

	private:
		const JudgeArgs *args;
		ServerWriter<JudgeReply> *writer;
};

class SimpleGrpcTask final: public SimpleTask{
	public:
	SimpleGrpcTask(const SimpleArgs *args, SimpleReply *reply):args(args), reply(reply){
	}
	int language() const override{
		return args->language();
	}
	std::string code() const override{
		return args->code();
	}
	std::string input() const override{
		return args->input();
	}

	void set_compileerror(std::string err) const override{
		reply->set_compileerror(true);
		reply->set_compileerrormessage(err);
	}

	void set_runtimeerror(std::string err) const override{
		reply->set_runtimeerror(true);
		reply->set_runtimeerrormessage(err);
	}

	void set_timeused(long long t) const override{
		reply->set_timeused(t);
	}

	void set_memoryused(long long m) const override{
		reply->set_memoryused(m);
	}

	void set_output(std::string o) const override{
		reply->set_output(o);
	}

	bool check_token(std::string t) const override{
		bool ret = t == args->token();
		if(!ret){
			reply->set_status(JudgeStatus::UNAUTHENTICATED);
		}
		return ret;
	}

	void set_status(uint32_t status) const override{
		switch(status){
			case STATUS_OK:
				reply->set_status(JudgeStatus::OK);
				break;
			case STATUS_BUSY:
				reply->set_status(JudgeStatus::BUSY);
				break;
			default:
				LOG(FATAL)<<"invalid status";
		}
	}

	private:
		const SimpleArgs *args;
		SimpleReply *reply;
};

class WJudgerImpl final : public WJudger::WJudger::Service {
	public:
	void setJudgers(std::unique_ptr<std::vector<Judger>> _judgers){
		judgers = std::move(_judgers);
	}

	private:
	std::unique_ptr<std::vector<Judger>> judgers;

	Status Judge(ServerContext *context, const JudgeArgs *args, ServerWriter<JudgeReply> *writer) override {
		safecall(unshare, CLONE_FS);

		/*
		for(Judger &judger: *judgers){
			if(judger.token_match("123456")){ //TODO: add token in args
				judger.judge(GrpcTask(args, writer));
				return Status::OK;
			}
		}*/
		const JudgeReply reply;//TODO: reply no match
		writer->Write(reply);
		return Status::OK;
	}

	Status Simple(ServerContext *context, const SimpleArgs *args, SimpleReply *reply) override{
		safecall(unshare, CLONE_FS);

		if(args->judgerid() < judgers->size()){
			Judger &judger = (*judgers)[args->judgerid()];
			judger.simple(SimpleGrpcTask(args, reply));
			return Status::OK;
		}else{
			return Status(StatusCode::NOT_FOUND, "judger not found");
		}
	}
};

void WServer::Run(std::unique_ptr<std::vector<Judger>> judgers){
	std::string address("[::]:9717");
	WJudgerImpl impl;
	impl.setJudgers(std::move(judgers));

	ServerBuilder builder;

	builder.AddListeningPort(address, InsecureServerCredentials());
	builder.RegisterService(&impl);

	std::unique_ptr<Server> server(builder.BuildAndStart());

	//server->Wait();
	oj_wait_shutdown();
}
