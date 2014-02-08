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
#include "tinch_pp/erlang_value_types.h"
#include "ext_term_grammar.h"
#include "tinch_pp/erl_any.h"
#include "term_conversions.h"
#include <boost/bind.hpp>
#include <boost/format.hpp>
#include <limits>
#include <cassert>
#include <cstdio>
#include <cmath>
#include <iostream>

using namespace tinch_pp;
using namespace tinch_pp::erl;
using namespace boost;

namespace {

bool match_int_value(msg_seq_iter& f, const msg_seq_iter& l, int32_t val)
{
  boost::int32_t res = 0;

  const bool success = binary_to_term<tinch_pp::integer>(f, l, res);

  return success && (val == res);
}

bool assign_matched_int(msg_seq_iter& f, const msg_seq_iter& l, int32_t* to_assign)
{
  assert(to_assign != 0);
  return binary_to_term<tinch_pp::integer>(f, l, *to_assign);
}

bool match_any_int(msg_seq_iter& f, const msg_seq_iter& l, const any& any_int)
{
  boost::int32_t ignore = 0;
  msg_seq_iter start = f;

  return binary_to_term<tinch_pp::integer>(f, l, ignore) ? any_int.save_matched_bytes(msg_seq(start, f)) : false;
}

}

int_::int_(int32_t a_val) 
  : val(a_val),
    to_assign(0),
    match_fn(bind(match_int_value, ::_1, ::_2, val))
 {}

int_::int_(boost::int32_t* a_to_assign)
  : val(0),
    to_assign(a_to_assign),
    match_fn(bind(assign_matched_int, ::_1, ::_2, to_assign))
{}

int_::int_(const any& any_int)
: val(0),
  to_assign(0),
  match_fn(bind(match_any_int, ::_1, ::_2, cref(any_int)))
{}

void int_::serialize(msg_seq_out_iter& out) const
{
  // In the name of optimization, Erlang tries to pack small integer values (0-0xFF).
  const bool is_small_int = (val >= 0) && (val <= std::numeric_limits<boost::uint8_t>::max());

  is_small_int ? term_to_binary<small_integer_g>(out, val) : term_to_binary<integer_ext_g>(out, val);
}

bool int_::match(msg_seq_iter& f, const msg_seq_iter& l) const
{
  return match_fn(f, l);
}

bool int_::match_string_element(msg_seq_iter& f, const msg_seq_iter& l) const
{
  const bool should_assign = (to_assign != 0);
  boost::uint8_t res = 0; // take care: range 0 - 255

  const bool success = qi::parse(f, l, qi::byte_, res);
  bool matched = true;

  if(success && should_assign)
    *to_assign = res;
  else
    matched = success && (val == res);
    
  return matched;
}

namespace {

bool match_value(msg_seq_iter& f, const msg_seq_iter& l, const tinch_pp::e_pid& val)
{
  tinch_pp::e_pid res;
  
  const bool success = binary_to_term<pid_ext>(f, l, res);

  return success && (val == res);
}

 bool assign_matched(msg_seq_iter& f, const msg_seq_iter& l, tinch_pp::e_pid* to_assign)
{
  assert(to_assign != 0);
  return binary_to_term<pid_ext>(f, l, *to_assign);
}

bool match_any_pid(msg_seq_iter& f, const msg_seq_iter& l, const any& match_any)
{
  tinch_pp::e_pid ignore;
  msg_seq_iter start = f;

  return binary_to_term<pid_ext>(f, l, ignore) ? match_any.save_matched_bytes(msg_seq(start, f)) : false;
}

bool equal_doubles(double x, double y)
{
  return fabs(x - y) < std::numeric_limits<double>::epsilon();
}

std::string encode_float(double val)
{
  using boost::format;

  std::string res = str( format("%.20e") % val );
  res.resize(constants::float_digits, 0);

  return res;
}

double decode_float(const std::string& encoded)
{
  double decoded = 0.0;
  // The following formating instruction is taken from the Erlang documentation.
  // It's safe because we guarantee a null-termination.
  const int items_matched = std::sscanf(encoded.c_str(), "%lf", &decoded);
  assert(1 == items_matched);

  return decoded;
}

bool match_float_value(msg_seq_iter& f, const msg_seq_iter& l, double val)
{
  std::string res;
  const bool success = binary_to_term<float_ext>(f, l, res);

  return success && equal_doubles(val, decode_float(res));
}

