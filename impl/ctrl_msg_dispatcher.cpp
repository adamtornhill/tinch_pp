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
#include "ctrl_msg_dispatcher.h"
#include "tinch_pp/exceptions.h"
#include "tinch_pp/erlang_types.h"
#include "constants.h"
#include "matchable_range.h"
#include "node_connection_access.h"
#include "utils.h"
#include <boost/lexical_cast.hpp>

using namespace tinch_pp;
using namespace tinch_pp::erl;

namespace tinch_pp {

void check_term_version(msg_seq_iter& first, const msg_seq_iter& last)
{
  const boost::uint8_t term_version = *first++;

  if(constants::magic_version != term_version) {
    
    const std::string reason = "Erroneous term version received. Got = " + 
                                 boost::lexical_cast<std::string>(term_version) +
                                 ", expected = " + boost::lexical_cast<std::string>(constants::magic_version);
    throw tinch_pp_exception(reason);
  }
}

void parse_header(msg_seq_iter& first, const msg_seq_iter& last)
{
  using namespace qi;

  if(!parse(first, last, omit[big_dword] >>  byte_(constants::pass_through)))
    throw tinch_pp_exception("Invalid header in ctrl-message: " + utils::to_printable_string(first, last));
  
  check_term_version(first, last);
}

// Each distributed Erlang operation has its own dispatcher.
// The dispatchers are implemented as a Chain Of Responsibility (design pattern, GoF).
class operation_dispatcher
{
public:
  operation_dispatcher(access_ptr a_access) 
    : access(a_access) {}
  
  operation_dispatcher(access_ptr a_access, dispatcher_ptr a_next)
    : access(a_access),
      next(a_next) {}

  virtual ~operation_dispatcher() {}

  void dispatch(msg_seq_iter& first, const msg_seq_iter& last) const
  {
    msg_seq_iter remember_first = first;

    if(!this->handle(first, last))
      try_next_in_chain(remember_first, last);
  }

  virtual bool handle(msg_seq_iter& first, const msg_seq_iter& last) const = 0;

private:
  void try_next_in_chain(msg_seq_iter& first, const msg_seq_iter& last) const
  {
    if(!next)
      throw tinch_pp_exception("Unsupported distributed operation. Action = operation ignored. Msg = " + 
                              utils::to_printable_string(first, last));

    next->dispatch(first, last);
  }

protected:
  access_ptr access;

private:
  dispatcher_ptr next;
};

// SEND operation:  tuple of {2, Cookie, ToPid} 
// This operation delivers the payload following the ctrl-msg to the ToPid.
class send_handler : public operation_dispatcher
{
public:
  send_handler(access_ptr access, dispatcher_ptr next) 
    : operation_dispatcher(access, next) {}

  virtual bool handle(msg_seq_iter& f, const msg_seq_iter& l) const
  {
    bool handled = false;
    e_pid to_pid;
     
    if(matchable_range(f, l).match(make_e_tuple(int_(constants::ctrl_msg_send), atom(any()), pid(&to_pid)))) {
      check_term_version(f, l);
      
      const msg_seq payload(f, l);
      access->deliver_received(payload, to_pid);
      handled = true;
    }

    return handled;
  }
};

// REG_SEND operation: tuple of {6, FromPid, Cookie, ToName}
// This operation delivers the payload following the ctrl-msg to a mailbox 
// matching the given ToName.
class reg_send_handler : public operation_dispatcher
{
public:
  reg_send_handler(access_ptr access) 
  : operation_dispatcher(access) {}

  reg_send_handler(access_ptr access, dispatcher_ptr next) 
    : operation_dispatcher(access, next) {}

  virtual bool handle(msg_seq_iter& f, const msg_seq_iter& l) const
  {
    bool handled = false;
    std::string to_name;
    e_pid from_pid; // what should I do with this one?
     
    if(matchable_range(f, l).match(make_e_tuple(int_(constants::ctrl_msg_reg_send), pid(&from_pid), atom(any()), atom(&to_name)))) {
      check_term_version(f, l);
      
      const msg_seq payload(f, l);
      access->deliver_received(payload, to_name);
      handled = true;
    }

    return handled;
  }
};

// LINK operation: tuple of {1, FromPid, ToPid}
// This operation sets-up a bidirectional link between the given processes.
// If the ToPid dies, it will trigger an exist-message sent to FromPid.
// If FromPid exits, we forward that information to the linked mailbox (see the EXIT operations below).
class link_handler : public operation_dispatcher
{
public:
  link_handler(access_ptr access) 
  : operation_dispatcher(access) {}

