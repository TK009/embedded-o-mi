

#include "OMIParser.h"
#include "ParserUtils.h"

// Parser memory pool
OmiParser parsers[ParserPoolSize] = {{0}};
// String storage for ids and string values
//StringStorage stringStorage // TODO
static char* storeTempString(OmiParser *p) {
    char* s = malloc(p->tempStringLength); // TODO change to string storge
    if (s)
        memcpy(s, p->tempString, p->tempStringLength);
    return s;
}
#define storeTempStringOrError(resultVariable) \
    resultVariable = p->stringCallback(p); \
    if (!resultVariable) return Err_OOM_String

OmiParser* OmiParser_init(OmiParser* p, uchar connectionId) {
    if (p) {
        memset(&p->pathStack, 0, sizeof(p->pathStack));
        Path_init(&(p->pathStack[0]), 1, NULL, s_Objects, 0);
        *p = (OmiParser){
            .connectionId = connectionId, // use same as i?
            .bytesRead = 0,
            .stPosition = 0,
            .tempStringLength = 0,
            .stringCallback = (StringCallback) storeTempString, // TODO: change to string storage
            .odfCallback = NULL, //odfHandler
            .stHash = {0},
            .st = OmiState_PreOmiEnvelope,
            //.parameters = OmiRequestParameters_INITIALIZER(arrival
            .tempString = "",
            .currentOdfPath = &p->pathStack[0]
            };
        memset(&p->parameters, 0, sizeof(p->parameters));
        p->parameters.arrival = getTimestamp();
        yxml_init(&p->xmlSt, &p->xmlBuffer, XmlParserBufferSize);
    }
    return p;
}
OmiParser* getParser(uchar connectionId) {
    for (int i = 0; i < ParserPoolSize; ++i) {
        OmiParser * p = &parsers[i];
        if (p->st == OmiState_Ready) {
            return OmiParser_init(p, connectionId);
        }
    }
    // TODO: parse ttls of requests and check if any expired to make room for the new
    return NULL;
}


bool omiParser_pushPath(OmiParser* p, OdfId id, PathFlags flags) {
    int depth = p->currentOdfPath->depth;
    if (depth >= OdfDepthLimit) return false;

    // FIXME: alloc id!
    Path* newPath = Path_init(p->currentOdfPath+1, depth+1, p->currentOdfPath, id, flags);
    p->currentOdfPath = newPath;
    return true;
}
Path* omiParser_popPath(OmiParser* p){
    Path* old = p->currentOdfPath;
    // FIXME: free id!
    p->currentOdfPath = p->currentOdfPath - 1;
    return old;
}


#define elementEquals(tag) (strcmp(p->xmlSt.elem, s_ ## tag) == 0)
#define requireTag(tag, state) \
    if (elementEquals(tag)) { \
        p->st = state; \
    } else return Err_InvalidElement


static inline OmiVersion parseVersion(strhash xmlns) {
    switch (xmlns) {
        case h_xmlnsOmi2:   return OmiV2_ns;
        case h_xmlnsOmi1:   return OmiV1_ns;
        case h_xmlnsOmi1_0: return OmiV1_0_ns;
        case h_v1:          return OmiV1;
        case h_v2:          return OmiV2;
        default:            return OmiV_unknown;
    }
}

static inline void dataToTempString(OmiParser *p){
    char* data = p->xmlSt.data;
    for (uint *i = &(p->tempStringLength); *data && *i < ParserMaxStringLength; ++*i) 
        p->tempString[*i] = *(data++);
}


static inline int processOmiEnvelope(OmiParser *p, yxml_ret_t r){
    switch (r) {
        case YXML_ATTRSTART:
            p->tempStringLength = 0;
            break;
        case YXML_ATTRVAL:
            dataToTempString(p);
            break;
        case YXML_ATTREND:
            switch (calcHashCode(p->xmlSt.attr)) {
                case h_xmlns:
                case h_version:
                    p->parameters.version = max(p->parameters.version, parseVersion(p->stHash.hash));
                    break;
                case h_ttl:
                    p->parameters.deadline = p->parameters.arrival + parseFloat(p->tempString); // time unit?; full seconds or ms?
                    break;
            }
            break;
        case YXML_ELEMSTART:
            switch (calcHashCode(p->xmlSt.elem)) {
                case h_read:
                    p->parameters.requestType = OmiRead; break;
                case h_write:
                    p->parameters.requestType = OmiWrite; break;
                case h_cancel:
                    p->parameters.requestType = OmiCancel; break;
                case h_call:
                    p->parameters.requestType = OmiCall; break;
                case h_response:
                    p->parameters.requestType = OmiResponse;
                    p->st = OmiState_Response;
                    return 0;
                default: return Err_InvalidElement;
            }
            p->st = OmiState_Verb;
            return 0;
        default: break; 
    }
    return 0;
}

