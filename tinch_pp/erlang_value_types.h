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
#ifndef ERLANG_VALUE_TYPES_H
#define ERLANG_VALUE_TYPES_H

#include "erl_object.h"
#include <boost/function.hpp>

namespace tinch_pp {
namespace erl {

class any;

// As an object is used in a match, it's either a value-match or a type-match.
// In the latter case, we want to assign the matched value. That difference 
// in behaviour is encapsulated by a match_fn.
typedef boost::function<bool (msg_seq_iter&, const msg_seq_iter&)> match_fn_type;

class int_ : public object
{
public:
  typedef boost::int32_t value_type;

  explicit int_(boost::int32_t val);

  explicit int_(boost::int32_t* to_assign);

  explicit int_(const any& match_any_int);

  virtual void serialize(msg_seq_out_iter& out) const;

  virtual bool match(msg_seq_iter& f, const msg_seq_iter& l) const;

  // Special case: Erlang packs a list of small values (<= 255) into a string-sequence.
  // Those values can only be matched by int_.
  bool match_string_element(msg_seq_iter& f, const msg_seq_iter& l) const;

  boost::int32_t value() const { return val; }

private:
  boost::int32_t val;
  boost::int32_t* to_assign;

  match_fn_type match_fn;
};

class pid : public object
{
public:
  typedef e_pid value_type;

  explicit pid(const e_pid& val);

  // Used for assigning a match during pattern matching.
  explicit pid(e_pid* e_pido_assign);

  explicit pid(const any& match_any);

  virtual void serialize(msg_seq_out_iter& out) const;

  virtual bool match(msg_seq_iter& f, const msg_seq_iter& l) const;

  e_pid value() const { return val; }

private:
  e_pid val;
  e_pid* e_pido_assign;

  match_fn_type match_fn;
};

class float_ : public object
{
public:
  typedef double value_type;

  explicit float_(double val);

  // Used for assigning a match during pattern matching.
  explicit float_(double* float_to_assign);

  explicit float_(const any& match_any);

  virtual void serialize(msg_seq_out_iter& out) const;

  virtual bool match(msg_seq_iter& f, const msg_seq_iter& l) const;

  double value() const { return val; }

private:
  double val;
  double* float_to_assign;

  match_fn_type match_fn;
};

class atom : public object
{
public:
  typedef std::string value_type;

  explicit atom(const std::string& val);

  explicit atom(std::string* to_assign);

  explicit atom(const any& match_any);

  virtual void serialize(msg_seq_out_iter& out) const;

  virtual bool match(msg_seq_iter& f, const msg_seq_iter& l) const;

  std::string value() const { return val; }

private:
  std::string val;
  std::string* to_assign;

  match_fn_type match_fn;
};

/// Encode a reference object (an object generated with make_ref/0).
class ref : public object
{
public:
  typedef new_reference_type value_type;

  explicit ref(const new_reference_type& val);

  // Used for assigning a match during pattern matching.
  explicit ref(new_reference_type* e_pido_assign);

  explicit ref(const any& match_any);

  virtual void serialize(msg_seq_out_iter& out) const;

  virtual bool match(msg_seq_iter& f, const msg_seq_iter& l) const;

  new_reference_type value() const { return val; }

private:
  new_reference_type val;
  new_reference_type* to_assign;

  match_fn_type match_fn;
};

/// Since R12B Erlang allows binaries that consist of bits that don't make up 
/// to an even number of bytes (bit-strings). A binary in tinch++ will be able 
/// to handle both cases.
class binary : public object
{
public:
  explicit binary(const binary_value_type& val);

  explicit binary(binary_value_type* to_assign);

  explicit binary(const any& match_any);

  virtual void serialize(msg_seq_out_iter& out) const;

  virtual bool match(msg_seq_iter& f, const msg_seq_iter& l) const;

  binary_value_type value() const { return val; }

private:
  binary_value_type val;
  binary_value_type* to_assign;

  match_fn_type match_fn;
};

}
}

#endif
