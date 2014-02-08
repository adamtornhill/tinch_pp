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
#include "tinch_pp/erl_string.h"
#include "tinch_pp/erl_any.h"
#include "string_matcher.h"
#include "ext_term_grammar.h"
#include "term_conversions.h"
#include <boost/bind.hpp>
#include <list>
#include <algorithm>
#include <cassert>

using namespace tinch_pp;
using namespace tinch_pp::erl;
using namespace boost;
using namespace std;

namespace {

typedef tinch_pp::detail::string_matcher matcher_s;

erl::int_ create_int(char val)
{
  const uint32_t int_val = val;
  return erl::int_(int_val);
}

bool match_string_value(msg_seq_iter& f, const msg_seq_iter& l, const string& val)
{
  // We're reusing the list implementation:
  std::list<int_> as_ints;
  std::transform(val.begin(), val.end(), back_inserter(as_ints), create_int);

  return matcher_s::match(as_ints, f, l);
}

char shrink_int(const erl::int_& val)
{
  return static_cast<char>(val.value());
}

bool assign_matched_string(msg_seq_iter& f, const msg_seq_iter& l, string* to_assign)
{
  assert(to_assign != 0);
  
  std::list<int_> as_ints;
  const bool res = matcher_s::assign_match(&as_ints, f, l);

  for_each(as_ints.begin(), as_ints.end(), bind(&string::push_back, to_assign,
                                                bind(shrink_int, ::_1)));

  return res;
}

bool match_any_string(msg_seq_iter& f, const msg_seq_iter& l, const any& match_any)
{
  std::string ignore;
  msg_seq_iter start = f;

  return assign_matched_string(f, l, &ignore) ? match_any.save_matched_bytes(msg_seq(start, f)) : false;
}

}

e_string::e_string(const std::string& a_val)
  : val(a_val),
    to_assign(0),
    match_fn(bind(match_string_value, ::_1, ::_2, cref(val))) {}

e_string::e_string(std::string* a_to_assign)
 : to_assign(a_to_assign),
   match_fn(bind(assign_matched_string, ::_1, ::_2, to_assign)) {}

e_string::e_string(const any& match_any)
  : to_assign(0),
    match_fn(bind(match_any_string, ::_1, ::_2, cref(match_any))) {}


void e_string::serialize(msg_seq_out_iter& out) const
{
  const serializable_string s(val);
  term_to_binary<string_ext_g>(out, s);
}

bool e_string::match(msg_seq_iter& f, const msg_seq_iter& l) const
{
  return match_fn(f, l);
}
