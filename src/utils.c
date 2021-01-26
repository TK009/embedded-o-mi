#include <stdbool.h>
#include <time.h> // timestamp
#include "utils.h"


eomi_time getTimestamp() {
    return time(NULL);
}

int stringLen(const char * string) {
    const char * i = string;
    for (; *i != '\0'; ++i) {}
    return i - string;
}

void calcHashCodeC(const char c, PartialHash *h) {
    h->hash += c * h->mult;
    h->mult = (h->mult << 5u) - h->mult;
}

strhash calcHashCodeL(const char * string, int len) {
    uint hash = 0;
    uint mult = 1;
    for (const char * c = string + len - 1; c >= string; --c) {
        hash += *c * mult;
        mult = (mult << 5u) - mult;
    }
    return hash;
}

strhash calcHashCode(const char * string) {
    return calcHashCodeL(string, stringLen(string));
}


// returns true if found, false if not found. returnIx contains index of found
// or the index of the next closest element.
int binarySearch(const void* v_collection, const void* needle, int length, int elemSize,
        compareFunc compare, int* returnIx) {
    
    const char* collection = (const char*) v_collection;
    int L = 0, R = length;
    while (L < R) {
        int midIndex = (L + R) / 2; // + 1 to get ceiling
        const void * middle = collection+(midIndex*elemSize);
        int cmp = compare(middle, needle);

        if      (cmp < 0) {L = midIndex + 1;}
        else if (cmp > 0) {R = midIndex;}
        else {
            *returnIx = midIndex;
            return true;
        }
    }
    *returnIx = L;
    return false;
}


