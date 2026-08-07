#ifndef MDNS_IPV4_SHIM_H
#define MDNS_IPV4_SHIM_H

#include <stdint.h>

/* ── Dummy IPv6 types (toolchain lacks IPv6 entirely) ───── */

#define AF_INET6 (-9999)

struct in6_addr {
    uint8_t s6_addr[16];
};

struct sockaddr_in6 {
    sa_family_t     sin6_family;
    in_port_t       sin6_port;
    uint32_t        sin6_flowinfo;
    struct in6_addr sin6_addr;
    uint32_t        sin6_scope_id;
};

static const struct in6_addr in6addr_any_dummy = { {0} };
#define in6addr_any in6addr_any_dummy

struct ipv6_mreq {
    struct in6_addr ipv6mr_multiaddr;
    unsigned int    ipv6mr_interface;
};

#define IPPROTO_IPV6          (-9999)
#define IPV6_JOIN_GROUP       (-9999)
#define IPV6_MULTICAST_HOPS   (-9999)
#define IPV6_MULTICAST_LOOP   (-9999)
#define IPV6_MULTICAST_IF     (-9999)

/* ── Include original header ────────────────────────────── */
#include "mdns.h"

/* ── Poison IPv6 functions so calls fail at compile time ── */
#define mdns_socket_open_ipv6    ERROR_IPv6_NOT_SUPPORTED_CALL_mdns_socket_open_ipv6
#define mdns_socket_setup_ipv6   ERROR_IPv6_NOT_SUPPORTED_CALL_mdns_socket_setup_ipv6
#define mdns_record_parse_aaaa   ERROR_IPv6_NOT_SUPPORTED_CALL_mdns_record_parse_aaaa

#endif