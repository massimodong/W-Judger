/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*-  */
/*
 * judge-task.h
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

#ifndef _JUDGE_TASK_H_
#define _JUDGE_TASK_H_

#include "common.h"

class JudgeTask
{
public:
	virtual int language() const = 0;
	virtual std::string code() const = 0;

protected:

private:

};

class SimpleTask{
	public:
		virtual int language() const = 0;
		virtual std::string code() const = 0;
		virtual std::string input() const = 0;

		virtual void set_compileerror(std::string) const = 0;
		virtual void set_runtimeerror(std::string) const = 0;
		virtual void set_timeused(long long) const = 0;
		virtual void set_memoryused(long long) const = 0;
		virtual void set_output(std::string) const = 0;
		virtual bool check_token(std::string) const = 0;
		virtual void set_status(uint32_t) const = 0;
	private:
};

#endif // _JUDGE_TASK_H_

