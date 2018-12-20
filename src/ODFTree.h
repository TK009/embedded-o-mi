#include "Settings.h"
#include "utils.h"

typedef char * OdfId;

// General idea is to build a prefix tree for O-DF
// Or linked list for Path
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

//Path PathFromSegments(uchar length, ...);

struct ODFTree {
    Path sortedPaths[ODFTREE_SIZE];
    int size;
};

typedef struct ODFTree ODFTree;

int odfBinarySearch(ODFTree* tree, Path* needle, int* resultIndex);

Path* addPath(ODFTree* tree, char** newPath);

//ODFTree mkODFTree();
