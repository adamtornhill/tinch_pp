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
#include "linker.h"
#include "mailbox_controller_type.h"
#include <boost/thread/locks.hpp>
#include "boost/bind.hpp"
#include <algorithm>

using namespace tinch_pp;
using namespace boost;
using namespace std;

namespace {
typedef boost::lock_guard<boost::mutex> mutex_guard;
}

namespace tinch_pp {
  bool operator==(const pair<e_pid, e_pid>& v1,
                  const pair<e_pid, e_pid>& v2);
}

linker::linker(mailbox_controller_type& a_mailbox_controller)
  : mailbox_controller(a_mailbox_controller)
{
}

void linker::link(const e_pid& from, const e_pid& to)
{
  const mutex_guard guard(links_lock);

  remove_link_between(from, to);

  establish_link_between(from, to);
}

void linker::unlink(const e_pid& from, const e_pid& to)
{
  const mutex_guard guard(links_lock);

  remove_link_between(from, to);
}

void linker::break_links_for_local(const e_pid& dying_process)
{
  const string exit_reason = "error";
  const notification_fn_type exit_request = bind(&mailbox_controller_type::request_exit,
                                                  ref(mailbox_controller),
                                                  cref(dying_process),
                                                  _1,
                                                  cref(exit_reason));
  on_broken_links(exit_request, dying_process);
}

void linker::close_links_for_local(const e_pid& dying_process, const string& reason)
{
  const notification_fn_type exit2_request = bind(&mailbox_controller_type::request_exit2,
                                                   ref(mailbox_controller),
                                                   cref(dying_process),
                                                   _1,
                                                   cref(reason));
  on_broken_links(exit2_request, dying_process);
}

void linker::establish_link_between(const e_pid& pid1, const e_pid& pid2)
{
  established_links.push_back(std::make_pair(pid1, pid2));
}

void linker::remove_link_between(const e_pid& pid1, const e_pid& pid2)
{
  established_links.remove(make_pair(pid1, pid2));
  established_links.remove(make_pair(pid2, pid1));
}

void linker::on_broken_links(const linker::notification_fn_type& notification_fn, const e_pid& dying_process)
{
  // Take care => if the broken link is between mailboxes on the same node, 
  // we'll get back into this context (exactly these situations is why I want Erlang...).
  linked_pids_type closed_links;

  {
    const mutex_guard guard(links_lock);

    closed_links = remove_links_from(dying_process);
  }

  for_each(closed_links.begin(), closed_links.end(), notification_fn);
}

linker::linked_pids_type linker::remove_links_from(const e_pid& dying_process)
{
  linked_pids_type removed_links;
  links_type::iterator end = established_links.end();
  links_type::iterator i = established_links.begin();

  while(i != end) {
    if(dying_process == i->first) {
      removed_links.push_back(i->second);
      i = established_links.erase(i);
    } else if(dying_process == i->second) {
      removed_links.push_back(i->first);
      i = established_links.erase(i);
    } else {
      ++i;
    }
  }

  return removed_links;
}

namespace tinch_pp {

bool operator==(const std::pair<e_pid, e_pid>& v1,
                const std::pair<e_pid, e_pid>& v2)
{
  return (v1.first == v2.first) && (v1.second == v2.second);
}

}

