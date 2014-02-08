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
#ifndef NETWORKER_H
#define NETWORKER_H

#include "node_connection_access.h"
#include <boost/shared_ptr.hpp>
#include <boost/bind.hpp>

namespace tinch_pp {

template<typename State>
class networker
{
public:
  typedef networker<State> OwnType;
  typedef void (State::*ReadCallback)(utils::msg_lexer&);
  typedef void (State::*WriteCallback)();

  networker(access_ptr a_access, State* a_state)
    : access(a_access),
      state(a_state) {}

  void trigger_read(ReadCallback callback)
  {
    access->trigger_checked_read(boost::bind(callback, state, ::_1));
  }

  void trigger_write(const msg_seq& msg, WriteCallback callback)
  {
    access->trigger_checked_write(msg, boost::bind(callback, state));
  }

  void trigger_write(const msg_seq& msg)
  {
    access->trigger_checked_write(msg, boost::bind(&OwnType::ignore_write_notification, this));
  }

private:
  void ignore_write_notification()
  {
  }

private:
  access_ptr access;
  State* state;
};

}

#endif
