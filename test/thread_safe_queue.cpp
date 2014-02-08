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
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <iostream>

using namespace tinch_pp;
using namespace tinch_pp::erl;

// USAGE:
// ======
// 1. Simply run the program - no need to start Erlang (EPMD) for this test!
//
// This test uses the tinch++ feature of sending messages to mailboxes on the 
// same tinch++ node. Such a feature can be used as an ordered, bidirectional 
// queue between different threads in an application.

namespace {

void control_thread(mailbox_ptr mbox, size_t times);

void worker_thread(mailbox_ptr mbox);

}

int main()
{
  node_ptr my_node = node::create("queue_test@127.0.0.1", "qwerty");

  // The worker won't leave until all orders are done.
  const size_t number_of_orders = 10;

  mailbox_ptr worker_mbox = my_node->create_mailbox("worker");
  boost::thread worker(boost::bind(&worker_thread, worker_mbox));

  mailbox_ptr control_mbox = my_node->create_mailbox("controller");
  boost::thread controller(boost::bind(&control_thread, control_mbox, number_of_orders));
  
  controller.join();
  worker.join();
}

namespace {

void control_thread(mailbox_ptr mbox, size_t times)
{
  if(times > 0) {
    std::cout << "controller: requesting order " << times << std::endl;

    mbox->send("worker", atom("work"));

    const matchable_ptr result = mbox->receive();

    if(!result->match(atom("done")))
      throw std::runtime_error("control_thread: unexpected reply from worker");

    std::cout << "controller: worker finished order " << times << std::endl;

    control_thread(mbox, --times);
  } else {
    mbox->send("worker", atom("go home"));
    // terminating...
  }
}

void worker_thread(mailbox_ptr mbox)
{
  const bool long_hard_day = true;

  while(long_hard_day) {
    const matchable_ptr request = mbox->receive();

    if(request->match(atom("work")))
      mbox->send("controller", atom("done"));
    else if(request->match(atom("go home")))
      return;
    else
      std::cerr << "Me, the worker, do not understand the controller..." << std::endl;
  }
}

}
