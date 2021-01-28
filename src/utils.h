#ifndef UTILS_H
#define UTILS_H

typedef signed char schar;
typedef unsigned char uchar;
typedef unsigned int uint;

typedef unsigned int eomi_time;

eomi_time getTimestamp();

int stringLen(const char * string);

#define FNV_prime 16777619
#define FNV_offset_basis 2166136261

typedef uint strhash;
typedef struct PartialHash {
    strhash hash;
} PartialHash;
static const PartialHash emptyPartialHash = {FNV_offset_basis};

//strhash calcHashCodeC(const char * string);
void calcHashCodeC(const char c, PartialHash *h);
strhash calcHashCodeL(const char * string, int len);
strhash calcHashCode(const char * string);

typedef schar (*compareFunc)(const void *, const void *);

#undef true
#undef false
#undef bool
typedef enum {false=0, true=1} bool;

int binarySearch(
        const void* v_collection,
        const void* needle,
        int length,
        int elemSize,
        compareFunc compare,
        int* returnIx);

#define max(a,b) \
    ({ __typeof__ (a) _a = (a); \
     __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })

#define min(a,b) \
    ({ __typeof__ (a) _a = (a); \
     __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })

#endif

#define Noop (void)0