 bool assign_matched_float(msg_seq_iter& f, const msg_seq_iter& l, double* to_assign)
{
  assert(to_assign != 0);
  std::string res;
  const bool success = binary_to_term<float_ext>(f, l, res);

  *to_assign = success ? decode_float(res) : 0.0;

  return success;
}

bool match_any_float(msg_seq_iter& f, const msg_seq_iter& l, const any& match_any)
{
  std::string ignore;
  msg_seq_iter start = f;

  return binary_to_term<float_ext>(f, l, ignore) ? match_any.save_matched_bytes(msg_seq(start, f)) : false;
}

bool match_atom_value(msg_seq_iter& f, const msg_seq_iter& l, const std::string& val)
{
  std::string res;
  const bool success = binary_to_term<atom_ext>(f, l, res);

  return success && (val == res);
}

bool assign_matched_atom(msg_seq_iter& f, const msg_seq_iter& l, std::string* to_assign)
{
  assert(to_assign != 0);
  return binary_to_term<atom_ext>(f, l, *to_assign);
}

bool match_any_atom(msg_seq_iter& f, const msg_seq_iter& l, const any& match_any)
{
  std::string ignore;
  msg_seq_iter start = f;

  return binary_to_term<atom_ext>(f, l, ignore) ? match_any.save_matched_bytes(msg_seq(start, f)) : false;
}

bool match_binary_value(msg_seq_iter& f, const msg_seq_iter& l, const binary_value_type& val)
{
  binary_value_type res;
  const bool success = binary_to_term<binary_ext>(f, l, res.value) ||
                       binary_to_term<bit_binary_ext>(f, l, res);

  return success && (res == val);
}

bool assign_matched_binary(msg_seq_iter& f, const msg_seq_iter& l, binary_value_type* to_assign)
{
  assert(to_assign != 0);

  return binary_to_term<binary_ext>(f, l, to_assign->value) || binary_to_term<bit_binary_ext>(f, l, *to_assign);
}

bool match_any_binary(msg_seq_iter& f, const msg_seq_iter& l, const any& match_any)
{
  msg_seq ignore_binary;
  binary_value_type ignore_bit_seq;
  msg_seq_iter start = f;

  if(binary_to_term<binary_ext>(f, l, ignore_binary))
    return match_any.save_matched_bytes(msg_seq(start, f));
  else if(binary_to_term<bit_binary_ext>(f, l, ignore_bit_seq))
    return match_any.save_matched_bytes(msg_seq(start, f));
  else
    return false;
}

bool match_ref_value(msg_seq_iter& f, const msg_seq_iter& l, const new_reference_type& val)
{
  new_reference_type res;
  const bool success = binary_to_term<new_reference_ext_p>(f, l, res);

  return success && (val == res);
}

bool assign_matched_ref(msg_seq_iter& f, const msg_seq_iter& l, new_reference_type* to_assign)
{
  assert(to_assign != 0);
  return binary_to_term<new_reference_ext_p>(f, l, *to_assign);
}

bool match_any_ref(msg_seq_iter& f, const msg_seq_iter& l, const any& match_any)
{
  new_reference_type ignore;
  msg_seq_iter start = f;

  return binary_to_term<new_reference_ext_p>(f, l, ignore) ? match_any.save_matched_bytes(msg_seq(start, f)) : false;
}

}

pid::pid(const e_pid& a_val)
  : val(a_val),
    e_pido_assign(0),
    match_fn(bind(match_value, ::_1, ::_2, cref(val)))
{}

pid::pid(e_pid* a_e_pido_assign)
  : e_pido_assign(a_e_pido_assign),
    match_fn(bind(assign_matched, ::_1, ::_2, e_pido_assign))
{}

