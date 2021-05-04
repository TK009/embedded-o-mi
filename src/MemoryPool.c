#include "MemoryPool.h"

MemoryPool* CreateDynamicMemoryPool(size_t elementSize, uint blockCount, Allocator *a){
    size_t      numElements      = PoolSize(blockCount);
    size_t      poolSizeBytes    = elementSize*numElements;
    uint*       reservedBitArray = a->calloc(blockCount,sizeof(uint));
    void*       poolData         = a->calloc(poolSizeBytes,sizeof(char));
    MemoryPool* result           = a->malloc(sizeof(*result));


    if (reservedBitArray && poolData && result) {
        *result = (MemoryPool){ 0, blockCount, elementSize, numElements, numElements, reservedBitArray, poolData };
        return result;
    }
    free(reservedBitArray);
    free(poolData);
    free(result);
    return NULL;
}

void FreeDynamicMemoryPool_(MemoryPool** p_pool, Allocator* a) {
    if (*p_pool) {
        MemoryPool *pool = *p_pool;
        a->free(pool->reservedBitArray);
        a->free(pool->data);
        a->free(pool);
        *p_pool = NULL;
    }
}

// Return NULL on failed allocation, otherwise pointer to allocated object
void* poolAlloc(MemoryPool *pool) {
    if (pool->freeCount == 0) return NULL;

    // Find first block containing free slots
    ushort blockNum = pool->currentBlock;
    uint block = 0;
    while ((block = pool->reservedBitArray[blockNum]) == 0xFFFFFFFF) {
        if (++blockNum >= pool->blockCount) blockNum = 0; // Infinite loop if pool->freeCount not correct
    }

    // Find actual slot in block
    int bitLocation = block != 0x7FFFFFFF? first0Bit(block): 31; // fix undefined behaviour (negation: -INT_MIN, underflow: INT_MIN - 1)
    int memoryOffset = (BlockSize * blockNum + bitLocation) * pool->elementSize;
    pool->reservedBitArray[blockNum] ^= 1u << bitLocation;
    pool->freeCount--;
    pool->currentBlock = blockNum;

    return (char*) pool->data + memoryOffset;
}

void* poolCAlloc(MemoryPool *pool) {
    void *p = poolAlloc(pool);
    if (p) memset(p, 0, pool->elementSize);
    return p;
}

bool poolExists(MemoryPool *pool, void* element) {
    if (element > pool->data && (char*)element < ((char*)pool->data)+pool->totalCount*pool->elementSize) { // extra checks
        int memoryOffset = (char*)element - (char*) pool->data;
        if (memoryOffset % pool->elementSize != 0) return false; // extra checks
        int elementNumber = memoryOffset / pool->elementSize;
        ushort blockNum = elementNumber / BlockSize;
        int bitLocation = elementNumber - blockNum * BlockSize;

        return pool->reservedBitArray[blockNum] & (1u << bitLocation);
    }
    return false;
}

// Free element indicated by pointer to the start of the element
void poolFree_(MemoryPool *pool, void** element) {
    if (*element) {
        int memoryOffset = (char*)*element - (char*) pool->data;
        int elementNumber = memoryOffset / pool->elementSize;
        ushort blockNum = elementNumber / BlockSize;
        int bitLocation = elementNumber - blockNum * BlockSize;

        //if (pool->reservedBitArray[blockNum] & (1 << bitLocation)) {
        pool->reservedBitArray[blockNum] ^= 1u << bitLocation;
        pool->freeCount++;
        //}
        *element = NULL;
    }
}

