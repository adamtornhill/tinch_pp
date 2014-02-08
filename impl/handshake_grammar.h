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
#ifndef HANDSHAKE_GRAMMAR_H
#define HANDSHAKE_GRAMMAR_H

// Implements a boost::spirit grammar for the handshake between connected nodes.
// The protocol is described in distribution_handshake.txt, included in the Erlang release.
//
// Every message in the handshake starts with a 16 bit big endian integer 
// which contains the length of the message (not counting the two initial bytes).
// Generally, this field is ignored during parsing (the receiver has already assembled 
// a complete package).

#include "constants.h"
#include "types.h"
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/karma.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_fusion.hpp>
#include <vector>
#include <string>

namespace karma = boost::spirit::karma;
namespace phoenix = boost::phoenix;
namespace qi = boost::spirit::qi;

namespace tinch_pp {

// Synthezised attribute-types for the grammars.
struct sent_name_type
{
  int version0;
  int version1;
  std::string name;
};
}
// Adapt in the global namespace.
BOOST_FUSION_ADAPT_STRUCT(
   tinch_pp::sent_name_type,
   (int, version0)
   (int, version1)
   (std::string, name))

namespace tinch_pp {

// send_name/receive_name:
//
// +---+--------+--------+-----+-----+-----+-----+-----+-----+-...-+-----+
// |'n'|Version0|Version1|Flag0|Flag1|Flag2|Flag3|Name0|Name1| ... |NameN|
// +---+--------+--------+-----+-----+-----+-----+-----+-----+-... +-----+
//
// We hardocde the version and the capability flags here.

struct send_name: karma::grammar<msg_seq_out_iter, serializable_string()>
{
  send_name() : base_type(start)
  {
    using boost::spirit::ascii::string; // TODO: good enough?
    using namespace karma;
    using phoenix::at_c;

    const boost::uint16_t version = constants::supported_version;
    const size_t fixed_length = 7;

    start = big_word[_1 = (at_c<0>(_val) + fixed_length)] << byte_('n') << 
      big_word(version) << big_dword(constants::capabilities) << string[_1 = at_c<1>(_val)];
  }

  karma::rule<msg_seq_out_iter, serializable_string()> start;
};

struct send_name_p : qi::grammar<msg_seq_iter, sent_name_type()>
{
  send_name_p() : base_type(start)
  {
    using namespace qi;

    start %= omit[big_word >> byte_('n')] >> big_word >> big_dword >> +char_;
  }

  qi::rule<msg_seq_iter, sent_name_type()> start;
};

// We ignore the versions and flags for now - TODO: add them for verification!
struct receive_name : qi::grammar<msg_seq_iter, std::string()>
{
  receive_name() : base_type(start)
  {
    using namespace qi;

    start = omit[big_word >> byte_('n') >> big_word >> big_dword] >> +char_;
  }

  qi::rule<msg_seq_iter, std::string()> start;
};

// receive_status/send_status:
//
// +---+-------+-------+ ... +-------+
// |'s'|Status0|Status1| ... |StatusN|
// +---+-------+-------+ ... +-------+

struct receive_status : qi::grammar<msg_seq_iter, std::string()>
{
  receive_status() : base_type(start)
  {
    using namespace qi;

    start = omit[big_word >> byte_('s')] >> +char_;
  }

  qi::rule<msg_seq_iter, std::string()> start;
};

struct receive_status_g : karma::grammar<msg_seq_out_iter, serializable_string()>
{
  receive_status_g() : base_type(start)
  {
    using namespace karma;
    using boost::spirit::ascii::string;
    using phoenix::at_c;

    const size_t fixed_length = 1;

    start = big_word[_1 = (at_c<0>(_val) + fixed_length)] << byte_('s') << string[_1 = at_c<1>(_val)];
  }

  karma::rule<msg_seq_out_iter, serializable_string()> start;
};


// recv_challenge/send_challenge:
//
// +---+--------+--------+-----+-----+-----+-----+-----+-----+-----+-----+---
// |'n'|Version0|Version1|Flag0|Flag1|Flag2|Flag3|Chal0|Chal1|Chal2|Chal3|
// +---+--------+--------+-----+-----+-----+-----+-----+-----+---- +-----+---
//    ------+-----+-...-+-----+
//     Name0|Name1| ... |NameN|
//    ------+-----+-... +-----+

struct challenge_attributes
{
  boost::uint32_t challenge;
  std::string node_name;
};

struct send_challenge_attributes
{
  send_challenge_attributes(boost::uint32_t a_challenge,
			    const std::string& a_node_name)
    : challenge(a_challenge),
      name_length(a_node_name.size()),
      node_name(a_node_name) {}

