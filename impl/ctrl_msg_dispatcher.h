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
#ifndef CTRL_MSG_DISPATCHER_H
#define CTRL_MSG_DISPATCHER_H

#include "types.h"
#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>

namespace tinch_pp {

class node_connection_access;
typedef boost::shared_ptr<node_connection_access> access_ptr;

class operation_dispatcher;
typedef boost::shared_ptr<operation_dispatcher> dispatcher_ptr;

// A control message is a tuple, sent between connected nodes, where the first element 
// indicates which distributed operation it encodes. Examples of operations include: send, registered send,
// exit, etc.
// This class parses the message and dispatches to the handle responsible for the encoded operation.
class ctrl_msg_dispatcher : boost::noncopyable
{
public:
  ctrl_msg_dispatcher(access_ptr operation_handler);

  void dispatch(msg_seq& msg) const;

private:
  access_ptr operation_handler;

  dispatcher_ptr operations_chain;
};

}

#endif
