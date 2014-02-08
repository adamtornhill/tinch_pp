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
#ifndef STRING_MATCHER_H
#define STRING_MATCHER_H

#include "ext_term_grammar.h"
#include "tinch_pp/erlang_value_types.h"
#include <boost/bind.hpp>
#include <algorithm>
#include <list>

namespace tinch_pp {
namespace detail {
  
struct string_matcher
{
  typedef std::list<erl::int_> list_type;

  static bool match(const list_type& val, msg_seq_iter& f, const msg_seq_iter& l)
  {
    using namespace boost;

    size_t parsed_length = 0;
    string_head_ext string_head_p;

    const bool success = qi::parse(f, l, string_head_p, parsed_length);
    const bool length_matched = success && (val.size() == parsed_length);

    // When packed as a string, there's no encoding-tag prepended to the individual elements
    return length_matched && (val.end() == std::find_if(val.begin(), val.end(), 
							                                                  bind(&erl::int_::match_string_element, 
                                                               ::_1, boost::ref(f), cref(l)) == false));
  }

  static bool assign_match(list_type* val, msg_seq_iter& f, const msg_seq_iter& l)
  {
    using namespace boost;

    size_t parsed_length = 0;
    string_head_ext string_head_p;

    bool success = qi::parse(f, l, string_head_p, parsed_length);

    for(size_t i = 0; success && (i < parsed_length); ++i) {
      erl::int_::value_type matched_value;

      success =  qi::parse(f, l, qi::byte_, matched_value);
      val->push_back(erl::int_(matched_value));
    }

    return success;
  }
};

}
}

#endif