  link_handler(access_ptr access, dispatcher_ptr next) 
    : operation_dispatcher(access, next) {}

  virtual bool handle(msg_seq_iter& f, const msg_seq_iter& l) const
  {
    bool handled = false;
    e_pid from_pid;
    e_pid to_pid;
     
    if(matchable_range(f, l).match(make_e_tuple(int_(constants::ctrl_msg_link), pid(&from_pid), pid(&to_pid)))) {
      access->request_link(from_pid, to_pid);
      handled = true;
    }

    return handled;
  }
};

// UNLINK operation: tuple of {4, FromPid, ToPid}
// This operation removes a previous link between the given processes.
class unlink_handler : public operation_dispatcher
{
public:
  unlink_handler(access_ptr access) 
  : operation_dispatcher(access) {}

  unlink_handler(access_ptr access, dispatcher_ptr next) 
    : operation_dispatcher(access, next) {}

  virtual bool handle(msg_seq_iter& f, const msg_seq_iter& l) const
  {
    bool handled = false;
    e_pid from_pid;
    e_pid to_pid;
     
    if(matchable_range(f, l).match(make_e_tuple(int_(constants::ctrl_msg_unlink), pid(&from_pid), pid(&to_pid)))) {
      access->request_unlink(from_pid, to_pid);
      handled = true;
    }

    return handled;
  }
};

// EXIT operation: tuple of {3, FromPid, ToPid, Reason}
// Erlang differs between a controlled shutdown and a violent death.
// This is an uncontrolled shutdown.
class exit_handler : public operation_dispatcher
{
public:
  exit_handler(access_ptr access) 
  : operation_dispatcher(access) {}

  exit_handler(access_ptr access, dispatcher_ptr next) 
    : operation_dispatcher(access, next) {}

  virtual bool handle(msg_seq_iter& f, const msg_seq_iter& l) const
  {
    bool handled = false;
    e_pid from_pid;
    e_pid to_pid;
    std::string reason;
     
    if(matchable_range(f, l).match(make_e_tuple(int_(constants::ctrl_msg_exit), pid(&from_pid), pid(&to_pid), atom(&reason)))) {
      access->request_exit(from_pid, to_pid, reason);
      handled = true;
    }

    return handled;
  }
};


// EXIT2 operation: tuple of {8, FromPid, ToPid, Reason}
// Erlang differs between a controlled shutdown and a violent death.
// This is an controlled shutdown, explicitly requested by the user (distributed operation = EXIT2).
class exit2_handler : public operation_dispatcher
{
public:
  exit2_handler(access_ptr access) 
  : operation_dispatcher(access) {}

  exit2_handler(access_ptr access, dispatcher_ptr next) 
    : operation_dispatcher(access, next) {}

  virtual bool handle(msg_seq_iter& f, const msg_seq_iter& l) const
  {
    bool handled = false;
    e_pid from_pid;
    e_pid to_pid;
    std::string reason;
     
    if(matchable_range(f, l).match(make_e_tuple(int_(constants::ctrl_msg_exit2), pid(&from_pid), pid(&to_pid), atom(&reason)))) {
      access->request_exit2(from_pid, to_pid, reason);
      handled = true;
    }

    return handled;
  }
};

}


ctrl_msg_dispatcher::ctrl_msg_dispatcher(access_ptr access)
  : operation_handler(access),
    operations_chain(new send_handler(access,     dispatcher_ptr(
                     new reg_send_handler(access, dispatcher_ptr(
                     new link_handler(access,     dispatcher_ptr(
                     new unlink_handler(access,   dispatcher_ptr(
                     new exit_handler(access,     dispatcher_ptr(
                     new exit2_handler(access)))))))))))) // a tribute to Lisp
{
}

void ctrl_msg_dispatcher::dispatch(msg_seq& msg) const
{
  // On each successfully parsed element, the first-iterator is advanced.
  msg_seq_iter first = msg.begin();
  msg_seq_iter last = msg.end();

  parse_header(first, last);

  // Header OK => now check the ctrl-message.
  operations_chain->dispatch(first, last);
}

