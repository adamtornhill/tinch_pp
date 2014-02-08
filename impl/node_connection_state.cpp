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
#include "node_connection_state.h"
#include "handshake_grammar.h"
#include "utils.h"
#include "node_connection_access.h"
#include "ext_message_builder.h"
#include "ctrl_msg_dispatcher.h"
#include "networker.h"
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>

using namespace tinch_pp;
using namespace boost;

// Implementation of the different states of a node_connection.
//
// The state objects are granted access to their connection through an access-inteface
// provided upon construction. The states use their access to trigger read and write
// operations. The node_connection performs the actual operations, ensuring that errors
// are properly handled and, upon read-operations, that at least one complete message
// has been received before the invoker (i.e. a state-object) is notified.
namespace {

struct connected : connection_state
{
  networker<connected> connection;
  ctrl_msg_dispatcher msg_dispatcher;

  connected(access_ptr access)
    : connection_state(access),
      connection(access, this),
      msg_dispatcher(access)
  {
  }

  void start_receiving()
  {
    connection.trigger_read(&connected::msg_received);
  }

  virtual void send(const msg_seq& payload, const tinch_pp::e_pid& self, const std::string& destination_name)
  {
    connection.trigger_write(build_reg_send_msg(payload, self, destination_name));
  }

  virtual void send(const msg_seq& payload, const tinch_pp::e_pid& destination_pid)
  {
    connection.trigger_write(build_send_msg(payload, destination_pid));
  }

  virtual void exit(const tinch_pp::e_pid& from_pid, const tinch_pp::e_pid& to_pid, const std::string& reason)
  {
    connection.trigger_write(build_exit_msg(from_pid, to_pid, reason));
  }

  virtual void exit2(const tinch_pp::e_pid& from_pid, const tinch_pp::e_pid& to_pid, const std::string& reason)
  {
    connection.trigger_write(build_exit2_msg(from_pid, to_pid, reason));
  }

  virtual void link(const tinch_pp::e_pid& from_pid, const tinch_pp::e_pid& to_pid)
  {
    connection.trigger_write(build_link_msg(from_pid, to_pid));
  }

  virtual void unlink(const tinch_pp::e_pid& from_pid, const tinch_pp::e_pid& to_pid)
  {
    connection.trigger_write(build_unlink_msg(from_pid, to_pid));
  }

  virtual void handle_io_error(const std::string& error) const
  {
    // Fire an exception and let the higher-level layers deal with it, probably
    // by dropping the connection.
     throw connection_io_error(error, access->peer_node_name());
  }

  bool is_tick(const msg_seq& msg) const
  {
    const size_t header_size = 4;
    const msg_seq tick(header_size, 0);

    return msg == tick;
  }

  void send_tock()
  {
    const size_t header_size = 4;
    const msg_seq tock(header_size, 0);

    connection.trigger_write(tock);
  }

  void msg_received(utils::msg_lexer& msgs)
  {
    try {
       msg_seq msg = *msgs.next_message();

       if(is_tick(msg))
         send_tock();
       else
         msg_dispatcher.dispatch(msg);

       connection.trigger_read(&connected::msg_received);
    } catch(const tinch_pp_exception& e) {
       // We want to ensure that the next read is triggered - perhaps we can fix the problem and continue.
       connection.trigger_read(&connected::msg_received);
       // throw e; TODO: How do I propagate this without destroying the ongoing write? Put the error in the mailboxes for this node?
    }
  }
};

// Take care: the transition to the connected state will dealloc our world right below our feets.
// Because we're running asynchronously, we must ensure things are done in the right order while
// still maintaining valid objects. The trick is to make a copy of the access smart-pointer.
void make_transition_to_connected(access_ptr access)
{
  boost::shared_ptr<connected> connected_state = access->change_state_to<connected>();
  access->handshake_complete();
  connected_state->start_receiving();
}

struct receiving_challenge_ack : connection_state
{
  networker<receiving_challenge_ack> connection;

