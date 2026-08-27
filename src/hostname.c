#include <string.h>
#include <sys/types.h>

#define SUFFIX ".local."
#define SUFFIX_LEN 7

static ssize_t get_nickname(char buf[], size_t size) {
    if (size < 4) {
        return -1;
    }

    memcpy(buf, "wiiu", 4);
    return 4;
}

ssize_t hostname_load(char buf[], size_t size) {
    ssize_t nick_len = get_nickname(buf, size - SUFFIX_LEN);
    if (nick_len <= 0) {
        return nick_len;
    }

    ssize_t total_len = nick_len + SUFFIX_LEN;

    /* This check should be redundant with the adjusted size from above

    if (total_len >= size) {
        return -1;
    }
    */

    memcpy(&buf[nick_len], SUFFIX, SUFFIX_LEN);
    return total_len;
}