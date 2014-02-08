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
#include "constants.h"

namespace {

  // Distribution capabilities.
  const boost::uint32_t extended_references  = 4;
  const boost::uint32_t extended_pid_ports   = 0x100;
  const boost::uint32_t support_bit_binaries = 0x400;

}

namespace tinch_pp {
namespace constants {

  const int node_type = 72; // hidden node (i.e. not native Erlang)
  const boost::uint16_t supported_version = 5; // R6B and later
  const boost::uint32_t capabilities = extended_references | extended_pid_ports | support_bit_binaries;

  const int magic_version = 131;
  const int pass_through = 112;

  const int ctrl_msg_link     = 1;
  const int ctrl_msg_unlink   = 4;
  const int ctrl_msg_send     = 2;
  const int ctrl_msg_reg_send = 6;
  const int ctrl_msg_exit     = 3;
  const int ctrl_msg_exit2    = 8;

  const int float_digits = 31;
}
}
