#pragma once

#include "address.hh"
#include "arp_message.hh"
#include "ethernet_frame.hh"
#include "ethernet_header.hh"
#include "ipv4_datagram.hh"

#include <cstddef>
#include <cstdint>
#include <queue>
#include <unordered_map>
#include <utility>
#include <vector>


class NetworkInterface
{
public:
  // An abstraction for the physical output port where the NetworkInterface sends Ethernet frames
  class OutputPort
  {
  public:
    virtual void transmit( const NetworkInterface& sender, const EthernetFrame& frame ) = 0;
    virtual ~OutputPort() = default;
  };


  NetworkInterface( std::string_view name,
                    std::shared_ptr<OutputPort> port,
                    const EthernetAddress& ethernet_address,
                    const Address& ip_address );


  void send_datagram( const InternetDatagram& dgram, const Address& next_hop );


  void recv_frame( const EthernetFrame& frame );

  void tick( size_t ms_since_last_tick );

  const std::string& name() const { return name_; }
  const OutputPort& output() const { return *port_; }
  OutputPort& output() { return *port_; }
  std::queue<InternetDatagram>& datagrams_received() { return datagrams_received_; }

private:
  std::string name_;

  std::shared_ptr<OutputPort> port_;
  void transmit( const EthernetFrame& frame ) const { port_->transmit( *this, frame ); }

  EthernetAddress ethernet_address_;

  Address ip_address_;

  std::queue<InternetDatagram> datagrams_received_ {};
  
  //structure using ip_as_number to identify data.
  using ip_as_number = decltype( ip_address_.ipv4_numeric() );
  
  std::unordered_map<ip_as_number, std::vector<InternetDatagram>> datagram_queue_   {};

//timer class to handle ARP expirations
class Timer {
public:
    size_t _ms{};

    constexpr Timer& tick(const size_t& ms_since_last_tick) {
        _ms += ms_since_last_tick;
        return *this;
    }

    constexpr bool expired(const size_t& TTL_ms) const {
        return _ms >= TTL_ms;
    }
};


  auto arp_request( uint16_t, const EthernetAddress&, uint32_t ) const noexcept -> ARPMessage;

  static constexpr size_t ARP_RESPONSE_TTL_ { 5'000 };

  static constexpr size_t ARP_ENTRY_TTL_ { 30'000 };

  std::unordered_map<ip_as_number, std::pair<EthernetAddress, Timer>> ARP_cache_{};  

  std::unordered_map<ip_as_number, Timer> arp_request_timers_ {};
 
};
