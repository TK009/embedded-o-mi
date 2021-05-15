#define _POSIX_C_SOURCE 200809L
#include "OdfTree.h"
#include "utils.h"
#include "OmiConstants.h"
#include <math.h>
#include <string.h>


// TODO: Path_init( 
Path* Path_init(Path* self, uchar depth, NodeType nodetype, Path* parent, OdfId odfId, PathFlags flags) {
    if (self) {
        uint parentHash = parent? parent->hashCode : 0;
        uchar odfIdLength = (uchar) strnlen(odfId, 0xFF);
        strhash idHashCode = calcHashCodeL(odfId, odfIdLength);
        // depth: description: +0, MetaData: +1, InfoItem: +2, Object: +3
        *self = (Path){
            .odfId = odfId,
            .parent = parent,
            .odfIdLength = odfIdLength,
            .idHashCode = idHashCode,
            .hashCode = idHashCode ^ parentHash,
            .depth = OdfDepth(depth, nodetype),
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

OdfTree* OdfTree_init(OdfTree* self, Path* pathStorageArray, int arrayCapacity) {
    if (self) {
        self->size = 1;
        self->capacity = arrayCapacity;
        self->sortedPaths = pathStorageArray;
        Path_init(&(self->sortedPaths[0]), 1, OdfObject, NULL, s_Objects, 0); // TODO use str constant
    }
    return self;
}
void OdfTree_destroy(OdfTree* self, Allocator* stringAllocator, Allocator* valueAllocator) {
    for (int i = 0; i < self->size; ++i) {
        Path * p = &self->sortedPaths[i];
        if (p->flags & PF_OdfIdMalloc)
            stringAllocator->free((void*)p->odfId);
        if (p->flags & PF_ValueMalloc) {
            if (PathGetNodeType(p) == OdfInfoItem) {
                valueAllocator->free(p);
                //valueAllocator->free(p->value.obj);
            }else
                stringAllocator->free(p->value.str);
        }
    }
}



schar pathCompare(const Path *a, const Path *b) {
    // recursively dive into root and calculate the order from root to bottom
    schar parentOrder = 0;
    if      (a->depth > b->depth) parentOrder = pathCompare(a->parent, b);
    else if (a->depth < b->depth) parentOrder = pathCompare(a, b->parent);
    else if (a->depth > ObjectsDepth) parentOrder = pathCompare(a->parent, b->parent);

    if (parentOrder != 0) return parentOrder;
    // parentOrder == 0
    if (a->depth == b->depth) {
        if (a->hashCode == b->hashCode) return 0;
        return strncmp(a->odfId, b->odfId, min(a->odfIdLength, b->odfIdLength)+1);
    }
    return a->depth - b->depth; // should be -1 or 1
}

// returns eomi_true if found, eomi_false if not found. `result` contains index of found
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

    for (int i = tree->size; i > index; --i) {
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

Path* addPath(OdfTree* tree, const char pathString[], NodeType lastNodeType) {
    const char* idStart = pathString;
    Path* parent = NULL;

    uchar depth = 1;
    
    // remove extra / from the start
    if (*pathString == '/') ++idStart;

    for (const char* current = pathString; *current != '\0';) {
        ++current; // increase at beginning instead of end

        // Detect the end of segment
        if (*current == '/' || *current == '\0') {
            //*current = '\0'; // odfId ending handled by odfIdLength instead

            int segmentLength = current - idStart;
            if (segmentLength > 0) { // remove double slashes
                // calc hash and collect variables for searching
                strhash idHash = calcHashCodeL(idStart, segmentLength);
                strhash parentHash = parent? parent->hashCode : 0;
                Path newSegment = {0};
                //Path newSegment = (Path){
                //    .depth = OdfDepth(depth, *current ? OdfObject : lastNodeType),
                //    .odfIdLength = (uchar) segmentLength,
                //    .odfId = idStart,
                //    .parent = parent,
                //    .idHashCode = idHash,
                //    .hashCode = idHash ^ parentHash,
                //    .flags = 0,
                //    .value.d = 0
                //};
                newSegment.depth = OdfDepth(depth, *current ? OdfObject : lastNodeType);
                newSegment.odfIdLength = (uchar) segmentLength;
                newSegment.odfId = idStart;
                newSegment.parent = parent;
                newSegment.idHashCode = idHash;
                newSegment.hashCode = idHash ^ parentHash;
                //newSegment.flags = 0;
                //newSegment.value.l = 0L;

                parent = addPathSegment(tree, &newSegment);
                if (!parent) return NULL;
                ++depth;
            }
            idStart = current + 1;
        }
    }
    return parent;
}

Path* addPathSegment(OdfTree * tree, Path * segment) {
    int resultIndex;
    if (odfBinarySearch(tree, segment, &resultIndex)) {
        return &tree->sortedPaths[resultIndex]; // found
    } else {
        if (tree->capacity <= tree->size) return NULL;
        // make room for new
        if (resultIndex < tree->size)
            _move(tree, resultIndex);
        // create a new path entry
        Path* newSegmentLoc = &tree->sortedPaths[resultIndex];
        //memcpy(newSegmentLoc, segment, sizeof(*newSegmentLoc));
        *newSegmentLoc = *segment;
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

// Returns the copied path in `destination` tree
Path* copyPath(Path * source, OdfTree * destination) {
    if (source->parent) copyPath(source->parent, destination);
    return addPathSegment(destination, source);
}


