/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*-  */
/*
 * judger.h
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

#ifndef _JUDGER_H_
#define _JUDGER_H_

#include <libconfig.h++>
#include "common.h"
#include "judge-task.h"
#include "sandbox.h"

#define STATUS_OK 0
#define STATUS_UNAUTHENTICATED 1
#define STATUS_BUSY 2

class Judger
{
public:
	Judger(std::string name, libconfig::Setting &);
	Judger(Judger &&) = default;
	Judger &operator=(Judger &&) = default;
	Judger (const Judger &) = delete;
	Judger &operator =(const Judger &) = delete;
	~Judger();

	void judge(const JudgeTask &);
	void simple(const SimpleTask &);
protected:

private:
	std::string name;
	std::string token;
	int sandbox_size;
	std::queue<std::unique_ptr<Sandbox>> sandboxes;

	std::unique_ptr<std::mutex> mutex;

	std::unique_ptr<Sandbox> fetch_sandbox();
	void return_sandbox(std::unique_ptr<Sandbox>);
};

#endif // _JUDGER_H_

