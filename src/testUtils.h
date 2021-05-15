#ifndef TEST_UTILS_H
#define TEST_UTILS_H

#include "utils.h"

#ifdef __cplusplus
extern "C" {
#endif

extern const Allocator nullocator;
void* nulloc(size_t s);
void* culloc(size_t s, size_t n);
char *stra(const char * str);

#ifdef __cplusplus
}
#endif
#endif
