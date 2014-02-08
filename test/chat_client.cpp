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
#include "tinch_pp/erlang_types.h"
#include <boost/thread.hpp>
#include <boost/assign/list_of.hpp>
#include <iostream>
#include <stdexcept>

using namespace tinch_pp;
using namespace tinch_pp::erl;
using namespace boost::assign;

// The chat client is intended as a simple test-application when testing tinch++ 
// in a distributed environment.
//
// USAGE:
// ======
// 1. Start the included Erlang chat_server with the cookie abcdef.
// 2. Start this program: chat_client <client-name> <own-node-name> <chat-server-node-name>
//    Example: chat_client Adam zarathustra.mydomain server@lambda.mydomain
// 3. Connect yet another client, either locally or on another machine.
// 4. Chat 'til you drop.
//
// You leave the chat by entering an empty line (RETURN).

namespace {

// Blocks until a chat message is published from another client.
// The chat messages are sent as {chat_msg, From, Msg}
void receive_published_msg(mailbox_ptr mbox)
{
  while(1) {
    matchable_ptr msg = mbox->receive();

    std::string publisher;
    std::string message;

    if(msg->match(make_e_tuple(atom("chat_msg"), e_string(&publisher), e_string(&message))))
      std::cout << publisher << " says: " << message << std::endl;
    else
      std::cerr << "Received something I couldn't interpret - a possible error!" << std::endl;
  }
}

class chat_client : boost::noncopyable
{
  typedef std::list<erl::object_ptr> rpc_arg_type;

public:
  chat_client(const std::string& client_name,
              const std::string& own_node_name,
              const std::string& chat_server_node)
              : own_node(node::create(client_name + "@" + own_node_name, "abcdef")), // hardcoded cookie
      rpc_invoker(own_node),
      mbox(own_node->create_mailbox()),
      chat_server(chat_server_node),
      msg_receiver(receive_published_msg, mbox)
  {
    enter_chat(client_name);
  }

  ~chat_client()
  {
    leave_chat();

    msg_receiver.detach();
    msg_receiver.join();
  }

  void publish(const std::string& msg)
  {
    const rpc_arg_type publication = list_of(make_string(msg))(make_pid(mbox->self()));

    do_rpc("publish", publication);
  }

private:

  void enter_chat(const std::string& client_name)
  {
    const rpc_arg_type registration = list_of(make_string(client_name))(make_pid(mbox->self()));

    matchable_ptr reply = do_rpc("register_client", registration);

    if(!reply->match(atom("ok")))
      throw std::runtime_error("Failed to register at the chat_server - is it running on node " + chat_server + " ?");
  }

  void leave_chat()
  {
    const rpc_arg_type unregistration = list_of(make_pid(mbox->self()));

    do_rpc("unregister_client", unregistration);
  }

  matchable_ptr do_rpc(const std::string& remote_fun, const rpc_arg_type& rpc_args)
  {
    return rpc_invoker.blocking_rpc(chat_server, 
                                    module_and_function_type("chat_server", remote_fun), 
                                    make_list(rpc_args));
  }

private:

  node_ptr own_node;
  rpc rpc_invoker;

  mailbox_ptr mbox;
  std::string chat_server;

  boost::thread msg_receiver;
};

std::string prompt_user(const std::string& user)
{
  std::cout << user << "> ";
  std::string msg;
  std::getline(std::cin, msg);

  return msg;
}

}

int main(int argc, char* argv[])
{
  if(argc < 4) {
    std::cerr << "Erroneous usage. Usage: chat_client <client-name> <own-node-name> <chat-server-node-name>" << std::endl;
    return -1;
  }

  const std::string client_name(argv[1]);
  const std::string own_node_name(argv[2]);
  const std::string server_node(argv[3]);

  chat_client client(client_name, own_node_name, server_node);

  bool chat_some_more = true;

  while(chat_some_more) {
    const std::string msg = prompt_user(client_name);

    if(chat_some_more = !msg.empty())
      client.publish(msg);
  }
}
