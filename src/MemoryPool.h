#ifndef MEMORYPOOL_H
#define MEMORYPOOL_H

#include <string.h> //memset
#include "utils.h"


typedef struct MemoryPool {
  ushort currentBlock; /* start here for searching a free slot, progress forward and loop back to start */
  ushort blockCount; // 32 sized blocks
  ushort elementSize; // in bytes
  uint freeCount;
  uint totalCount; // = 32 * blockCount
  uint *reservedBitArray;
  void *data; // Size = elementSize * totalCount
} MemoryPool;

#define BlockSize (sizeof(uint)*8)
#define poolSize(BLOCK_COUNT) (BLOCK_COUNT*BlockSize)

#define CreateMemoryPool(TYPE, NAME, BLOCK_COUNT) \
  static uint NAME ## Free[BLOCK_COUNT] = {0}; \
  static TYPE NAME ## Data[poolSize(BLOCK_COUNT)] = {0}; \
  static MemoryPool NAME = { 0, BLOCK_COUNT, sizeof(TYPE), 32*BLOCK_COUNT, 32*BLOCK_COUNT, (uint *)&(NAME ## Free), (void*)&(NAME ## Data) };

// Return NULL on failed allocation, otherwise pointer to allocated object
void* poolAlloc(MemoryPool *pool);
// Zero the memory region
void* poolCAlloc(MemoryPool *pool);

// Free element indicated by pointer to the start of the element
void poolFree_(MemoryPool *pool, void** element);
#define poolFree(pool, ptr) poolFree_((pool), (void**)&(ptr))

#endif
