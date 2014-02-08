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
#ifndef EXT_TERM_GRAMMAR_H
#define EXT_TERM_GRAMMAR_H

#include "types.h"
#include "constants.h"
#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/karma.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_fusion.hpp>
#include <boost/spirit/home/phoenix/container.hpp>

// Namespace aliases.
namespace qi = boost::spirit::qi;
namespace karma = boost::spirit::karma;

namespace tinch_pp {

// Implementation of an EBNF-like grammar representing the external term format used in the 
// distribution mechanism of Erlang.
// The external term format is a binary protocol (see reference XXX).
// The format is parsed with Boost Sprit QI and genereated through Karma.

namespace type_tag {
  const int bit_binary_ext    = 77;
  const int atom_cache_ref    = 82;
  const int small_integer     = 97;
  const int integer           = 98;
  const int float_ext         = 99;
  const int atom_ext          = 100;
  const int pid               = 103;
  const int small_tuple       = 104;
  const int nil_ext           = 106;
  const int string_ext        = 107;
  const int list              = 108;
  const int binary_ext        = 109;
  const int new_reference_ext = 114;
}

template<const int tag>
struct ext_byte : qi::grammar<msg_seq_iter, boost::uint8_t()>
{
  ext_byte() : base_type(start)
  {
    using qi::byte_;
    using qi::omit;

    start = omit[byte_(tag)] >> byte_;
  }

  qi::rule<msg_seq_iter, boost::uint8_t()> start;
};

template<const int tag>
struct ext_byte_g : karma::grammar<msg_seq_out_iter, boost::uint8_t()>
{
  ext_byte_g() : base_type(start)
  {
    using karma::byte_;

    start = byte_(tag) << byte_;
  }

  karma::rule<msg_seq_out_iter, boost::uint8_t()> start;
};

typedef ext_byte<type_tag::small_integer> small_integer;
typedef ext_byte_g<type_tag::small_integer> small_integer_g;

typedef ext_byte<type_tag::atom_cache_ref> atom_cache_ref;

struct integer_ext : qi::grammar<msg_seq_iter, boost::int32_t()>
{
  integer_ext() : base_type(start)
  {
    using qi::big_dword;
    using qi::byte_;
    using qi::omit;

    start = omit[byte_(type_tag::integer)] >> big_dword;
  }

  qi::rule<msg_seq_iter, boost::int32_t()> start;
};

struct integer_ext_g : karma::grammar<msg_seq_out_iter, int()>
{
  integer_ext_g() : base_type(start)
  {
    using namespace karma;

    start = byte_(type_tag::integer) << big_dword;
  }

  karma::rule<msg_seq_out_iter, int()> start;
};

// Erlang sends integers on different formats depending on their size.
// This grammar parses any of them.
struct integer : qi::grammar<msg_seq_iter, boost::int32_t()>
{
  integer() : base_type(start)
    {
      start = small_int_ | integer_ext_;
    }

  // Grammars defined in this module.
  integer_ext integer_ext_;
  small_integer small_int_;

  qi::rule<msg_seq_iter, boost::int32_t()> start;
};

// Erlang sends floating point numbers as string (formated with "%.20e").
struct float_ext : qi::grammar<msg_seq_iter, std::string()>
{
  float_ext() : base_type(start)
    {
      using namespace qi;

      start = byte_(type_tag::float_ext) >> repeat(constants::float_digits)[char_];
    }

  qi::rule<msg_seq_iter, std::string()> start;
};

struct float_ext_g : karma::grammar<msg_seq_out_iter, std::string()>
{
  float_ext_g() : base_type(start)
  {
    using namespace karma;

    start %= byte_(type_tag::float_ext) << repeat(constants::float_digits)[char_];
  }

  karma::rule<msg_seq_out_iter, std::string()> start;
};

struct atom_ext : qi::grammar<msg_seq_iter, std::string()>
{
  atom_ext() : base_type(start)
  {
    using qi::big_word;
    using qi::byte_;
    using boost::phoenix::ref;

    // EBNF forces the repeat directive to be a constant. However, with this spirit hack, 
    // we make it variable at runtime by assigning it in a semantic action.
    // TODO: should be possible with local Spirit variables, right?
    header = byte_(type_tag::atom_ext) >> big_word[ref(atom_length) = qi::_1];
    start = header >> qi::repeat(ref(atom_length))[qi::char_];
  }

  boost::uint16_t atom_length;
 
  qi::rule<msg_seq_iter, qi::unused_type> header;
  qi::rule<msg_seq_iter, std::string()> start;
};

struct atom_ext_g : karma::grammar<msg_seq_out_iter, serializable_string()>
{
  atom_ext_g() : base_type(start)
  {
    using namespace karma;

    start %= byte_(type_tag::atom_ext) << big_word << *char_;
  }

  karma::rule<msg_seq_out_iter, serializable_string()> start;
};

struct pid_ext : qi::grammar<msg_seq_iter, e_pid()>
{
  pid_ext() : base_type(start)
  {
    using namespace qi;

    start = omit[byte_(type_tag::pid)] >> atom_p >> big_dword >> big_dword >> byte_;
  }

  atom_ext atom_p;
  qi::rule<msg_seq_iter, e_pid()> start;
};

struct pid_ext_g : karma::grammar<msg_seq_out_iter, serializable_e_pid()>
{
  pid_ext_g() : base_type(start)
  {
    using namespace karma;

    start %= byte_(type_tag::pid) << atom_g << big_dword << big_dword << byte_;
  }