static inline int processVerb(OmiParser *p, yxml_ret_t r) {
    switch (r) {
        case YXML_ATTRSTART:
            p->tempStringLength = 0;
            break;
        case YXML_ATTRVAL:
            dataToTempString(p);
            break;
        case YXML_ATTREND:
            switch (calcHashCode(p->xmlSt.attr)) {
                case h_msgformat:
                    if (strcmp(p->tempString, s_odf))
                        p->parameters.format = OmiOdf;
                    else return Err_InvalidDataFormat;
                case h_interval:
                    switch (p->parameters.requestType) {
                        case OmiRead:
                            p->parameters.requestType = OmiSubscribe; break;
                        case OmiSubscribe: 
                            break; // FIXME
                        default: return Err_InvalidAttribute; // interval can only be used with read to create a subscription.
                    }
                    p->parameters.deadline = parseFloat(p->tempString); // NOTE time unit?; full seconds or ms?
                    break;
                case h_callback:
                    storeTempStringOrError(p->parameters.callbackAddr);
                    break;
                default:
                    break;
            }
            break;
        case YXML_ELEMSTART:
            switch (calcHashCode(p->xmlSt.elem)) {
                case h_msg:
                    p->st = OmiState_Msg;
                    return 0;
                case h_requestID: // requestID allowed for every request for now (only cancel requires it)
                    p->st = OmiState_RequestID;
                    return 0;
                default: return Err_InvalidElement;
            }
        default: break;
    }
    return 0;
}
static inline int processResult(OmiParser *p, yxml_ret_t r){
    if (r == YXML_ELEMSTART) {
        requireTag(return, OmiState_Return);
    }
    return 0;
}

