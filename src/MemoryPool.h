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

#define BlockSize (sizeof(uint)*8) // 32, because bitstring consists of uints
#define PoolSize(BLOCK_COUNT) (BLOCK_COUNT*BlockSize)

// size will be BLOCK_COUNT*BlockSize
#define CreateStaticMemoryPool(TYPE, NAME, BLOCK_COUNT) \
  static uint NAME ## Free[BLOCK_COUNT] = {0}; \
  static TYPE NAME ## Data[PoolSize(BLOCK_COUNT)] = {0}; \
  static MemoryPool NAME = { 0, BLOCK_COUNT, sizeof(TYPE), PoolSize(BLOCK_COUNT), PoolSize(BLOCK_COUNT), (uint *)&(NAME ## Free), (void*)&(NAME ## Data) };

// Create pool using the given allocation function
MemoryPool* CreateDynamicMemoryPool(size_t elementSize, uint blockCount, Allocator* a);
void FreeDynamicMemoryPool_(MemoryPool** p_pool, Allocator* a);
#define FreeDynamicMemoryPool(pool, a) FreeDynamicMemoryPool_(&(pool), (a))

// Return NULL on failed allocation, otherwise pointer to allocated object
void* poolAlloc(MemoryPool *pool);
// Zero the memory region
void* poolCAlloc(MemoryPool *pool);

// Free element indicated by pointer to the start of the element
void poolFree_(MemoryPool *pool, void** element);
#define poolFree(pool, ptr) poolFree_((pool), (void**)&(ptr))

#endif