  boost::uint32_t challenge;
  size_t name_length;
  std::string node_name;
};

}

// Adapt in the global namespace.
BOOST_FUSION_ADAPT_STRUCT(
   tinch_pp::challenge_attributes,
   (boost::uint32_t, challenge)
   (std::string, node_name));

BOOST_FUSION_ADAPT_STRUCT(
   tinch_pp::send_challenge_attributes,
   (boost::uint32_t, challenge)
   (size_t, name_length)
   (std::string, node_name));

namespace tinch_pp {

struct recv_challenge : qi::grammar<msg_seq_iter, challenge_attributes()>
{
  recv_challenge() : base_type(start)
  {
    using namespace qi;

    start %= omit[big_word >> byte_('n') >> big_word >> big_dword] >> big_dword >> +char_;
  }

  qi::rule<msg_seq_iter, challenge_attributes()> start;
};

struct send_challenge_g : karma::grammar<msg_seq_out_iter, send_challenge_attributes()>
{
  send_challenge_g() : base_type(start)
  {
    using boost::spirit::ascii::string;
    using namespace karma;
    using phoenix::at_c;

    const size_t fixed_length = 11;

    start = big_word[_1 = (at_c<1>(_val) + fixed_length)] << byte_('n') << 
                          big_word(constants::supported_version) << big_dword(constants::capabilities) << 
                          big_dword[_1 = at_c<0>(_val)] << string[_1 = at_c<2>(_val)];
  }

  karma::rule<msg_seq_out_iter, send_challenge_attributes()> start;
};


// send_challenge_reply/recv_challenge_reply:
//
// +---+-----+-----+-----+-----+-----+-----+-----+-----+
// |'r'|Chal0|Chal1|Chal2|Chal3|Dige0|Dige1|Dige2|Dige3|
// +---+-----+-----+-----+-----+-----+-----+---- +-----+
struct challenge_reply_attributes
{
  challenge_reply_attributes()
    : challenge(0) {}

  challenge_reply_attributes(boost::uint32_t a_challenge, const msg_seq& a_digest)
    : challenge(a_challenge),
      digest(a_digest) {}

  boost::uint32_t challenge;
  msg_seq digest;
};

}

// Adapt in the global namespace.
BOOST_FUSION_ADAPT_STRUCT(
   tinch_pp::challenge_reply_attributes,
   (boost::uint32_t, challenge)
   (tinch_pp::msg_seq, digest));

namespace tinch_pp {

struct challenge_reply_p : qi::grammar<msg_seq_iter, challenge_reply_attributes()>
{
  challenge_reply_p() : base_type(start)
  {
    using namespace qi;

    const size_t digest_length = 16;
    const size_t length = 5 + digest_length;

    start = big_word(length) >> byte_('r') >> big_dword >> repeat(digest_length)[byte_];
  }

  qi::rule<msg_seq_iter, challenge_reply_attributes()> start;
};

struct challenge_reply : karma::grammar<msg_seq_out_iter, challenge_reply_attributes()>
{
  challenge_reply() : base_type(start)
  {
    using namespace karma;

    const size_t digest_length = 16;
    const size_t length = 5 + digest_length;

    start %= big_word(length) << byte_('r') << big_dword << repeat(digest_length)[byte_];
  }

  karma::rule<msg_seq_out_iter, challenge_reply_attributes()> start;
};

// recv_challenge_ack/send_challenge_ack
//
// +---+-----+-----+-----+-----+
// |'a'|Dige0|Dige1|Dige2|Dige3|
// +---+-----+-----+---- +-----+

struct recv_challenge_ack_p : qi::grammar<msg_seq_iter, msg_seq()>
{
  recv_challenge_ack_p() : base_type(start)
  {
    using namespace qi;

    const size_t digest_length = 16;
    const size_t length = 1 + digest_length;

    start = big_word(length) >> byte_('a') >> repeat(digest_length)[byte_];
  }

  qi::rule<msg_seq_iter, msg_seq()> start;
};

struct send_challenge_ack_g : karma::grammar<msg_seq_out_iter, msg_seq()>
{
  send_challenge_ack_g() : base_type(start)
  {
    using namespace karma;

    const size_t digest_length = 16;
    const size_t length = 1 + digest_length;

    start = big_word(length) << byte_('a') << repeat(digest_length)[byte_];
  }

  karma::rule<msg_seq_out_iter, msg_seq()> start;
};

}

#endif
