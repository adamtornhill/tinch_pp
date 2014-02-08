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
#ifndef EPMD_PROTOCOL_H
#define EPMD_PROTOCOL_H

// General
// =======
//
// The protocol between EPMD (Erlang Port Mapper Daemon) and our distributed C++ node.
// The EPMD protocol is quite simple:
// 1) Establish a socket connection to EPMD.
// 2) Register the node at EPMD with an ALIVE2_REQ.
// 3) EPMD replies with a ALIVE2_RESP.
// 4) Unregister the node by closing the socket (We reamin registered as long as we keep the socket connected).
//
// Once registered, other requests are issued to EPMD. These requests are used to fetch 
// information about other nodes, to which we want to connect. The typical request is 
// a PORT_PLEASE2_REQ used to get the listening port of another node. These requests are 
// sent as one-shot: 
// 1) connect over TCP/IP, 
// 2) send the request,
// 3) read the reply, and
// 4 close the socket connection.
//
// Implementation
// ==============
// EPMD uses a binary protocol (see reference http://ftp.sunet.se/pub/lang/erlang/doc/apps/erts/erl_dist_protocol.html).
// We use boost spirit to generate (Karma) those binary messages and to parse them (Qi). Spirit allows an EBNF-like 
// grammar for representing the Erlang distribution protocol.

#include "types.h"
#include <boost/fusion/adapted/struct/adapt_struct.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
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
namespace epmd {

// After the socket connection has been established, we register our node with an ALIVE2_REQ.
struct alive2_req_params
{
   alive2_req_params(const std::string& a_node_name, int a_port)
    : msg_length(13 /* size of fixed header part*/ + a_node_name.size()),
      node_name(a_node_name),
      node_name_length(a_node_name.size()),
      port(a_port)
  {}

  size_t msg_length;
  std::string node_name;
  size_t node_name_length; // Simpler to generate this way.
  int port;
};

}
}

// Adapt in the global namespace.
BOOST_FUSION_ADAPT_STRUCT(
   tinch_pp::epmd::alive2_req_params,
   (size_t, msg_length)
   (std::string, node_name)
   (size_t, node_name_length)
   (int, port))

namespace tinch_pp {
namespace epmd {

struct alive2_req : karma::grammar<msg_seq_out_iter, alive2_req_params()>
{
    alive2_req() : base_type(start)
    {
      using phoenix::at_c;
      using boost::spirit::ascii::string; // TODO: good enough?
      //using namespace boost::spirit;
      using namespace karma;
     
      const int msg_number = 120;
      const int node_type = 72; // hidden node (i.e. not native Erlang)
      const int protocol = 0;
      const int supported_version = 5; // R6B and later
      const int extra_info_length = 0;

      start = big_word[_1 = at_c<0>(_val)] << byte_(msg_number) << // header
	big_word[_1 = at_c<3>(_val)] << byte_(node_type) << byte_(protocol) <<
	big_word(supported_version) << big_word(supported_version) <<
	big_word[_1 = at_c<2>(_val)] << string[_1 = at_c<1>(_val)] << 
	big_word(extra_info_length);
    }

    karma::rule<msg_seq_out_iter, alive2_req_params()> start;
};

struct alive2_resp_result
{
  int result;
  boost::uint16_t creation;
};

}
}

// Adapt in the global namespace.
BOOST_FUSION_ADAPT_STRUCT(
   tinch_pp::epmd::alive2_resp_result,
   (int, result)
   (boost::uint16_t, creation))

namespace tinch_pp {
namespace epmd {

struct alive2_resp : qi::grammar<msg_seq_iter, alive2_resp_result()>
{
  alive2_resp() : base_type(start)
    {
      using namespace qi;

      start %= omit[byte_(121)] >> byte_ >> big_word;
    }

  qi::rule<msg_seq_iter, alive2_resp_result()> start;
};

struct port2_resp_result : qi::grammar<msg_seq_iter, int()>
{
  port2_resp_result() : base_type(start)
    {
      using qi::byte_;
      using qi::omit;

      start = omit[byte_(119)] >> byte_;
    }

  qi::rule<msg_seq_iter, int()> start;
};

}
}

#endif