  atom_ext_g atom_g;
  karma::rule<msg_seq_out_iter, serializable_e_pid()> start;
};

struct new_reference_ext_p : qi::grammar<msg_seq_iter, new_reference_type()>
{
  new_reference_ext_p() : base_type(start)
  {
    using namespace qi;
    using boost::phoenix::ref;

    // Parse the header for side-effects only; we need to extract the ID length.
    // The ID contains a sequence of big-endian unsigned integers (4 bytes each, 
    // so N' is a multiple of 4), but should be regarded as uninterpreted data.
    const size_t len_multiple = 4;

    header = omit[byte_(type_tag::new_reference_ext) >> big_word[ref(id_length) = qi::_1]];
    start = header >> node_name_p >> byte_ >> qi::repeat(ref(id_length) * len_multiple)[qi::char_];
  }

  atom_ext node_name_p;
  boost::uint16_t id_length;
  qi::rule<msg_seq_iter, qi::unused_type> header;
  qi::rule<msg_seq_iter, new_reference_type()> start;
};

struct new_reference_ext_g : karma::grammar<msg_seq_out_iter, new_reference_g_type()>
{
  new_reference_ext_g() : base_type(start)
  {
    using namespace karma;

    start %= byte_(type_tag::new_reference_ext) << big_word << atom_g << byte_ << *char_;
  }

  atom_ext_g atom_g;
  karma::rule<msg_seq_out_iter, new_reference_g_type()> start;
};

struct small_tuple_head_ext : qi::grammar<msg_seq_iter, int()>
{
  small_tuple_head_ext() : base_type(start)
  {
    using namespace qi;

    start = omit[byte_(type_tag::small_tuple)] >> byte_;
  }

  qi::rule<msg_seq_iter, int()> start;
};

struct small_tuple_head_g : karma::grammar<msg_seq_out_iter, int()>
{
  small_tuple_head_g() : base_type(start)
  {
    using namespace karma;

    start = byte_(type_tag::small_tuple) << byte_;
  }

  karma::rule<msg_seq_out_iter, int()> start;
};

struct list_head_ext : qi::grammar<msg_seq_iter, size_t()>
{
  list_head_ext() : base_type(start)
  {
    using namespace qi;

    start = omit[byte_(type_tag::list)] >> big_dword;
  }

  qi::rule<msg_seq_iter, size_t()> start;
};

struct list_head_g : karma::grammar<msg_seq_out_iter, size_t()>
{
  list_head_g() : base_type(start)
  {
    using namespace karma;

    start = byte_(type_tag::list) << big_dword;
  }

  karma::rule<msg_seq_out_iter, size_t()> start;
};

// String does NOT have a corresponding Erlang representation, but is an optimization for sending 
// lists of bytes (integer in the range 0-255) more efficiently over the distribution.
struct string_head_ext : qi::grammar<msg_seq_iter, size_t()>
{
  string_head_ext() : base_type(start)
  {
    using namespace qi;

    start = omit[byte_(type_tag::string_ext)] >> big_word;
  }

  qi::rule<msg_seq_iter, size_t()> start;
};

struct string_ext_g : karma::grammar<msg_seq_out_iter, serializable_string()>
{
  string_ext_g() : base_type(start)
  {
    using namespace karma;

    start = byte_(type_tag::string_ext) << big_word << *char_;
  }

  karma::rule<msg_seq_out_iter, serializable_string()> start;
};

struct binary_ext : qi::grammar<msg_seq_iter, msg_seq()>
{
  binary_ext() : base_type(start)
  {
    using qi::big_dword;
    using qi::byte_;
    using boost::phoenix::ref;

    // EBNF forces the repeat directive to be a constant. However, with this spirit hack, 
    // we make it variable at runtime by assigning it in a semantic action.
    header = byte_(type_tag::binary_ext) >> big_dword[ref(binary_length) = qi::_1];
    start = header >> qi::repeat(ref(binary_length))[byte_];
  }

  boost::uint32_t binary_length;
 
  qi::rule<msg_seq_iter, qi::unused_type> header;
  qi::rule<msg_seq_iter, msg_seq()> start;
};

struct binary_ext_g : karma::grammar<msg_seq_out_iter, serializable_seq()>
{
  binary_ext_g() : base_type(start)
  {
    using namespace karma;

    start %= byte_(type_tag::binary_ext) << big_dword << *byte_;
  }

  karma::rule<msg_seq_out_iter, serializable_seq()> start;
};

//

struct bit_binary_ext : qi::grammar<msg_seq_iter, binary_value_type()>
{
  bit_binary_ext() : base_type(start)
  {
    using qi::big_dword;
    using qi::byte_;
    using boost::phoenix::ref;

    // EBNF forces the repeat directive to be a constant. However, with this spirit hack, 
    // we make it variable at runtime by assigning it in a semantic action.
    header = byte_(type_tag::bit_binary_ext) >> big_dword[ref(binary_length) = qi::_1];
    start = header >> byte_ >> qi::repeat(ref(binary_length))[byte_];
  }

  boost::uint32_t binary_length;
 
  qi::rule<msg_seq_iter, qi::unused_type> header;
  qi::rule<msg_seq_iter, binary_value_type()> start;
};

struct bit_binary_ext_g : karma::grammar<msg_seq_out_iter, serializable_bit_seq()>
{
  bit_binary_ext_g() : base_type(start)
  {
    using namespace karma;

    start %= byte_(type_tag::bit_binary_ext) << big_dword << byte_ << *byte_;
  }

  karma::rule<msg_seq_out_iter, serializable_bit_seq()> start;
};

//

}

#endif