  receiving_challenge_ack(access_ptr access)
    : connection_state(access),
      connection(access, this)
  {
    connection.trigger_read(&receiving_challenge_ack::challenge_ack_read);
  }

  void challenge_ack_read(utils::msg_lexer& msgs)
  {
    // Here's the place to check that B has calculated the correct digest.
    msg_seq ack_msg = *msgs.next_message();

    msg_seq digest;
    recv_challenge_ack_p ack_p;
    utils::parse(ack_msg, ack_p, digest);

    if(!is_correct(digest))
      throw tinch_pp_exception("Handshake failure with remote node - check your cookies!");

    make_transition_to_connected(access);
  }

private:
  bool is_correct(const msg_seq& digest)
  {
    const msg_seq expected_digest = utils::calculate_digest(access->own_challenge(), access->cookie());

    return expected_digest == digest;
  }
};

struct sending_challenge_ack : connection_state
{
  msg_seq challenge_ack;
  networker<sending_challenge_ack> connection;

  sending_challenge_ack(access_ptr access)
    : connection_state(access),
      connection(access, this)
  {
  }

  void send_ack(boost::uint32_t challenge)
  {
    msg_seq digest = utils::calculate_digest(challenge, access->cookie());
    send_challenge_ack_g challenge_ack_g;

    utils::generate(challenge_ack, challenge_ack_g, digest);
    connection.trigger_write(challenge_ack, &sending_challenge_ack::challenge_ack_written);
  }

  void challenge_ack_written()
  {
    make_transition_to_connected(access);
  }

};

struct sending_challenge_reply : connection_state
{
  msg_seq reply;
  networker<sending_challenge_reply> connection;

  sending_challenge_reply(access_ptr access)
    : connection_state(access),
      connection(access, this)
  {
  }

  void send(boost::uint32_t challenge_from_B, access_ptr access)
  {
    challenge_reply_attributes attributes(access->own_challenge(),
                                          utils::calculate_digest(challenge_from_B, access->cookie()));
    challenge_reply reply_g;
    utils::generate(reply, reply_g, attributes);

    connection.trigger_write(reply, &sending_challenge_reply::reply_sent);
  }

  void reply_sent()
  {
    access->change_state_to<receiving_challenge_ack>();
  }
};

struct waiting_for_challenge_reply : connection_state
{
  networker<waiting_for_challenge_reply> connection;

  waiting_for_challenge_reply(access_ptr access)
    : connection_state(access),
      connection(access, this)
  {
    connection.trigger_read(&waiting_for_challenge_reply::reply_received);
  }
private:
  void reply_received(utils::msg_lexer& read_msgs)
  {
    msg_seq msg = *read_msgs.next_message();
    challenge_reply_p reply_p;
    challenge_reply_attributes reply_attr;

    utils::parse(msg, reply_p, reply_attr);

    if(is_correct_digest(reply_attr))
      access->change_state_to<sending_challenge_ack>()->send_ack(reply_attr.challenge);
    else
      access->report_failure("A sent an erroneous digest (check the cookies on your nodes).");
  }

  bool is_correct_digest(const challenge_reply_attributes& reply_attr)
  {
    const msg_seq expected_digest = utils::calculate_digest(access->own_challenge(), access->cookie());

    return expected_digest == reply_attr.digest;
  }
};

struct receiving_challenge : connection_state
{
  networker<receiving_challenge> connection;

  receiving_challenge(access_ptr access)
    : connection_state(access),
      connection(access, this)
  {
  }

  void receive_challenge(utils::msg_lexer& read_msgs)
  {
    if(read_msgs.has_complete_msg())
      challenge_read(read_msgs);
    else
      connection.trigger_read(&receiving_challenge::challenge_read);
  }

  void challenge_read(utils::msg_lexer& read_msgs)
  {
    challenge_attributes attributes;
    recv_challenge challenge_p;
    utils::parse(*read_msgs.next_message(), challenge_p, attributes);

    access->change_state_to<sending_challenge_reply>()->send(attributes.challenge, access);
  }
};

struct waiting_for_status : connection_state
{
  networker<waiting_for_status> connection;

