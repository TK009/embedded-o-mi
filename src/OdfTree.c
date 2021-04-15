#define _POSIX_C_SOURCE 200809L
#include "OdfTree.h"
#include "utils.h"
#include <math.h>
#include <string.h>


// TODO: Path_init( 
Path* Path_init(Path* self, uchar depth, Path* parent, OdfId odfId, PathFlags flags) {
    if (self) {
        uint parentHash = parent? parent->hashCode : 0;
        uchar odfIdLength = (uchar) strnlen(odfId, 0xFF);
        strhash idHashCode = calcHashCodeL(odfId, odfIdLength);
        *self = (Path){
            .odfId = odfId,
            .parent = parent,
            .odfIdLength = odfIdLength,
            .idHashCode = idHashCode,
            .hashCode = idHashCode ^ parentHash,
            .depth = depth,
            .flags = flags,
            .value.l = 0LL
        };
        //self->odfId = odfId;
        //self->parent = parent;
        //self->odfIdLength = odfIdLength;
        //self->idHashCode = idHashCode;
        //self->hashCode = idHashCode ^ parentHash;
        //self->depth = depth;
        //self->flags = flags;
        //self->value.l = (int64) 0;
    }
    return self;
}

OdfTree* OdfTree_init(OdfTree* self) {
    if (self) {
        self->size = 1;
        self->capacity = ODFTREE_SIZE;
        Path_init(&(self->sortedPaths[0]), 1, NULL, "Objects", 0); // TODO use str constant
    }
    return self;
}
void OdfTree_destroy(OdfTree* self, Allocator* stringAllocator, Allocator* valueAllocator) {
    for (int i = 0; i < self->size; ++i) {
        Path * p = &self->sortedPaths[i];
        if (p->flags & PF_OdfIdMalloc)
            stringAllocator->free((void*)p->odfId);
        if (p->flags & PF_ValueMalloc) {
            if (p->flags & PF_IsInfoItem)
                valueAllocator->free(p->value.obj);
            else
                stringAllocator->free(p->value.str);
        }
    }
}

// return id
//int _binarySearch(OdfTree * tree, Path needle, int left, int right) {
//    int middle = (left + right) / 2; // left biased
//    
//    if (middle == left) return -1;
//}
// 

schar pathCompare(const Path *a, const Path *b) {
    // recursively dive into root and calculate the order from root to bottom
    schar parentOrder = 0;
    if      (a->depth > b->depth) parentOrder = pathCompare(a->parent, b);
    else if (a->depth < b->depth) parentOrder = pathCompare(a, b->parent);
    else if (a->depth > 1)        parentOrder = pathCompare(a->parent, b->parent);

    if (parentOrder != 0) return parentOrder;
    // parentOrder == 0
    if (a->depth == b->depth) {
        if (a->hashCode == b->hashCode) return 0;
        return strncmp(a->odfId, b->odfId, min(a->odfIdLength, b->odfIdLength)+1);
    }
    return a->depth - b->depth; // should be -1 or 1
}

// returns true if found, false if not found. `result` contains index of found
// or the index of the next closest element.
int odfBinarySearch(const OdfTree* tree, const Path* needle, int* result) {
    return binarySearch(
            tree->sortedPaths,
            needle,
            tree->size,
            sizeof(Path),
            (compareFunc) pathCompare,
            result);
}

//int getPath(OdfTree tree, Path needle) {
//    binarySearch()
//    return 0;
//}


// Move array forward one step starting from given index
void _move(OdfTree* tree, int index) {
    //memmove(tree->sortedPaths+1, tree->sortedPaths, (tree->size - index) * sizeof(Path));

    Path * indexPointer = tree->sortedPaths + index;

    for (int i = tree->size + 1; i > index; --i) {
        Path * moving = tree->sortedPaths+i-1;
        if (moving->parent >= indexPointer)
            moving->parent++;
        tree->sortedPaths[i] = *moving;
    }
}
void _unmove(OdfTree* tree, int index) {
    Path * indexPointer = tree->sortedPaths + index;

    for (int i = index; i < tree->size-1; ++i) {
        Path * moving = tree->sortedPaths+i+1;
        if (moving->parent >= indexPointer)
            moving->parent--;
        tree->sortedPaths[i] = *moving;
    }
}

Path* addPath(OdfTree* tree, const char pathString[]) {
    const char* idStart = pathString;
    Path* parent = NULL;

    uchar depth = 1;
    

    for (const char* current = pathString; *current != '\0';) {
        ++current; // increase at beginning instead of end

        // Detect the end of segment
        if (*current == '/' || *current == '\0') {
            //*current = '\0'; // odfId ending handled by odfIdLength instead

            int segmentLength = current - idStart;
            // calc hash and collect variables for searching
            strhash idHash = calcHashCodeL(idStart, segmentLength);
            strhash parentHash = parent? parent->hashCode : 0;
            Path newSegment = {
                .depth = depth,
                .odfIdLength = (uchar) segmentLength,
                .odfId = idStart,
                .parent = parent,
                .idHashCode = idHash,
                .hashCode = idHash ^ parentHash,
                .flags = 0
            };

            parent = addPathSegment(tree, &newSegment);
            if (!parent) return NULL;
            idStart = current + 1;
            ++depth;
        }
    }
    return parent;
}

Path* addPathSegment(OdfTree * tree, Path * segment) {
    if (tree->capacity <= tree->size) return NULL;
    int resultIndex;
    if (odfBinarySearch(tree, segment, &resultIndex)) {
        return &tree->sortedPaths[resultIndex]; // found
    } else {
        // make room for new
        _move(tree, resultIndex);
        // create a new path entry
        Path* newSegmentLoc = &tree->sortedPaths[resultIndex];
        memcpy(newSegmentLoc, segment, sizeof(*newSegmentLoc));
        tree->size++;
        return newSegmentLoc;
    }
    
}
void removePathSegment(OdfTree * tree, const Path * segment) {
    int resultIndex;
    if (odfBinarySearch(tree, segment, &resultIndex)) {
        //tree->sortedPaths[resultIndex];
        _unmove(tree, resultIndex);
        tree->size--;
    }
}


