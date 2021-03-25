#include <stdbool.h>
#include <time.h> // timestamp
#include <string.h>
#include "utils.h"


eomi_time getTimestamp() {
    return time(NULL);
}

int stringLen(const char * string) {
    const char * i = string;
    for (; *i != '\0'; ++i) {}
    return i - string;
}

// 
void calcHashCodeC(const char c, PartialHash *h) {
    h->hash ^= c;
    h->hash *= FNV_prime;
}

strhash calcHashCodeL(const char * string, int len) {
    uint hash = FNV_offset_basis;
    for (int i = 0; i < len; ++i){
      hash ^= (uint)string[i];
      hash *= FNV_prime;
    }
    return hash;
}

strhash calcHashCode(const char * string) {
    uint hash = FNV_offset_basis;
    char c;
    while ((c = *string++)) {
      hash ^= (uint)c;
      hash *= FNV_prime;
    }
    return hash;
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

#ifndef YIELD
void yield(){}
#endif


int bitCount(uint u) {
    uint uCount;

    uCount = u
        - ((u >> 1) & 033333333333)
        - ((u >> 2) & 011111111111);
    return ((uCount + (uCount >> 3)) & 030707070707) % 63;
}

int first1Bit(int i) {
    return bitCount((i&(-i))-1);
}
int first0Bit(int i) {
    i = ~i;
    return bitCount((i&(-i))-1);
}
schar int_compare(const int *a, const int *b) { return (*a < *b)? -1 : ((*a > *b)? 1 : 0); }
schar uint_compare(const uint *a, const uint *b) { return (*a < *b)? -1 : ((*a > *b)? 1 : 0); }

HString* HString_init(HString* hs, char* str){ if (hs) *hs = (HString){str, calcHashCode(str)}; return hs; }
schar HString_compare(const HString *a, const HString *b) {
    return (a->hash < b->hash)? -1 : ((a->hash > b->hash)? 1 : strcmp(a->value, b->value));
}

OString * OString_init(OString* self, char * string){
    if (self) *self = (OString){string, 0, 0, 0};
    return self;
}
strhash OString_hash(OString* self){
    if (self->flags & OS_hasHash) return self->hash;
    self->hash = calcHashCodeL(self->data, OString_len(self));
    self->flags |= OS_hasHash;
    return self->hash;
}
strhash OString_len(OString* self){
    if (self->flags & OS_hasLen) return self->length;
    self->length = strlen(self->data);
    self->flags |= OS_hasLen;
    return self->length;
}

