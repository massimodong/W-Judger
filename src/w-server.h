/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*-  */
/*
 * w-server.h
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

#ifndef _W_SERVER_H_
#define _W_SERVER_H_

#include "common.h"
#include "grpcpp/grpcpp.h"
#include "wjudger.pb.h"
#include "wjudger.grpc.pb.h"
#include "judger.h"

class WServer
{
public:
	void Run(std::unique_ptr<std::vector<Judger>>);

protected:

private:

};

#endif // _W_SERVER_H_

