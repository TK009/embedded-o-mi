#ifndef TEST_UTILS_H
#define TEST_UTILS_H

#include "utils.h"
#include "OdfTree.h"

#ifdef __cplusplus
extern "C" {
#endif

extern Allocator nullocator;
void* nulloc(size_t s);
void* culloc(size_t s, size_t n);
char *stra(const char * str);

void OdfTree_destroy(OdfTree* self, Allocator* idAllocator, Allocator* valueAllocator);

#ifdef __cplusplus
}
#endif
#endif
