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
#ifndef ERL_UTILS_H
#define ERL_UTILS_H

#include "types.h"
#include "tinch_pp/exceptions.h"
#include <boost/asio.hpp>
#include <boost/spirit/include/qi_parse.hpp>
#include <boost/spirit/include/karma_generate.hpp>
#include <boost/optional.hpp>
#include <string>
#include <iterator>
#include <deque>

namespace tinch_pp {
namespace utils {

std::string node_name(const std::string& node);

std::string node_host(const std::string& node);

// Used to generate diagnostic messages.
std::string to_printable_string(const msg_seq& msg);

std::string to_printable_string(msg_seq_iter& first, const msg_seq_iter& last);

boost::asio::ip::tcp::socket& connect_socket(boost::asio::io_service& io_service, 
					     boost::asio::ip::tcp::socket& socket, 
					     const std::string& host,
					     int port);

template <typename S, typename P, typename T>
void parse(S& stream, P& p, T& attr)
{
    typename S::iterator f = stream.begin();
    typename S::iterator l = stream.end();

    if(!boost::spirit::qi::parse(f, l, p, attr)) {
      throw tinch_pp_exception("Parse failure!"); // TODO: more info - let qi throw!
    }
}

template <typename S, typename G, typename T>
void generate(S& stream, G& g, const T& param)
{
  std::back_insert_iterator<S> out(stream);

  if(!boost::spirit::karma::generate(out, g, param)) {
    throw tinch_pp_exception("Generate failed!"); // TODO: more info - let karma throw!
  }
}

// A digest is a (16 bytes) MD5 hash of [the Challenge (as text) concatenated
// with the cookie (as text).
msg_seq calculate_digest(boost::uint32_t challenge,
                         const std::string& cookie);

// TCP/IP is a stream based protocol. We need a way to split concatenated messages, but 
// also handle short reads. The msg_lexer is our friend although it only guarantees  
// basic exception safety.
class msg_lexer : boost::noncopyable
{
public:
  bool add(const msg_seq& msg);

  bool has_complete_msg() const;

  boost::optional<msg_seq> next_message();

protected:
  ~msg_lexer();

private:
  void handle_new_condition();
  void extract_msg(size_t msg_size);
  void strip_incomplete(size_t start);

  // Erlang uses different message formats for handshake and between connected nodes.
  // For the msg_lexer, what differs is the size of the message-length field.
  // We abstract is away with a TEMPLATE METHOD (design pattern).
  virtual boost::optional<size_t> read_msg_size(const msg_seq& msg) const = 0;

  msg_seq incomplete;
  std::deque<msg_seq> msgs;
};

// Used for a 2 bytes msg-length field.
class msg_lexer_handshake : public msg_lexer
{
  virtual boost::optional<size_t> read_msg_size(const msg_seq& msg) const;
};

// Used for a 4 bytes msg-length field.
class msg_lexer_connected : public msg_lexer
{
  virtual boost::optional<size_t> read_msg_size(const msg_seq& msg) const;
};

}
}

#endif
