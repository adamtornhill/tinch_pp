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
#include "tinch_pp/exceptions.h"

using namespace tinch_pp;

tinch_pp_exception::tinch_pp_exception(const std::string& a_reason)
  : reason("tinch++ exception: " + a_reason)
{
}

tinch_pp_exception::~tinch_pp_exception() throw()
{
}

const char* tinch_pp_exception::what() const throw()
{
  return reason.c_str();
}

connection_io_error::connection_io_error(const std::string& reason, 
					 const std::string& a_node_name)
  : tinch_pp_exception(reason),
    node_name(a_node_name)
{
}

connection_io_error::~connection_io_error() throw()
{
}

std::string connection_io_error::node() const
{
  return node_name;
}

mailbox_receive_tmo::mailbox_receive_tmo()
  : tinch_pp_exception("Timed out (user requested) while waiting for a message to arrive.")
{
}

mailbox_receive_tmo::~mailbox_receive_tmo() throw()
{
}

link_broken::link_broken(const std::string& reason, const e_pid& pid)
  : tinch_pp_exception("Link to remote process broken."),
    reason_(reason),
    pid_(pid)
{
}

link_broken::~link_broken() throw() 
{
}

encoding_error::encoding_error(const std::string& failed_term, const std::string& details)
  : tinch_pp_exception("Failed to encode the term " + failed_term + ". Reason: " + details),
    term_(failed_term)
{
}

encoding_error::~encoding_error() throw()
{
}

