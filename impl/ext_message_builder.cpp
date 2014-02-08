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
#include "ext_message_builder.h"
#include "tinch_pp/erlang_types.h"
#include "constants.h"
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <algorithm>

using namespace tinch_pp;
using namespace tinch_pp::erl;
using namespace boost;
using namespace std;

namespace {

  // Design: The different send methods are similiar, just the control message differs.
  // Thus, we parameterize with a generator (basically emulating closures).
  typedef function<void (msg_seq_out_iter&)> add_ctrl_msg_fn;
     
  void add_ctrl_message_send(msg_seq_out_iter& out, const tinch_pp::e_pid& destination_pid);

  void add_ctrl_message_reg_send(msg_seq_out_iter& out, const tinch_pp::e_pid& self, const string& destination_name);

  void add_ctrl_message_exit(msg_seq_out_iter& out, 
                               const int request, 
                               const tinch_pp::e_pid& from, 
                               const tinch_pp::e_pid& to,
                               const string& reason);

  void add_ctrl_message_linkage(msg_seq_out_iter& out, 
                               const int request,
                               const tinch_pp::e_pid& from, 
                               const tinch_pp::e_pid& to);

  msg_seq build(const add_ctrl_msg_fn& add_ctrl_msg);

  msg_seq build(const msg_seq& payload, const add_ctrl_msg_fn& add_ctrl_msg);
}

namespace tinch_pp {
 
msg_seq build_send_msg(const msg_seq& payload, const e_pid& destination_pid)
{
  return build(payload, bind(&add_ctrl_message_send, _1, cref(destination_pid)));
}

msg_seq build_reg_send_msg(const msg_seq& payload, const e_pid& self, const string& destination_name)
{
  return build(payload, bind(&add_ctrl_message_reg_send, _1, cref(self), cref(destination_name)));
}

msg_seq build_exit_msg(const e_pid& from_pid, const e_pid& to_pid, const std::string& reason)
{
  return build(bind(&add_ctrl_message_exit, _1, constants::ctrl_msg_exit, cref(from_pid), cref(to_pid), cref(reason)));
}

msg_seq build_exit2_msg(const e_pid& from_pid, const e_pid& to_pid, const std::string& reason)
{
  return build(bind(&add_ctrl_message_exit, _1, constants::ctrl_msg_exit2, cref(from_pid), cref(to_pid), cref(reason)));
}

msg_seq build_link_msg(const e_pid& from_pid, const e_pid& to_pid)
{
  return build(bind(&add_ctrl_message_linkage, _1, constants::ctrl_msg_link, cref(from_pid), cref(to_pid)));
}

msg_seq build_unlink_msg(const e_pid& from_pid, const e_pid& to_pid)
{
  return build(bind(&add_ctrl_message_linkage, _1, constants::ctrl_msg_unlink, cref(from_pid), cref(to_pid)));
}

}

namespace {

 void add_message_header(msg_seq_out_iter& out)
  {
    *out++ = constants::pass_through;
    *out++ = constants::magic_version;
  }

  // Used when sending to a PID.
  void add_ctrl_message_send(msg_seq_out_iter& out, const tinch_pp::e_pid& destination_pid)
  {
    add_message_header(out);

    const string no_cookie;

    // SEND: tuple of {2, Cookie, ToPid} 
    make_e_tuple(int_(constants::ctrl_msg_send), atom(no_cookie), pid(destination_pid)).serialize(out);
  }

  // Used when sending to a registered name on a node.
  void add_ctrl_message_reg_send(msg_seq_out_iter& out, const tinch_pp::e_pid& self, const string& destination_name)
  {
    add_message_header(out);

    const std::string no_cookie;

    // REG_SEND: tuple of {6, FromPid, Cookie, ToName} 
    make_e_tuple(int_(constants::ctrl_msg_reg_send), pid(self), atom(no_cookie), atom(destination_name)).serialize(out);
  }

  // Used when a linked mailbox dies.
  void add_ctrl_message_exit(msg_seq_out_iter& out, 
                             const int request, 
                             const tinch_pp::e_pid& from, 
                             const tinch_pp::e_pid& to,
                             const string& reason)
  {
    add_message_header(out);

    // EXIT and EXIT2: tuple of {request, FromPid, ToPid, Reason}
    make_e_tuple(int_(request), pid(from), pid(to), atom(reason)).serialize(out);
  }

  // Reused for both link and unlink messages (same layout).
  void add_ctrl_message_linkage(msg_seq_out_iter& out, 
                               const int request,
                               const tinch_pp::e_pid& from, 
                               const tinch_pp::e_pid& to)
  {
    add_message_header(out);

    // LINK and UNLINK: tuple of {request, FromPid, ToPid}
    make_e_tuple(int_(request), pid(from), pid(to)).serialize(out);
  }

  const size_t header_size = 4;

  void fill_in_message_size(msg_seq& msg)
  {
    const size_t size = msg.size() - header_size;

    msg_seq_iter size_iter = msg.begin();
    karma::generate(size_iter, karma::big_dword, size);
  }

  void append_payload(msg_seq_out_iter& out, const msg_seq& payload)
  {
    // Preprend the magic version to the term (as they say in that movie, there can only be one);
    // compound types don't include a version for each of their elements.
    *out++ = constants::magic_version;

    copy(payload.begin(), payload.end(), out);
  }

  msg_seq build(const add_ctrl_msg_fn& add_ctrl_msg)
  {
    msg_seq msg(header_size);
    msg_seq_out_iter out(msg);

    add_ctrl_msg(out);

    fill_in_message_size(msg);

    return msg;
  }

  msg_seq build(const msg_seq& payload, const add_ctrl_msg_fn& add_ctrl_msg)
  {
    msg_seq msg(header_size);
    msg_seq_out_iter out(msg);

    add_ctrl_msg(out);

    append_payload(out, payload);

    fill_in_message_size(msg);

    return msg;
  }

}
