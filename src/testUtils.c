#include "testUtils.h"
#include <string.h>
#include "StringStorage.h"

void* nulloc(size_t s){
  (void) s;
  return NULL;
}
void* culloc(size_t s, size_t n){
  (void) s; (void)n;
  return NULL;
}
Allocator nullocator = {.malloc = nulloc, .calloc = culloc, .free = free};

// strdup for tests, it is not in c standard
char *stra(const char * str){
    size_t size = strlen(str)+1;
    char * new = malloc(size);
    strcpy(new, str);
    return new;
}
void OdfTree_destroy(OdfTree* self, Allocator* stringAllocator, Allocator* valueAllocator) {
    for (int i = 0; i < self->size; ++i) {
        Path * p = &self->sortedPaths[i];
        if (p->flags & PF_OdfIdMalloc)
            //stringAllocator->free((void*)p->odfId);
            freeString(p->odfId);
        if (p->flags & PF_ValueMalloc) {
            if (PathGetNodeType(p) == OdfInfoItem) {
                valueAllocator->free(p);
                //valueAllocator->free(p->value.obj);
            }else
                freeString(p->value.str);
        }
    }
}
