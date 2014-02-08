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
#ifndef ERL_LIST_H
#define ERL_LIST_H

#include "impl/list_matcher.h"
#include "impl/string_matcher.h"
#include "erl_object.h"
#include <boost/bind.hpp>
#include <list>
#include <algorithm>
#include <cassert>

namespace tinch_pp {
namespace erl {

template<typename T>
class list : public object
{
public:
  typedef std::list<T> list_type;
  typedef list<T> own_type;

  list(const list_type& contained)
    : val(contained),
      to_assign(0),
      match_fn(boost::bind(&own_type::match_value, ::_1, ::_2, ::_3))
  {
  }

  list(list_type* contained)
    : to_assign(contained),
      match_fn(boost::bind(&own_type::assign_matched, ::_1, ::_2, ::_3, contained))
  {
  }

  virtual void serialize(msg_seq_out_iter& out) const
  {
    using namespace boost;
    
    list_head_g g;
    karma::generate(out, g, val.size());

    std::for_each(val.begin(), val.end(), bind(&object::serialize, ::_1, boost::ref(out)));
    karma::generate(out, karma::byte_(tinch_pp::type_tag::nil_ext));
  }

  virtual bool match(msg_seq_iter& f, const msg_seq_iter& l) const
  {
    return match_fn(this, f, l);
  }

  list_type value() const 
  {
     return val;
  }

private:
  static bool match_value(const own_type* self, msg_seq_iter& f, const msg_seq_iter& l)
  {
    return matcher::match(self->value(), f, l);
  }

  static bool assign_matched(const own_type*, msg_seq_iter& f, const msg_seq_iter& l, list_type* dest)
  {
    assert(0 != dest);
    return matcher::assign_match(dest, f, l);
  }

private:
  typedef detail::list_matcher<list_type> matcher;

  list_type val;
  list_type* to_assign;
  
  typedef boost::function<bool (const own_type*, msg_seq_iter&, const msg_seq_iter&)> match_list_fn_type;
  match_list_fn_type match_fn;
};

template<>
class list<erl::int_> : public object
{
public:
  typedef std::list<erl::int_> list_type;
  typedef list<erl::int_> own_type;

  list(const list_type& contained)
    : val(contained),
      to_assign(0),
      match_fn(boost::bind(&own_type::match_value, ::_1, ::_2, ::_3))
  {
  }

  list(list_type* contained)
    : to_assign(contained),
      match_fn(boost::bind(&own_type::assign_matched, ::_1, ::_2, ::_3, contained))
  {
  }

  virtual void serialize(msg_seq_out_iter& out) const
  {
    using namespace boost;
    
    list_head_g g;
    karma::generate(out, g, val.size());

    std::for_each(val.begin(), val.end(), bind(&erl::int_::serialize, ::_1, boost::ref(out)));
    karma::generate(out, karma::byte_(tinch_pp::type_tag::nil_ext));
  }

  virtual bool match(msg_seq_iter& f, const msg_seq_iter& l) const
  {
    return match_fn(this, f, l);
  }

  list_type value() const 
  {
     return val;
  }

private:
  static bool match_value(const own_type* self, msg_seq_iter& f, const msg_seq_iter& l) 
  {
    // Erlang has an optimization for sending lists of small values (<=255), where 
    // the values are packed into a string.
    const bool is_packed_as_string = (type_tag::string_ext == *f);

    return is_packed_as_string ? matcher_s::match(self->value(), f, l) : matcher_l::match(self->value(), f, l);
  }

  static bool assign_matched(const own_type*, msg_seq_iter& f, const msg_seq_iter& l, list_type* dest)
  {
    // Erlang has an optimization for sending lists of small values (<=255), where 
    // the values are packed into a string.
    const bool is_packed_as_string = (type_tag::string_ext == *f);

    assert(0 != dest);
    return is_packed_as_string ? matcher_s::assign_match(dest, f, l) : matcher_l::assign_match(dest, f, l);
  }

private:
  
  typedef detail::list_matcher<list_type> matcher_l;
  typedef detail::string_matcher matcher_s;

  list_type val;
  list_type* to_assign;

  typedef boost::function<bool (const own_type*, msg_seq_iter&, const msg_seq_iter&)> match_list_fn_type;
  match_list_fn_type match_fn;
};

template<typename T>
tinch_pp::erl::list<typename T::value_type> make_list(const T& t) // T is a std::list
{
  tinch_pp::erl::list<typename T::value_type> wrapper(t);

  return wrapper;
}

template<typename T>
tinch_pp::erl::list<typename T::value_type> make_list(T* t) // T is a std::list
{
  tinch_pp::erl::list<typename T::value_type> wrapper(t);

  return wrapper;
}

}
}

#endif
