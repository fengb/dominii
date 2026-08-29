#include <string.h>

#include "strutil.h"

const char *strutil_find(const char *haystack, size_t size,
                         const char *needle) {
    unsigned int needle_size = strlen(needle);
    for (int i = 0; i < size - needle_size; i++) {
        if (memcmp(haystack + i, needle, needle_size) == 0) {
            return haystack + i;
        }
    }
    return NULL;
}