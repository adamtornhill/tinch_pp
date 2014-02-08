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
#ifndef NODE_ACCESS_H
#define NODE_ACCESS_H

#include "types.h"

namespace tinch_pp {

class mailbox;
typedef boost::shared_ptr<mailbox> mailbox_ptr;

class node_access
{
protected:
  ~node_access() {};
public:
  virtual std::string name() const = 0;
  
  virtual void close_mailbox(const e_pid& id, const std::string& name) = 0;

  virtual void close_mailbox_async(const e_pid& id, const std::string& name) = 0;

  virtual void link(const e_pid& local_pid, const e_pid& remote_pid) = 0;

  virtual void unlink(const e_pid& local_pid, const e_pid& remote_pid) = 0;

  virtual std::string cookie() const = 0;

  virtual void deliver(const msg_seq& msg, const e_pid& to) = 0;

  virtual void deliver(const msg_seq& msg, const std::string& to) = 0;

  virtual void deliver(const msg_seq& msg, 
		                     const std::string& to_name, 
		                     const std::string& on_given_node,
		                     const e_pid& from_pid) = 0;

  // TODO: Extract a separate interface for the Erlang operations.

  virtual void receive_incoming(const msg_seq& msg, const e_pid& to) = 0;

  virtual void receive_incoming(const msg_seq& msg, const std::string& to) = 0;

  virtual void incoming_link(const e_pid& from, const e_pid& to) = 0;

  virtual void incoming_unlink(const e_pid& from, const e_pid& to) = 0;

  virtual void incoming_exit(const e_pid& from, const e_pid& to, const std::string& reason) = 0;

  virtual void incoming_exit2(const e_pid& from, const e_pid& to, const std::string& reason) = 0;
};

}

#endif
