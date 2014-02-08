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
#ifndef CONTROL_MSG_H
#define CONTROL_MSG_H

#include <boost/shared_ptr.hpp>

namespace tinch_pp {

class connection_state;
typedef boost::shared_ptr<connection_state> connection_access_ptr;

/// A control_msg encodes a distributed operation sent to another node.
/// The control_msg gets serialized on Erlangs external format as a tuple, 
/// where the first element indicates which distributed operation it encodes.
/// Examples on distributed operations are: link, unlink, send, etc.
///
/// This abstraction is an implementation of the design pattern COMMAND.
/// It allows us to add new operations in an iterative fashion without 
/// modifying the interface of the node_connection (the open-closed principle).
class control_msg
{
protected:
  inline ~control_msg() {}

public:
  virtual void execute(connection_access_ptr connection_access) = 0;
};

}

#endif
