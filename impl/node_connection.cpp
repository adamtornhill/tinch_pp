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
#include "node_connection.h"
#include "utils.h"
#include "node_access.h"
#include "control_msg.h"
#include "node_connection_state.h"

#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <sstream>

// Design
// ======
// All I/O is done through asynchronous TCP/IP.
// We use OBJECTS FOR STATES to represent the state of the connection.
// The state objects are granted access to their connection through an access-inteface 
// provided upon construction. The states use their access to trigger read and write 
// operations. The node_connection performs the actual operations, ensuring that errors 
// are properly handled and, upon read-operations, that at least one complete message 
// has been received before the invoker (i.e. a state-object) is notified.

using namespace tinch_pp;
using namespace boost;
using  boost::asio::ip::tcp;

node_connection_ptr node_connection::create(asio::io_service& io_service, 
					    node_access& node,
					    const std::string& peer_node)
{
  node_connection_ptr connection(new node_connection(io_service, node, peer_node));
  connection->init();

  return connection;
}

node_connection_ptr node_connection::create(asio::io_service& io_service, 
					    node_access& node)
{
  node_connection_ptr connection(new node_connection(io_service, node));
  connection->init();

  return connection;
}

void node_connection::init()
{
  state = initial_state(shared_from_this());
}

node_connection::node_connection(asio::io_service& io_service, 
				                             node_access& a_node,
				                             const std::string& a_peer_node)
  : connection(io_service),
    node(a_node),
    async_tcp_ip(connection, bind(&node_connection::handle_io_error, this, ::_1)),
    peer_name(utils::node_name(a_peer_node)),
    node_name(node.name()),
    received_msgs(&handshake_msgs),
    own_challenge_(0)
{ 
}

node_connection::node_connection(asio::io_service& io_service,
				                             node_access& a_node)
  : connection(io_service),
    node(a_node),
    async_tcp_ip(connection, bind(&node_connection::handle_io_error, this, ::_1)),
    node_name(node.name()),
    received_msgs(&handshake_msgs),
    own_challenge_(0)
{
}

node_connection::~node_connection()
{
}

std::string node_connection::cookie() const
{
  return node.cookie();
}

// originator
void node_connection::start_handshake_as_A(const handshake_success_fn_type& handshake_success_fn,
                                           challenge_type own_challenge)
{
  handshake_success.connect(handshake_success_fn);
  own_challenge_ = own_challenge;
  state->initiate_handshake(node_name);
}

// other node is originator
void node_connection::start_handshake_as_B(const handshake_success_fn_type& handshake_success_fn,
                                           challenge_type own_challenge)
{
  handshake_success.connect(handshake_success_fn);
  own_challenge_ = own_challenge;
  state->read_incoming_handshake();
}

void node_connection::request(control_msg& distributed_operation)
{
  distributed_operation.execute(state);
}

void node_connection::handshake_complete()
{
  // Erlang uses a diferent message format once connected.
  received_msgs = &connected_msgs;

  handshake_success(true);
}

void node_connection::report_failure(const std::string& reason)
{
  state = initial_state(shared_from_this());

  handshake_success(false);
}

connection_state_ptr node_connection::change_state(connection_state_ptr new_state)
{
  state = new_state;

  return state;
}

void node_connection::trigger_checked_read(const message_read_fn& callback)
{
  async_tcp_ip.trigger_read(callback, received_msgs);
}

void node_connection::trigger_checked_write(const msg_seq& msg, 
					    const message_written_fn& callback)
{
  async_tcp_ip.trigger_write(msg, callback);
}

void node_connection::deliver_received(const msg_seq& msg, const e_pid& to)
{
  node.receive_incoming(msg, to);
}

void node_connection::deliver_received(const msg_seq& msg, const std::string& to)
{
  node.receive_incoming(msg, to);
}

void node_connection::request_link(const e_pid& from, const e_pid& to)
{
  node.incoming_link(from, to);
}

void node_connection::request_unlink(const e_pid& from, const e_pid& to)
{
  node.incoming_unlink(from, to);
}

void node_connection::request_exit(const e_pid& from, const e_pid& to, const std::string& reason)
{
  node.incoming_exit(from, to, reason);
}

void node_connection::request_exit2(const e_pid& from, const e_pid& to, const std::string& reason)
{
  node.incoming_exit2(from, to, reason);
}

void node_connection::handle_io_error(const boost::system::error_code& error)
{
  std::stringstream out;
  out << error;

  state->handle_io_error(out.str());
}

std::string node_connection::peer_node_name() const
{
  if(!peer_name)
    throw tinch_pp_exception("Unknown peer node for " + node_name);

  return *peer_name;
}
