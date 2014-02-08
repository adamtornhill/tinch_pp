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
#include "actual_node.h"
#include "utils.h"
#include "actual_mailbox.h"
#include "link_policies.h"
#include "tinch_pp/exceptions.h"
#include "control_msg_send.h"
#include "control_msg_reg_send.h"
#include "ScopeGuard.h"
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <iostream>
#include <cassert>

using namespace tinch_pp;
using namespace boost;

namespace {

using tinch_pp::e_pid;
typedef boost::lock_guard<boost::mutex> mutex_guard;

std::string valid_node_name(const std::string& user_provided)
{
  // TODO: verify the name, throw exception!
  // => move to utilities!
  return user_provided;
}

template<typename T, typename Key>
void remove_expired(const Key& key, T& mboxes)
{
  mboxes.erase(key);
}

std::string key_to_name(const std::string& name) { return name; }

std::string key_to_name(const e_pid& p)
{
  const std::string name = "<" + p.node_name + ":" + lexical_cast<std::string>(p.id) + ":" +
                           lexical_cast<std::string>(p.serial) + ":" + 
                           lexical_cast<std::string>(p.creation) + ">";
  return name;
}

template<typename Key, typename T>
shared_ptr<actual_mailbox> fetch_mailbox(const Key& name,
                                         T& registered_mboxes)
{
  typename T::iterator mbox = registered_mboxes.find(name);

  if(mbox == registered_mboxes.end())
    throw tinch_pp_exception("Failed to deliver message - mailbox not known. Name = " + key_to_name(name));

  shared_ptr<actual_mailbox> destination = mbox->second.lock();

  if(!destination) {
    remove_expired(name, registered_mboxes);
    throw tinch_pp_exception("Failed to deliver message - mailbox expired (check your lifetime management). Name = " + key_to_name(name));
  }

  return destination;
}

}

namespace tinch_pp {

  bool operator <(const e_pid& p1, const e_pid& p2);

}

actual_node::actual_node(const std::string& a_node_name, const std::string& a_cookie)
  : work(io_service),
    epmd(io_service, "127.0.0.1", 4369),
    node_name_(valid_node_name(a_node_name)),
    cookie_(a_cookie),
    connector(*this, io_service),
    async_io_runner(&actual_node::run_async_io, this),
    // variabled used to build pids:
    pid_id(1), serial(0), creation(0),
    mailbox_linker(*this),
    remote_link_dispatcher(make_remote_link_dispatcher(*this, mailbox_linker, bind(&actual_node::request, this, _1, _2))),
    local_link_dispatcher(make_local_link_dispatcher(*this))
{
}

actual_node::~actual_node()
{
  // Terminate all ongoing operations and kill the thread used to dispatch async I/O.
  // As an alternative, we could invoke work.reset() and allow all ongoing operations 
  // to complete => better?
  io_service.stop();
  async_io_runner.join();
}

void actual_node::publish_port(port_number_type incoming_connections_port)
{
  epmd.connect();
  creation = epmd.alive2_request(utils::node_name(node_name_), incoming_connections_port);
  
  connector.start_accept_incoming(incoming_connections_port);
}

bool actual_node::ping_peer(const std::string& peer_node_name)
{
  bool pong = false;

  try {
    pong = !!connector.get_connection_to(peer_node_name);
  } catch(const tinch_pp_exception&) {
    // don't let a failed connection atempt propage through this API.
  }

  return pong;
}

mailbox_ptr actual_node::create_mailbox()
{
  shared_ptr<actual_mailbox> mbox(new actual_mailbox(*this, make_pid(), io_service));

  const mutex_guard guard(mailboxes_lock);

  const bool added = mailboxes.insert(std::make_pair(mbox->self(), mbox)).second;
  assert(added);

  return mbox;
}

mailbox_ptr actual_node::create_mailbox(const std::string& registered_name)
{
  shared_ptr<actual_mailbox> mbox(new actual_mailbox(*this, make_pid(), io_service, registered_name));

  const mutex_guard guard(mailboxes_lock);

  mailboxes.insert(std::make_pair(mbox->self(), mbox));
  Loki::ScopeGuard insert_guard = Loki::MakeGuard(bind(&actual_node::remove, this, ::_1), mbox);

  registered_mailboxes.insert(std::make_pair(registered_name, mbox));

  insert_guard.Dismiss();

  return mbox;
}

void actual_node::close_mailbox(const e_pid& id, const std::string& name)
{
  const std::string reason = "normal";
  
  close_mailbox(id, name, reason);
}

// This function is invoked as a mailbox gets closed due to an exception.
// We must take extreme care not to fire another exception, which would terminate the program.
void actual_node::close_mailbox_async(const e_pid& id, const std::string& name)
{
  const std::string reason = "error";

  io_service.post(bind(&actual_node::close_mailbox, this, id, name, reason));
}

void actual_node::close_mailbox(const e_pid& id, const std::string& name, const std::string& reason)
{
  // Depending on if the mailbox is linked and, in that case, to whom, this 
  // action might result in a call to self => ensure the mutex isn't locked.
  mailbox_linker.close_links_for_local(id, reason);

  {
    const mutex_guard guard(mailboxes_lock);

    remove(id, name);
  }
}