static inline int processReturn(OmiParser *p, yxml_ret_t r){
    switch (r) {
        case YXML_ATTRSTART:
            p->tempStringLength = 0;
            break;
        case YXML_ATTRVAL:
            dataToTempString(p);
            break;
        case YXML_ATTREND:
            switch (calcHashCode(p->xmlSt.attr)) {
                case h_returnCode:

                case h_description: // TODO: debug log?
                default: break;
            }
            break;
        case YXML_ELEMSTART:
            switch (calcHashCode(p->xmlSt.elem)) {
                case h_requestID:
                    p->st = OmiState_RequestID;
                    return 0;
                case h_msg:
                    p->st = OmiState_Msg;
                    return 0;
                default:
                    return Err_InvalidElement;
            }
        default: break;
    }
    return 0;
}
static inline int processRequestID(OmiParser *p, yxml_ret_t r){
    switch (r) {
        case YXML_CONTENT:
            dataToTempString(p);
            break;
        case YXML_ELEMEND:
            if (p->parameters.requestType == OmiResponse)
                p->st = OmiState_Return;
            else
                p->st = OmiState_Verb;
            break;
        case YXML_ELEMSTART: // not allowed
            return Err_InvalidElement;
        default: break;
    }
    return 0;
}
static inline int processMsg(OmiParser *p, yxml_ret_t r){
    if (p->parameters.format == OmiOdf) {
        if (r == YXML_ELEMSTART) {
            requireTag(Objects, OdfState_Objects);
        }
    } else {
        return Err_InvalidDataFormat; // Format not implemented
    }
    return 0;
}
static inline int processObjects(OmiParser *p, yxml_ret_t r){
    // TODO parse version
    if (r == YXML_ELEMSTART) {
        requireTag(Object, OdfState_Object);
    }
    // TODO: parse attributes as metadata
    return 0;
}
static inline int processObject(OmiParser *p, yxml_ret_t r){
    switch (r) {
        case YXML_ATTRSTART:
            p->tempStringLength = 0;
            break;
        case YXML_ATTRVAL:
            dataToTempString(p);
            break;
        case YXML_ATTREND:
            switch (calcHashCode(p->xmlSt.attr)) {
                case h_type: // TODO: Object "type" attribute
                    break; 
                default: break; // TODO Save other attributes as metadata
            }
        case YXML_ELEMSTART:
            requireTag(id, OdfState_Id);
            break;
        default: break;
    }
    return 0;
}
static inline int processId(OmiParser *p, yxml_ret_t r){
    const char * id;
    switch (r) {
        case YXML_CONTENT:
            dataToTempString(p);
            break;
        case YXML_ELEMEND:
            id = p->stringCallback(p);
            omiParser_pushPath(p, id, 0);
            // FIXME: maybe not here ?: odfCallback(p->currentOdfPath);

            p->st = OdfState_ObjectInfoItems;
            break;
        case YXML_ELEMSTART: // not allowed
            return Err_InvalidElement;
        default: break;
    }
    return 0;
}
static inline int processObjectInfoItems(OmiParser *p, yxml_ret_t r){
    switch (r) {
        case YXML_ELEMSTART:
            switch (calcHashCode(p->xmlSt.elem)) {
                case h_description:
                    p->st = OdfState_Description;
                    break;
                case h_InfoItem:
                    p->st = OdfState_InfoItem;
                    break;
                case h_Object: // Technically should only be after all InfoItems, but now allowed also before
                    p->st = OdfState_Object;
                    break;
                default:
                    return Err_InvalidElement;
            }
            break;
        case YXML_ELEMEND:
            if (elementEquals(Object)) {
                omiParser_popPath(p);
                if (p->currentOdfPath->depth <= 1)
                    p->st = OdfState_End;
                else
                    p->st = OdfState_ObjectObjects;
            }
            break;
        default: break;
    }
    return 0;
}
static inline int processObjectObjects(OmiParser *p, yxml_ret_t r){
    switch (r) {
        case YXML_ELEMSTART:
            requireTag(Object, OdfState_Object);
            break;
        case YXML_ELEMEND:
            if (p->currentOdfPath->depth == 1){
                p->st = OdfState_End;
            } else {
                omiParser_popPath(p);
            }
        default: break;
    }
    return 0;
}
static inline int processDescription(OmiParser *p, yxml_ret_t r){
    switch (r) {
        case YXML_CONTENT:
            dataToTempString(p);
            break;
        case YXML_ELEMEND:
            // TODO: Save description

            if (p->currentOdfPath->parent->flags & PF_IsInfoItem)
                p->st = OdfState_InfoItem;
            else // if (inside object)
                p->st = OdfState_ObjectInfoItems;
            break;
        case YXML_ELEMSTART: // not allowed
            return Err_InvalidElement;
        default: break;
    }
    return 0;
}
static inline int processInfoItem(OmiParser *p, yxml_ret_t r){
    switch (r) {
        case YXML_ATTRSTART:
            p->tempStringLength = 0;
            break;
        case YXML_ATTRVAL:
            dataToTempString(p);
            break;
        case YXML_ATTREND:
            switch (calcHashCode(p->xmlSt.attr)) {
                case h_name:
                    omiParser_pushPath(p, p->tempString, PF_IsInfoItem);
                    break; 
                case h_type: // TODO: Save InfoItem "type" attribute
                    break;
                default: break; // TODO: Save others as metadata
            }
        case YXML_ELEMSTART:
            switch (calcHashCode(p->xmlSt.elem)) {
                case h_description:
                    p->st = OdfState_Description;
                    break;
                case h_MetaData:
                    p->st = OdfState_MetaData;
                    omiParser_pushPath(p, s_MetaData, PF_IsMetaData);
                    break;
                case h_value:
                    p->st = OdfState_Value;
            }
            break;
        case YXML_ELEMEND:
            omiParser_popPath(p);
            if (p->currentOdfPath->parent->flags & PF_IsMetaData)
                p->st = OdfState_MetaData;
            else //if (inside Object)
                p->st = OdfState_ObjectInfoItems;
            break;
        default: break;
    }
    return 0;
}

