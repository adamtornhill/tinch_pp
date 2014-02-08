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
#ifndef NODE_CONNECTION_STATE_H
#define NODE_CONNECTION_STATE_H

#include "types.h"
#include <boost/shared_ptr.hpp>

namespace tinch_pp {

class node_connection_access;
typedef boost::shared_ptr<node_connection_access> access_ptr;

class connection_state
{
public:
  connection_state(access_ptr a_access)
    : access(a_access)
  {
  }

  virtual ~connection_state() = 0;

  // TODO: make all these functions report an error (add a std::string name() method)!!!!!!!!!!!!!!!
  virtual void initiate_handshake(const std::string& node_name) {}

  virtual void read_incoming_handshake() {}

  virtual void send(const msg_seq& msg, const e_pid& destination_pid) {}

  virtual void send(const msg_seq& msg, const e_pid& self, const std::string& destination_name) {}

  // TODO: Take care with this one; exit is called by a dying process - do not throw an exception!
  virtual void exit(const e_pid& from_pid, const e_pid& to_pid, const std::string& reason) {}

  virtual void exit2(const e_pid& from_pid, const e_pid& to_pid, const std::string& reason) {}

  virtual void link(const e_pid& from_pid, const e_pid& to_pid) {}

  virtual void unlink(const e_pid& from_pid, const e_pid& to_pid) {}

  // The error is delegated to the current state because we want to react differently 
  // depending on if we have established a connection or not.
  virtual void handle_io_error(const std::string& error) const;

protected:
  access_ptr access;
};

typedef boost::shared_ptr<connection_state> connection_state_ptr;

connection_state_ptr initial_state(access_ptr access);

}

#endif
