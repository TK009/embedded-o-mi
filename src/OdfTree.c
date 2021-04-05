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
    if (self) self->size = 0;
    return self;
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

            int segmentIx = 0;
            if (odfBinarySearch(tree, &newSegment, &segmentIx)){
                if (*current == '\0')
                    return tree->sortedPaths + segmentIx; // found existing
            } else {
                // make room for new
                _move(tree, segmentIx);

                // create a new path entry
                Path* newSegmentLoc = tree->sortedPaths + segmentIx;
                memcpy(newSegmentLoc, &newSegment, sizeof(*newSegmentLoc));
                tree->size++;
            }

            // local vars
            idStart = current + 1;
            parent = &tree->sortedPaths[segmentIx];
            ++depth;
        }
    }
    return parent;
}


