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
#include <stdexcept>

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
// The reply is first matched on erl::any, which is expected to 
// always succeed. Then, a submatch is performed on the value 
// assigned by the any-match.
namespace {

void match_any_atom(mailbox_ptr mbox);

void match_any_binary(mailbox_ptr mbox);

void match_any_bitstring(mailbox_ptr mbox);

void match_any_negative_int(mailbox_ptr mbox);

void match_any_small_int(mailbox_ptr mbox);

void match_any_medium_int(mailbox_ptr mbox);

void match_any_nested_tuples(mailbox_ptr mbox);

void match_any_list(mailbox_ptr mbox);

void match_any_small_list(mailbox_ptr mbox);

void match_any_empty_tuple(mailbox_ptr mbox);

void match_any_string(mailbox_ptr mbox);

void match_any_float(mailbox_ptr mbox);

void match_any_binary(mailbox_ptr mbox);

typedef boost::function<void (mailbox_ptr)> sender_fn_type;

}

int main()
{
  node_ptr my_node = node::create("net_adm_test_node@127.0.0.1", "abcdef");

  mailbox_ptr mbox = my_node->create_mailbox();

  const sender_fn_type senders[] = {bind(match_any_atom, ::_1),
                                    bind(match_any_binary, ::_1),
                                    bind(match_any_bitstring, ::_1),
                                    bind(match_any_negative_int, ::_1),
                                    bind(match_any_small_int, ::_1),
                                    bind(match_any_medium_int, ::_1),
                                    bind(match_any_nested_tuples, ::_1),
                                    bind(match_any_empty_tuple, ::_1),
                                    bind(match_any_list, ::_1),
                                    bind(match_any_small_list, ::_1),
                                    bind(match_any_string, ::_1),
                                    bind(match_any_float, ::_1),
                                    bind(match_any_binary, ::_1)};
  const size_t number_of_senders = sizeof senders / sizeof senders[0];

  for(size_t i = 0; i < number_of_senders; ++i)
    senders[i](mbox);
}

