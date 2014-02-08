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
#ifndef RPC_H
#define RPC_H

#include "erlang_value_types.h"
#include "erl_list.h"
#include "impl/types.h"

#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>
#include <utility>
#include <string>

namespace tinch_pp {

typedef std::pair<std::string, std::string> module_and_function_type;

/// The arguments may be of different types. Thus, we need to 
/// heap allocate them in order to provide polymorphic behaviour.
/// Implementation note: I don't like that requirement; investigate 
/// a cleaner way (intrusive containers)?
typedef erl::list<erl::object_ptr> rpc_argument_type;

class node;
typedef boost::shared_ptr<node> node_ptr;

class mailbox;
typedef boost::shared_ptr<mailbox> mailbox_ptr;

class matchable;
typedef boost::shared_ptr<matchable> matchable_ptr;

/// This is a convenience class for remote procedure calls.
/// A remote procedure call is a method to call a function on a remote node and collect the answer.
///
/// NOTE: The rpc protocol expects the remote node to be an Erlang node. If it's not, invoking an 
/// rpc has unpredictable results (i.e. do NOT do it unless you really, really know what you do).
class rpc : boost::noncopyable
{
public:
   rpc(node_ptr own_node);

   /// Invokes the given remote function on the remote node.
   /// This call blocks until a reply is received.
   matchable_ptr blocking_rpc(const std::string& remote_node,
                              const module_and_function_type& remote_fn,
                              const rpc_argument_type& arguments);

   /// Invokes the given remote function on the remote node.
   /// This call blocks until a reply is received or until the given time has passed.
   /// In case of a time-out, an tinch_pp::receive_tmo_exception is thrown.
   matchable_ptr blocking_rpc(const std::string& remote_node,
                              const module_and_function_type& remote_fn,
                              const rpc_argument_type& arguments,
                              time_type_sec tmo);

   // TODO: implement on a rainy day...may be a bit like agents in Clojure?

   //matchable_ptr async_rpc(const std::string& remote_node,
                              //const module_and_function_type& remote_fn,
                              //const rpc_argument_type& arguments,
                              //const rpc_callback& calback);

   //matchable_ptr async_rpc(const std::string& remote_node,
                              //const module_and_function_type& remote_fn,
                              //const rpc_argument_type& arguments,
                              //const rpc_callback& calback,
                              //time_type tmo);

private:
   mailbox_ptr mbox;
};

}

#endif
