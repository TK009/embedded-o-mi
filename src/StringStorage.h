#ifndef STRING_STORAGE_H
#define STRING_STORAGE_H

#include "Skiplist.h"
#include "MemoryPool.h"
//#include "Settings.h"

// block = 32 units
#ifndef StringStorageMaxStringBlocks
#define StringStorageMaxStringBlocks 10
#endif

#ifndef StringAllocator
#define StringAllocator malloc
#endif
#ifndef StringFree
#define StringFree free
#endif

typedef struct StringMetadata {
    ushort numUsages;
} StringMetadata;
//inline StringMetadata* StringMetadata_init(StringMetadata* ptr){
//} 

// mainly for testing purposes:
extern MemoryPool stringStorageEntries;
extern MemoryPool stringStorageMetadatas;
extern MemoryPool stringWrappers;
extern Skiplist stringStorage;

void StringStorage_init();

// Stores a copy of str, returns the stored version. Reuses the string if it exists.
const char * storeHString(const HString *str);
// Stores a copy of str, returns the stored version. Reuses the string if it exists.
const char * storeString(const char *str);
// Reduces the reference counter, internal str can be freed if reference counter goes to zero
void freeHString(const HString *str);
void freeString(const char *str);
// only checks whether str exists
bool hStringExists(const HString *str);
bool stringExists(const char *str);

extern Allocator stringStorageFreecator;

#endif
