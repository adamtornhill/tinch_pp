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
#ifndef ACTUAL_MAILBOX_H
#define ACTUAL_MAILBOX_H

#include "tinch_pp/mailbox.h"
#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>
#include <boost/function.hpp>
#include <list>
#include <utility>

namespace tinch_pp {

class node_access;

class actual_mailbox : public mailbox
{
public:
  actual_mailbox(node_access& node, const e_pid& own_pid, boost::asio::io_service& service);

  actual_mailbox(node_access& node, const e_pid& own_pid, 
                 boost::asio::io_service& service, 
                 const std::string& own_name);

  ~actual_mailbox();

  // Implementation of the mailbox-interface:
  //
  virtual e_pid self() const;

  virtual std::string name() const;

  virtual void send(const e_pid& to, const erl::object& message);

  virtual void send(const std::string& to_name, const erl::object& message);

  virtual void send(const std::string& to_name, const std::string& node, const erl::object& message);

  // Blocks until a message is received in this mailbox.
  // Returns the received messages as a matchable allowing Erlang-style pattern matching.
  virtual matchable_ptr receive();

  // Blocks until a message is received in this mailbox or until the given time has passed.
  // Returns the received messages as a matchable allowing Erlang-style pattern matching.
  // In case of a time-out, an tinch_pp::mailbox_receive_tmo is thrown.
  virtual matchable_ptr receive(time_type_sec tmo);

  virtual void close();

  virtual void link(const e_pid& e_pido_link);

  virtual void unlink(const e_pid& e_pido_unlink);

  // The public interface for the implementation (i.e. the owning node):
  //
  void on_incoming(const msg_seq& msg);

  // Invoked as a linked (remote) process exits.
  void on_link_broken(const std::string& reason, const e_pid& pid);

private:
  // Used to abstract away the common pattern of lock-and-notify as a process 
  // sends some (message or exit) event to this mailbox.
  void notify_receive(const boost::function<void ()>& receive_action);

  void wait_for_at_least_one_message(boost::unique_lock<boost::mutex>& lock);

  msg_seq pick_first_msg();

  void receive_tmo(const boost::system::error_code& error);

  void report_broken_link();

private:
  node_access& node;
  e_pid own_pid;
  boost::asio::io_service& service;
  std::string own_name;

  typedef std::list<msg_seq> received_msgs_type;
  received_msgs_type received_msgs;

  // The client typically blocks in a receive until a message arrives.
  // The messages arrive (from a node_connection) in another thread context.
  boost::condition_variable message_received_cond;
  boost::mutex received_msgs_mutex;
  bool message_ready;

  // A mailbox may be linked to multiple Erlang processes and/or other mailboxes.
  // Thus, we need to handle the case where multiple links break. Each broken link 
  // is reported as an exception in the next receive.
  typedef std::list<std::pair<std::string, e_pid> > broken_links_type;
  broken_links_type broken_links;
};

}

#endif
