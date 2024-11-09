#pragma once

#include "byte_stream.hh"
#include "tcp_receiver_message.hh"
#include "tcp_sender_message.hh"

#include <cstdint>
#include <functional>
#include <queue>

class RetransmissionTimer {

private:
  bool is_active_ = false;
  uint64_t RTO_time_;
  uint64_t timer_ = 0;


public:
  explicit RetransmissionTimer(uint64_t initial_RTO_ms) : RTO_time_(initial_RTO_ms) {}

  auto is_active() const noexcept { 
    return is_active_; 
    }
  auto is_expired() const noexcept { 
    return is_active_ && timer_ >= RTO_time_; 
    }
  auto reset() noexcept { 
    timer_ = 0; 
    }
  auto exponential_backoff() noexcept { 
    RTO_time_ *= 2; 
    }
  auto reload(uint64_t initial_RTO_ms) noexcept { 
    RTO_time_ = initial_RTO_ms; reset(); 
    }
  auto start() noexcept { 
    is_active_ = true; reset(); 
    }
  auto stop() noexcept { 
    is_active_ = false; reset(); 
    }
  auto tick(uint64_t ms_since_last_tick) noexcept -> RetransmissionTimer& {
    if (is_active_) {
      timer_ += ms_since_last_tick;
    }
    return *this;
  }

};


class TCPSender
{
public:
  /* Construct TCP sender with given default Retransmission Timeout and possible ISN */
  TCPSender( ByteStream&& input, Wrap32 isn, uint64_t initial_RTO_ms )
    : input_( std::move( input ) ), isn_( isn ), initial_RTO_ms_( initial_RTO_ms ), timer_( initial_RTO_ms )
  {}

  /* Generate an empty TCPSenderMessage */
  TCPSenderMessage make_empty_message() const;

  /* Receive and process a TCPReceiverMessage from the peer's receiver */
  void receive( const TCPReceiverMessage& msg );

  /* Type of the `transmit` function that the push and tick methods can use to send messages */
  using TransmitFunction = std::function<void( const TCPSenderMessage& )>;

  /* Push bytes from the outbound stream */
  void push( const TransmitFunction& transmit );

  /* Time has passed by the given # of milliseconds since the last time the tick() method was called */
  void tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit );

  // Accessors
  uint64_t sequence_numbers_in_flight() const;  // How many sequence numbers are outstanding?
  uint64_t consecutive_retransmissions() const; // How many consecutive *re*transmissions have happened?
  Writer& writer() { return input_.writer(); }
  const Writer& writer() const { return input_.writer(); }

  // Access input stream reader, but const-only (can't read from outside)
  const Reader& reader() const { return input_.reader(); }

private:
  // Variables initialized in constructor
  ByteStream input_;
  Wrap32 isn_;
  uint64_t initial_RTO_ms_;
  uint64_t total_inflight_ {};
  uint64_t total_retransmission_ {};
  uint64_t next_abs_seqno_ {};
  uint64_t ack_abs_seqno_ {};
  uint16_t window_size_ { 1 };
  RetransmissionTimer timer_;
  bool FIN_sent_ {};
  bool SYN_sent_ {};
  std::queue<TCPSenderMessage> outstanding_message_ {};
};
