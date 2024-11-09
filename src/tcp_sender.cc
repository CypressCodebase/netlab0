#include "tcp_sender.hh"
#include "tcp_config.hh"
#include "wrapping_integers.hh"
#include <cstddef>
#include <cstdint>
#include <algorithm>
#include <string_view>
#include <utility>

using namespace std;

uint64_t TCPSender::sequence_numbers_in_flight() const
{
  return total_inflight_;
}

uint64_t TCPSender::consecutive_retransmissions() const
{
  return total_retransmission_;
}

void TCPSender::push(const TransmitFunction& transmit) {
    uint64_t effective_window = window_size_ == 0 ? 1 : window_size_;

    bool done = false;
    while (!done) {
        if (total_inflight_ >= effective_window || FIN_sent_) {
            break; // Exit if there's no space left in the window or FIN has already been sent
        }

        auto segment = make_empty_message();

        // Set SYN flag for the first message only
        if (!SYN_sent_) {
            SYN_sent_ = true;
            segment.SYN = true;
        }

        // Calculate how much payload can be added to this segment
        uint64_t remaining_space = effective_window - total_inflight_;
        size_t max_payload = min(TCPConfig::MAX_PAYLOAD_SIZE, remaining_space - segment.sequence_length());

        string& payload = segment.payload;

        // Populate payload with data from the reader within the allowed size
        while (payload.size() < max_payload && reader().bytes_buffered() > 0) {
            string_view data = reader().peek();
            data = data.substr(0, max_payload - payload.size());
            payload += data;
            input_.reader().pop(data.size());
        }

        // Assign FIN flag if at end of stream and space allows
        if (!FIN_sent_ && reader().is_finished() && remaining_space > segment.sequence_length()) {
            segment.FIN = true;
            FIN_sent_ = true;
        }

        // Stop loop if segment is empty (no SYN, FIN, or payload)
        if (segment.sequence_length() == 0) {
            done = true;
            continue;
        }

        // Send the segment, start timer if inactive, update counters
        transmit(segment);

        if (!timer_.is_active()) {
            timer_.start();
        }

        next_abs_seqno_ += segment.sequence_length();
        total_inflight_ += segment.sequence_length();
        outstanding_message_.emplace(move(segment));
    }
}


TCPSenderMessage TCPSender::make_empty_message() const
{
  return { Wrap32::wrap( next_abs_seqno_, isn_ ), false, {}, false, input_.has_error() };
}

void TCPSender::receive(const TCPReceiverMessage& msg) {

    bool acknowledged = false;
    // Early exit on error or RST flag
    if (input_.has_error() || msg.RST) {
        if (msg.RST) {
            input_.set_error();
        }
        return;
    }

    // Update window size and check for acknowledgment presence
    window_size_ = msg.window_size;
    if (!msg.ackno.has_value()) {
        return;
    }

    // Unwrap the received acknowledgment sequence number and validate it
    uint64_t ack_seqno_received = msg.ackno->unwrap(isn_, next_abs_seqno_);
    if (ack_seqno_received > next_abs_seqno_) {
        return;
    }



    // Process all outstanding messages until we reach an unacknowledged one
    while (!outstanding_message_.empty()) {
        const auto& current_message = outstanding_message_.front();
        
        // Check if the current message is fully acknowledged
        if (ack_abs_seqno_ + current_message.sequence_length() > ack_seqno_received) {
            break;
        }

        // Update state based on acknowledgment and remove the message
        acknowledged = true;
        ack_abs_seqno_ += current_message.sequence_length();
        total_inflight_ -= current_message.sequence_length();
        outstanding_message_.pop();
    }

    // Reset the retransmission count and adjust the timer if needed
    if (acknowledged) {
        total_retransmission_ = 0;
        timer_.reload(initial_RTO_ms_);
        if (outstanding_message_.empty()) {
            timer_.stop();
        } else {
            timer_.start();
        }
    }
}


void TCPSender::tick(uint64_t ms_since_last_tick, const TransmitFunction& transmit) {
    // Update the timer and check if it has expired
    auto& timer_status = timer_.tick(ms_since_last_tick);
    if (!timer_status.is_expired() || outstanding_message_.empty()) {
        return;
    }

    // Retransmit the first outstanding message
    transmit(outstanding_message_.front());

    // If the window size is non-zero, apply exponential backoff
    if (window_size_ > 0) {
        ++total_retransmission_;
        timer_.exponential_backoff();
    }

    // Reset the timer after retransmission
    timer_.reset();
}
