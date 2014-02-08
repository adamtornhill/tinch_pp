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
#include "link_policies.h"
#include "node_access.h"
#include "linker.h"
#include "control_msg_exit.h"
#include "control_msg_exit2.h"
#include "control_msg_link.h"
#include "control_msg_unlink.h"

using namespace tinch_pp;

namespace {

using tinch_pp::e_pid;

struct link_operations_on_same_node : link_operation_dispatcher_type
{
  link_operations_on_same_node(node_access& a_node) : node(a_node) {}

  void link(const e_pid& local_pid, const e_pid& remote_pid)
  {
    node.incoming_link(local_pid, remote_pid);
  }

  void unlink(const e_pid& local_pid, const e_pid& remote_pid)
  {
    node.incoming_unlink(local_pid, remote_pid);
  }

  void request_exit(const e_pid& from_pid, const e_pid& to_pid, const std::string& reason)
  {
    node.incoming_exit(from_pid, to_pid, reason);
  }

  void request_exit2(const e_pid& from_pid, const e_pid& to_pid, const std::string& reason)
  {
    node.incoming_exit2(from_pid, to_pid, reason);
  }

private:
  node_access& node;
};

struct link_operations_on_remote_node : link_operation_dispatcher_type
{
  typedef boost::function<void (control_msg&, const std::string&)> request_fn;

  link_operations_on_remote_node(node_access& a_node,
                                 linker& a_mailbox_linker,
                                 const request_fn& a_requester) 
   : node(a_node),
     mailbox_linker(a_mailbox_linker),
     requester(a_requester)
  {}

  void link(const e_pid& local_pid, const e_pid& remote_pid)
  {
    control_msg_link link_msg(local_pid, remote_pid);
    requester(link_msg, remote_pid.node_name);

    mailbox_linker.link(local_pid, remote_pid);
  }

  void unlink(const e_pid& local_pid, const e_pid& remote_pid)
  {
    mailbox_linker.unlink(local_pid, remote_pid);

    control_msg_unlink unlink_msg(local_pid, remote_pid);
    requester(unlink_msg, remote_pid.node_name);
  }

  void request_exit(const e_pid& from_pid, const e_pid& to_pid, const std::string& reason)
  {
    control_msg_exit exit_msg(from_pid, to_pid, reason);

    requester(exit_msg, to_pid.node_name);
  }

  void request_exit2(const e_pid& from_pid, const e_pid& to_pid, const std::string& reason)
  {
    control_msg_exit2 exit2_msg(from_pid, to_pid, reason);

    requester(exit2_msg, to_pid.node_name);
  }

private:
  node_access& node;
  linker& mailbox_linker;
  request_fn requester;
};

}

namespace tinch_pp {

link_operation_dispatcher_type::~link_operation_dispatcher_type() {}

link_operation_dispatcher_type_ptr make_remote_link_dispatcher(node_access& node,
                                                               linker& mailbox_linker,
                                                               const request_fn& requester)
{
  link_operation_dispatcher_type_ptr remote_dispatcher(new link_operations_on_remote_node(node, mailbox_linker, requester));

  return remote_dispatcher;
}

link_operation_dispatcher_type_ptr make_local_link_dispatcher(node_access& node)
{
  link_operation_dispatcher_type_ptr local_dispatcher(new link_operations_on_same_node(node));

  return local_dispatcher;
}

}

