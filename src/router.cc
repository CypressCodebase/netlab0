#include "router.hh"

#include <iostream>
#include <limits>
#include <algorithm>

using namespace std;

// route_prefix: The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
// prefix_length: For this route to be applicable, how many high-order (most-significant) bits of
//    the route_prefix will need to match the corresponding bits of the datagram's destination address?
// next_hop: The IP address of the next hop. Will be empty if the network is directly attached to the router (in
//    which case, the next hop address should be the datagram's final destination).
// interface_num: The index of the interface to send the datagram out on.
void Router::add_route( const uint32_t route_prefix,
                        const uint8_t prefix_length,
                        const optional<Address> next_hop,
                        const size_t interface_num )
{
  cerr << "DEBUG: adding route " << Address::from_ipv4_numeric( route_prefix ).ip() << "/"
       << static_cast<int>( prefix_length ) << " => " << ( next_hop.has_value() ? next_hop->ip() : "(direct)" )
       << " on interface " << interface_num << "\n";

  // Your code here.
  routing_table_.insert({interface_num,{ route_prefix,prefix_length, next_hop}});    
}

// Go through all the interfaces, and route every incoming datagram to its proper outgoing interface.
void Router::route() {
  // Iterate through all interfaces
  for (size_t interface_id = 0; interface_id < _interfaces.size(); ++interface_id) {
    auto& interface = _interfaces[interface_id];

    // Process all received datagrams
    while (!interface->datagrams_received().empty()) {
      auto datagram = interface->datagrams_received().front();
      interface->datagrams_received().pop();

      // Initialize variables for route matching
      uint8_t longest_prefix = 0;
      optional<Address> next_hop;
      size_t matched_interface = 0;
      bool route_found = false;

      // Search for the best route (longest prefix match)
      for (const auto& [interface_num, route] : routing_table_) {
        uint32_t route_prefix = get<0>(route);
        uint8_t prefix_length = get<1>(route);
        uint32_t destination_ip = datagram.header.dst;

        // Compute the prefix match
        uint32_t xor_result = route_prefix ^ destination_ip;
        uint8_t match_length = std::min(static_cast<uint8_t>(countl_zero(xor_result)), prefix_length);

        // Check if this route matches and is better than the current best match
        if (match_length == prefix_length && match_length >= longest_prefix) {
          longest_prefix = match_length;
          next_hop = get<2>(route);
          matched_interface = interface_num;
          route_found = true;
        }
      }

      // If no route is found, drop the datagram
      if (!route_found) {
        continue;
      }

      // Process TTL and route the datagram
      if (datagram.header.ttl > 1) {
        datagram.header.ttl--;
        datagram.header.compute_checksum();

        // Send the datagram to the next hop or directly to the destination
        if (next_hop.has_value()) {
          _interfaces[matched_interface]->send_datagram(datagram, next_hop.value());
        } else {
          _interfaces[matched_interface]->send_datagram(datagram, Address::from_ipv4_numeric(datagram.header.dst));
        }
      }
    }
  }
}

