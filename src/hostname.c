#include <mocha/mocha.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

#include "logger.h"
#include "strutil.h"

#define MOUNTPOINT "xxuniquemagicslcxx"

#define SUFFIX ".local."
#define SUFFIX_LEN 7

#define IS_HEX(letter)                                                         \
    (((letter) >= '0' && (letter) <= '9') ||                                   \
     ((letter) >= 'a' && (letter) <= 'f') ||                                   \
     ((letter) >= 'A' && (letter) <= 'F'))

#define IS_ALPHANUMERIC(letter)                                                \
    (((letter) >= '0' && (letter) <= '9') ||                                   \
     ((letter) >= 'a' && (letter) <= 'z') ||                                   \
     ((letter) >= 'A' && (letter) <= 'Z'))

#define STARTS_WITH(haystack, needle)                                          \
    (strncmp((needle), (haystack), strlen(needle)) == 0)

static int mount_wrapper(const char *mount, const char *dev,
                         const char *mountTo) {
    int res = Mocha_MountFS(mount, dev, mountTo);
    if (res == MOCHA_RESULT_ALREADY_EXISTS) {
        res = Mocha_MountFS(mount, nullptr, mountTo);
    }
    if (res == MOCHA_RESULT_SUCCESS) {
        DEBUG_FUNCTION_LINE_INFO("Mounted %s:/", mount);
    } else {
        DEBUG_FUNCTION_LINE_ERR("Failed to mount %s: %s [%d]", mount,
                                Mocha_GetStatusStr(res), res);
    }
    return res;
}

static ssize_t read_slc_file(const char *path, char buf[], size_t size) {
    assert(STARTS_WITH(path, MOUNTPOINT ":/"));

    if (mount_wrapper(MOUNTPOINT, "/dev/slc01", "/vol/storage_slc01") !=
        MOCHA_RESULT_SUCCESS) {
        return -1;
    }

    FILE *file = fopen(path, "rb");
    if (!file) {
        Mocha_UnmountFS(MOUNTPOINT);
        return -2;
    }

    size_t len = fread(buf, 1, size, file);

    fclose(file);
    Mocha_UnmountFS(MOUNTPOINT);
    return len;
}

static const char *find_nickname_chunk(const char *bytes, size_t size) {
    const char *eof = bytes + size;
    const char *match = strutil_find(bytes, size, "<nickname");
    if (!match) {
        return NULL;
    }
    match = strutil_find(match, eof - match, ">");
    if (!match) {
        return NULL;
    }
    assert(match[0] == '>');

    while (!IS_HEX(match[0])) {
        match++;

        if (match >= eof) {
            return NULL;
        }
    }

    return match;
}

static int parse_hex(char c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    } else if (c >= 'A' && c <= 'F') {
        return 10 + c - 'A';
    } else if (c >= 'a' && c <= 'f') {
        return 10 + c - 'a';
    } else {
        return -1;
    }
}

static ssize_t get_nickname(char out[], size_t size) {
    assert(size >= 11);

    char file_buf[1024];
    int file_len = read_slc_file(MOUNTPOINT ":/sys/proc/prefs/wii_acct.xml",
                                 file_buf, sizeof(file_buf));
    if (file_len <= 0) {
        return -1;
    }

    const char *nickname_chunk = find_nickname_chunk(file_buf, file_len);
    if (!nickname_chunk) {
        return -1;
    }

    const char *eof = file_buf + file_len;
    int nick_len = eof - nickname_chunk;

    int c = 0;
    uint16_t scratch = 0;
    int i;
    for (i = 0; i < nick_len; i++) {
        int val = parse_hex(nickname_chunk[i]);
        if (val < 0) {
            break;
        }

        /* I think this is unncessary
         * 4 successive shifts should remove the original value

        if (i == 0) {
            scratch = val;
        }
        */

        scratch = scratch << 4 | val;

        if (i % 4 == 3) {
            if (scratch == 0) {
                break;
            }

            out[c++] = IS_ALPHANUMERIC(scratch) ? (char)scratch : '-';

            if (c >= size) {
                DEBUG_FUNCTION_LINE_ERR(
                    "Buffer out of space, out=%.*s scratch=%d c=%d, i=%d", c,
                    out, scratch, c, i);
                return -1;
            }
        }
    }

    if (i % 4 != 3) {
        DEBUG_FUNCTION_LINE_ERR(
            "I think this goofed, out=%.*s scratch=%d c=%d, i=%d", c, out,
            scratch, c, i);
        return -1;
    }

    return c;
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