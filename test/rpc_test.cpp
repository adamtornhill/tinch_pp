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
#include "tinch_pp/rpc.h"
#include "tinch_pp/mailbox.h"
#include "tinch_pp/exceptions.h"
#include "tinch_pp/erlang_types.h"
#include <boost/bind.hpp>
#include <boost/assign/list_of.hpp>
#include <iostream>
#include <stdexcept>

using namespace tinch_pp;
using namespace tinch_pp::erl;
using namespace boost::assign;

// USAGE:
// ======
// 1. Start an Erlang node with the cookie abcdef.
// 2. Start the Erlang program reflect_msg:
//          (testnode@127.0.0.1)4> reflect_msg:start_link().
// 3. Start this program (will íssue different types of RPC, all based on sending one message and getting the same message back).

namespace {

const std::string remote_node_name("testnode@127.0.0.1");

void test_blocking(rpc& rpc_invoker);

void test_blocking_with_tmo(rpc& rpc_invoker);

void test_blocking_with_invalid_fn(rpc& rpc_invoker);

}

int main()
{
  const std::string own_node_name("net_adm_test_node@127.0.0.1");
  
  node_ptr my_node = node::create(own_node_name, "abcdef");

  rpc rpc_invoker(my_node);

  try {
    test_blocking(rpc_invoker);
    test_blocking_with_tmo(rpc_invoker);
    test_blocking_with_invalid_fn(rpc_invoker);

    std::cout << "RPC reply correct!" << std::endl;
  } catch(const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
  }
}

namespace {

const std::list<erl::object_ptr> msg_to_echo = list_of(make_atom("hello"));

const module_and_function_type remote_fn("reflect_msg", "echo");

void check_reply(const matchable_ptr reply,
                 const std::string& context)
{
  if(!reply->match(make_e_tuple(atom("ok"), atom("hello"))))
    throw std::runtime_error("RPC failed, context = " + context);
}

void test_blocking(rpc& rpc_invoker)
{
  matchable_ptr reply = rpc_invoker.blocking_rpc(remote_node_name, 
                                                 remote_fn, 
                                                 make_list(msg_to_echo));
  check_reply(reply, "Blocking RPC");
}

void test_blocking_with_tmo(rpc& rpc_invoker)
{
  const time_type_sec tmo = 42;

  matchable_ptr reply = rpc_invoker.blocking_rpc(remote_node_name, 
                                                 remote_fn, 
                                                 make_list(msg_to_echo),
                                                 tmo);
  check_reply(reply, "Blocking RPC with time out");
}

void test_blocking_with_invalid_fn(rpc& rpc_invoker)
{
  try {
    const time_type_sec tmo = 2;

    const module_and_function_type non_existent_remote_fn("reflect_msg", "qwerty");

    (void) rpc_invoker.blocking_rpc(remote_node_name, non_existent_remote_fn, make_list(msg_to_echo), tmo);

    throw std::runtime_error("RPC to non-existent function returned normally!");
  } catch(const tinch_pp_exception&) {}
}

}
