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
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/assign/list_of.hpp>
#include <iostream>

using namespace tinch_pp;
using namespace tinch_pp::erl;
using namespace boost;
using namespace boost::assign;

// USAGE:
// ======
// 1. Start an Erlang node with the cookie abcdef.
// 2. Start the Erlang program reflect_msg:
//          (testnode@127.0.0.1)4> reflect_msg:start_link().
// 3. Start this program. The program will send different messages 
// to reflect_msg, which echoes the messages back.

namespace {

void echo_atom(mailbox_ptr mbox);

void echo_binary(mailbox_ptr mbox);

void echo_bit_string(mailbox_ptr mbox);

void echo_nested_tuples(mailbox_ptr mbox, const std::string& name);

void echo_list(mailbox_ptr mbox);

void echo_empty_tuple(mailbox_ptr mbox);

void echo_string(mailbox_ptr mbox, const std::string& msg_name);

void echo_float(mailbox_ptr mbox, const std::string& msg_name);

// TODO: we can send such a list, but not match it...yet!
void echo_heterogenous_list(mailbox_ptr mbox);

typedef boost::function<void (mailbox_ptr)> sender_fn_type;

}

int main()
{
  node_ptr my_node = node::create("my_test_node@127.0.0.1", "abcdef");

  mailbox_ptr mbox = my_node->create_mailbox();

  const sender_fn_type senders[] = {bind(echo_atom, ::_1), bind(echo_atom, ::_1),
                                    bind(echo_binary, ::_1), bind(echo_binary, ::_1),
                                    bind(echo_bit_string, ::_1),
                                    bind(echo_nested_tuples, ::_1, "start"), bind(echo_nested_tuples, ::_1, "next"),
                                    bind(echo_empty_tuple, ::_1),
                                    bind(echo_list, ::_1), bind(echo_list, ::_1),
                                    bind(echo_string, ::_1, "start"), bind(echo_string, ::_1, "next"),
                                    bind(echo_float, ::_1, "start"), bind(echo_float, ::_1, "next")};
  const size_t number_of_senders = sizeof senders / sizeof senders[0];

  for(size_t i = 0; i < number_of_senders; ++i)
    senders[i](mbox);

  mbox->close();
}

namespace {

const std::string remote_node_name("testnode@127.0.0.1");
const std::string to_name("reflect_msg");

// All messages have the following format: {echo, self(), Msg}

void echo_atom(mailbox_ptr mbox)
{
  mbox->send(to_name, remote_node_name, make_e_tuple(atom("echo"), pid(mbox->self()), atom("hello")));

  const matchable_ptr reply = mbox->receive();

  std::string name;

  if(reply->match(atom("hello")))
    std::cout << "Matched atom(hello)" << std::endl;
  else if(reply->match(atom(&name)))
    std::cout << "Matched atom(" << name << ")" << std::endl;
  else
    std::cerr << "No match - unexpected message!" << std::endl;
}

void echo_binary(mailbox_ptr mbox)
{
  const msg_seq byte_stream = list_of(1)(2)(3)(42);
  const binary_value_type data(byte_stream);

  mbox->send(to_name, remote_node_name, make_e_tuple(atom("echo"), pid(mbox->self()), binary(data)));

  const matchable_ptr reply = mbox->receive();

  if(reply->match(binary(data)))
    std::cout << "Matched binary([1, 2, 3, 42])" << std::endl;
  else
    std::cerr << "No match for binary - unexpected message!" << std::endl;
}

void echo_bit_string(mailbox_ptr mbox)
{
  const msg_seq byte_stream = list_of(1)(2)(3)(0xF0);
  const int padding_bits    = 4;
  const binary_value_type data(byte_stream, padding_bits);

  mbox->send(to_name, remote_node_name, make_e_tuple(atom("echo"), pid(mbox->self()), binary(data)));

  const matchable_ptr reply = mbox->receive();

  if(reply->match(binary(data)))
    std::cout << "Matched binary with padding ([1, 2, 3, 0xF0:4] )" << std::endl;
  else
    std::cerr << "No match for binary bit-string - unexpected message!" << std::endl;
}

void echo_nested_tuples(mailbox_ptr mbox, const std::string& name)
{
  mbox->send(to_name, remote_node_name, make_e_tuple(atom("echo"), pid(mbox->self()), 
                                                     make_e_tuple(atom(name), 
                                                     make_e_tuple(atom("nested"), int_(42)))));
  const matchable_ptr reply = mbox->receive();

  if(reply->match(make_e_tuple(atom("start"), erl::any())))
    std::cout << "Matched {start, _}" << std::endl;
  else if(reply->match(make_e_tuple(atom("next"), make_e_tuple(atom("nested"), int_(42)))))
    std::cout << "Matched {next, {nested, 42}}" << std::endl;
  else
    std::cerr << "No match - unexpected message!" << std::endl;
}

void echo_empty_tuple(mailbox_ptr mbox)
{
  mbox->send(to_name, remote_node_name, make_e_tuple(atom("echo"), pid(mbox->self()), make_e_tuple()));
  const matchable_ptr reply = mbox->receive();

  if(reply->match(make_e_tuple()))
    std::cout << "Matched empty tuple {}" << std::endl;
  else
    std::cerr << "No match - unexpected message!" << std::endl;
}

void echo_string(mailbox_ptr mbox, const std::string& msg_name)
{
  mbox->send(to_name, remote_node_name, make_e_tuple(atom("echo"), pid(mbox->self()), 
                                                     make_e_tuple(atom(msg_name), e_string("my string"))));

  const matchable_ptr reply = mbox->receive();

  std::string matched_val;

  if(reply->match(make_e_tuple(atom("start"), erl::any())))
    std::cout << "Matched string {start, _}" << std::endl;
  else if(reply->match(make_e_tuple(atom("next"), e_string(&matched_val))))
    std::cout << "Matched string {start, " << matched_val << "}" << std::endl;
  else
    std::cerr << "No match - unexpected message!" << std::endl;
}

void echo_float(mailbox_ptr mbox, const std::string& msg_name)
{
  const double value = 1234567.98765;
  mbox->send(to_name, remote_node_name, make_e_tuple(atom("echo"), pid(mbox->self()), 
                                                     make_e_tuple(atom(msg_name), float_(value))));

  const matchable_ptr reply = mbox->receive();

  double matched_val;

  if(reply->match(make_e_tuple(atom("start"), erl::any())))
    std::cout << "Matched float {start, _}" << std::endl;
  else if(reply->match(make_e_tuple(atom("next"), float_(&matched_val))))
    std::cout << "Matched float {start, " << matched_val << "}" << std::endl;
  else
    std::cerr << "No match - unexpected message!" << std::endl;
}

void echo_list(mailbox_ptr mbox)
{
  const std::list<erl::object_ptr> send_numbers = list_of(make_int(1))(make_int(2))(make_int(1000));

  mbox->send(to_name, remote_node_name, make_e_tuple(atom("echo"), pid(mbox->self()), 
                                                     make_e_tuple(atom("numbers"), make_list(send_numbers))));

  const matchable_ptr reply = mbox->receive();

  std::list<int_> numbers;

  if(reply->match(make_e_tuple(atom("numbers"), make_list(&numbers))))
    std::cout << "Matched {numbers, List} with List size = " << numbers.size() << std::endl;
  else if(reply->match(make_e_tuple(atom("start"), erl::any())))
    std::cout << "Matched {start, _}" << std::endl;
  else
    std::cerr << "No match - unexpected message!" << std::endl;
}

}
