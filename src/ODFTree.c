#include "ODFTree.h"
#include "utils.h"
#include <string.h>
#include <math.h>

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
int odfBinarySearch(ODFTree* tree, Path* needle, int* result) {
    return binarySearch(
            tree->sortedPaths,
            &needle,
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
    memmove(tree->sortedPaths+1, tree->sortedPaths, tree->size - index);
}

Path* addPath(ODFTree* tree, char** pathString) {
    char* idStart = *pathString;
    Path* parent = NULL;

    uchar depth = 1;
    uchar segmentLength = 0;
    

    for (char* current = *pathString; *current != '\0';) {
        ++current; // increase at beginning instead of end

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
                segmentLength
            };

            int segmentIx = 0;
            if (odfBinarySearch(tree, &newSegment, &segmentIx)){
                return tree->sortedPaths + segmentIx; // found existing
            } else {
                //make room for new
                _move(tree, segmentIx);

                Path* newSegmentLoc = tree->sortedPaths + segmentIx;
                memcpy(newSegmentLoc, &newSegment, sizeof(newSegment));
                tree->size++;
            }
            Path* segment = &tree->sortedPaths[segmentIx];

            // vars
            parent = segment;
            segmentLength = 0;
            ++depth;
        } else {
            ++segmentLength;
        }
    }
    return parent;
}


