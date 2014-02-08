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
#include "node_async_tcp_ip.h"
#include "utils.h"
#include <boost/bind.hpp>
#include <cassert>

using namespace tinch_pp;
using namespace boost;

namespace {
  const size_t read_buffer_size = 128;

// Selectors for hiding the underlaying pair.
template<typename T>
const msg_seq& message(const T& t) { return t.first; }

template<typename T>
message_written_fn callback_fn(const T& t) { return t.second; }
}

node_async_tcp_ip::node_async_tcp_ip(asio::ip::tcp::socket& a_connection,
				     const error_handler_fn& a_error_handler)
  : connection(a_connection),
    error_handler(a_error_handler)
{
}

void node_async_tcp_ip::trigger_read(const message_read_fn& callback,
				     utils::msg_lexer* received_msgs)
{
  read_buffer.clear();
  read_buffer.resize(read_buffer_size);

  connection.async_read_some(asio::buffer(read_buffer),
			                          bind(&node_async_tcp_ip::checked_read, this, 
				                         callback,
				                         received_msgs,
				                         asio::placeholders::error,
				                         asio::placeholders::bytes_transferred));
}

void node_async_tcp_ip::trigger_write(const msg_seq& msg, const message_written_fn& callback)
{
   // Store the message in a write queue; we must ensure that it lives for the duration of the operation.
   const bool write_in_progress = !write_queue.empty();
   write_queue.push_back(msg_and_callback(msg, callback));
   
   if (!write_in_progress) { // otherwise we fire the next write once the first one has completed.
     asio::async_write(connection, asio::buffer(message(write_queue.front())), 
                       bind(&node_async_tcp_ip::checked_write, this, 
                       asio::placeholders::error,
                       asio::placeholders::bytes_transferred));
   }
}

void node_async_tcp_ip::checked_read(const message_read_fn& callback,
				     utils::msg_lexer* received_msgs,
				     const boost::system::error_code& error,
				     size_t bytes_transferred)
{
  if(error) {
    error_handler(error);
    return;
  }
     
  // Remove extraneous bytes...
  read_buffer.resize(bytes_transferred);

  // ...and check if we got at least one, complete message.
  if(received_msgs->add(read_buffer))
    callback(*received_msgs);
  else
    trigger_read(callback, received_msgs);
}

void node_async_tcp_ip::checked_write(const boost::system::error_code& error,
				                                  size_t bytes_transferred)
{
   assert(!write_queue.empty());

   if(error) {
      error_handler(error);
      return;
   }

   const message_written_fn callback = callback_fn(write_queue.front());

   write_queue.pop_front();

   if(!write_queue.empty()) {
      asio::async_write(connection, asio::buffer(message(write_queue.front())), 
                       bind(&node_async_tcp_ip::checked_write, this, 
                       asio::placeholders::error,
                       asio::placeholders::bytes_transferred));
   }

   callback();
}
