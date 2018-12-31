#ifndef ODFTREE_H
#define ODFTREE_H

#include "Settings.h"
#include "utils.h"

typedef const char * OdfId;

// General idea is to build a prefix tree for O-DF
// ...in other words, linked list of each Path sharing same elements
struct Path {
    OdfId odfId;
    struct Path* parent;
    uint idHashCode;
    uint hashCode;
    uchar depth;
    uchar odfIdLength;
};
typedef struct Path Path;

void mkPath(Path* self, uchar depth, Path* parent, char * odfId);

schar pathCompare(const Path* a, const Path* b);

//Path mkPath(char* pathString);


struct ODFTree {
    Path sortedPaths[ODFTREE_SIZE];
    int size;
};

typedef struct ODFTree ODFTree;

int odfBinarySearch(const ODFTree* tree, const Path* needle, int* resultIndex);

Path* addPath(ODFTree* tree, const char newPath[]);

#endif
