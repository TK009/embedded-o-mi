#ifndef REQUEST_HANDLER_H
#define REQUEST_HANDLER_H

#include "utils.h"
#include "OMIParser.h"
#include "OdfTree.h"
#include "MemoryPool.h"
#include "ConnectionHandler.h"

#ifdef __cplusplus
extern "C" {
#endif

// NOTE: Remember to OdfTree_init !
extern OdfTree tree;
extern Allocator LatestValuesAllocator;
extern MemoryPool LatestValues;
extern MemoryPool HandlerInfoPool;
void LatestValue_destroy(Path * p);

void RequestHandler_init();
ErrorResponse handleRequestPath(OmiParser *p, Path *path, OdfParserEvent event);
ErrorResponse handleWrite(OmiParser *p, Path *path, OdfParserEvent event);
ErrorResponse handleRead(OmiParser *p, Path *path, OdfParserEvent event);
ErrorResponse handleCancel(OmiParser * p, Path *path, OdfParserEvent event);

void addSubscription(OmiRequestParameters* p, Path* path, void* param);

// parameters: connectionId, content?
typedef void (*ResponseCallback)(int, const char *);


#ifdef __cplusplus
}
#endif
#endif
