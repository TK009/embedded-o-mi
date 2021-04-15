#ifndef UTILS_H
#define UTILS_H

#include <stdlib.h> // size_t

# define packed __attribute__((packed))

typedef signed char schar;
typedef unsigned char uchar;
typedef unsigned int uint;
typedef unsigned short ushort;
typedef long long int64;

typedef unsigned int eomi_time;

eomi_time getTimestamp();

int stringLen(const char * string);


#define FNV_prime 16777619
#define FNV_offset_basis -2128831035 //2166136261

typedef uint strhash;
typedef struct PartialHash {
    strhash hash;
} PartialHash;
extern const PartialHash emptyPartialHash;

//strhash calcHashCodeC(const char * string);
void calcHashCodeC(const char c, PartialHash *h);
strhash calcHashCodeL(const char * string, int len);
strhash calcHashCode(const char * string);

typedef schar (*compareFunc)(const void *, const void *);
schar int_compare(const int *a, const int *b);
schar uint_compare(const uint *a, const uint *b);

// Typed bool
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


#define Noop (void)0

void yield();

int bitCount(uint u);
int first1Bit(int i);
int first0Bit(int i);

typedef const struct Allocator{
    void* (*malloc)(size_t size);
    void* (*calloc)(size_t n, size_t size);
    void* (*realloc)(void *ptr, size_t size);
    void (*free)(void *ptr);
    void (*nullFree)(void **ptr);
} Allocator;

extern Allocator stdAllocator;

#define _NEW_ARGS(T, ...) T ## _init(malloc(sizeof(T)), __VA_ARGS__)
#define _NEW(T) T ## _init(malloc(sizeof(T)))
#define NEW(...) IF_1_ELSE(__VA_ARGS__)(_NEW(__VA_ARGS__))(_NEW_ARGS(__VA_ARGS__))
#define DEL(T) do{ T ## _destroy(T); free(T); T=NULL;} while (0)
// TODO
// EXAMPLE:
// typedef struct {
//   int a;
//   double b;
// } B;
// #define B_INITIALIZER(F) { .a = 1, .b = F }
//  
// static inline
// B* B_init(B *t, double f) {
//   if (t) *t = (B)B_INITIALIZER(f);
//     return t;
// }
// B* aB = B_init(malloc(sizeof(B)), 0.7);
// B* anotherB = NEW(B, 99.0);
//  
// SIMPLER VERSION
//void *memdup(void* mem, const void *src, size_t sz);
//void *memdup(void* mem, const void *src, size_t sz) {
//    return mem ? memcpy(mem, src, sz) : NULL;
//}
//#define ALLOC_INIT(alloc, type, ...)   \
//    (type *)memdup(alloc(sizeof(type)), (type[]){ __VA_ARGS__  }, sizeof(type))

// String with guaranteed hash and length
typedef struct HString{
    ushort length;
    strhash hash;
    const char* value;
} HString;
HString* HString_init(HString* hs, const char* str);
// Hash u.integer ordering, not lexicographical order
schar HString_compare(const HString *a, const HString *b);

typedef struct Pair {
    void* fst;
    void* snd;
} Pair;


// String which has lazy length and lazy hash value stored (lazy=calculated when first needed)
typedef struct OString {
    ushort length;
    strhash hash;
    char * data;
} OString;

OString * OString_init(OString* self, char * string);
strhash OString_hash(OString* self);
strhash OString_len(OString* self);

#define sizeof_array(a) (sizeof(a) / sizeof(*(a)))

#endif
