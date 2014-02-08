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
#include "tinch_pp/exceptions.h"
#include "tinch_pp/erlang_types.h"
#include <iostream>
#include <stdexcept>

using namespace tinch_pp;
using namespace tinch_pp::erl;

// This program tests the case where a remote program links to our mailbox.
//
// USAGE:
// ======
// 1. Start an Erlang node with the cookie abcdef.
// 2. Start the Erlang program link_tester:
//          (testnode@127.0.0.1)4> link_tester:start_link().
// 3. Start this program (will communicate its PID to the link_tester).
//    Once linked, three automatic testcases are performed:
//    1) The local mailbox requests an unlink before closing.
//       Nothing should be reported on stdout in the Erlang shell.
//    2) We will break the established link. This should result in 
//       a report on stdout in the Erlang shell. The testcase continues 
//       by re-establishing a link from a new mailbox.
//    3) We break the established link by simulating an error condition. 
//       This should result in another report on stdout in the Erlang shell. 
//       The testcase continues by re-establishing a link from a new mailbox.
// 4. Invoke link_tester:stop(). This request will break the link, which 
//    shall be reported to this testprogram.
//
// Alternative 4: Instead of a controlled shutdown, simply kill the link_tester process.
//                We now expect a different reason for its exit.

namespace {

void local_unlinks(node_ptr my_node);

void local_breaks_link(node_ptr my_node);

void local_breaks_due_to_error(node_ptr my_node);

void remote_breaks_link(node_ptr my_node);

const std::string remote_node_name("testnode@127.0.0.1");
}

int main()
{
  const std::string own_node_name("link_test_node@127.0.0.1");

  node_ptr my_node = node::create(own_node_name, "abcdef");

  local_unlinks(my_node);

  local_breaks_link(my_node);

  local_breaks_due_to_error(my_node);

  remote_breaks_link(my_node);
}

namespace {

const time_type_sec tmo = 5;

void local_unlinks(node_ptr my_node)
{
  mailbox_ptr mbox = my_node->create_mailbox();

  mbox->send("link_tester", remote_node_name, make_e_tuple(atom("request_pid"), pid(mbox->self())));

  tinch_pp::e_pid remote_pid;
  const matchable_ptr pid_respone = mbox->receive(tmo);

  if(!pid_respone->match(make_e_tuple(atom("link_pid"), pid(&remote_pid))))
    throw std::domain_error("Unexpected response from remote node - check the program versions.");

  mbox->send("link_tester", remote_node_name, make_e_tuple(atom("pid"), pid(mbox->self())));

  const matchable_ptr m = mbox->receive(tmo);

  if(!m->match(atom("link_created")))
    throw std::domain_error("Unexpected response from remote node - check the program versions.");

  mbox->unlink(remote_pid);
  mbox->close();

  std::cout << "Mailbox unlinked and closed - ensure nothing was dumped on the Erlang shell." << std::endl;
  std::cout << "Press <enter> to continue..." << std::endl;

  std::string msg;
  std::getline(std::cin, msg);
}

void local_breaks_link(node_ptr my_node)
{
  mailbox_ptr mbox = my_node->create_mailbox();

  mbox->send("link_tester", remote_node_name, make_e_tuple(atom("pid"), pid(mbox->self())));

  const matchable_ptr m   = mbox->receive(tmo);

  if(!m->match(atom("link_created")))
    throw std::domain_error("Unexpected response from remote node - check the program versions.");

  mbox->close();

  std::cout << "Link broken -check stdout in the Erlang shell." << std::endl;
}

void local_breaks_due_to_error(node_ptr my_node)
{
  try {
    mailbox_ptr mbox = my_node->create_mailbox();

    mbox->send("link_tester", remote_node_name, make_e_tuple(atom("pid"), pid(mbox->self())));

    const matchable_ptr m   = mbox->receive(tmo);

    if(!m->match(atom("link_created")))
      throw std::domain_error("Unexpected response from remote node - check the program versions.");

    throw std::runtime_error("on purpose");
  } catch(const std::runtime_error&) {}

  std::cout << "Link broken (reason = error) -check stdout in the Erlang shell." << std::endl;
}

void remote_breaks_link(node_ptr my_node)
{
  mailbox_ptr mbox = my_node->create_mailbox();

  mbox->send("link_tester", remote_node_name, make_e_tuple(atom("pid"), pid(mbox->self())));

  const matchable_ptr m   = mbox->receive(tmo);

  if(!m->match(atom("link_created")))
    throw std::domain_error("Unexpected response from remote node - check the program versions.");

  std::cout << "Link created - waiting for the link to break..." << std::endl;

  try {
    const time_type_sec dont_hang_forever = 120;
    (void) mbox->receive(dont_hang_forever);
  } catch(const link_broken& report) {
    std::cout << "Expected result - link broken with reason = " << report.reason() << std::endl;
  }
}

}
