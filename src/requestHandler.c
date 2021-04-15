#include "requestHandler.h"
#include "StringStorage.h"
#include "ParserUtils.h"

OdfTree tree;
CreateStaticMemoryPool(LatestValue, LatestValues, ODFTREE_BLOCKS)

// FIXME: better allocator selection
//void LatestValue_destroy(LatestValue * p) {
//    if (p->current.typeString) StringFree((void*)p->current.typeString);
//    if (p->upcoming.typeString) StringFree((void*)p->upcoming.typeString);
//    LatestValues_free(p);
//}
// TODO:
//LatestValuesAllocator.free = LatestValue_destroy;

struct RequestHandler {
    ResponseCallback respond;
};

ErrorResponse handleWrite(OmiParser *p, Path *path, OdfParserEvent event) {
    Path* storedPathSegment = NULL;
    int resultIndex;
    const char * tmpStr;
    switch (event.type) {
        case PE_Path:
            storedPathSegment = addPathSegment(&tree, path);
            if (!storedPathSegment) return Err_OOM;
            storedPathSegment->odfId = storeString(path->odfId);
            storedPathSegment->flags |= PF_OdfIdMalloc;
            if (storedPathSegment->odfId == NULL) {
                removePathSegment(&tree, path);
                return Err_OOM_String;
            }
            if (storedPathSegment->parent == path->parent) { // new was created
                // find parent
                if (odfBinarySearch(&tree, path->parent, &resultIndex)) {
                    storedPathSegment->parent = &tree.sortedPaths[resultIndex];
                } else { // Error; cleanup the mess
                    tmpStr = storedPathSegment->odfId;
                    removePathSegment(&tree, storedPathSegment);
                    freeString(tmpStr);
                    return Err_InternalError;
                }

                // TODO: save created-event here, delay trigger until value is processed

                // create value structure if InfoItem
                if ((path->flags & (PF_IsInfoItem|PF_ValueMalloc)) == PF_IsInfoItem) {
                    storedPathSegment->value.obj = poolCAlloc(&LatestValues);
                    if (!storedPathSegment->value.obj) { //Error, cleanup mess
                        tmpStr = storedPathSegment->odfId;
                        removePathSegment(&tree, storedPathSegment);
                        freeString(tmpStr);
                        return Err_OOM;
                    }
                    storedPathSegment->flags |= PF_ValueMalloc;
                }
            }
            // metadata-like value
            if (event.data) {
                storedPathSegment->value.str = event.data; // NOTE: free not needed
            }
            break;
        case PE_ValueUnixTime:
            if (odfBinarySearch(&tree, path, &resultIndex)){
                storedPathSegment = &tree.sortedPaths[resultIndex];
                if (storedPathSegment->value.obj) {
                    storedPathSegment->value.latest->upcoming.timestamp = parseInt(event.data); // FIXME parser error checking
                    p->stringAllocator->nullFree((void**)&event.data);
                    return Err_OK;
                }
            }
            if (event.data) p->stringAllocator->free(event.data);
            return Err_InternalError; // break;
        case PE_ValueDateTime:
            if (odfBinarySearch(&tree, path, &resultIndex)){
                storedPathSegment = &tree.sortedPaths[resultIndex];
                if (storedPathSegment->value.obj) {
                    storedPathSegment->value.latest->upcoming.timestamp = parseDateTime(event.data); // FIXME parser error checking
                    p->stringAllocator->nullFree((void**)&event.data);
                    return Err_OK;
                }
            }
            if (event.data) p->stringAllocator->free(event.data);
            return Err_InternalError; // break;
        case PE_ValueType:
            // NOTE: Unknown value type: use string type for storage!
            path->flags = (path->flags & ~PF_ValueType) | V_String;

            //if (odfBinarySearch(&tree, path, &resultIndex)) {
            //    storedPathSegment = &tree.sortedPaths[resultIndex];
            //    storedPathSegment->value.latest->upcoming.typeString = event.data; // NOTE: free not needed
            //    return Err_OK;
            //}
            if (event.data) p->stringAllocator->free(event.data);
            return Err_OK; // break;
        case PE_ValueData:

            if (odfBinarySearch(&tree, path, &resultIndex)) {
                storedPathSegment = &tree.sortedPaths[resultIndex];
                // update flags, retaining old, and reset type just in case, TODO: handle type change if history is implemented
                storedPathSegment->flags = (storedPathSegment->flags & ~PF_ValueType) | path->flags;

                LatestValue * latest = storedPathSegment->value.latest;
                bool changed = false, updated = false;
                // store value
                if (event.data) { // String type value
                    latest->upcoming.value.str = event.data; // NOTE: free not needed
                    if (latest->current.value.str == NULL ||
                            strcmp(latest->upcoming.value.str, latest->current.value.str) != 0) {
                        changed = true;
                    }
                } else {
                    latest->upcoming.value = path->value;
                }
                // check for missing timestamp
                if (!latest->upcoming.timestamp)
                    latest->upcoming.timestamp = getTimestamp();

                // events:
                if (latest->current.timestamp < latest->upcoming.timestamp) updated = true;
                if (((storedPathSegment->flags & PF_ValueType) != V_String)
                        && (latest->current.value.l != latest->upcoming.value.l)) changed = true;

                //TODO: trigger events here
                //

                // update
                // TODO: change of value type not allowed atm.
                latest->current = latest->upcoming;
                latest->upcoming = (SingleValue) {0, 0, {.l = 0}};

                return Err_OK;
            }

            if (event.data) p->stringAllocator->free(event.data);
            return Err_InternalError; // break;
        case PE_RequestID:
            if (event.data) p->stringAllocator->free(event.data);
            return Err_InvalidElement; // actually invalid
        case PE_RequestEnd:
            if (p->lastPath) responseFullSuccess(&p->parameters);
            if (event.data) p->stringAllocator->free(event.data);
            return Err_OK;
    }
    return Err_OK;
}
ErrorResponse handleRead(OmiParser *p, Path *path, OdfParserEvent event) {
    Path* storedPathSegment = NULL;
    Path* currentPath = NULL;
    Path* lastResponsePath = NULL;
    int resultIndex;
    //const char * tmpStr;
    switch (event.type) {
        case PE_Path:
            // TODO: response
            // Needs to know what and how many elements to close
            //   if end: close remaining last.parents
            if (odfBinarySearch(&tree, path, &resultIndex)) {
                storedPathSegment = &tree.sortedPaths[resultIndex];
                // check if first event: response needs to be started
                if (! p->lastPath) responseStartWithObjects(&p->parameters, 200);

                // Was the last path a leaf? Not leaf if last is a parent of current
                if (p->lastPath != storedPathSegment->parent) {
                    // leaf; recursively respond the subtree
                    lastResponsePath = p->lastPath;
                    if(odfBinarySearch(&tree, p->lastPath, &resultIndex)) {
                        for (int i = resultIndex+1; i < tree.size; ++i) {
                            currentPath = &tree.sortedPaths[i];
                            if (currentPath->depth >= p->lastPath->depth) break;
                            if (currentPath->depth <= lastResponsePath->depth)
                                responseCloseOdfNode(&p->parameters, lastResponsePath);
                            responseStartOdfNode(&p->parameters, currentPath);
                            lastResponsePath = currentPath;
                        }
                    }
                    // close the non-common parents between last (request or response) and current/next (request) path 
                    while (lastResponsePath->depth < storedPathSegment->depth) {
                        responseCloseOdfNode(&p->parameters, currentPath);
                        lastResponsePath = lastResponsePath->parent;
                    }
                } else {
                    // not a leaf; add the node to the response already
                    responseStartOdfNode(&p->parameters, storedPathSegment);
                }

                p->lastPath = storedPathSegment;
            }
            if (event.data) p->stringAllocator->free(event.data);
            return Err_OK;
        case PE_RequestID: // Poll request
            if (event.data) p->stringAllocator->free(event.data);
            return Err_NotImplemented;
        case PE_RequestEnd:
            if (p->lastPath) responseEndWithObjects(&p->parameters);
            if (event.data) p->stringAllocator->free(event.data);
            return Err_OK;
        default:
            if (event.data) p->stringAllocator->free(event.data);
            return Err_InternalError;
    }
}

ErrorResponse handleRequestPath(OmiParser *p, Path *path, OdfParserEvent event) {
    switch (p->parameters.requestType) {
        case OmiWrite: case OmiResponse:
            return handleWrite(p, path, event);
        case OmiRead:
            return handleRead(p, path, event);
        default:
            if (event.data) p->stringAllocator->free(event.data);
            return Err_NotImplemented;
    }
}
