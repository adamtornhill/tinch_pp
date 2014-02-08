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
#include "node_connector.h"
#include "tinch_pp/exceptions.h"
#include "node_access.h"
#include "epmd_requestor.h"
#include <boost/bind.hpp>
#include <ctime>
#include <algorithm>

using namespace tinch_pp;
using namespace boost;
using boost::asio::ip::tcp;
using namespace std;

namespace {

node_connection_ptr request_node_connection(asio::io_service& io_service,
					    const std::string& peer_node,
					    node_access& node);
}

node_connector::node_connector(node_access& a_node,
			       asio::io_service& a_io_service)
  : node(a_node),
    io_service(a_io_service),
    // initialize our random challenge generator
    challenge_dist(0, 0xFFFFFF),
    generator(static_cast<unsigned int>(std::time(0))),
    challenge_generator(generator, challenge_dist)
{
}

void node_connector::start_accept_incoming(port_number_type port_no)
{
  incoming_connections_acceptor.reset(new asio::ip::tcp::acceptor(io_service, tcp::endpoint(tcp::v4(), port_no)));

  trigger_accept();
}

void node_connector::trigger_accept()
{
  node_connection_ptr new_connection = node_connection::create(io_service, node);

  incoming_connections_acceptor->async_accept(new_connection->socket(),
					      boost::bind(&node_connector::handle_accept, this, 
							  new_connection,
							  asio::placeholders::error));
}

node_connection_ptr node_connector::get_connection_to(const std::string& peer_node_name)
{
  unique_lock<mutex> lock(node_connections_mutex);

  node_connections_type::iterator existing = node_connections.find(peer_node_name);

  if(existing != node_connections.end())
    return existing->second;

  return make_new_connection(peer_node_name, lock);
}

void node_connector::drop_connection_to(const std::string& node_name)
{
  unique_lock<mutex> lock(node_connections_mutex);

  node_connections.erase(node_name);
}

vector<string> node_connector::connected_nodes() const
{
  unique_lock<mutex> lock(node_connections_mutex);

  vector<string> connected;

  transform(node_connections.begin(), node_connections.end(), back_inserter(connected),
	    bind(&node_connections_type::value_type::first, ::_1));

  return connected;
}

// Always invoked with the mutex locked.
node_connection_ptr node_connector::make_new_connection(const std::string& peer_node_name,
							unique_lock<mutex>& lock)
{
  node_connection_ptr new_connection = request_node_connection(io_service, peer_node_name, node);
  new_connection->start_handshake_as_A(bind(&node_connector::handshake_success, this, ::_1), 
                                       challenge_generator());

  const bool success = wait_for_handshake_result(lock);

  if(!success)
    throw tinch_pp_exception("Failed to connect to the node = " + peer_node_name);

  node_connections.insert(node_connections_type::value_type(peer_node_name, new_connection));

  return new_connection;
}

// Tricky - we must ensure that the node has finished its handshake procedure (asynchronous).
bool node_connector::wait_for_handshake_result(unique_lock<mutex>& lock)
{
  while(!handshake_done)
    handshake_cond.wait(lock);

  const bool result = *handshake_done;
  handshake_done = boost::none;

  return result;
}

// Callback (signal) from the connection.
void node_connector::handshake_success(bool handshake_result)
{
  {
    lock_guard<mutex> lock(node_connections_mutex);

    handshake_done = handshake_result;
  }
  handshake_cond.notify_one();
}

void node_connector::handshake_success_as_B(node_connection_ptr potentially_connected, bool succeeded)
{
  if(succeeded) {
    const std::string peer_node_name = potentially_connected->peer_node_name();

    node_connections.insert(node_connections_type::value_type(peer_node_name, potentially_connected));
  }

  // Ready for the next connection.
  trigger_accept();
}

void node_connector::handle_accept(node_connection_ptr new_connection,
				   const boost::system::error_code& error)
{
  if(!error) {
    // The new_connection signals once its handshake is done - then it's added to the container and
    // the next accept is triggered.
    new_connection->start_handshake_as_B(bind(&node_connector::handshake_success_as_B, this, new_connection, ::_1),
                                         challenge_generator());
  } else {
    trigger_accept();
  }
}

namespace {

node_connection_ptr request_node_connection(asio::io_service& io_service, 
					    const std::string& peer_node,
					    node_access& node)
{
  const std::string remote_host(utils::node_host(peer_node));
  const std::string peer_name(utils::node_name(peer_node));

  epmd_requestor epmd(io_service, remote_host, 4369);

  const port_number_type port = epmd.port_please2_request(peer_name);

  node_connection_ptr connection = node_connection::create(io_service, node, peer_node);
  utils::connect_socket(io_service, connection->socket(), remote_host, port);

  return connection;
}

}