pid::pid(const any& match_any)
   : e_pido_assign(0),
     match_fn(bind(match_any_pid, ::_1, ::_2, cref(match_any)))
{}

void pid::serialize(msg_seq_out_iter& out) const
{
  const serializable_e_pid p(val);

  term_to_binary<pid_ext_g>(out, p);
}

bool pid::match(msg_seq_iter& f, const msg_seq_iter& l) const
{
  return match_fn(f, l);
}

float_::float_(double a_val)
  : val(a_val),
    float_to_assign(0),
    match_fn(bind(match_float_value, ::_1, ::_2, val)) {}

  // Used for assigning a match during pattern matching.
float_::float_(double* a_float_to_assign)
  : float_to_assign(a_float_to_assign),
    match_fn(bind(assign_matched_float, ::_1, ::_2, float_to_assign)) {}

float_::float_(const any& match_any)
   : float_to_assign(0),
     match_fn(bind(match_any_float, ::_1, ::_2, cref(match_any))) {}

void float_::serialize(msg_seq_out_iter& out) const
{
  term_to_binary<float_ext_g>(out, encode_float(val));
}

bool float_::match(msg_seq_iter& f, const msg_seq_iter& l) const
{
  return match_fn(f, l);
}

atom::atom(const std::string& a_val)
  : val(a_val),
    to_assign(0),
    match_fn(bind(match_atom_value, ::_1, ::_2, cref(val))) {}

atom::atom(std::string* a_to_assign)
  : to_assign(a_to_assign),
    match_fn(bind(assign_matched_atom, ::_1, ::_2, to_assign)) {}

atom::atom(const any& match_any)
   : to_assign(0),
     match_fn(bind(match_any_atom, ::_1, ::_2, cref(match_any))) {}

void atom::serialize(msg_seq_out_iter& out) const
{
  const serializable_string s(val);
  term_to_binary<atom_ext_g>(out, s);
}

bool atom::match(msg_seq_iter& f, const msg_seq_iter& l) const
{
  return match_fn(f, l);
}

// Binary
//
binary::binary(const binary_value_type& a_val)
  : val(a_val),
    to_assign(0),
    match_fn(bind(match_binary_value, ::_1, ::_2, cref(val))) {}

binary::binary(binary_value_type* a_to_assign)
  : to_assign(a_to_assign),
    match_fn(bind(assign_matched_binary, ::_1, ::_2, to_assign)) {}

binary::binary(const any& match_any)
   : to_assign(0),
     match_fn(bind(match_any_binary, ::_1, ::_2, cref(match_any))) {}

void binary::serialize(msg_seq_out_iter& out) const
{
  const bool has_padding_bits = 0 < val.padding_bits;

  if(has_padding_bits) {
    const serializable_bit_seq s(val.value, val.padding_bits);
    term_to_binary<bit_binary_ext_g>(out, s);
  } else {
    const serializable_seq s(val.value);
    term_to_binary<binary_ext_g>(out, s);
  }
}

bool binary::match(msg_seq_iter& f, const msg_seq_iter& l) const
{
  return match_fn(f, l);
}

ref::ref(const new_reference_type& a_val)
  : val(a_val),
    to_assign(0),
    match_fn(bind(match_ref_value, ::_1, ::_2, cref(val))) {}

  // Used for assigning a match during pattern matching.
ref::ref(new_reference_type* a_to_assign)
  : to_assign(a_to_assign),
    match_fn(bind(assign_matched_ref, ::_1, ::_2, to_assign)) {}

ref::ref(const any& match_any)
   : to_assign(0),
     match_fn(bind(match_any_ref, ::_1, ::_2, cref(match_any))) {}

void ref::serialize(msg_seq_out_iter& out) const
{
  new_reference_g_type gtype(val);

  term_to_binary<new_reference_ext_g>(out, gtype);
}

bool ref::match(msg_seq_iter& f, const msg_seq_iter& l) const
{
  return match_fn(f, l);
}


