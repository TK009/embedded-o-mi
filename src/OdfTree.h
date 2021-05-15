#ifndef ODFTREE_H
#define ODFTREE_H

#include "Settings.h"
#include "utils.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef const char * OdfId;
typedef uint PathFlags;

typedef struct LatestValue LatestValue;
typedef union AnyValue {
    char b;
    float f;
    double d;
    //uint ui;
    int i;
    int64 l;
    char * str;
    LatestValue * latest;
    void * obj;
} AnyValue;

typedef struct SingleValue {
    eomi_time timestamp;
    const char * typeString; // For unknown types, enumeration is not enough
    AnyValue value;
} SingleValue;

typedef enum NodeType {
    OdfDescription = 0,
    OdfMetaData = 1,
    OdfInfoItem = 2,
    OdfObject = 3,
} NodeType;

#define OdfDepth(depth, nodetype) (((depth) << 2) + (nodetype))
#define PathGetDepth(p) ((p)->depth >> 2)
#define PathGetNodeType(p) ((p)->depth & 3)
#define PathSetNodeType(p, nodetype) ((p)->depth = (p)->depth & ~3 | (nodetype))
#define PathIsNonMeta(p) ((p)->depth & 2)
#define ObjectsDepth OdfDepth(1,OdfObject)



// General idea is to build a prefix tree for O-DF
// ...in other words, linked list of each Path sharing same elements
struct Path {
    uchar depth; // Objects: 1 FIXME: moving to top breaks test (error?)
    uchar odfIdLength;
    OdfId odfId;
    struct Path* parent;
    uint idHashCode;
    uint hashCode;
    PathFlags flags;
    AnyValue value;
};
typedef struct Path Path;

Path* Path_init(Path* self, uchar depth, NodeType nodetype, Path* parent, OdfId odfId, PathFlags flags);

schar pathCompare(const Path* a, const Path* b);

//Path mkPath(char* pathString);

typedef struct OdfTree {
    int size;
    int capacity;
    Path *sortedPaths;
    //Path sortedPaths[ODFTREE_SIZE];
    //LatestValue latestValuesData[ODFTREE_SIZE];
} OdfTree;

OdfTree* OdfTree_init(OdfTree* self, Path* pathStorageArray, int arrayCapacity);
void OdfTree_destroy(OdfTree* self, Allocator* idAllocator, Allocator* valueAllocator);

// returns eomi_true if found, eomi_false if not found. `result` contains index of found
// or the index of the next closest element.
int odfBinarySearch(const OdfTree* tree, const Path* needle, int* resultIndex);

Path* addPath(OdfTree* tree, const char pathString[], NodeType lastNodeType);
Path* addPathSegment(OdfTree * tree, Path * segment);
void removePathSegment(OdfTree * tree, const Path * segment);

Path* copyPath(Path * source, OdfTree * destination);

typedef enum ValueType {
    V_String = 0, // the default
    V_Int = 1,
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

// NOTE: most of these are placeholders only
typedef enum PathFlag {
    PF_ValueType             = 1 | 2 | 4 | 8, // For InfoItem values
    //PF_IsInfoItem            = 1 << 4, // Object tag or otherwise InfoItem or one below
    //PF_IsMetaData            = 1 << 5, // MetaData tag
    //PF_IsDescription         = 1 <<  , // description tag
    PF_IsNewWithoutValue     = 1 << 6, // Path written but no value yet, suitable for interval=-2 new item subs
    PF_IsReadOnly            = 1 << 7, // write request not allowed 
    PF_HasEventSub           = 1 << 8, // write to this triggers event subscription(s)
    PF_HasPollSub            = 1 << 9, // write to this should be saved for poll
    PF_HasResponsibleScript  = 1 << 10, // write to this should be passed to script
    PF_HasOnWriteScript      = 1 << 11, // after write script should be run
    PF_IsOwnerSecret         = 1 << 12, // only authenticated owner can rw or ro
    PF_IsLeaf                = 1 << 13, // Leaf node in O-DF; To be used in requests only
    PF_IsTemporary           = 1 << 14, // in ram, disappears on poweroff
    PF_HasSystemWriteHandler = 1 << 15, // Actuator or system setting
    PF_HasSystemReadHandler  = 1 << 16, // Sensor or system metric
    //PF_IsMetaDataChild       = 1 << 17, // Child of metadata
    PF_isDirty               = 1 << 18, // Has modifications that should be saved to flash
    PF_OdfIdMalloc           = 1 << 19, // odfId string is malloc'd and should be freed when destroying this Path
    PF_ValueMalloc           = 1 << 20, // odfId string is malloc'd and should be freed when destroying this Path
    //PF_ValueChangeSub = 1 << 21, // Value change event instead of a new value based on timestamp
    //FP_NewItemSub = 1 << 22, // interval=-2 subscription of new info items in the sub structure
    //PF_isExternalReadOnly    = 1 <<  // Script can write
} PathFlag;


#ifdef __cplusplus
}
#endif
#endif
