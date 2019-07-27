#ifndef UTILS_H
#define UTILS_H

typedef signed char schar;
typedef unsigned char uchar;
typedef unsigned int uint;

typedef unsigned int time;

time getTimestamp();

int stringLen(const char * string);

typedef uint strhash;
typedef struct PartialHash {
    strhash hash=0;
    strhash mult=1;
} PartialHash;

strhash calcHashCodeC(const char * string);
strhash calcHashCodeL(const char * string, int len);
strhash calcHashCode(const char * string);

typedef schar (*compareFunc)(const void *, const void *);

typedef enum {true, false} bool;

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

