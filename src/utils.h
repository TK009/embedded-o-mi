#ifndef UTILS_H
#define UTILS_H

typedef signed char schar;
typedef unsigned char uchar;
typedef unsigned int uint;

int stringLen(char * string);

typedef uint strhash;
strhash calcHashCodeL(char * string, int len);
strhash calcHashCode(char * string);

typedef schar (*compareFunc)(const void *, const void *);

int binarySearch(
        void* v_collection,
        void* needle,
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
