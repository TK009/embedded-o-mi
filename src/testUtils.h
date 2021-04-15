#ifndef TEST_UTILS_H
#define TEST_UTILS_H
#include "utils.h"
extern const Allocator nullocator;
void* nulloc(size_t s);
void* culloc(size_t s, size_t n);
char *stra(const char * str);
#endif