static inline int processMetaData(OmiParser *p, yxml_ret_t r){
    switch (r) {
        case YXML_ELEMSTART:
            requireTag(InfoItem, OdfState_InfoItem);
            break;
        case YXML_ELEMEND:
            omiParser_popPath(p);
            p->st = OdfState_InfoItem;
            break;
        case YXML_ATTRSTART:
            return Err_InvalidAttribute;
        default:
            break;
    }
    return 0;
}
static inline int processValue(OmiParser *p, yxml_ret_t r){
    switch (r) {
        case YXML_ATTRSTART:
            p->tempStringLength = 0;
            break;
        case YXML_ATTRVAL:
            dataToTempString(p);
            break;
        case YXML_ATTREND:
            switch (calcHashCode(p->xmlSt.attr)) {
                case h_unixTime: // TODO: Save time
                    break; 
                case h_dateTime: // TODO: Save time
                    break;
                case h_type: // TODO: save type
                    break;
                default: break; // TODO: Save others as metadata
            }
            p->tempStringLength = 0;
            break;
        case YXML_CONTENT:
            dataToTempString(p);
            break;
        case YXML_ELEMSTART:
            if (!elementEquals(value)) { // FIXME: Error?
                return Err_InvalidElement;
            }
            break;
        case YXML_ELEMEND:
            // FIXME: store value, call odfCallback
            // p->tempString
            p->st = OdfState_InfoItem;
            break;
        default:
            break;
    }
    return 0;

}


#define StateMachineHandler(fun) \
    ret = fun(p, r); \
    if (!ret) return ret; \
    break

// Finite state machine here
// Pass only zero ended cstrings and initialized parser state
ErrorResponse runParser(OmiParser * p, char * inputChunk) {
    while (*inputChunk) {
        yxml_ret_t r = runXmlParser(p, &inputChunk, ParserSinglePassLength);
        yield();
        // Global effects like errors and non interesting ok case
        // TODO Error handling
        switch (r) {
            case YXML_OK:
                continue;
            case YXML_EREF: // Invalid character ref
            case YXML_ECLOSE: // Nonmatching close tag
            case YXML_ESTACK: // Stack overflow (OOM)
            case YXML_ESYN: // Miscellaneous error
                return r;
            default:
                break;
        }
        // TODO 
        // To handle strings, a string storage is made with a block of memory and hash table with a skip list; skip list needs memory pool
        // Buffer is needed for strings not yet fully received
        ErrorResponse ret;
        switch (p->st) {
            case OmiState_Ready:
                return Err_InternalError;
            case OmiState_PreOmiEnvelope:
                if (r == YXML_ELEMSTART){
                    requireTag(omiEnvelope, OmiState_OmiEnvelope);
                }
                break;
            case OmiState_OmiEnvelope:
                StateMachineHandler(processOmiEnvelope);
            case OmiState_Verb:
                StateMachineHandler(processVerb);
            case OmiState_Response:
                if (r == YXML_ELEMSTART) {
                    requireTag(result, OmiState_Result);
                }
                break;
            case OmiState_Result:
                StateMachineHandler(processResult);
            case OmiState_RequestID:
                StateMachineHandler(processRequestID);
            case OmiState_Return:
                StateMachineHandler(processReturn);
            case OmiState_Msg:
                StateMachineHandler(processMsg);
            case OdfState_Objects:
                StateMachineHandler(processObjects);
            case OdfState_Object:
                StateMachineHandler(processObject);
            case OdfState_Id:
                StateMachineHandler(processId);
            case OdfState_ObjectObjects:
                StateMachineHandler(processObjectObjects);
            case OdfState_ObjectInfoItems:
                StateMachineHandler(processObjectInfoItems);
            case OdfState_InfoItem:
                StateMachineHandler(processInfoItem);
            case OdfState_Description:
                StateMachineHandler(processDescription);
            case OdfState_MetaData:
                StateMachineHandler(processMetaData);
            case OdfState_Value:
                StateMachineHandler(processValue);
            case OdfState_End:
                if (r == YXML_ELEMEND && elementEquals(omiEnvelope)) {
                    yxml_ret_t r = yxml_eof(&p->xmlSt);
                    if (r < 0) return r;
                    else return Err_End;
                }
                if (r == YXML_ELEMSTART) return Err_InvalidElement;
        }
    }
    return 0; // TODO: ?
}

// Returns some interesting parser result, unless reaches the maxBytes processed
// **inputChunkP: pointer to pointer of current position in source string
yxml_ret_t runXmlParser(OmiParser * p, char ** inputChunkP, uint maxBytes) {
    char * doc = *inputChunkP;
    char * stop = doc + maxBytes;
    yxml_ret_t r = YXML_OK;
    for(;(r != YXML_OK) || *doc || (doc == stop); doc++) {
        r = yxml_parse(&p->xmlSt, *doc);
    }
    return r;
}
