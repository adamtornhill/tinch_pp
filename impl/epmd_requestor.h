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
#ifndef EPMD_REQUESTOR_H
#define EPMD_REQUESTOR_H

// Encapsulates the communication with EPMD (Erlang Port Mapper Daemon).
// EPMD requests are rare so, for simplicity, all requests are modelled 
// as syncrhonous.
#include "types.h"
#include <boost/asio.hpp>
#include <boost/utility.hpp>
#include <string>

namespace tinch_pp {

class epmd_requestor : boost::noncopyable
{
public:
  epmd_requestor(boost::asio::io_service& io_service, const std::string& epmd_host, port_number_type epmd_port);

  void connect();

  creation_number_type alive2_request(const std::string& node_name, port_number_type incoming_connections);

  port_number_type port_please2_request(const std::string& peer_node);

private:
  boost::asio::io_service& io_service;
  boost::asio::ip::tcp::socket epmd_socket;

  std::string epmd_host;
  port_number_type epmd_port;
};


}

#endif
