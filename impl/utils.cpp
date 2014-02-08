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
#include "utils.h"
#include "md5.h"
#include <boost/lexical_cast.hpp>
#include <boost/spirit/include/qi_binary.hpp>
#include <boost/regex.hpp>
#include <cassert>

namespace asio = boost::asio;
namespace qi = boost::spirit::qi;
using boost::asio::ip::tcp;
using boost::optional;

namespace tinch_pp {
namespace utils {

typedef std::pair<std::string, std::string> name_host_pair;

name_host_pair split_node_name(const std::string& node)
{
  // TODO: find a perfect pattern for a domain and specify the Erlang rules for a name!
  boost::regex pattern("([\\w\\.\\-]+)@([\\w\\.\\-]+)");
  boost::smatch m;

  if(!boost::regex_match(node, m, pattern)) {
    const std::string reason = "The given node = " + node + " is invalid. Check the Erlang docs for a valid format.";
    throw tinch_pp_exception(reason);
  }

  return make_pair(m[1].str(), m[2].str());
} 

std::string node_name(const std::string& node)
{
  return split_node_name(node).first;
}

std::string node_host(const std::string& node)
{
  return split_node_name(node).second;
}

// Used to generate diagnostic messages.
std::string to_printable_string(const msg_seq& msg)
{
   std::string s;
   msg_seq::const_iterator end = msg.end();
   for(msg_seq::const_iterator i = msg.begin(); i != end; ++i) {
      const std::string val = boost::lexical_cast<std::string>(static_cast<int>(*i));
      s += val + " ";
   }

   return s;
}

std::string to_printable_string(msg_seq_iter& first, const msg_seq_iter& last)
{
  const msg_seq m(first, last);

  return to_printable_string(m);
}

tcp::socket& connect_socket(asio::io_service& io_service, 
			    tcp::socket& socket, 
			    const std::string& host,
			    int port)
{
  tcp::resolver resolver(io_service);
  tcp::resolver::query query(host, boost::lexical_cast<std::string>(port));
  tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
  tcp::resolver::iterator end;
  
  boost::system::error_code error = boost::asio::error::host_not_found;
  while(error && endpoint_iterator != end) {
      socket.close();
      socket.connect(*endpoint_iterator++, error);
  }

  if (error) {
    const std::string reason = "Failed to connect to host = " + host + 
      ", port = " + boost::lexical_cast<std::string>(port);
    throw tinch_pp_exception(reason);
  }

  return socket;
}

// A digest is a (16 bytes) MD5 hash of [the Challenge (as text) concatenated
// with the cookie (as text).
msg_seq calculate_digest(boost::uint32_t challenge,
                         const std::string& cookie) {
  md5_state_t state;
  md5_byte_t digest[16] = {0};

  const std::string chall = boost::lexical_cast<std::string>(challenge);
  const std::string cs = cookie + chall;

  md5_init(&state);
  md5_append(&state, reinterpret_cast<const md5_byte_t*>(cs.c_str()), cs.size());
  md5_finish(&state, digest);

  return msg_seq(&digest[0], &digest[16]);
}

// msg_lexer implementation
//

msg_lexer::~msg_lexer()
{
}

bool msg_lexer::add(const msg_seq& msg)
{
  if(!incomplete.empty())
    incomplete.insert(incomplete.end(), msg.begin(), msg.end());
  else
    incomplete = msg;

  handle_new_condition();

  return has_complete_msg();
}

bool msg_lexer::has_complete_msg() const 
{ 
  return !msgs.empty(); 
}

optional<msg_seq> msg_lexer::next_message()
{
  optional<msg_seq> m;

  if(!msgs.empty()) {
    m = msgs.front();
    msgs.pop_front();
  }

  return m;
}

void msg_lexer::handle_new_condition()
{
  // A parse failure is not an error - we simply don't have the whole message yet.
  if(optional<size_t> msg_and_header_size = this->read_msg_size(incomplete)) {
    const size_t left = incomplete.size();

    if(left == *msg_and_header_size) {
      msgs.push_back(incomplete);
      incomplete.clear();
    } else if(left > *msg_and_header_size) {
      // concatenated messages
      extract_msg(*msg_and_header_size);
      strip_incomplete(*msg_and_header_size);
	
      // check if we got more completed messages.
      handle_new_condition();
    } else {
      // short read => do nothing until we get more date.
    }
  }
}

void msg_lexer::extract_msg(size_t msg_size)
{
  const msg_seq first_msg(&incomplete[0], &incomplete[msg_size]);
  msgs.push_back(first_msg);
}

void msg_lexer::strip_incomplete(size_t start)
{
  assert(incomplete.size() > start);
  const size_t left = incomplete.size() - start;

  // NOTE: it's OK to take the address one position past the storage as long as we don't dereference.
  const char* first = &incomplete[start];
  const msg_seq rest(first, first + left);
  incomplete = rest;
}

// Implementation of concrete lexers:
//

optional<size_t>  msg_lexer_handshake::read_msg_size(const msg_seq& msg) const
{
  optional<size_t> msg_and_header_size;
  boost::uint16_t msg_size = 0;
  msg_seq_citer f = msg.begin();
  msg_seq_citer l = msg.end();

  // A parse failure is not an error - we simply don't have the whole message yet.
  if(qi::parse(f, l, qi::big_word, msg_size))
    msg_and_header_size = 2 + msg_size;

  return msg_and_header_size;
}

// Used for a 4 bytes msg-length field.
optional<size_t>  msg_lexer_connected::read_msg_size(const msg_seq& msg) const
{
  optional<size_t> msg_and_header_size;
  boost::uint32_t msg_size = 0;
  msg_seq_citer f = msg.begin();
  msg_seq_citer l = msg.end();

  // A parse failure is not an error - we simply don't have the whole message yet.
  if(qi::parse(f, l, qi::big_dword, msg_size))
    msg_and_header_size = 4 + msg_size;

  return msg_and_header_size;
}

}
}
