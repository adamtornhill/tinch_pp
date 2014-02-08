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
#ifndef ERL_STRING_H
#define ERL_STRING_H

#include "erl_object.h"
#include <boost/function.hpp>
#include <string>

namespace tinch_pp {
namespace erl {

class any;

// As an object is used in a match, it's either a value-match or a type-match.
// In the latter case, we want to assign the matched value. That difference 
// in behaviour is encapsulated by a match_fn.
typedef boost::function<bool (msg_seq_iter&, const msg_seq_iter&)> match_fn_type;

/// String does NOT have a corresponding Erlang representation, but is an 
/// optimization for sending lists of bytes (integer in the range 0-255) 
/// more efficiently over the distribution.
class e_string : public object
{
public:
  explicit e_string(const std::string& val);

  explicit e_string(std::string* to_assign);

  explicit e_string(const any& match_any);

  virtual void serialize(msg_seq_out_iter& out) const;

  virtual bool match(msg_seq_iter& f, const msg_seq_iter& l) const;

  std::string value() const { return val; }

private:
  std::string val;
  std::string* to_assign;

  match_fn_type match_fn;
};

}
}

#endif