  waiting_for_status(access_ptr access)
    : connection_state(access),
      connection(access, this)
  {
    connection.trigger_read(&waiting_for_status::status_read);
  }

  void status_read(utils::msg_lexer& read_msgs)
  {
    std::string status;
    receive_status status_p;
    utils::parse(*read_msgs.next_message(), status_p, status);

    dispatch(status, read_msgs);
  }

  void dispatch(const std::string& status, utils::msg_lexer& read_msgs) const
  {
    const bool ok = (status == "ok") || (status == "ok_simultaneous");

    if(ok) {
      shared_ptr<receiving_challenge> new_state = access->change_state_to<receiving_challenge>();
      new_state->receive_challenge(read_msgs);
    } else {
      const std::string problem = "Handshake not OK, B sent status = " + status;
      access->report_failure(problem);
    }
  }
};

struct sending_status : connection_state
{
  msg_seq status_msg;
  msg_seq challenge_msg;
  networker<sending_status> connection;

  sending_status(access_ptr access)
    : connection_state(access),
      connection(access, this)
  {
    send_status();
    send_challenge();
  }

private:
  void send_status()
  {
    receive_status_g status_g;
    const serializable_string status("ok"); // TODO: handle other alternatives in the next version...

    utils::generate(status_msg, status_g, status);

    connection.trigger_write(status_msg);
  }

  void send_challenge()
  {
    send_challenge_g challenge_g;
    send_challenge_attributes challenge(access->own_challenge(), access->own_name());

    utils::generate(challenge_msg, challenge_g, challenge);

    connection.trigger_write(challenge_msg, &sending_status::challenge_written);
  }

  void challenge_written()
  {
    access->change_state_to<waiting_for_challenge_reply>();
  }
};

struct connected_tcp : connection_state
{
  msg_seq out_msg;
  networker<connected_tcp> connection;

  connected_tcp(access_ptr access)
    : connection_state(access),
      connection(access, this)
  {
  }

  virtual void initiate_handshake(const std::string& node_name)
  {
    send_name send_name_g;
    serializable_string name(node_name);

    utils::generate(out_msg, send_name_g, name);

    connection.trigger_write(out_msg, &connected_tcp::handshake_sent);
  }

  virtual void read_incoming_handshake()
  {
    connection.trigger_read(&connected_tcp::name_read);
  }

private:
  void handshake_sent()
  {
    access->change_state_to<waiting_for_status>();
  }

  // Another node has initiated the connection and sent its name.
  void name_read(utils::msg_lexer& read_msgs)
  {
    sent_name_type sent_name;
    send_name_p name_p;
    msg_seq msg = *read_msgs.next_message();

    utils::parse(msg, name_p, sent_name);

    if(supported_version(sent_name)) {
      access->got_peer_name(sent_name.name);
      access->change_state_to<sending_status>();
    } else {
      const std::string erroneous_version = "The connecting node " + sent_name.name +
	                                    " uses an unsupported version. We support version = " +
	                                    lexical_cast<std::string>(constants::supported_version) +
	                                    ", but the node has " +
	                                    lexical_cast<std::string>(sent_name.version0) + " -> " +
                                            lexical_cast<std::string>(sent_name.version1);
      access->report_failure(erroneous_version);
    }
  }

  bool supported_version(const sent_name_type& received) const
  {
    return (constants::supported_version >= received.version0) &&
           (constants::supported_version <= received.version1);
  }
};

}

namespace tinch_pp {

connection_state_ptr initial_state(access_ptr access)
{
  connection_state_ptr initial(new connected_tcp(access));

  return initial;
}

connection_state::~connection_state() {}

void connection_state::handle_io_error(const std::string& error) const
{
  // During handshake, which is the default case, we just report a failure.
  // The node_connector will react to that and terminate the connection attempt.
  access->report_failure(error);
}

}

