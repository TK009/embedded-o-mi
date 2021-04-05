#include "testUtils.h"

void* nulloc(size_t s){
  (void) s;
  return NULL;
}
void* culloc(size_t s, size_t n){
  (void) s; (void)n;
  return NULL;
}
const Allocator nullocator = {.malloc = nulloc, .calloc = culloc, .free = free};
