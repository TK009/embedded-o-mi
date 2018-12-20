#include <stdbool.h>
#include "utils.h"

int stringLen(char * string) {
    char * i = string;
    for (; *i != '\0'; ++i) {}
    return i - string;
}

strhash calcHashCodeL(char * string, int len) {
    uint hash = 0;
    uint mult = 1;
    for (char * c = string + len - 1; c >= string; --c) {
        hash += *c * mult;
        mult = (mult << 5u) - mult;
    }
    return hash;
}

strhash calcHashCode(char * string) {
    return calcHashCodeL(string, stringLen(string));
}

//void qsort(void *base, size_t nmemb, size_t size,
//                          int (*compar)(const void *, const void *));
//
//       void qsort_r(void *base, size_t nmemb, size_t size,
//                                 int (*compar)(const void *, const void *, void *),
//                                                   void *arg);

// returns true if found, false if not found. returnIx contains index of found
// or the index of the next closest element.
int binarySearch(void* v_collection, void* needle, int length, int elemSize,
        compareFunc compare, int* returnIx) {
    
    char* collection = (char*) v_collection;
    int L = 0, R = length;
    while (L < R) {
        int midIndex = (L + R) / 2; // + 1 to get ceiling
        void * middle = collection+(midIndex*elemSize);
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


