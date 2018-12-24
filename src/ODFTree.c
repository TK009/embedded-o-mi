#define _POSIX_C_SOURCE 200809L
#include "ODFTree.h"
#include "utils.h"
#include <math.h>
#include <string.h>


void mkPath(Path* self, uchar depth, Path* parent, char * odfId) {
    self->odfId = odfId;
    self->parent = parent;
    self->odfIdLength = strnlen(odfId, 0xFF);

    self->idHashCode = calcHashCodeL(odfId, self->odfIdLength);

    uint parentHash = parent? parent->hashCode : 0;
    self->hashCode = self->idHashCode ^ parentHash;

    self->depth = depth;
}

// return id
//int _binarySearch(ODFTree * tree, Path needle, int left, int right) {
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
int odfBinarySearch(const ODFTree* tree, const Path* needle, int* result) {
    return binarySearch(
            tree->sortedPaths,
            needle,
            tree->size,
            sizeof(Path),
            (compareFunc) pathCompare,
            result);
}

//int getPath(ODFTree tree, Path needle) {
//    binarySearch()
//    return 0;
//}


// Move array forward one step starting from given index
void _move(ODFTree* tree, int index) {
    //memmove(tree->sortedPaths+1, tree->sortedPaths, (tree->size - index) * sizeof(Path));

    Path * indexPointer = tree->sortedPaths + index;

    for (int i = tree->size + 1; i > index; --i) {
        Path * moving = tree->sortedPaths+i-1;
        if (moving->parent > indexPointer)
            moving->parent += sizeof(Path);
        tree->sortedPaths[i] = *moving;
    }
}

Path* addPath(ODFTree* tree, const char pathString[]) {
    const char* idStart = pathString;
    Path* parent = NULL;

    uchar depth = 1;
    int segmentLength = 0;
    

    for (const char* current = pathString; *current != '\0';) {
        ++current; // increase at beginning instead of end
        ++segmentLength;

        // Detect the end of segment
        if (*current == '/' || *current == '\0') {
            //*current = '\0'; // odfId ending handled by odfIdLength instead

            // calc hash and collect variables for searching
            strhash idHash = calcHashCodeL(idStart, segmentLength);
            strhash parentHash = parent? parent->hashCode : 0;
            Path newSegment = {
                idStart,
                parent,
                idHash,
                idHash ^ parentHash,
                depth,
                (uchar) segmentLength
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
            segmentLength = -1;
            ++depth;
        }
    }
    return parent;
}