namespace {

const std::string remote_node_name("testnode@127.0.0.1");
const std::string to_name("reflect_msg");

matchable_ptr receive_any(mailbox_ptr mbox)
{
  const matchable_ptr reply = mbox->receive();

  matchable_ptr any_result;

  if(!reply->match(erl::any(&any_result)))
    throw std::runtime_error("Failed to match on 'any' - must succeed => internal error!");

  return any_result;
}

// All messages have the following format: {echo, self(), Msg}

void match_any_atom(mailbox_ptr mbox)
{
  mbox->send(to_name, remote_node_name, make_e_tuple(atom("echo"), pid(mbox->self()), atom("hello")));

  const matchable_ptr reply = receive_any(mbox);

  if(reply->match(atom("hello")))
    std::cout << "Matched any atom(hello)" << std::endl;
  else
    std::cerr << "match_any_atom: No match - unexpected message!" << std::endl;
}

void match_any_binary(mailbox_ptr mbox)
{
  const msg_seq byte_stream = list_of(1)(2)(3)(42);
  const binary_value_type data(byte_stream);
  
  mbox->send(to_name, remote_node_name, make_e_tuple(atom("echo"), pid(mbox->self()), binary(data)));

  const matchable_ptr reply = receive_any(mbox);

  if(reply->match(binary(data)))
    std::cout << "Matched any binary <<1,2,3,42>>" << std::endl;
  else
    std::cerr << "match_any_binary: No match - unexpected message!" << std::endl;
}

void match_any_bitstring(mailbox_ptr mbox)
{
  const msg_seq byte_stream = list_of(1)(2)(3)(32);
  const int padding_bits    = 5;
  const binary_value_type data(byte_stream, padding_bits);

  mbox->send(to_name, remote_node_name, make_e_tuple(atom("echo"), pid(mbox->self()), binary(data)));

  const matchable_ptr reply = receive_any(mbox);

  if(reply->match(binary(data)))
    std::cout << "Matched any binary bitstring <<1,2,3,32:5>>" << std::endl;
  else
    std::cerr << "match_any_bintstring: No match - unexpected message!" << std::endl;
}

void match_any_negative_int(mailbox_ptr mbox)
{
  mbox->send(to_name, remote_node_name, make_e_tuple(atom("echo"), pid(mbox->self()), int_(-1)));

  const matchable_ptr reply = receive_any(mbox);

  if(reply->match(int_(-1)))
    std::cout << "Matched any int_(-1)" << std::endl;
  else
    std::cerr << "match_any_negative_int: No match - unexpected message!" << std::endl;
}

void match_any_small_int(mailbox_ptr mbox)
{
  mbox->send(to_name, remote_node_name, make_e_tuple(atom("echo"), pid(mbox->self()), int_(2)));

  const matchable_ptr reply = receive_any(mbox);

  if(reply->match(int_(2)))
    std::cout << "Matched any int_(2)" << std::endl;
  else
    std::cerr << "match_any_small_int: No match - unexpected message!" << std::endl;
}

// NOTE: Erlang supports big ints too, but I haven't implemented them yet.
void match_any_medium_int(mailbox_ptr mbox)
{
  mbox->send(to_name, remote_node_name, make_e_tuple(atom("echo"), pid(mbox->self()), int_(10000)));

  const matchable_ptr reply = receive_any(mbox);

  if(reply->match(int_(10000)))
    std::cout << "Matched any int_(10000)" << std::endl;
  else
    std::cerr << "match_any_medium_int: No match - unexpected message!" << std::endl;
}

void match_any_nested_tuples(mailbox_ptr mbox)
{
  mbox->send(to_name, remote_node_name, make_e_tuple(atom("echo"), pid(mbox->self()), 
                                                     make_e_tuple(atom("next"), 
                                                     make_e_tuple(atom("nested"), int_(42)))));
  const matchable_ptr reply = receive_any(mbox);

  if(reply->match(make_e_tuple(atom("next"), make_e_tuple(atom("nested"), int_(42)))))
    std::cout << "Matched any {next, {nested, 42}}" << std::endl;
  else
    std::cerr << "match_any_nested_tuples: No match - unexpected message!" << std::endl;
}

void match_any_empty_tuple(mailbox_ptr mbox)
{
  mbox->send(to_name, remote_node_name, make_e_tuple(atom("echo"), pid(mbox->self()), make_e_tuple()));
  const matchable_ptr reply = receive_any(mbox);

  if(reply->match(make_e_tuple()))
    std::cout << "Matched any empty tuple {}" << std::endl;
  else
    std::cerr << "match_any_empty_tuple: No match - unexpected message!" << std::endl;
}

void match_any_string(mailbox_ptr mbox)
{
  const std::string value_to_match = "my string";

  mbox->send(to_name, remote_node_name, make_e_tuple(atom("echo"), pid(mbox->self()), 
                                                     make_e_tuple(atom("next"), e_string(value_to_match))));

  const matchable_ptr reply = receive_any(mbox);

  if(reply->match(make_e_tuple(atom("next"), e_string(value_to_match))))
    std::cout << "Matched any string {start, " << value_to_match << "}" << std::endl;
  else
    std::cerr << "match_any_string: No match - unexpected message!" << std::endl;
}

void match_any_float(mailbox_ptr mbox)
{
  const double value = 1234567.98765;
  mbox->send(to_name, remote_node_name, make_e_tuple(atom("echo"), pid(mbox->self()), 
                                                     make_e_tuple(atom("next"), float_(value))));

  const matchable_ptr reply = receive_any(mbox);

  if(reply->match(make_e_tuple(atom("next"), float_(value))))
    std::cout << "Matched float {start, " << value << "}" << std::endl;
  else
    std::cerr << "match_any_float: No match - unexpected message!" << std::endl;
}

void match_any_list(mailbox_ptr mbox)
{
  const std::list<erl::object_ptr> send_numbers = list_of(make_int(1))(make_int(2))(make_int(1000));

  mbox->send(to_name, remote_node_name, make_e_tuple(atom("echo"), pid(mbox->self()), 
                                                     make_e_tuple(atom("numbers"), make_list(send_numbers))));

  const matchable_ptr reply = receive_any(mbox);

  if(reply->match(make_e_tuple(atom("numbers"), make_list(send_numbers))))
    std::cout << "Matched any {numbers, List} with List size = " << send_numbers.size() << std::endl;
  else
    std::cerr << "match_any_list: No match - unexpected message!" << std::endl;
}

void match_any_small_list(mailbox_ptr mbox)
{
  const std::list<int_> send_numbers = list_of(int_(1))(int_(2))(int_(3));

  mbox->send(to_name, remote_node_name, make_e_tuple(atom("echo"), pid(mbox->self()), 
                                                     make_e_tuple(atom("small_list"), make_list(send_numbers))));

  const matchable_ptr reply = receive_any(mbox);

  if(reply->match(make_e_tuple(atom("small_list"), make_list(send_numbers))))
    std::cout << "Matched any {small_list, List} with List size = " << send_numbers.size() << std::endl;
  else
    std::cerr << "match_any_small_list: No match - unexpected message!" << std::endl;
}

}
