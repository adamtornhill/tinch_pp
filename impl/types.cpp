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
#include "types.h"
#include "tinch_pp/exceptions.h"

#include <boost/lexical_cast.hpp>

namespace tinch_pp {

bool operator ==(const serializable_string& s1, const serializable_string& s2)
{
   return s1.val == s2.val;
}

bool operator ==(const e_pid& p1, const e_pid& p2)
{
  return (p1.node_name == p2.node_name) && 
         (p1.id == p2.id) && 
         (p1.serial == p2.serial) && 
         (p1.creation == p2.creation);
}

bool operator ==(const new_reference_type& r1, const new_reference_type& r2)
{
  return (r1.node_name == r2.node_name) &&
         (r1.creation == r2.creation) &&
         (r1.id == r2.id);
}

// TODO: Separate the value types into private and public. Expose the public in the API.
binary_value_type::binary_value_type()
 : padding_bits(0) {}

binary_value_type::binary_value_type(const value_type& binary_data)
: padding_bits(0),
  value(binary_data) {}

// Specifies a bit-string. The given number of unused bits are counted 
// from the lowest significant bits in the last byte. Must be within the range [1..7].
binary_value_type::binary_value_type(const value_type& binary_data,
                                     int unused_bits_in_last_byte)
: padding_bits(unused_bits_in_last_byte),
  value(binary_data) 
{
  if(padding_bits < 0 || 7 < padding_bits)
    throw encoding_error("bitstring", "The padding must be in range 0..7, you provided " + boost::lexical_cast<std::string>(padding_bits));
	
	if(padding_bits != 0 && value.empty())
    throw encoding_error("bitstring", "Padding on a zero length bitstring isn't allowed");

 // Ensure that we pad with zeroes (thank you, Jinterface for this tip!).
 if(!value.empty())
   value[value.size() - 1] &= ~((1 << padding_bits) - 1);
}

bool operator==(const binary_value_type& left, const binary_value_type& right)
{
  return (left.padding_bits == right.padding_bits) &&
          (left.value == right.value);
}

}
