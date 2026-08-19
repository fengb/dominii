#include <arpa/inet.h>
#include <nn/ac.h>

#include "mdns_ipv4_shim.h"

#include "logger.h"

static bool s_engine_running = false;

static int sock;
static struct sockaddr_in s_local_ip;

static int
query_callback(int sock, const struct sockaddr* from, size_t addrlen,
               mdns_entry_type_t entry, uint16_t query_id, uint16_t rtype,
               uint16_t rclass, uint32_t ttl, const void* data, size_t size,
               size_t name_offset, size_t name_length, size_t record_offset,
               size_t record_length, void* user_data) {
    // We only care about incoming questions asking for A records
    if (entry != MDNS_ENTRYTYPE_QUESTION || rtype != MDNS_RECORDTYPE_A) {
        return 0;
    }
    
    char name_buf[256];
    mdns_string_t queried_name = mdns_string_extract(data, size, &name_offset, name_buf, sizeof(name_buf));
    
    if (queried_name.length == 10 && strncasecmp(queried_name.str, "wiiu.local", 10) == 0) {
        mdns_record_t answer = {
            .name = queried_name,
            .type = MDNS_RECORDTYPE_A,
            .rclass = 0,
            .ttl = 120,
            .data = {
                .a = {
                    .addr = s_local_ip
                }
            }
        };
        
        char send_buf[1024];

        if (rclass & MDNS_UNICAST_RESPONSE) {
            mdns_query_answer_unicast(sock, from, addrlen, send_buf, sizeof(send_buf),
                                      query_id, rtype, queried_name.str, queried_name.length,
                                      answer, NULL, 0, NULL, 0);
        } else {
            mdns_query_answer_multicast(sock, send_buf, sizeof(send_buf), answer, NULL, 0, NULL, 0);
        }
    }

    DEBUG_FUNCTION_LINE_INFO("Received mDNS query/packet");
    return 0;
}

int engine_start(int argc, const char **argv) {
    if (NNResult_IsSuccess(ACInitialize())) {
        uint32_t ip_address;
        if (NNResult_IsSuccess(ACGetAssignedAddress(&ip_address))) {
            s_local_ip.sin_family = AF_INET,
            s_local_ip.sin_addr.s_addr = htonl(INADDR_ANY),
            s_local_ip.sin_addr.s_addr = ip_address;
        }
    }

    sock = mdns_socket_open_ipv4(&s_local_ip);
    if (sock < 0) {
        DEBUG_FUNCTION_LINE_ERR("Failed to open IPv4 mDNS socket");
        return -1;
    }
    
    // Switch socket from non-blocking (set by library) to blocking mode
    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags & ~O_NONBLOCK);
    
    
    DEBUG_FUNCTION_LINE_INFO("Listening for mDNS traffic on port 5353...");
    
    s_engine_running = true;
    while (s_engine_running) {
        char recv_buf[2048];

        // mdns_socket_listen handles recvfrom and triggers the callback
        int res = mdns_socket_listen(sock, recv_buf, sizeof(recv_buf), query_callback, NULL);
        if (res < 0) {
            // Handle socket error or shutdown
            break;
        }
    }
    
    mdns_socket_close(sock);
    sock = 0;
    s_engine_running = false;
    
    return 0;
}

int engine_stop() {
    s_engine_running = false;
    return 0;
}