// Copyright (c) 2010, Adam Petersen <adam@adampetersen.se>. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification, are
// permitted provided that the following conditions are met:
//
//   1. Redistributions of source code must retain the above copyright notice, this list of
//      conditions and the following disclaimer.
//
//   2. Redistributions in binary form must reproduce the above copyright notice, this list
//      of conditions and the following disclaimer in the documentation and/or other materials
//      provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY Adam Petersen ``AS IS'' AND ANY EXPRESS OR IMPLIED
// WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
// FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL Adam Petersen OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
// ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
// ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#include "control_msg_send.h"
#include "control_msg_reg_send.h"
#include "control_msg_exit.h"
#include "control_msg_exit2.h"
#include "control_msg_link.h"
#include "control_msg_unlink.h"
#include "node_connection_state.h"

using namespace tinch_pp;

control_msg_send::control_msg_send(const msg_seq& a_msg, 
                                   const e_pid& a_destination_pid)
  : msg(a_msg),
    destination_pid(a_destination_pid)
{
}

void control_msg_send::execute(connection_access_ptr connection)
{
  connection->send(msg, destination_pid);
}

control_msg_reg_send::control_msg_reg_send(const msg_seq& a_msg, 
                                           const std::string& a_to_name, 
                                           const e_pid& a_from_pid)
  : msg(a_msg),
    to_name(a_to_name),
    from_pid(a_from_pid)
{
}

void control_msg_reg_send::execute(connection_access_ptr connection)
{
  connection->send(msg, from_pid, to_name);
}

control_msg_exit::control_msg_exit(const e_pid& a_from_pid,
                                   const e_pid& a_to_pid,
                                   const std::string& a_reason)
  : from_pid(a_from_pid),
    to_pid(a_to_pid),
    reason(a_reason)
{
}

void control_msg_exit::execute(connection_access_ptr connection)
{
  connection->exit(from_pid, to_pid, reason);
}

control_msg_exit2::control_msg_exit2(const e_pid& a_from_pid,
                                     const e_pid& a_to_pid,
                                     const std::string& a_reason)
  : from_pid(a_from_pid),
    to_pid(a_to_pid),
    reason(a_reason)
{
}

void control_msg_exit2::execute(connection_access_ptr connection)
{
  connection->exit2(from_pid, to_pid, reason);
}

control_msg_link::control_msg_link(const e_pid& a_from_pid,
                                   const e_pid& a_to_pid)
  : from_pid(a_from_pid),
    to_pid(a_to_pid)
{
}

void control_msg_link::execute(connection_access_ptr connection)
{
  connection->link(from_pid, to_pid);
}

control_msg_unlink::control_msg_unlink(const e_pid& a_from_pid,
                                       const e_pid& a_to_pid)
  : from_pid(a_from_pid),
    to_pid(a_to_pid)
{
}

void control_msg_unlink::execute(connection_access_ptr connection)
{
  connection->unlink(from_pid, to_pid);
}
