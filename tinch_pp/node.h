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
#ifndef NODE_H
#define NODE_H

#include "impl/types.h"

#include <boost/shared_ptr.hpp>
#include <map>

namespace tinch_pp {

class node;
typedef boost::shared_ptr<node> node_ptr;

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
class node 
{
public:
  // TODO: Implement => Creates a node using the default cookie (read from your user.home).
  //static node_ptr(const std::string& node_name);

  /// Creates a node with the given name and the given cookie.
  /// Typically, this is the first function called in a tinch++ application.
  static node_ptr create(const std::string& node_name, const std::string& cookie);

  virtual ~node();

  //node(const std::string& node_name, const std::string& cookie, int incoming_connections_port);

  /// If you want other nodes to connect to this one, it has to be registered at EPMD.
  /// This method does that.
  virtual void publish_port(port_number_type incoming_connections_port) = 0;

  /// Attempts to establish a connection to the given node.
  /// Note that connections are established implicitly as the first message to a node is sent.
  virtual bool ping_peer(const std::string& peer_node_name) = 0;

  /// Create an unnamed mailbox to communicate with other mailboxes and/or Erlang process.
  /// All messages are sent and received through mailboxes.
  virtual mailbox_ptr create_mailbox() = 0;

  /// Create an named mailbox to communicate with other mailboxes and/or Erlang process.
  /// Messages can be sent to this mailbox by using its registered name or its pid.
  virtual mailbox_ptr create_mailbox(const std::string& registered_name) = 0;

  /// Returns a vector with the names of all nodes connected to this one.
  virtual std::vector<std::string> connected_nodes() const = 0;
};

}

#endif
