#include "network_interface.hh"
#include "address.hh"
#include "arp_message.hh"
#include "ethernet_frame.hh"
#include "ethernet_header.hh"
#include "exception.hh"
#include "ipv4_datagram.hh"
#include "parser.hh"

#include <iostream>
#include <utility>
#include <vector>
#include <ranges>
using namespace std;


// Constructor for NetworkInterface
NetworkInterface::NetworkInterface(string_view iface_name,
                                   shared_ptr<OutputPort> port,
                                   const EthernetAddress& eth_address,
                                   const Address& ip_addr)
    : name_(iface_name),
      port_(notnull("OutputPort", move(port))),
      ethernet_address_(eth_address),
      ip_address_(ip_addr) {
    cerr << "DEBUG: Interface initialized with Ethernet address " << to_string(eth_address) << " and IP address "
         << ip_addr.ip() << "\n";
}

// Sending an IPv4 datagram
void NetworkInterface::send_datagram(const InternetDatagram& dgram, const Address& next_hop) {
    const ip_as_number next_hop_ip = next_hop.ipv4_numeric();

    // Check ARP cache for the next hop's Ethernet address
    if (ARP_cache_.contains(next_hop_ip)) {
        const EthernetAddress& dest_eth = ARP_cache_[next_hop_ip].first;
        EthernetFrame out_frame = { {dest_eth, ethernet_address_, EthernetHeader::TYPE_IPv4}, serialize(dgram) };
        return transmit(out_frame);
    }

    // Queue the datagram for delivery after ARP resolution
    datagram_queue_[next_hop_ip].emplace_back(dgram);

    // Avoid sending redundant ARP requests
    if (arp_request_timers_.contains(next_hop_ip)) {
        return;
    }

    arp_request_timers_.emplace(next_hop_ip, Timer{});

    // Generate and broadcast an ARP request
    const ARPMessage new_arp_request = arp_request(ARPMessage::OPCODE_REQUEST, {}, next_hop_ip);
    EthernetFrame arp_frame = { {ETHERNET_BROADCAST, ethernet_address_, EthernetHeader::TYPE_ARP}, serialize(new_arp_request) };
    transmit(arp_frame);
}

// Receiving an Ethernet frame
void NetworkInterface::recv_frame(const EthernetFrame& frame) {
    const auto& frame_dst = frame.header.dst;

    // Ignore frames not addressed to this interface
    if (frame_dst != ethernet_address_ && frame_dst != ETHERNET_BROADCAST) {
        return;
    }

    // Handle IPv4 frames
    if (frame.header.type == EthernetHeader::TYPE_IPv4) {
        InternetDatagram ipv4_payload;
        if (parse(ipv4_payload, frame.payload)) {
            datagrams_received_.push(move(ipv4_payload));
        }
        return;
    }

    // Handle ARP frames
    if (frame.header.type == EthernetHeader::TYPE_ARP) {
        ARPMessage received_arp;
        if (!parse(received_arp, frame.payload)) {
            return;
        }

        const ip_as_number sender_ip = received_arp.sender_ip_address;
        const EthernetAddress sender_eth = received_arp.sender_ethernet_address;

        // Update ARP cache with the sender's information
        ARP_cache_[sender_ip] = {sender_eth, Timer{}};

        // Respond to ARP requests directed at this interface
        if (received_arp.opcode == ARPMessage::OPCODE_REQUEST &&
            received_arp.target_ip_address == ip_address_.ipv4_numeric()) {
            const ARPMessage reply = arp_request(ARPMessage::OPCODE_REPLY, sender_eth, sender_ip);
            EthernetFrame reply_frame = { {sender_eth, ethernet_address_, EthernetHeader::TYPE_ARP}, serialize(reply) };
            transmit(reply_frame);
        }

        // Process any queued datagrams awaiting this ARP resolution
        if (datagram_queue_.contains(sender_ip)) {
            for (const auto& queued_dgram : datagram_queue_[sender_ip]) {
                EthernetFrame ipv4_frame = { {sender_eth, ethernet_address_, EthernetHeader::TYPE_IPv4}, serialize(queued_dgram) };
                transmit(ipv4_frame);
            }
            datagram_queue_.erase(sender_ip);
            arp_request_timers_.erase(sender_ip);
        }
    }
}

// Periodic timer tick for cleanup
void NetworkInterface::tick(const size_t elapsed_ms) {
    // Remove expired ARP cache entries
    erase_if(ARP_cache_, [&](std::pair<const ip_as_number, std::pair<EthernetAddress, Timer>>& entry) -> bool {
        return entry.second.second.tick(elapsed_ms).expired(ARP_ENTRY_TTL_);
    });

    // Remove expired ARP request timers
    erase_if(arp_request_timers_, [&](std::pair<const ip_as_number, Timer>& entry) -> bool {
        return entry.second.tick(elapsed_ms).expired(ARP_RESPONSE_TTL_);
    });
}
// ARP request generation
auto NetworkInterface::arp_request(const uint16_t operation_code,
                                   const EthernetAddress& target_eth,
                                   const uint32_t target_ip) const noexcept -> ARPMessage {
    ARPMessage arp_msg;
    arp_msg.opcode = operation_code;
    arp_msg.sender_ethernet_address = ethernet_address_;
    arp_msg.target_ethernet_address = target_eth;
    arp_msg.sender_ip_address = ip_address_.ipv4_numeric();
    arp_msg.target_ip_address = target_ip;
      return arp_msg;
}

