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
#ifndef LINK_POLICIES_H
#define LINK_POLICIES_H

#include "types.h"
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>

namespace tinch_pp {

// In case a link/unlink/exit is requested, we have to differentiate between 
// remote process (= located on another node) and mailboxes located on this node.
// We encapsulate those two cases in different dispatchers.
class link_operation_dispatcher_type
{
public:
  virtual ~link_operation_dispatcher_type();

  virtual void link(const e_pid& local_pid, const e_pid& remote_pid) = 0;

  virtual void unlink(const e_pid& local_pid, const e_pid& remote_pid) = 0;

  virtual void request_exit(const e_pid& from_pid, const e_pid& to_pid, const std::string& reason) = 0;

  virtual void request_exit2(const e_pid& from_pid, const e_pid& to_pid, const std::string& reason) = 0;
};

class node_access;
class linker;
class control_msg;

typedef boost::shared_ptr<link_operation_dispatcher_type> link_operation_dispatcher_type_ptr;

typedef boost::function<void (control_msg&, const std::string&)> request_fn;

link_operation_dispatcher_type_ptr make_remote_link_dispatcher(node_access& a_node,
                                                               linker& a_mailbox_linker,
                                                               const request_fn& a_requester);

link_operation_dispatcher_type_ptr make_local_link_dispatcher(node_access& a_node);

}

#endif
