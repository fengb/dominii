#include <arpa/inet.h>
#include <coreinit/thread.h>
#include <coreinit/time.h>
#include <errno.h>
#include <nn/ac.h>

#include "logger.h"
#include "mdns_ipv4_shim.h"

static bool s_engine_running = false;

static struct sockaddr_in s_local_ip;

static const mdns_string_t MACHINE_NAME = {
    .str = "wiiu.local.",
    .length = 11,
};

static int query_callback(int sock, const struct sockaddr *from, size_t addrlen,
                          mdns_entry_type_t entry, uint16_t query_id,
                          uint16_t rtype, uint16_t rclass, uint32_t ttl,
                          const void *data, size_t size, size_t name_offset,
                          size_t name_length, size_t record_offset,
                          size_t record_length, void *user_data) {
    DEBUG_FUNCTION_LINE_INFO("Received mDNS packet");

    // We only care about incoming questions asking for A records
    if (entry != MDNS_ENTRYTYPE_QUESTION || rtype != MDNS_RECORDTYPE_A) {
        return 0;
    }

    char scratch_buf[256];
    mdns_string_t queried_name = mdns_string_extract(
        data, size, &name_offset, scratch_buf, sizeof(scratch_buf));

    if (queried_name.length != MACHINE_NAME.length &&
        strncasecmp(queried_name.str, MACHINE_NAME.str, MACHINE_NAME.length) !=
            0) {
        return 0;
    }

    mdns_record_t answer = {.name = MACHINE_NAME,
                            .type = MDNS_RECORDTYPE_A,
                            .rclass = 0,
                            .ttl = 120,
                            .data = {.a = {.addr = s_local_ip}}};

    if (rclass & MDNS_UNICAST_RESPONSE) {
        mdns_query_answer_unicast(sock, from, addrlen, scratch_buf,
                                  sizeof(scratch_buf), query_id, rtype,
                                  queried_name.str, queried_name.length, answer,
                                  NULL, 0, NULL, 0);
    } else {
        mdns_query_answer_multicast(sock, scratch_buf, sizeof(scratch_buf),
                                    answer, NULL, 0, NULL, 0);
    }

    return 0;
}

static int engine_connect() {
    if (!NNResult_IsSuccess(ACInitialize())) {
        return -1;
    }

    uint32_t ip_address;
    if (!NNResult_IsSuccess(ACGetAssignedAddress(&ip_address))) {
        return -2;
    }

    DEBUG_FUNCTION_LINE_INFO(
        "Address: %u.%u.%u.%u:%d\n", (ip_address >> 24) & 0xff,
        (ip_address >> 16) & 0xff, (ip_address >> 8) & 0xff,
        (ip_address >> 0) & 0xff, MDNS_PORT);

    s_local_ip.sin_family = AF_INET;
    s_local_ip.sin_port = htons(MDNS_PORT);
    s_local_ip.sin_addr.s_addr = ip_address;

    int sock = mdns_socket_open_ipv4(&s_local_ip);
    if (sock < 0) {
        DEBUG_FUNCTION_LINE_ERR("Failed to open IPv4 mDNS socket\n");
        return -3;
    }

    return sock;
}

static int engine_serve(int sock) {
    DEBUG_FUNCTION_LINE_INFO("Listening for mDNS traffic on port 5353...");

    while (s_engine_running) {
        char recv_buf[1024]; // Max supported size is tested to be 1478

        // mdns_socket_listen handles recvfrom and triggers the callback
        size_t records = mdns_socket_listen(sock, recv_buf, sizeof(recv_buf),
                                            query_callback, NULL);
        if (records > 0) {
            continue;
        }

        switch (errno) {
        case EWOULDBLOCK:
            // case EAGAIN: // dupe of EWOULDBLOCK within Wii U
            // Timeout / no data yet
            OSSleepTicks(OSMillisecondsToTicks(50));
            continue;
        default:
            // Handle socket error or shutdown
            return -1;
        }
    }

    return 0;
}

int engine_start(int argc, const char **argv) {
    s_engine_running = true;
    unsigned int backoff = 0;
    while (s_engine_running) {
        int sock = engine_connect();
        if (sock < 0) {
            OSSleepTicks(OSSecondsToTicks(1ULL << backoff));
            backoff += (backoff < 6);
            continue;
        }

        int res = engine_serve(sock);
        mdns_socket_close(sock);
        backoff = 0;
        OSSleepTicks(OSSecondsToTicks(1ULL));

        switch (res) {
        case 0:
            break;

            // handle errors

        default:
            // Unhandled errors;
        }
    }

    s_engine_running = false;
    return 0;
}

int engine_stop() {
    s_engine_running = false;
    return 0;
}