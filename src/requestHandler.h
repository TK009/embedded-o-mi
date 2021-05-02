#ifndef REQUEST_HANDLER_H
#define REQUEST_HANDLER_H

#include "utils.h"
#include "OMIParser.h"
#include "OdfTree.h"
#include "MemoryPool.h"
#include "ConnectionHandler.h"

// NOTE: Remember to OdfTree_init !
extern OdfTree tree;
extern Allocator LatestValuesAllocator;
extern MemoryPool LatestValues;
void LatestValue_destroy(Path * p);

ErrorResponse handleRequestPath(OmiParser *p, Path *path, OdfParserEvent event);
ErrorResponse handleWrite(OmiParser *p, Path *path, OdfParserEvent event);
ErrorResponse handleRead(OmiParser *p, Path *path, OdfParserEvent event);
ErrorResponse handleCancel(OmiParser * p, Path *path, OdfParserEvent event);

// parameters: connectionId, content?
typedef void (*ResponseCallback)(int, const char *);


#endif
