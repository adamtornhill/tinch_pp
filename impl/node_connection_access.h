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
#ifndef NODE_CONNECTION_ACCESS_H
#define NODE_CONNECTION_ACCESS_H

#include "types.h"
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/function.hpp>

namespace tinch_pp {

class connection_state;
typedef boost::shared_ptr<connection_state> connection_state_ptr;

class node_connection_access;
typedef boost::shared_ptr<node_connection_access> access_ptr;

// A callback funtion provided by the requester of an synchronous read.
// Invoked once a complete message has been read.
namespace utils { class msg_lexer; }

typedef boost::function<void (utils::msg_lexer&)> message_read_fn;
typedef  boost::function<void ()> message_written_fn;

typedef boost::uint32_t challenge_type;

// An interface exposed solely to the states for callbacks: a node_connection typically 
// implements this as a Private Interface.
class node_connection_access : public boost::enable_shared_from_this<node_connection_access>
{
 public:
  virtual ~node_connection_access() {}

  virtual std::string own_name() const = 0;
  virtual std::string peer_node_name() const = 0;
  virtual std::string cookie() const = 0;

  virtual challenge_type own_challenge() const = 0;
  
  // When connecting as B, the peer node isn't known until it 
  // has communicated its name.
  virtual void got_peer_name(const std::string& name) = 0;

  virtual void handshake_complete() = 0;

  virtual void report_failure(const std::string& reason) = 0;

  virtual void trigger_checked_read(const message_read_fn& callback) = 0;

  virtual void trigger_checked_write(const msg_seq& msg, const message_written_fn& callback) = 0;

  //
  // Interface for incoming message send operations:
  virtual void deliver_received(const msg_seq& msg, const e_pid& to) = 0;

  virtual void deliver_received(const msg_seq& msg, const std::string& to) = 0;

  //
  // Interface for interprocess links:
  virtual void request_link(const e_pid& from, const e_pid& to) = 0;

  virtual void request_unlink(const e_pid& from, const e_pid& to) = 0;

  virtual void request_exit(const e_pid& from, const e_pid& to, const std::string& reason) = 0;

  virtual void request_exit2(const e_pid& from, const e_pid& to, const std::string& reason) = 0;

  template<typename new_state>
  boost::shared_ptr<new_state> change_state_to() 
  { 
    connection_state_ptr next(new new_state(shared_from_this()));
    return boost::dynamic_pointer_cast<new_state>(this->change_state(next));
  }

  virtual connection_state_ptr change_state(connection_state_ptr new_state) = 0;
};

}

#endif
