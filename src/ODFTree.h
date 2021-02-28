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
    uchar depth; // FIXME: moving to top breaks test (error?)
    uchar odfIdLength;
    uint flags;
};
typedef struct Path Path;

Path* Path_init(Path* self, uchar depth, Path* parent, char * odfId);

schar pathCompare(const Path* a, const Path* b);

//Path mkPath(char* pathString);


struct ODFTree {
    Path sortedPaths[ODFTREE_SIZE];
    int size;
};

typedef struct ODFTree ODFTree;

int odfBinarySearch(const ODFTree* tree, const Path* needle, int* resultIndex);

Path* addPath(ODFTree* tree, const char newPath[]);

typedef enum ValueType {
    V_Int = 0,
    V_Float,
    V_Long,
    V_Double,
    V_Short,
    V_Byte,
    V_Boolean,
    V_ULong,
    V_UInt,
    V_UShort,
    V_UByte,
    V_ODF,
} ValueType;

typedef enum PathFlag {
    F_ValueType             = 1 & 2 & 4 & 8, // For InfoItem values
    F_IsInfoItem            = 1 << 4, // Object or other otherwise
    F_IsMetaData            = 1 << 5, // or other
    F_IsDescription         = 1 << 6,
    F_IsReadOnly            = 1 << 7,
    F_HasEventSub           = 1 << 8,
    F_HasPollSub            = 1 << 9,
    F_HasResponsibleScript  = 1 << 10,
    F_HasOnWriteScript      = 1 << 11,
    F_IsOwnerSecret         = 1 << 12,
    F_IsLeaf                = 1 << 13, // To be used in requests only
    F_IsTemporary           = 1 << 14, // in ram, disappears in reboot
    F_HasSystemWriteHandler = 1 << 15, // Actuator or system setting
    F_HasSystemReadHandler  = 1 << 16, // Sensor or system metric
    F_IsMetaDataChild       = 1 << 17, // Child of metadata
} PathFlag;

#endif
