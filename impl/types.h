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
#ifndef TYPES_H
#define TYPES_H

#include <boost/fusion/adapted/struct/adapt_struct.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/cstdint.hpp>
#include <vector>
#include <string>

namespace tinch_pp {

typedef std::vector<char> msg_seq;
typedef std::back_insert_iterator<msg_seq> msg_seq_out_iter;
typedef msg_seq::iterator msg_seq_iter;
typedef msg_seq::const_iterator msg_seq_citer;

// TODO: boost types!
typedef int creation_number_type;
typedef int port_number_type;

typedef boost::uint32_t time_type_sec;

// This is me giving up - I can't get the binary generators to work with attributes transformations :-(
struct serializable_string
{
  explicit serializable_string(const std::string& a_val)
    : size(a_val.size()),
      val(a_val) {}

  size_t size;
  std::string val;
};

struct serializable_seq
{
  explicit serializable_seq(const msg_seq& a_val)
    : size(a_val.size()),
    val(a_val) {}

  size_t size;
  msg_seq val;
};

struct binary_value_type
{
  typedef std::vector<char> value_type;

  /// Specifies an empty binary, typically used to assign to in a pattern match.
  binary_value_type();

  /// Specifies a "normal" binary consisting of a complete number of bytes.
  binary_value_type(const value_type& binary_data);

  /// Specifies a bit-string. The given number of unused bits are counted 
  /// from the lowest significant bits in the last byte. Must be within the range [1..7].
  binary_value_type(const value_type& binary_data,
                    int unused_bits_in_last_byte);

  int padding_bits;
  value_type value;
};

bool operator==(const binary_value_type& left, const binary_value_type& right);

struct serializable_bit_seq
{
  // used to represent bit_binary_ext
  serializable_bit_seq(const msg_seq& a_val,
                       int a_unused_bits)
   : size(a_val.size()),
      val(a_val),
      unused_bits(a_unused_bits) {}

  size_t size;
  int unused_bits;
  msg_seq val;
};


bool operator ==(const serializable_string& s1, const serializable_string& s2);

struct e_pid
{
  e_pid(const std::string& a_node_name,
        boost::uint32_t a_id,
        boost::uint32_t a_serial,
        boost::uint32_t a_creation)
  : node_name(a_node_name),
    id(a_id), serial(a_serial), creation(a_creation) {}

  // Creates an invalid pid (not that nice, but necessary(?)
  // when matching received messages).
  e_pid()
    : node_name(""), id(0), serial(0), creation(0) {}

  std::string node_name;
  boost::uint32_t id;
  boost::uint32_t serial;
  boost::uint32_t creation;
};

bool operator ==(const e_pid& p1, const e_pid& p2);

struct serializable_e_pid
{
  explicit serializable_e_pid(const e_pid& p)
  : node_name(p.node_name),
    id(p.id), serial(p.serial), creation(p.creation) {}

  serializable_string node_name;
  boost::uint32_t id;
  boost::uint32_t serial;
  boost::uint32_t creation;
};

struct new_reference_type
{
   new_reference_type(const std::string& a_node_name,
		      boost::uint32_t a_creation,
		      const msg_seq& a_id)
  : node_name(a_node_name),
    creation(a_creation),
    id(a_id.begin(), a_id.end()) {}

  // Creates an invalid new_reference (not that nice, but necessary(?)
  // when matching received messages).
  new_reference_type()
    : node_name(""), creation(0) {}

  std::string node_name;
  boost::uint32_t creation;
  msg_seq id; // should be regarded as uninterpreted data
};

bool operator ==(const new_reference_type& r1, const new_reference_type& r2);

// Intended for easier serialization.
struct new_reference_g_type
{
  new_reference_g_type(const new_reference_type& ref)
    : id_length(ref.id.size() / sizeof(boost::uint32_t)),
      node_name(ref.node_name),
      creation(ref.creation),
      id(ref.id.begin(), ref.id.end()) {}

  size_t id_length;
  serializable_string node_name;
  boost::uint32_t creation;
  msg_seq id; // should be regarded as uninterpreted data
};

}

// Adapt in the global namespace.
BOOST_FUSION_ADAPT_STRUCT(
   tinch_pp::serializable_string,
   (size_t, size)
   (std::string, val))

BOOST_FUSION_ADAPT_STRUCT(
   tinch_pp::serializable_seq,
   (size_t, size)
   (tinch_pp::msg_seq, val))

BOOST_FUSION_ADAPT_STRUCT(
   tinch_pp::binary_value_type,
   (int, padding_bits)
   (tinch_pp::msg_seq, value))

BOOST_FUSION_ADAPT_STRUCT(
   tinch_pp::serializable_bit_seq,
   (size_t, size)
   (int, unused_bits)
   (tinch_pp::msg_seq, val))

BOOST_FUSION_ADAPT_STRUCT(
   tinch_pp::e_pid,
   (std::string, node_name)
   (boost::uint32_t, id)
   (boost::uint32_t, serial)
   (boost::uint32_t, creation))

BOOST_FUSION_ADAPT_STRUCT(
   tinch_pp::serializable_e_pid,
   (tinch_pp::serializable_string, node_name)
   (boost::uint32_t, id)
   (boost::uint32_t, serial)
   (boost::uint32_t, creation))

BOOST_FUSION_ADAPT_STRUCT(
   tinch_pp::new_reference_type,
   (std::string, node_name)
   (boost::uint32_t, creation)
   (tinch_pp::msg_seq, id))

BOOST_FUSION_ADAPT_STRUCT(
   tinch_pp::new_reference_g_type,
   (size_t, id_length)
   (tinch_pp::serializable_string, node_name)
   (boost::uint32_t, creation)
   (tinch_pp::msg_seq, id))

#endif
