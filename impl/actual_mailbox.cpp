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
#include "actual_mailbox.h"
#include "tinch_pp/erl_object.h"
#include "node_access.h"
#include "matchable_seq.h"
#include "tinch_pp/exceptions.h"
#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <exception>

using namespace tinch_pp;
using namespace boost;

namespace {

  msg_seq serialized(const erl::object& message);

}

actual_mailbox::actual_mailbox(node_access& a_node,
			                            const e_pid& a_own_pid,
                               asio::io_service& a_service)
  : node(a_node),
    own_pid(a_own_pid),
    service(a_service),
    message_ready(false)
{
}

actual_mailbox::actual_mailbox(node_access& a_node,
			                            const e_pid& a_own_pid,
                               asio::io_service& a_service,
			                            const std::string& a_own_name)
  : node(a_node),
    own_pid(a_own_pid),
    service(a_service),
    own_name(a_own_name),
    message_ready(false)
{
}

actual_mailbox::~actual_mailbox()
{
  if(!std::uncaught_exception()) {
    // The mailbox went out of scope => simply remove it and report a closed link.
    try {
      node.close_mailbox(self(), own_name);
    } catch(...) {}// don't let any exceptions propagate from the destructor.
  } else {
    // This is the trickiest part => the mailbox is being destructed due to a yet 
    // uncaught exception. If we trigger another exception we'll terminate.
    // Thus, we post a request to remove the mailbox (and possibly report a broken 
    // link). The actual removal is done asynchronously. The solution isn't optimal 
    // (in theory, there's a riskt the copying of the handler might throw), but I 
    // couldn't find a better one. Any idea?
    node.close_mailbox_async(self(), own_name);
  }
}

void actual_mailbox::close()
{
  node.close_mailbox(self(), own_name);
}

void actual_mailbox::link(const e_pid& e_pido_link)
{
  node.link(self(), e_pido_link);
}

void actual_mailbox::unlink(const e_pid& e_pido_unlink)
{
  node.unlink(self(), e_pido_unlink);
}

tinch_pp::e_pid actual_mailbox::self() const
{
  return own_pid;
}

std::string actual_mailbox::name() const
{
  return own_name;
}

void actual_mailbox::send(const e_pid& to_pid, const erl::object& message)
{
  node.deliver(serialized(message), to_pid);
}

void actual_mailbox::send(const std::string& to_name, const erl::object& message)
{
  node.deliver(serialized(message), to_name);
}

void actual_mailbox::send(const std::string& to_name, const std::string& on_given_node, const erl::object& message)
{
  node.deliver(serialized(message), to_name, on_given_node, self());
}

matchable_ptr actual_mailbox::receive()
{
  unique_lock<mutex> lock(received_msgs_mutex);

  wait_for_at_least_one_message(lock);

  const matchable_ptr msg(new matchable_seq(pick_first_msg()));

  return msg;
}

matchable_ptr actual_mailbox::receive(time_type_sec tmo)
{
  asio::deadline_timer timer(service, posix_time::seconds(tmo));

  timer.async_wait(bind(&actual_mailbox::receive_tmo, this, asio::placeholders::error));
  
  matchable_ptr msg = receive();

  timer.cancel();

  return msg;
}

// Always invoked with the mutex locked.
void actual_mailbox::wait_for_at_least_one_message(unique_lock<mutex>& lock)
{
  if(!received_msgs.empty())
    return;

  while(!message_ready)
    message_received_cond.wait(lock);
}

msg_seq actual_mailbox::pick_first_msg()
{
  message_ready = false;

  if(!broken_links.empty())
    report_broken_link();

  if(received_msgs.empty())
    throw mailbox_receive_tmo();

  const msg_seq msg(received_msgs.front());

  received_msgs.pop_front();

  return msg;
}

void actual_mailbox::on_incoming(const msg_seq& msg)
{
  notify_receive(bind(&received_msgs_type::push_front, ref(received_msgs), cref(msg)));
}

void actual_mailbox::on_link_broken(const std::string& reason, const e_pid& pid)
{
  const broken_links_type::value_type info(reason, pid);

  notify_receive(bind(&broken_links_type::push_back, ref(broken_links), cref(info)));
}

// Used to abstract away the common pattern of lock-and-notify.
void actual_mailbox::notify_receive(const function<void ()>& receive_action)
{
  {
    lock_guard<mutex> lock(received_msgs_mutex);

    receive_action();

    message_ready = true;
  }
  message_received_cond.notify_one();
}

namespace { void no_op() {} }

void actual_mailbox::receive_tmo(const boost::system::error_code& error)
{
  const bool is_old_notification = error && (error == boost::asio::error::operation_aborted);

  if(is_old_notification)
    return;

  notify_receive(no_op);
}

void actual_mailbox::report_broken_link()
{
  // NOTE: called in the context of receive - mutex already locked.
  const broken_links_type::value_type info = broken_links.front();
  
  broken_links.pop_front();

  throw link_broken(info.first, info.second);
}

namespace {

msg_seq serialized(const erl::object& message)
{
  msg_seq s;
  msg_seq_out_iter out(s);

  message.serialize(out);

  return s;
}

}
