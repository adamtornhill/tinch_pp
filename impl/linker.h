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
#ifndef LINKER_H
#define LINKER_H

#include "types.h"
#include "boost/utility.hpp"
#include <boost/thread/mutex.hpp>
#include <boost/function.hpp>
#include <utility>
#include <list>

namespace tinch_pp {

class mailbox_controller_type;

/// The linker maintains the bidirectional process links.
/// Any mailbox or remote Erlang process can request a link to another PID.
/// If the mailbox or process exits, an EXIT message is sent to the linked PID.
class linker : boost::noncopyable
{
public:
  linker(mailbox_controller_type& mailbox_controller);

  void link(const e_pid& from, const e_pid& to);

  void unlink(const e_pid& from, const e_pid& to);

  // Erlang differs between a controlled shutdown and a violent death.
  // This is an uncontrolled termination (distributed operation = EXIT).
  void break_links_for_local(const e_pid& dying_process);

  // Erlang differs between a controlled shutdown and a violent death.
  // This is an controlled shutdown, explicitly requested by the user (distributed operation = EXIT2).
  void close_links_for_local(const e_pid& dying_process, const std::string& reason);

private:
  void establish_link_between(const e_pid& pid1, const e_pid& pid2);

  void remove_link_between(const e_pid& pid1, const e_pid& pid2);

  typedef std::list<e_pid> linked_pids_type;

  linked_pids_type remove_links_from(const e_pid& dying_process);

  typedef boost::function<void (const e_pid& /* remote pid */)> notification_fn_type;

  void on_broken_links(const notification_fn_type& notification_fn, const e_pid& dying_process);

private:
  mailbox_controller_type& mailbox_controller;

  // The control messages (Link, Exit, etc) are received in the context of 
  // the I/O thread. We always lock on API level before delegating to worker functions.
  boost::mutex links_lock;

  typedef std::pair<e_pid, e_pid> link_type;
  typedef std::list<link_type> links_type;

  links_type established_links;
};

}

#endif
