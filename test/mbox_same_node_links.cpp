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
#include "tinch_pp/exceptions.h"
#include <iostream>
#include <stdexcept>

using namespace tinch_pp;
using namespace tinch_pp::erl;

// USAGE:
// ======
// 1. Simply run the program - no need to start Erlang (EPMD) for this test!
//
// This test links mailboxes located on the same tinch++ node.
// The feature is typically used in the case where tinch++ serves as
// an ordered, bidirectional queue between different threads in an application.

namespace {

  void break_link(node_ptr my_node);

  void break_in_other_direction(node_ptr my_node);

  void unlink_and_break(node_ptr my_node);

  void break_on_scope_exit(node_ptr my_node);

  void break_on_error(node_ptr my_node);
}

int main()
{
  node_ptr my_node = node::create("queue_test@127.0.0.1", "qwerty");

  break_link(my_node);

  break_in_other_direction(my_node);

  unlink_and_break(my_node);

  break_on_scope_exit(my_node);

  break_on_error(my_node);
}

namespace {

void expect_broken_link(mailbox_ptr mbox,
                        const std::string& testcase)
{
  const time_type_sec tmo = 2;

  try {
    (void) mbox->receive(tmo);
    std::cerr << "Failed to report a broken link (" + testcase + ")!" << std::endl;
  } catch(const link_broken&) {
    std::cout << "Success - broken link reported (" + testcase + ")." << std::endl;
  }
}

void break_link(node_ptr my_node)
{
  mailbox_ptr worker_mbox = my_node->create_mailbox("worker");
  mailbox_ptr control_mbox = my_node->create_mailbox("controller");

  worker_mbox->link(control_mbox->self());

  worker_mbox->close();

  expect_broken_link(control_mbox, "break_link");
}

void break_in_other_direction(node_ptr my_node)
{
  mailbox_ptr worker_mbox = my_node->create_mailbox("worker");
  mailbox_ptr control_mbox = my_node->create_mailbox("controller");

  worker_mbox->link(control_mbox->self());

  control_mbox->close();

  expect_broken_link(worker_mbox, "reversed direction");
}

void unlink_and_break(node_ptr my_node)
{
  mailbox_ptr worker_mbox = my_node->create_mailbox("worker");
  mailbox_ptr control_mbox = my_node->create_mailbox("controller");

  worker_mbox->link(control_mbox->self());

  control_mbox->unlink(worker_mbox->self());

  control_mbox->close();
  worker_mbox->close();

  std::cout << "Successfull unlink." << std::endl;
}

void break_on_scope_exit(node_ptr my_node)
{
  mailbox_ptr worker_mbox = my_node->create_mailbox("worker");

  { // introduce a new scope => the destructor of the mailbox will close it
    mailbox_ptr control_mbox = my_node->create_mailbox("controller");

    worker_mbox->link(control_mbox->self());
  }

  expect_broken_link(worker_mbox, "out-of-scope");
}

void break_on_error(node_ptr my_node)
{
  mailbox_ptr worker_mbox = my_node->create_mailbox("worker");

  try {
    // introduce a new scope and simulate an error that propages above the 
    // scope of the linked mailbox
    mailbox_ptr control_mbox = my_node->create_mailbox("controller");

    worker_mbox->link(control_mbox->self());

    throw std::runtime_error("on purpose");

  } catch(const std::runtime_error&) {}

  expect_broken_link(worker_mbox, "closed on exception");
}

}
