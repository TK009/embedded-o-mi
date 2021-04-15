#include "testUtils.h"
#include <string.h>

void* nulloc(size_t s){
  (void) s;
  return NULL;
}
void* culloc(size_t s, size_t n){
  (void) s; (void)n;
  return NULL;
}
const Allocator nullocator = {.malloc = nulloc, .calloc = culloc, .free = free};

// strdup for tests, it is not in c standard
char *stra(const char * str){
    size_t size = strlen(str)+1;
    char * new = malloc(size);
    strcpy(new, str);
    return new;
}