void actual_node::link(const e_pid& local_pid, const e_pid& remote_pid)
{
  dispatcher_for(remote_pid)->link(local_pid, remote_pid);
}

void actual_node::unlink(const e_pid& local_pid, const e_pid& remote_pid)
{
  dispatcher_for(remote_pid)->unlink(local_pid, remote_pid);
}

std::vector<std::string> actual_node::connected_nodes() const
{
  return connector.connected_nodes();
}

void actual_node::remove(mailbox_ptr mailbox)
{
  remove(mailbox->self(), mailbox->name());
}

void actual_node::remove(const e_pid& id, const std::string& name)
{
   mailboxes.erase(id);
  // Not all mailboxes have an registered name, but erase is fine anyway (no throw).
  registered_mailboxes.erase(name);
}

void actual_node::deliver(const msg_seq& msg, const e_pid& to_pid)
{
  node_connection_ptr connection = connector.get_connection_to(to_pid.node_name);
  control_msg_send send_msg(msg, to_pid);

  connection->request(send_msg);
}

void actual_node::deliver(const msg_seq& msg, const std::string& to_name)
{
  this->receive_incoming(msg, to_name);
}

void actual_node::deliver(const msg_seq& msg, const std::string& to_name, 
		                 const std::string& given_node, const e_pid& from_pid)
{
  
  node_connection_ptr connection = connector.get_connection_to(given_node);
  control_msg_reg_send reg_send_msg(msg, to_name, from_pid);

  connection->request(reg_send_msg);
}

void actual_node::receive_incoming(const msg_seq& msg, const e_pid& to)
{
  const mutex_guard guard(mailboxes_lock);

  shared_ptr<actual_mailbox> destination = fetch_mailbox(to, mailboxes);
  destination->on_incoming(msg);
}

void actual_node::receive_incoming(const msg_seq& msg, const std::string& to)
{
  const mutex_guard guard(mailboxes_lock);

  shared_ptr<actual_mailbox> destination = fetch_mailbox(to, registered_mailboxes);
  destination->on_incoming(msg);
}

void actual_node::incoming_link(const e_pid& from, const e_pid& to)
{
  mailbox_linker.link(from, to);
}

void actual_node::incoming_unlink(const e_pid& from, const e_pid& to)
{
  mailbox_linker.unlink(from, to);
}

void actual_node::incoming_exit(const e_pid& from, const e_pid& to, const std::string& reason)
{
  const mutex_guard guard(mailboxes_lock);

  shared_ptr<actual_mailbox> linked_mailbox = fetch_mailbox(to, mailboxes);

  linked_mailbox->on_link_broken(reason, from);

  mailbox_linker.unlink(from, to);
}

void actual_node::incoming_exit2(const e_pid& from, const e_pid& to, const std::string& reason)
{
  // Erlang makes a difference between a termination (exit) and a controlled shutdown (exit2).
  // However, it doesn't really make a difference for us => treat them the same way.
  incoming_exit(from, to, reason);
}

void actual_node::request(control_msg& distributed_operation, const std::string& destination)
{
  node_connection_ptr connection = connector.get_connection_to(destination);

  connection->request(distributed_operation);
}

void actual_node::request_exit(const e_pid& from_pid, const e_pid& to_pid, const std::string& reason)
{
  dispatcher_for(to_pid)->request_exit(from_pid, to_pid, reason);
}

void actual_node::request_exit2(const e_pid& from_pid, const e_pid& to_pid, const std::string& reason)
{
  dispatcher_for(to_pid)->request_exit2(from_pid, to_pid, reason);
}

link_operation_dispatcher_type_ptr actual_node::dispatcher_for(const e_pid& destination)
{
  return destination.node_name != node_name_ ? remote_link_dispatcher : local_link_dispatcher;
}

void actual_node::run_async_io()
{
  for(;;) {
    // Error handling: this is our recovery point.
    // All async networking, and the corresponding handlers, execute in this context.
    try {
      io_service.run();
      break; // normal exit
    } catch(const connection_io_error& io_error) {
      std::cerr << "I/O error for " << io_error.node() << " error: "<< io_error.what() << std::endl;

      // TODO: consider linked mailboxes?
      connector.drop_connection_to(io_error.node());
    } catch(const tinch_pp_exception& e) {
      std::cerr << e.what() << std::endl; // TODO: cleaner...
    }
  }
}

tinch_pp::e_pid actual_node::make_pid()
{
  const e_pid new_pid(node_name_, pid_id, serial, creation);

  update_pid_fields();
  
  return new_pid;
}

void actual_node::update_pid_fields()
{
  // The PID semantics aren't particularly well-specified, but 
  // the algorithm is available in OtpLocalactual_node::createPid() in Jinterface.
  const boost::uint32_t max_pid_id = 0x7fff;
  const boost::uint32_t max_serial = 0x1fff; // 13 bits
  
  ++pid_id;

  if(pid_id > max_pid_id) {
    pid_id = 0;
    
    serial = (serial + 1) > max_serial ? 0 : serial + 1;
  }
}

namespace tinch_pp {

bool operator <(const e_pid& p1, const e_pid& p2)
{
  // Don't care about the name => it's always identical on the receiving node.
  return (p1.id < p2.id) || (p1.serial < p2.serial);
}

}

