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
#ifndef MAILBOX_H
#define MAILBOX_H

#include "impl/types.h"
#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>
#include <string>

namespace tinch_pp {

namespace erl {
  class object;
}
class matchable;
typedef boost::shared_ptr<matchable> matchable_ptr;

struct e_pid;

/// A mailbox is the distributed equivalence to an Erlang process.
/// Each mailbox is associated with its own PID.
/// Messages from other nodes are received through the mailbox and,  
/// correspondingly, outgoing messages are sent from the mailbox.
///
/// Lifetime: a mailbox is only valid as long as its node, from 
/// which it is created, exists.
///
/// In the current version of tinch++, a mailbox is implicitly closed as 
/// it goes out of scope (future versions will include an explicit close-method).
class mailbox : boost::noncopyable
{
public:
  virtual ~mailbox() {}

  virtual e_pid self() const = 0;

  virtual std::string name() const = 0;

  /// Sends the message to a remote pid.
  virtual void send(const e_pid& to, const erl::object& message) = 0;

  /// Sends the message to the named mailbox created on the same tinch++ node as this mailbox.
  /// This functionality can be seen as a thread-safe queue for the application.
  virtual void send(const std::string& to_name, const erl::object& message) = 0;

  /// Sends the message to the process registered as to_name on the given node.
  virtual void send(const std::string& to_name, const std::string& node, const erl::object& message) = 0;

  /// Blocks until a message is received in this mailbox.
  /// Returns the received messages as a matchable allowing Erlang-style pattern matching.
  /// In case your mailbox is linked to an Erlang process or another mailbox, broken links 
  /// are reported in the receive operation through a tinch_pp::link_broken exception.
  virtual matchable_ptr receive() = 0;

  /// Blocks until a message is received in this mailbox or until the given time has passed.
  /// Returns the received messages as a matchable allowing Erlang-style pattern matching.
  /// In case of a time-out, an tinch_pp::receive_tmo_exception is thrown.
  /// In case your mailbox is linked to an Erlang process or another mailbox, broken links 
  /// are reported in the receive operation through a tinch_pp::link_broken exception.
  virtual matchable_ptr receive(time_type_sec tmo) = 0;

  /// Closes this mailbox. After this call completes, no more operations are 
  /// allowed on the mailbox.
  /// If there are links established to other mailboxes or Erlang processes, they will 
  /// be broken with exit reason 'normal'.
  virtual void close() = 0;

  /// Link to a remote mailbox or Erlang process. 
  /// If the remote process exits, a receive on this mailbox will throw a tinch_pp::link_broken exception. 
  /// Similarly, if this mailbox is closed, the linked process will receive an Erlang exit signal.
  virtual void link(const e_pid& e_pido_link) = 0;
  
  /// Remove a link to a remote mailbox or Erlang process.
  virtual void unlink(const e_pid& e_pido_unlink) = 0;
};

}

#endif
