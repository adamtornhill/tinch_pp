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
//
// DESCRIPTION:
// ============
// This testcase sends different types of data to an echo-process.
// The reply is assigned to a variable of the expected type. This program 
// is an example of a type-safe assignment of Erlang data.
namespace {

void assign_atom(mailbox_ptr mbox);

void assign_binary(mailbox_ptr mbox);

void assign_bitstring(mailbox_ptr mbox);

void assign_nested_tuples(mailbox_ptr mbox);

void assign_list(mailbox_ptr mbox);

void assign_string(mailbox_ptr mbox);

void assign_float(mailbox_ptr mbox);

// TODO: we can send such a list, but not match it...yet!
void assign_heterogenous_list(mailbox_ptr mbox);

typedef boost::function<void (mailbox_ptr)> sender_fn_type;

}

int main()
{
  node_ptr my_node = node::create("patterns@127.0.0.1", "abcdef");

  mailbox_ptr mbox = my_node->create_mailbox();

  const sender_fn_type senders[] = {bind(assign_atom, ::_1),
                                    bind(assign_binary, ::_1),
                                    bind(assign_bitstring, ::_1),
                                    bind(assign_nested_tuples, ::_1),
                                    bind(assign_list, ::_1),
                                    bind(assign_string, ::_1),
                                    bind(assign_float, ::_1)};
  const size_t number_of_senders = sizeof senders / sizeof senders[0];

  for(size_t i = 0; i < number_of_senders; ++i)
    senders[i](mbox);
}

namespace {

const std::string remote_node_name("testnode@127.0.0.1");
const std::string to_name("reflect_msg");

// All messages have the following format: {echo, self(), Msg}

void assign_atom(mailbox_ptr mbox)
{
  mbox->send(to_name, remote_node_name, make_e_tuple(atom("echo"), pid(mbox->self()), atom("hello")));

  const matchable_ptr reply = mbox->receive();

  std::string name;

  if(reply->match(atom(&name)))
    std::cout << "Matched atom(" << name << ")" << std::endl;
  else
    std::cerr << "No match - unexpected message!" << std::endl;
}

void assign_binary(mailbox_ptr mbox)
{
  const msg_seq byte_stream = list_of(0)(0)(0)(42);
  const binary_value_type data(byte_stream);

  mbox->send(to_name, remote_node_name, make_e_tuple(atom("echo"), pid(mbox->self()), binary(data)));

  const matchable_ptr reply = mbox->receive();

  binary_value_type assigned_data;

  if(reply->match(binary(&assigned_data)) && (data == assigned_data))
    std::cout << "Assigned binary data." << std::endl;
  else
    std::cerr << "No match for binary data - unexpected message!" << std::endl;
}

void assign_bitstring(mailbox_ptr mbox)
{
  const msg_seq byte_stream = list_of(1)(2)(3)(0xFF);
  const int padding_bits    = 7;
  const binary_value_type data(byte_stream, padding_bits);

  mbox->send(to_name, remote_node_name, make_e_tuple(atom("echo"), pid(mbox->self()), binary(data)));

  const matchable_ptr reply = mbox->receive();

  binary_value_type assigned_data;

  if(reply->match(binary(&assigned_data)) && (data == assigned_data))
    std::cout << "Assigned binary bitstring." << std::endl;
  else
    std::cerr << "No match for binary bitstring - unexpected message!" << std::endl;
}

void assign_nested_tuples(mailbox_ptr mbox)
{
  mbox->send(to_name, remote_node_name, make_e_tuple(atom("echo"), pid(mbox->self()), 
                                                     make_e_tuple(atom("start"), 
                                                     make_e_tuple(atom("nested"), int_(42)))));
  const matchable_ptr reply = mbox->receive();

  std::string atom_val;
  boost::int32_t int_val = 0;

  if(reply->match(make_e_tuple(atom("start"), make_e_tuple(atom(&atom_val), int_(&int_val)))))
    std::cout << "Matched {start, {" << atom_val << ", " << int_val << "}}" << std::endl;
  else
    std::cerr << "No match - unexpected message!" << std::endl;
}


void assign_string(mailbox_ptr mbox)
{
  mbox->send(to_name, remote_node_name, make_e_tuple(atom("echo"), pid(mbox->self()), 
                                                     make_e_tuple(atom("start"), e_string("my string"))));

  const matchable_ptr reply = mbox->receive();

  std::string matched_val;

  if(reply->match(make_e_tuple(atom("start"), e_string(&matched_val))))
    std::cout << "Matched string {start, " << matched_val << "}" << std::endl;
  else
    std::cerr << "No match - unexpected message!" << std::endl;
}

void assign_float(mailbox_ptr mbox)
{
  const double value = 1234567.98765;
  mbox->send(to_name, remote_node_name, make_e_tuple(atom("echo"), pid(mbox->self()), 
                                                     make_e_tuple(atom("start"), float_(value))));

  const matchable_ptr reply = mbox->receive();

  double matched_val;

  if(reply->match(make_e_tuple(atom("start"), float_(&matched_val))))
    std::cout << "Matched float {start, " << matched_val << "}" << std::endl;
  else
    std::cerr << "No match - unexpected message!" << std::endl;
}

void assign_list(mailbox_ptr mbox)
{
  const std::list<erl::object_ptr> send_numbers = list_of(make_int(1))(make_int(2))(make_int(1000));

  mbox->send(to_name, remote_node_name, make_e_tuple(atom("echo"), pid(mbox->self()), 
                                                     make_e_tuple(atom("numbers"), make_list(send_numbers))));

  const matchable_ptr reply = mbox->receive();

  std::list<int_> numbers;

  if(reply->match(make_e_tuple(atom("numbers"), make_list(&numbers))))
    std::cout << "Matched {numbers, List} with List size = " << numbers.size() << std::endl;
  else
    std::cerr << "No match - unexpected message!" << std::endl;
}

}
