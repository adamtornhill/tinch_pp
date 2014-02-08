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
#ifndef NODE_CONNECTION_H
#define NODE_CONNECTION_H

#include "utils.h"
#include "node_connection_access.h"
#include "node_async_tcp_ip.h"
#include <boost/optional.hpp>
#include <boost/signal.hpp>
#include <boost/function.hpp>

namespace tinch_pp {

typedef boost::function<void (bool /*success*/)> handshake_success_fn_type;

class control_msg;
class node_access;
class node_connection;
typedef boost::shared_ptr<node_connection> node_connection_ptr;

class node_connection : boost::noncopyable, 
                        public node_connection_access
{
public:
  static node_connection_ptr create(boost::asio::io_service& io_service, 
				    node_access& node,
				    const std::string& peer_node);

  static node_connection_ptr create(boost::asio::io_service& io_service, 
				    node_access& node);

  ~node_connection();

  boost::asio::ip::tcp::socket& socket()
  {
    return connection;
  }

  virtual std::string peer_node_name() const;

  // See distribution_handshake.txt in the Erlang release.
  void start_handshake_as_A(const handshake_success_fn_type& handshake_success_fn, challenge_type own_challenge);
  void start_handshake_as_B(const handshake_success_fn_type& handshake_success_fn, challenge_type own_challenge);

  // A control_msg encodes a distributed operation sent to another node.
  // Examples of such operations are: link, unlink, send, etc.
  void request(control_msg& distributed_operation);

private:
  // Because we would need shared_from_this in the constructor, which isn't allowed,
  // we need a two-step creation procedure. create_node_connection below does that.

  // Used to initiate a connection to node_name.
  node_connection(boost::asio::io_service& io_service, 
		  node_access& node,
		  const std::string& peer_node);

  // Used as another node initates the connection.
  node_connection(boost::asio::io_service& io_service, node_access& node);

  void init();

private:
  // Implementation of the callback functions from the states.
  virtual std::string own_name() const { return node_name; }
  virtual void got_peer_name(const std::string& name) { peer_name = name; }
  virtual std::string cookie() const;

  virtual challenge_type own_challenge() const { return own_challenge_; }

  virtual void handshake_complete();
  virtual void report_failure(const std::string& reason);
  virtual connection_state_ptr change_state(connection_state_ptr new_state);

  virtual void trigger_checked_read(const message_read_fn& callback);

  virtual void trigger_checked_write(const msg_seq& msg, const message_written_fn& callback);

  virtual void deliver_received(const msg_seq& msg, const e_pid& to);

  virtual void deliver_received(const msg_seq& msg, const std::string& to);

  virtual void request_link(const e_pid& from, const e_pid& to);

  virtual void request_unlink(const e_pid& from, const e_pid& to);

  virtual void request_exit(const e_pid& from, const e_pid& to, const std::string& reason);

  virtual void request_exit2(const e_pid& from, const e_pid& to, const std::string& reason);

  void handle_io_error(const boost::system::error_code& error);
  
private:
  boost::asio::ip::tcp::socket connection;
  node_access& node;
  node_async_tcp_ip async_tcp_ip;

  // In case the other node is the originator, we don't now its name until later.
  boost::optional<std::string> peer_name;
  std::string node_name;

  // The state machine used to establish and maintain a connection.
  // Implemented using the design pattern OBJECTS FOR STATE.
  connection_state_ptr state;

  utils::msg_lexer_handshake handshake_msgs;
  utils::msg_lexer_connected connected_msgs;
  utils::msg_lexer* received_msgs;

  typedef boost::signal<void (bool /*success*/)> handshake_complete_signal_type;
  handshake_complete_signal_type handshake_success;

  challenge_type own_challenge_;
};

}

#endif
