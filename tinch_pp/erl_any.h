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
#ifndef ERL_ANY_H
#define ERL_ANY_H

#include "erl_object.h"
#include "matchable.h"
#include <boost/function.hpp>
#include <map>

namespace tinch_pp {
namespace erl {
/// erl::any corresponds to a wildcard( _ in Erlang) in a pattern used for matching.
/// A position with  erl::any always succeeds in a match as long as it refers to a 
/// valid element in the matchable.
/// It's also possible to assign the value matched by  erl::any. In that case, the 
/// matched term is kept in a matchable, intended for further matching.
///
/// Example:
///
/// msg->match(make_e_tuple(atom("hello"), any()))
///
/// Succeeds for all tuples of two elements where the first is the atom 'hello'.
///
/// matchable_ptr hello_msg;
/// msg->match(make_e_tuple(atom("hello"), any(&hello_msg)))
///
/// Succeeds for all tuples of two elements where the first is the atom 'hello'.
/// Upon a successful match, the hello_msg is assigned and may be matched further:
/// e_pid reply_pid;
/// hello_msg->match(make_e_tuple(atom("some_details"), e_pid(&reply_pid)))

class any : public object
{
public:
   any();

   explicit any(matchable_ptr* to_assign);

   virtual bool match(msg_seq_iter& f, const msg_seq_iter& l) const;

   /// No, this shouldn't be publicly exposed; it's a design flaw.
   /// The function is used internally in the lib to feed data to the matchable in case 
   /// the client requested it.
   bool save_matched_bytes(const msg_seq& raw_msg_part) const;

private:
   // Should we be able to serialize an erl_any? Possible, but does it make sense?
   virtual void serialize(msg_seq_out_iter& out) const {} // no implementation!

   typedef boost::function<bool (msg_seq_iter& f, const msg_seq_iter& l, const any&)> term_matcher_type;
   typedef int term_id_type;
   typedef std::map<term_id_type, term_matcher_type> dynamic_element_matcher_type;

   static const dynamic_element_matcher_type dynamic_element_matcher;

   static bool match_dynamically(msg_seq_iter& f, const msg_seq_iter& l, const any& instance);

   matchable_ptr placeholder;
   matchable_ptr* to_assign;
   mutable msg_seq matched_raw_bytes;
};

}
}

#endif
