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
#ifndef ACTUAL_NODE_H
#define ACTUAL_NODE_H

#include "tinch_pp/node.h"
#include "impl/node_connector.h"
#include "impl/epmd_requestor.h"
#include "impl/node_access.h"
#include "impl/linker.h"
#include "impl/mailbox_controller_type.h"

#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/utility.hpp>
#include <boost/weak_ptr.hpp>
#include <string>
#include <map>

namespace tinch_pp {

class mailbox;
typedef boost::shared_ptr<mailbox> mailbox_ptr;
class actual_mailbox;
class control_msg;
class link_operation_dispatcher_type;
typedef boost::shared_ptr<link_operation_dispatcher_type> link_operation_dispatcher_type_ptr;

/// A node represents one, distributed C++ node connected to the EPMD.
/// A node is responsible for establishing connections with other 
/// nodes on the Erlang infrastructure. There are two ways to estblish connections:
/// 1) The C++ node as originator (by sending a message or pinging), or 
/// 2) another, remote node initiates the connection handshake.
/// As a node registers itself at EPMD, it provides a port number. The node is 
/// responsible for listening to incoming connections at that port (case 2 above).
/// 
/// Networking strategy:
/// Calls to EPMD are rare. Thus, all communication with EPMD is synchronous.
/// The communication between connected nodes uses asynchronous TCP/IP.
///
/// Threading:
/// A separate thread is used for asynchronous I/O. Shared data is protected 
/// through mutexes.
class actual_node : public node,
                    node_access,
                    mailbox_controller_type,
                    boost::noncopyable 
{
public:
  // Creates a node using the default cookie (read from your user.home).
  //node(const std::string& node_name);

  actual_node(const std::string& node_name, const std::string& cookie);

  ~actual_node();

  // Implementation of the node interface:
  //

  /// If you want other nodes to connect to this one, it has to be registered at EPMD.
  /// This method does that.
  virtual void publish_port(port_number_type incoming_connections_port);

  /// Attempts to establish a connection to the given node.
  /// Note that connections are established implicitly as the first message to a node is sent.
  virtual bool ping_peer(const std::string& peer_node_name);

  /// Create an unnamed mailbox to communicate with other mailboxes and/or Erlang process.
  /// All messages are sent and received through mailboxes.
  virtual mailbox_ptr create_mailbox();

  /// Create an named mailbox to communicate with other mailboxes and/or Erlang process.
  /// Messages can be sent to this mailbox by using its registered name or its pid.
  virtual mailbox_ptr create_mailbox(const std::string& registered_name);

  /// Returns a vector with the names of all nodes connected to this one.
  virtual std::vector<std::string> connected_nodes() const;

private:
  // Take care - order of initialization matters (io_service always first).
  boost::asio::io_service io_service;
  // Prevent the io_service.run() from finishing (we don't want it to return 
  // before the node dies).
  boost::asio::io_service::work work;

  epmd_requestor epmd;

  std::string node_name_;
  std::string cookie_;
 
  node_connector connector;

   // A separate thread that runs the io_service in order to perform aynchronous requests...
  boost::thread async_io_runner;
  // ...and executes the following function:
  void run_async_io();

  // The Pids are built up from the node_name and the following variables:
  boost::uint32_t pid_id; // calculated
  boost::uint32_t serial; // calculated
  int creation;           // default 0 (zero), obtained from EPMD when registered.

  e_pid make_pid();
  void update_pid_fields();

  // Use a weak-pointer to break the cyclic ownership => now, the 
  // client owns the mailbox.
  typedef boost::weak_ptr<actual_mailbox> actual_mailbox_ptr;
  typedef std::map<e_pid, actual_mailbox_ptr> mailboxes_type;
  mailboxes_type mailboxes;

  typedef std::map<std::string, actual_mailbox_ptr> registered_mailboxes_type;
  registered_mailboxes_type registered_mailboxes;

  boost::mutex mailboxes_lock;

  linker mailbox_linker;

  link_operation_dispatcher_type_ptr remote_link_dispatcher;
  link_operation_dispatcher_type_ptr local_link_dispatcher;

  // Returns the link dispatcher (see comment above) for the given destination.
  link_operation_dispatcher_type_ptr dispatcher_for(const e_pid& destination);

  void remove(mailbox_ptr mailbox);
  void remove(const e_pid& id, const std::string& name);

  void request(control_msg& distributed_operation, const std::string& destination);

  void close_mailbox(const e_pid& id, const std::string& name, const std::string& reason);

private:
  // Implementation of node_access.
  //
  virtual std::string name() const { return node_name_; }
  
  virtual void close_mailbox(const e_pid& id, const std::string& name);

  virtual void close_mailbox_async(const e_pid& id, const std::string& name);

  virtual void link(const e_pid& local_pid, const e_pid& remote_pid);

  virtual void unlink(const e_pid& local_pid, const e_pid& remote_pid);

  virtual std::string cookie() const { return cookie_; }

  virtual void deliver(const msg_seq& msg, const e_pid& to);

  virtual void deliver(const msg_seq& msg, const std::string& to);

  virtual void deliver(const msg_seq& msg, const std::string& to_name, 
		                     const std::string& on_given_node, const e_pid& from_pid);

  virtual void receive_incoming(const msg_seq& msg, const e_pid& to);

  virtual void receive_incoming(const msg_seq& msg, const std::string& to);

  virtual void incoming_link(const e_pid& from, const e_pid& to);

  virtual void incoming_unlink(const e_pid& from, const e_pid& to);

  virtual void incoming_exit(const e_pid& from, const e_pid& to, const std::string& reason);

  virtual void incoming_exit2(const e_pid& from, const e_pid& to, const std::string& reason);

  // Implementation of mailbox_controller_type.
  //
  virtual void request_exit(const e_pid& from_pid, const e_pid& to_pid, const std::string& reason);

  virtual void request_exit2(const e_pid& from_pid, const e_pid& to_pid, const std::string& reason);
};

}

#endif
