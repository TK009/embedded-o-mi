#include "requestHandler.h"
#include "StringStorage.h"
#include "ParserUtils.h"
#include "MemoryPool.h"
#include <string.h>
#include "OmiConstants.h"
#include "ScriptEngine.h"

Path pathsStorage[ODFTREE_SIZE];
OdfTree tree;
CreateStaticMemoryPool(LatestValue, LatestValues, ODFTREE_BLOCKS)

void RequestHandler_init() {
    OdfTree_init(&tree, pathsStorage, ODFTREE_SIZE);
}

struct RequestHandler {
    ResponseCallback respond;
};

CreateStaticMemoryPool(HandlerInfo, HandlerInfoPool, 4)

// FIXME: better allocator selection
void LatestValue_destroy(Path * path) {
    LatestValue * p = path->value.latest;
    if (p->current.typeString) StringFree((void*)p->current.typeString);
    if (p->upcoming.typeString) StringFree((void*)p->upcoming.typeString);
    if (p->writeHandler) HandlerInfoPool_free(p->writeHandler);
    if ((path->flags & PF_ValueType) == V_String && p->current.value.str) freeString(p->current.value.str);
    LatestValues_free(p);
}

// param must be HandlerInfo**
void addSubscription(OmiRequestParameters* p, Path* path, void* param) {
    (void) p;
    HandlerInfo ** newSub = param;
    if (! path->value.latest) return; // FIXME: add to object also to get new children to the sub
    // prepend the list to reuse the parent list on children, while children can have more items
    HandlerInfo ** currentHandler = &(path->value.latest->writeHandler);
    // allocate new here if next pointer needs to be different.
    if ((*currentHandler) != (*newSub)->nextOther) {
        HandlerInfo * replacingNewSub = (HandlerInfo*) poolAlloc(&HandlerInfoPool);
        memcpy(replacingNewSub, *newSub, sizeof(**newSub));
        replacingNewSub->nextOther = *currentHandler; // link to earlier subs on this path
        (*currentHandler)->prevOther = replacingNewSub;

        (*newSub)->another = replacingNewSub; // link from the last same to enable easy removal

        *newSub = replacingNewSub;
    }
    *currentHandler = *newSub;
}

HandlerInfo** getHandlerInfoPtr(Path * path) {
    NodeType type = PathGetNodeType(path);
    if (type == OdfInfoItem && path->flags & PF_ValueMalloc){
        return & path->value.latest->writeHandler;
    } else if (type == OdfObject) {
        return (HandlerInfo**) & path->value.obj;
    }
    return NULL;
}

ErrorResponse handleScript(OmiParser *p, Path *path, OdfParserEvent event) {
    (void) event;
    // path is InfoItem
    if (PathGetNodeType(path) != OdfInfoItem) return Err_OK;

    // Not needed because write and read handlers are stored separately so they shouldn't mix
    //HandlerInfo* handlerInfo = path->value.latest->writeHandler;
    //if (!handlerInfo || p->parameters.requestType != handlerInfo->callbackInfo.requestType)
    //    return Err_OK; // InternalError?

    // find correct MetaData from children
    // FIXME assumes low number of metadata infoitems; maybe use better search algo instead
    Path * scriptInfoItem = NULL;
    Path * end = tree.size + (Path *)&tree.sortedPaths[0];
    for (Path *child = path+1; child->depth > path->depth && child < end; ++child) {
        if (child->idHashCode == h_onwrite) { // TODO: check for read and call requests
            scriptInfoItem = child;
            if (scriptInfoItem->value.latest && scriptInfoItem->value.latest->current.value.str) {
                HString script;
                HString_init(&script, scriptInfoItem->value.latest->current.value.str);
                return ScriptEngine_run(p, path, &script);
            }
            break;
        }
    }
    return Err_OK; // Internal error ?
}

