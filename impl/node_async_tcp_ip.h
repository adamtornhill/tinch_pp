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
#ifndef ASYNC_TCP_IP
#define ASYNC_TCP_IP

#include "types.h"
#include <boost/utility.hpp>
#include <boost/asio.hpp>
#include <boost/function.hpp>
#include <deque>
#include <utility>

namespace tinch_pp {

// Design decision: we make this class specific for the node_connections, although I believe 
// it could be more general. Let's see later...

namespace utils {
  class msg_lexer;
}

// The client registers callbacks to be invoked upon successfull completion of the 
// requested operation.
typedef boost::function<void (utils::msg_lexer&)> message_read_fn;
typedef boost::function<void ()> message_written_fn;

// All errors detected by this class are reported to a provided error handler.
// This allows the client to add some context before possibly throwing an exception.
typedef boost::function<void (const boost::system::error_code& /* reason */)> error_handler_fn;

class node_async_tcp_ip : boost::noncopyable
{
public:
  node_async_tcp_ip(boost::asio::ip::tcp::socket& connection,
		                  const error_handler_fn& error_handler);

  virtual void trigger_read(const message_read_fn& callback, utils::msg_lexer* received_msgs);

  virtual void trigger_write(const msg_seq& msg, const message_written_fn& callback);

private:
  void checked_read(const message_read_fn& callback,
		                  utils::msg_lexer* received_msgs,
		                  const boost::system::error_code& error,
		                  size_t bytes_transferred);

  void checked_write(const boost::system::error_code& error,
		                   size_t bytes_transferred);

private:
  boost::asio::ip::tcp::socket& connection;
  error_handler_fn error_handler;

  // Buffers used to handle reads and writes (the memory must be maintained during the whole 
  // asynchronous operation).
  msg_seq read_buffer;

  // Remembers the requested callback for each written message.
  typedef std::pair<msg_seq, message_written_fn> msg_and_callback;
  typedef std::deque<msg_and_callback> write_queue_type;
  write_queue_type write_queue;
};

}

#endif
