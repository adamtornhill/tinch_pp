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
#include "tinch_pp/node.h"
#include "tinch_pp/mailbox.h"
#include "tinch_pp/erlang_types.h"
#include <iostream>

using namespace tinch_pp;
using namespace tinch_pp::erl;

// This program tests the case where a C++ node initates a link to a remote Erlang process.
//
// USAGE:
// ======
// 1. Start an Erlang node with the cookie abcdef.
// 2. Start the Erlang program link_tester:
//          (testnode@127.0.0.1)4> link_tester:start_link().
// 3. Start this program (will request the PID of the link tester).
//    As the link_tester communicates its PID, we link to it and ensure
//    that the link was successfully created. We then purposely break it.
// 4. Ensure that the link_tester reports a broken link (stdout in the Erlang shell).

namespace {

  void test_break_link(node_ptr my_node);
}

int main()
{
  const std::string own_node_name("link_test_node@127.0.0.1");

  node_ptr my_node = node::create(own_node_name, "abcdef");

  test_break_link(my_node);
}

namespace {

const std::string remote_node_name("testnode@127.0.0.1");

const time_type_sec tmo = 5;

void test_break_link(node_ptr my_node)
{
  mailbox_ptr mbox = my_node->create_mailbox();

  mbox->send("link_tester", remote_node_name, make_e_tuple(atom("request_pid"), pid(mbox->self())));

  tinch_pp::e_pid remote_pid;
  const matchable_ptr pid_respone = mbox->receive(tmo);

  if(!pid_respone->match(make_e_tuple(atom("link_pid"), pid(&remote_pid))))
    throw std::domain_error("Unexpected response from remote node - check the program versions.");

  mbox->link(remote_pid);

  mbox->close();

  std::cout << "Ensure the link_tester reported the broken link in the Erlang shell." << std::endl;
}

}