ErrorResponse handleWrite(OmiParser *p, Path *path, OdfParserEvent event) {
    Path* storedPathSegment = NULL;
    int resultIndex;
    const char * tmpStr;
    switch (event.type) {
        case PE_Path:
            if (!path->parent) return Err_OK; // Objects
            storedPathSegment = addPathSegment(&tree, path);
            if (!storedPathSegment) return Err_OOM;
            if (storedPathSegment->parent == path->parent) { // new was created (does not have internal parent yet)
                // store id
                HString odfId = {.value=path->odfId, .hash=path->idHashCode, .length=path->odfIdLength};
                storedPathSegment->odfId = storeHString(&odfId);
                storedPathSegment->flags |= PF_OdfIdMalloc;
                if (storedPathSegment->odfId == NULL) {
                    removePathSegment(&tree, path);
                    return Err_OOM_String;
                }
                // find parent
                if (odfBinarySearch(&tree, path->parent, &resultIndex)) {
                    storedPathSegment->parent = &tree.sortedPaths[resultIndex];
                } else { // Error; cleanup the mess
                    tmpStr = storedPathSegment->odfId;
                    removePathSegment(&tree, storedPathSegment);
                    freeString(tmpStr);
                    return Err_InternalError;
                }

                // TODO: save create-event here, delay trigger until value is processed

                // create value structure if InfoItem
                if (PathGetNodeType(path) == OdfInfoItem && !(path->flags & PF_ValueMalloc)) {
                    storedPathSegment->value.obj = poolCAlloc(&LatestValues);
                    if (!storedPathSegment->value.obj) { //Error, cleanup mess
                        tmpStr = storedPathSegment->odfId;
                        removePathSegment(&tree, storedPathSegment);
                        freeString(tmpStr);
                        return Err_OOM;
                    }
                    storedPathSegment->flags |= PF_ValueMalloc | PF_IsNewWithoutValue;
                    // TODO: add subscriptions to Object(s) elsewhere,
                    // if parent has subscriptions, copy them to child to keep subtree logic
                    HandlerInfo ** h = getHandlerInfoPtr(storedPathSegment->parent);
                    storedPathSegment->value.latest->writeHandler = h ? *h : NULL;
                }
            }
            // metadata-like value
            if (event.data) {
                storedPathSegment->flags |= PF_ValueMalloc;
                storedPathSegment->value.str = storeString(event.data);
                p->stringAllocator->free(event.data);
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
            if (path->flags & PF_ValueType != V_String){
                path->flags = (path->flags & ~PF_ValueType & ~PF_IsNewWithoutValue) | V_String | PF_IsNewWithoutValue;
            }
            //if (odfBinarySearch(&tree, path, &resultIndex)) {
            //    storedPathSegment = &tree.sortedPaths[resultIndex];
            //    storedPathSegment->value.latest->upcoming.typeString = event.data; // FIXME: free not needed
            //    return Err_OK;
            //}
            if (event.data) p->stringAllocator->free(event.data);
            return Err_OK; // break;
        case PE_ValueData:

            if (odfBinarySearch(&tree, path, &resultIndex)) {
                storedPathSegment = &tree.sortedPaths[resultIndex];

                LatestValue * latest = storedPathSegment->value.latest;

                bool wasAllocatedString =
                    (storedPathSegment->flags & (PF_ValueType | PF_ValueMalloc)) == (V_String | PF_ValueMalloc);

                bool changed = false, updated = false;

                // store value
                if (event.data) { // String type value
                    latest->upcoming.value.str = storeString(event.data);
                    if (!wasAllocatedString ||
                            (latest->current.value.str &&
                            strcmp(latest->upcoming.value.str, latest->current.value.str) != 0)) {
                        changed = true;
                    }
                } else {
                    latest->upcoming.value = path->value;
                    if (latest->upcoming.value.l != latest->current.value.l)
                        changed = true;
                }
                // check for missing timestamp
                if (!latest->upcoming.timestamp)
                    latest->upcoming.timestamp = getTimestamp();

                // events:
                if (latest->current.timestamp < latest->upcoming.timestamp) {
                    updated = true;
                }

                if (changed || updated) { // FIXME: why are both needed?
                    // add internal script subscription:
                    if (path->idHashCode == h_onwrite && path->parent->idHashCode == h_MetaData && event.data) {
                        // NOTE: allocated string re-used, FIXME: use HString as event data
                        HString script = {.value = event.data, .length = p->tempStringLength, .hash = p->stHash.hash};
                        // Check that code can be parsed
                        ErrorResponse parseResult = ScriptEngine_testParse(&script);
                        if (parseResult) {
                            p->stringAllocator->free(event.data);
                            return parseResult;
                        }
                        // create internal subscription if this is the first write
                        if (storedPathSegment->flags & PF_IsNewWithoutValue) {


                            Path * subscriptionTarget = storedPathSegment->parent->parent;
                            HandlerInfo* subInfo = NULL;

                            // Check existing // REPLACED WITH PF_IsNewWithoutValue FLAG
                            //HandlerInfo* handler = NULL;
                            //// TODO LatestValue structure needed for Object subscriptions
                            //
                            //if (subscriptionTarget->value.latest)
                            //    handler = storedPathSegment->parent->parent->value.latest->writeHandler;
                            //for (; handler; handler = handler->nextOther) {
                            //    if (handler->callbackInfo.connectionId == -2) {// script marker TODO: enum
                            //        subInfo = handler;
                            //        break;
                            //    }
                            //}
                            if (!subInfo) subInfo = (HandlerInfo*) poolAlloc(&HandlerInfoPool);
                            if (!subInfo) {
                                p->stringAllocator->free(event.data);
                                return Err_OOM;
                            }
                            *subInfo = (HandlerInfo){
                                .handlerType = HT_Script, 
                                .callbackInfo = p->parameters, 
                                .handler = handleScript,
                                .another = NULL,
                                .nextOther = NULL,
                                .prevOther = NULL,
                                //.parentPath = storedPathSegment->parent->parent,
                            };
                            subInfo->callbackInfo.connectionId = -2; // script write responses should not go anywhere
                            addSubscription(NULL, subscriptionTarget, &subInfo);
                        }
                    }
                    if (storedPathSegment->flags & PF_IsNewWithoutValue) {
                        // free old
                        if (wasAllocatedString && latest->current.value.str)
                            freeString(latest->current.value.str);
                        //storedPathSegment->flags &= ~PF_IsNewWithoutValue;
                    }
                    // update flags, retaining old, and reset type just in case, TODO: handle type change if history is implemented
                    storedPathSegment->flags = (storedPathSegment->flags & ~PF_ValueType & ~PF_IsNewWithoutValue) | path->flags;
                }
                //if (((storedPathSegment->flags & PF_ValueType) != V_String)
                //        && (latest->current.value.l != latest->upcoming.value.l)) changed = true;

                if (event.data) p->stringAllocator->free(event.data);

                // FIXME: optimize event responses (do not read the already found path)
                // update
                latest->current = latest->upcoming;
                latest->upcoming = (SingleValue) {0, 0, {.l = 0}};
                // Trigger events here
                if (updated)
                    for (HandlerInfo * handler = latest->writeHandler; handler; handler = handler->nextOther) {
                        // How to handle events and connections for subs
                        // 1. if event && non-started callback response:
                        //      todo: multiple event responses to the same ws connection requires implementation of a Queue
                        //      (openConnection(cb)); responseStartWithObjects();
                        // 2. track last path for all responses; create parents for each new response node
                        //      First path: loop parents; save last path
                        //      The rest: use subtree to close and open the next path
                        OmiRequestParameters requestParams = p->parameters;
                        p->parameters = handler->callbackInfo;
                        OdfParserEvent readEvent = OdfParserEvent(PE_Path, NULL);
                        if (!p->parameters.lastPath) {
                            // TODO: Open/find connection here
                            // connectionId = requestHandler.connectionFor(p->parameters.callbackAddr);
                            // cancel sub if not successful
                            // handleCancel()

                            if (p->parameters.callbackAddr && p->parameters.callbackAddr[0] == '0' && p->parameters.connectionId >= 0){ // skip on internal callbacks
                                connectionHandler.connections[p->parameters.connectionId].responsibleHandler = handler;
                                p->callbackOpenFlag |= 1 << p->parameters.connectionId;
                            }

                            for (Path * pathToOpen = p->pathStack; pathToOpen < p->currentOdfPath; ++pathToOpen){
                                ErrorResponse res = handler->handler(p, pathToOpen, readEvent);
                                // error reporting?
                                if (res) {
                                    p->parameters = requestParams;
                                    return res;
                                }
                            }
                        }
                        // Run handler for the rest
                        ErrorResponse res = handler->handler(p, storedPathSegment, readEvent);
                        // 
                        handler->callbackInfo = p->parameters;
                        p->parameters = requestParams;
                        if (res) {
                            p->parameters = requestParams;
                            return res;
                        }
                    }

                return Err_OK;
            }

            if (event.data) p->stringAllocator->free(event.data);
            return Err_InternalError; // break;
        case PE_RequestID:
            if (event.data) p->stringAllocator->free(event.data);
            return Err_InvalidElement; // actually invalid
        case PE_RequestEnd:
            // Subs 3. close nodes, requests (and connections, if started from here)
            //  use subtree to close
            while (p->callbackOpenFlag) {
                int connectionId = first1Bit(p->callbackOpenFlag);
                HandlerInfo * sub = connectionHandler.connections[connectionId].responsibleHandler;
                OmiRequestParameters requestParams = p->parameters;
                p->parameters = sub->callbackInfo;
                sub->handler(p, NULL, event);
                p->parameters.lastPath = NULL;
                p->parameters = requestParams;
                p->callbackOpenFlag ^= 1 << connectionId;
            }

            // Respond
            responseFullSuccess(&p->parameters); // FIXME: error cases: empty write? ...
            if (event.data) p->stringAllocator->free(event.data);
            return Err_OK;
    }
    return Err_OK;
}

typedef void (*NodeFunction)(OmiRequestParameters*, Path*, void*);
void respOpen(OmiRequestParameters* p, Path* path, void* extra){
    (void) extra;
    responseStartOdfNode(p, path);
}
void respClose(OmiRequestParameters* p, Path* path, void* extra){
    (void) extra;
    responseCloseOdfNode(p, path);
}
void noopCb(OmiRequestParameters* p, Path* n, void* e){
    (void) p, (void) n, (void) e;
}

// next: Node which marks the end for the subtree  = Objects when request is ended
void odfSubtree(OmiParser * p, Path * root, Path * next,
        NodeFunction open, NodeFunction close, void* param){
    
    // Include the root always, this way descriptions and MetaData can be requested
    open(&p->parameters, root, param);

    Path* currentPath = NULL;
    // leaf; recursively respond the subtree
    Path * lastResponsePath = root;
    int resultIndex;
    if(odfBinarySearch(&tree, root, &resultIndex)) {
        for (int i = resultIndex+1; i < tree.size; ++i) { // root already included, continue on the next index
            currentPath = &tree.sortedPaths[i];
            // Back to the same level as root; stop
            if (currentPath->depth <= root->depth) break;
            // (Stored) tree leaf encountered, go up to parents until at the level of currentPath
            while (currentPath->depth <= lastResponsePath->depth){
                close(&p->parameters, lastResponsePath, param);
                lastResponsePath = lastResponsePath->parent;
            }
            // Descriptions and Metadatas can be removed here unless specifically requested (root = request leaf)
            if (PathIsNonMeta(currentPath)){
                // Go down a level
                open(&p->parameters, currentPath, param);
                lastResponsePath = currentPath;
            } else { // Skip the whole sub tree of the currentPath
                for (++i; i < tree.size && tree.sortedPaths[i].depth > currentPath->depth; ++i); // skip loop body
                --i; // went one too far
            }
        }
    }
    // close the non-common parents between last (request or response) and current/next (request) path 
    while (lastResponsePath && PathGetDepth(lastResponsePath) >= PathGetDepth(next)) {
        // Ignoring descriptions and metadatas unless root/request leaf
        if (PathIsNonMeta(lastResponsePath) || pathCompare(lastResponsePath, root) == 0) {
            close(&p->parameters, lastResponsePath, param);
        }
        lastResponsePath = lastResponsePath->parent;
    }
}



ErrorResponse handleRead(OmiParser *p, Path *path, OdfParserEvent event) {
    Path* next = NULL;
    int resultIndex;
    NodeFunction open, close;
    HandlerInfo *subInfo;

    if (p->parameters.requestType == OmiRead) {
        open = respOpen;
        close = respClose;
    } else if (p->parameters.requestType == OmiSubscribe && p->parameters.interval == -1) {
        open = addSubscription; // NOTE: only used for subtree function, in order to only save the sub to request leafs and their children
        close = noopCb;
        subInfo = (HandlerInfo*) poolAlloc(&HandlerInfoPool);
        if (!subInfo) return Err_OOM;
        *subInfo = (HandlerInfo){
            .handlerType = HT_Subscription, 
            .callbackInfo = p->parameters, 
            .handler = handleRead,
            .another = NULL,
            .nextOther = NULL,
            .prevOther = NULL,
            //.parentPath = NULL,
        };
        subInfo->callbackInfo.requestType = OmiRead;
        subInfo->callbackInfo.lastPath = NULL;
    }
    switch (event.type) {
        case PE_Path:
            // Needs to know what and how many elements to close
            if (odfBinarySearch(&tree, path, &resultIndex)) {
                next = &tree.sortedPaths[resultIndex];
                // check if first event: response needs to be started
                if (! p->parameters.lastPath && p->parameters.requestType == OmiRead)
                    responseStartWithObjects(&p->parameters, 200);

                // Was the last path a leaf? Not leaf if last is a parent of current
                if (p->parameters.lastPath != next->parent) { // lastpath is leaf as next has different parent
                    odfSubtree(p, p->parameters.lastPath, next, open, close, &subInfo);
                } else if (p->parameters.lastPath) {
                    // lastpath not a leaf; add the node to the response already
                    responseStartOdfNode(&p->parameters, p->parameters.lastPath);
                }

                p->parameters.lastPath = next;
            } else {
                // 404
                if (p->parameters.requestType == OmiSubscribe) {
                    responseFullFailure(&p->parameters, 404, path->odfId, p);
                } else {
                    odfSubtree(p, p->parameters.lastPath, &tree.sortedPaths[0], respOpen, respClose, &subInfo);
                    responseEndWithFailure(&p->parameters, 404, path->odfId);
                }
                return Err_NotFound;
            }
            if (event.data) p->stringAllocator->free(event.data);
            return Err_OK;
        case PE_RequestEnd:
            //   if end: close remaining last.parents, reuse the bottom part of subtree function
            odfSubtree(p, p->parameters.lastPath, &tree.sortedPaths[0], open, close, &subInfo);
            if (p->parameters.requestType == OmiRead) {
                if (p->parameters.lastPath) {
                    responseEndWithObjects(&p->parameters);
                }
            } else if (p->parameters.requestType == OmiSubscribe && p->parameters.interval == -1) {
                uint requestId = subInfo - (HandlerInfo *) HandlerInfoPool.data;
                responseRequestId(&p->parameters, requestId);
            }
            if (event.data) p->stringAllocator->free(event.data);
            return Err_OK;
        default:
            if (event.data) p->stringAllocator->free(event.data);
            return Err_InternalError;
    }
}

ErrorResponse handleCancel(OmiParser * p, Path *path, OdfParserEvent event) {
    (void) path;
    uchar requestId;
    if (event.data) { // simple data type, free it already here for cleaner code
        requestId = *event.data;
        p->stringAllocator->free(event.data);
    } else return Err_InvalidElement;

    if (event.type == PE_RequestID) {
        HandlerInfo* subInfo = requestId + (HandlerInfo*)HandlerInfoPoolData;
        if (poolExists(&HandlerInfoPool, subInfo)) {
            for (HandlerInfo * next = subInfo; next; subInfo = next) {
                HandlerInfo * prev = subInfo->prevOther;
                next = subInfo->nextOther;
                if (prev) prev->nextOther = next;
                if (next) next->prevOther = prev;
                next = subInfo->another;
                poolFree(&HandlerInfoPool, subInfo);
            }
            responseFullSuccess(&p->parameters);
        } else {
            return Err_NotFound;
        }
    }
    return Err_OK;
}


// Use read handler because of identical access pattern
//ErrorResponse handleSubscription(OmiParser *p, Path *path, OdfParserEvent event) {
//    Path* next = NULL;
//    int resultIndex;
//    switch (event.type) {
//        case PE_Path:
//            break;
//        case PE_RequestEnd:
//            break;
//        default:
//            if (event.data) p->stringAllocator->free(event.data);
//            return Err_InternalError;
//    }
//    return 0;
//}

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
