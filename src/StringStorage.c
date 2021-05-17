#include "StringStorage.h"

CreateStaticMemoryPool(SkiplistEntry, stringStorageEntries, StringStorageMaxStringBlocks)
CreateStaticMemoryPool(StringMetadata, stringStorageMetadatas, StringStorageMaxStringBlocks)
CreateStaticMemoryPool(HString, stringWrappers, StringStorageMaxStringBlocks)

Skiplist stringStorage;

// default
Allocator StringStorageAllocator = StdAllocator;

void StringStorage_init() {
    Skiplist_init(&stringStorage, (compareFunc)HString_compare, &stringStorageEntriesAllocator);
}

// Stores a copy of str, returns the stored version. Reuses the string if it exists.
const char * storeHString(const HString *str) {
    StringMetadata *newMeta = poolAlloc(&stringStorageMetadatas);
    //if (!newMeta) return NULL; // Skiplist entries will run out earlier
    HString *newStringWrapper = poolAlloc(&stringWrappers);
    //if (!newMeta) return NULL; // Skiplist entries will run out earlier

    newMeta->numUsages = 1;
    memcpy(newStringWrapper, str, sizeof(HString));

    Pair replaced = Skiplist_set(&stringStorage, newStringWrapper, newMeta);
    if (replaced.fst) { // the same string already stored
        StringMetadata * oldMeta = replaced.snd;
        newMeta->numUsages += oldMeta->numUsages;
        poolFree(&stringStorageMetadatas, oldMeta);
        poolFree(&stringWrappers, newStringWrapper);
        return ((HString *)replaced.fst)->value;
    } else { // new stored string
        if (replaced.snd == (void*)1) { // Malloc failed for skiplist
            poolFree(&stringStorageMetadatas, newMeta);
            poolFree(&stringWrappers, newStringWrapper);
            return NULL;
        }
        char * copiedStr = StringStorageAllocator.malloc(str->length+1);
        copiedStr = strncpy(copiedStr, str->value, str->length);
        copiedStr[str->length] = '\0';
        newStringWrapper->value = copiedStr;
        return copiedStr;
    }
}

const char * storeString(const char *str) {
    HString tmp;
    return storeHString(HString_init(&tmp, str));
}

void freeString(const char *str) {
    HString tmp;
    freeHString(HString_init(&tmp, str));
}
// Reduces the reference counter, internal str can be freed if reference counter goes to zero
void freeHString(const HString *str) {
    SkiplistEntry * prev[MAX_SKIPLIST_HEIGHT];
    SkiplistEntry * elem = Skiplist_findP(&stringStorage, str, prev);
    if (!elem) return;
    HString * string = elem->key;
    StringMetadata * meta = elem->value;
    if (meta->numUsages == 1) {
        Skiplist_del(&stringStorage, elem, prev);
        StringStorageAllocator.free((void*) string->value);
        poolFree(&stringStorageMetadatas, meta);
        poolFree(&stringWrappers, string);
        return;
    }
    meta->numUsages--;
}
// only checks whether str exists
eomi_bool hStringExists(const HString *str) {
    return Skiplist_get(&stringStorage, str) != NULL;
}
eomi_bool stringExists(const char *str) {
    HString tmp;
    return hStringExists(HString_init(&tmp, str));
}
