

#include "OMIParser.h"
#include "ParserUtils.h"
#include "OmiConstants.h"
#include "requestHandler.h"

// Parser memory pool
OmiParser parsers[ParserPoolSize] = {{0}};
// String storage for ids and string values
//StringStorage stringStorage // TODO

char* storeTempString(OmiParser *p, const char * str, size_t stringLength) {
    (void) p;
    size_t memoryLen = stringLength + 1; // + 1: null character
    char* s = p->stringAllocator->malloc(memoryLen);
    if (s) {
        memcpy(s, str, memoryLen);
        s[stringLength] = '\0';
    }
    return s;
}
#define endTempString(p) \
    p->tempString[p->tempStringLength] = '\0'

#define storeTempStringOrError(resultVariable) \
    endTempString(p); \
    resultVariable = storeTempString(p, p->tempString, p->tempStringLength); \
    if (!resultVariable) return Err_OOM_String


OmiParser* OmiParser_init(OmiParser* p, int connectionId) {
    if (p) {
        *p = (OmiParser){
            .bytesRead = 0,
            .stPosition = 0,
            .tempStringLength = 0,
            .stringAllocator = &stdAllocator, // TODO: change to string storage
            .odfCallback = handleRequestPath, //odfHandler
            .stHash = emptyPartialHash,
            .st = OmiState_PreOmiEnvelope,
            //.parameters = OmiRequestParameters_INITIALIZER(arrival
            .tempString = "",
            .currentOdfPath = &p->pathStack[0],
            .callbackOpenFlag = 0,
            };
        memset(&p->pathStack, 0, sizeof(p->pathStack));
        Path_init(&(p->pathStack[0]), 1, OdfObject, NULL, s_Objects, 0);
        memset(&p->parameters, 0, sizeof(p->parameters));
        p->parameters.arrival = getTimestamp();
        p->parameters.connectionId = connectionId; // use same as i?
        yxml_init(&p->xmlSt, &p->xmlBuffer, XmlParserBufferSize);
    }
    return p;
}
OmiParser* getParser(int connectionId) {
    for (int i = 0; i < ParserPoolSize; ++i) {
        OmiParser * p = &parsers[i];
        if (p->st == OmiState_Ready) {
            return OmiParser_init(p, connectionId);
        }
    }
    // TODO: parse ttls of requests and check if any expired to make room for the new
    return NULL;
}


ErrorResponse OmiParser_pushPath(OmiParser* p, OdfId id, NodeType type){ // flags) {
    int depth = PathGetDepth(p->currentOdfPath);
    if (depth >= OdfDepthLimit) return Err_TooDeepOdf;

    // allocates the id!
    char * copiedId = storeTempString(p, id, strlen(id));
    if (!copiedId) return Err_OOM;
    Path* newPath = Path_init(p->currentOdfPath+1, depth+1, type, p->currentOdfPath, copiedId, 0);
    p->currentOdfPath = newPath;
    return p->odfCallback(p, p->currentOdfPath, OdfParserEvent(PE_Path, NULL));
}
Path* OmiParser_popPath(OmiParser* p){
    Path* old = p->currentOdfPath;
    if (!old) return NULL;
    if (old->idHashCode != h_Objects)
        free((void*) old->odfId); // TODO: change hardcoded free function if custom allocator is used
    //if (old->flags & PF_ValueMalloc) // free handled by the odfCallback
    //    free((void*) old->value.str); // TODO: change hardcoded free function if custom allocator is used
    if (old == p->pathStack) // root
        p->currentOdfPath = NULL;
    else
        p->currentOdfPath = p->currentOdfPath - 1;
    return old;
}

void OmiParser_destroy(OmiParser* p){
    while (OmiParser_popPath(p)) {};
    if (p->parameters.callbackAddr) free(p->parameters.callbackAddr);
    p->st = OmiState_Ready;
}

#define elementEquals(tag) (strcmp(p->xmlSt.elem, s_ ## tag) == 0)
#define requireTag(tag, state) \
    if (elementEquals(tag)) { \
        p->st = state; \
    } else return Err_InvalidElement


OmiVersion parseOmiVersion(strhash xmlns) {
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
    for (uint *i = &(p->tempStringLength); *data && *i < ParserMaxStringLength-1; ++*i) {
        char c = *(data++);
        p->tempString[*i] = c;
        calcHashCodeC(c, &p->stHash);
    }
}
static inline void initTempString(OmiParser *p) {
    p->tempStringLength = 0;
    p->stHash = emptyPartialHash;
}


static inline int processOmiEnvelope(OmiParser *p, yxml_ret_t r){
    int ttl = 0;
    switch (r) {
        case YXML_ATTRSTART:
            initTempString(p);
            break;
        case YXML_ATTRVAL:
            dataToTempString(p);
            break;
        case YXML_ATTREND:
            switch (calcHashCode(p->xmlSt.attr)) {
                case h_xmlns:
                case h_version:
                    p->parameters.version = max(p->parameters.version, parseOmiVersion(p->stHash.hash));
                    break;
                case h_ttl:
                    ttl = parseFloat(p->tempString);
                    p->parameters.deadline =  (ttl > 0?
                            p->parameters.arrival + (uint) ttl
                            : 36500u * 24 * 3600); // time unit?; full seconds used instead of ms?
                    break;
                default: break; //proprietary attr
            }
            break;
        case YXML_ELEMSTART:
            switch (calcHashCode(p->xmlSt.elem)) {
                case h_response:
                    p->parameters.requestType = OmiResponse;
                    p->st = OmiState_Response;
                    return 0;
                case h_read:
                    p->parameters.requestType = OmiRead; break;
                case h_write:
                    p->parameters.requestType = OmiWrite; break;
                case h_cancel:
                    p->parameters.requestType = OmiCancel; break;
                case h_call:
                    p->parameters.requestType = OmiCall; break;
                case h_delete:
                    p->parameters.requestType = OmiDelete; break;
                default: return Err_InvalidElement;
            }
            p->st = OmiState_Verb;
            return 0;
        default: break;
    }
    return 0;
}

/* strategy for coverage
static inline int verbElems(OmiParser *p){
    switch (calcHashCode(p->xmlSt.elem)) {
        case h_msg:
            p->st = OmiState_Msg;
            return 0;
        case h_requestID: // requestID allowed for every request for now (only cancel requires it)
            p->st = OmiState_RequestID;
            return 0;
        default: return Err_InvalidElement;
    }
}*/

static inline int processVerb(OmiParser *p, yxml_ret_t r) {
    switch (r) {
        case YXML_ATTRSTART:
            initTempString(p);
            break;
        case YXML_ATTRVAL:
            dataToTempString(p);
            break;
        case YXML_ATTREND:
            switch (calcHashCode(p->xmlSt.attr)) {
                case h_msgformat:
                    if (p->stHash.hash == (uint) h_odf)
                        p->parameters.format = OmiOdf;
                    else return Err_InvalidDataFormat;
                    break;
                case h_interval:
                    switch (p->parameters.requestType) {
                        case OmiRead:
                            p->parameters.requestType = OmiSubscribe; break;
                        default: return Err_InvalidAttribute; // interval can only be used with read to create a subscription.
                    }
                    p->parameters.interval = parseFloat(p->tempString); // NOTE time unit?; full seconds or ms?
                    break;
                case h_callback:
                    //if (p->tempStringLength == 1 && p->tempString[0] == '0') { // current connection
                    //    p->tempString[0] = (char) p->parameters.connectionId + 10; // we don't want to use 0 (null)
                    //}
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
                    break;
                case h_requestID: // requestID allowed for every request for now (only cancel requires it)
                    p->st = OmiState_RequestID;
                    break;
                default: return Err_InvalidElement;
            }
            return Err_OK;
        case YXML_ELEMEND:
            p->st = OdfState_End;
        default: break;
    }
    return 0;
}
            //return verbElems(p);
static inline int processResponse(OmiParser *p, yxml_ret_t r){
    switch (r) {
        case YXML_ELEMSTART:
            requireTag(result, OmiState_Result);
            break;
        case YXML_ELEMEND:
            p->st = OdfState_End;
            return p->odfCallback(p, NULL, OdfParserEvent(PE_RequestEnd,NULL));
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
                    break;
                case h_msg:
                    p->st = OmiState_Msg;
                    break;
                default:
                    return Err_InvalidElement;
            }
        case YXML_ELEMEND:
            if (elementEquals(response)) // </result>: after ELEMEND, elem is set to parent tag
                p->st = OmiState_Response;
        default: break;
    }
    return 0;
}
static inline int processRequestID(OmiParser *p, yxml_ret_t r){
    char * rId;
    switch (r) {
        case YXML_CONTENT:
            dataToTempString(p);
            break;
        case YXML_ELEMEND:
            if (p->parameters.requestType == OmiResponse)
                p->st = OmiState_Return;
            else
                p->st = OmiState_Verb;
            storeTempStringOrError(rId);
            rId[0] = (uchar) parseInt(rId);
            rId[1] = '\0';
            return p->odfCallback(p, NULL, OdfParserEvent(PE_RequestID, rId));
        case YXML_ELEMSTART: // not allowed
            return Err_InvalidElement;
        default: return Err_InvalidAttribute;
    }
    return 0;
}
static inline int processMsg(OmiParser *p, yxml_ret_t r){
    if (p->parameters.format == OmiOdf) {
        if (r == YXML_ELEMSTART) {
            requireTag(Objects, OdfState_Objects);
            return p->odfCallback(p, p->currentOdfPath, OdfParserEvent(PE_Path, NULL));
        }
    }
    //else { // Error returned earlier when reading message format
    //return Err_InvalidDataFormat; // Format not implemented
    //}
    return 0;
}
static inline int processObjects(OmiParser *p, yxml_ret_t r){
    // TODO parse version
    if (r == YXML_ELEMSTART) {
        requireTag(Object, OdfState_Object);
    } else if (r == YXML_ELEMEND) {
        p->st = OdfState_End;
        return p->odfCallback(p, NULL, OdfParserEvent(PE_RequestEnd,NULL));
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
            break;
        case YXML_ELEMSTART:
            requireTag(id, OdfState_Id);
            initTempString(p);
            break;
        default: break;
    }
    return 0;
}
static inline int processId(OmiParser *p, yxml_ret_t r){
        // TODO: handle attrs like idType
        //case YXML_ATTRSTART:
        //    p->tempStringLength = 0;
        //    break;
        //case YXML_ATTRVAL:
        //    dataToTempString(p);
        //    break;
        //case YXML_ATTREND:
        //    ...
        //    p->tempStringLength = 0;
    switch (r) {
        case YXML_CONTENT:
            dataToTempString(p);
            break;
        case YXML_ELEMEND:
            p->st = OdfState_ObjectInfoItems;
            endTempString(p);
            return OmiParser_pushPath(p, p->tempString, OdfObject);
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
                    initTempString(p);
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
            // Commented; we can go to ObjectObjects as it is essentially the same as Objects
            //if (PathGetDepth(p->currentOdfPath) == 2)
            //    p->st = OdfState_Objects;
            //else
            p->st = OdfState_ObjectObjects;
            //}
            OmiParser_popPath(p);
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
            if (PathGetDepth(p->currentOdfPath) == 1){
                p->st = OdfState_End;
                return p->odfCallback(p, NULL, OdfParserEvent(PE_RequestEnd,NULL));
            } else {
                OmiParser_popPath(p);
            }
        default: break;
    }
    return 0;
}
static inline int processDescription(OmiParser *p, yxml_ret_t r){
    char * descriptionString;
    switch (r) {
        case YXML_CONTENT:
            dataToTempString(p);
            break;
        case YXML_ELEMEND:
            storeTempStringOrError(descriptionString);
            if (PathGetNodeType(p->currentOdfPath) == OdfInfoItem)
                p->st = OdfState_InfoItem;
            else // if (inside object)
                p->st = OdfState_ObjectInfoItems;

            return p->odfCallback(p,
                    Path_init(p->currentOdfPath+1, PathGetDepth(p->currentOdfPath)+1, OdfDescription, p->currentOdfPath, s_description, 0),
                    OdfParserEvent(PE_Path, descriptionString));
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
                    endTempString(p);
                    return OmiParser_pushPath(p, p->tempString, OdfInfoItem);
                case h_type: // TODO: Save InfoItem "type" attribute
                    break;
                default: break; // TODO: Save others as metadata
            }
            break;
        case YXML_ELEMSTART:
            switch (calcHashCode(p->xmlSt.elem)) {
                case h_description:
                    initTempString(p);
                    p->st = OdfState_Description;
                    break;
                case h_MetaData:
                    p->st = OdfState_MetaData;
                    return OmiParser_pushPath(p, s_MetaData, OdfMetaData);
                case h_value:
                    initTempString(p); // zero for content in case no attrs before value.
                    p->st = OdfState_Value;
                    break;
                default:
                    return Err_InvalidElement;
            }
            break;
        case YXML_ELEMEND:
            OmiParser_popPath(p);
            if (PathGetNodeType(p->currentOdfPath) == OdfMetaData)
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
            OmiParser_popPath(p);
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
    char * data = NULL;
    strhash hash;
    switch (r) {
        case YXML_ATTRSTART:
            initTempString(p);
            break;
        case YXML_ATTRVAL:
            dataToTempString(p);
            break;
        case YXML_ATTREND:
            storeTempStringOrError(data);
            hash = p->stHash.hash;
            initTempString(p); // zero for content if attrs ended. Due to returns, cannot be after the switch
            switch (calcHashCode(p->xmlSt.attr)) {
                case h_unixTime:
                    return p->odfCallback(p, p->currentOdfPath, OdfParserEvent(PE_ValueUnixTime, data));
                case h_dateTime:
                    return p->odfCallback(p, p->currentOdfPath, OdfParserEvent(PE_ValueDateTime, data));
                case h_type:
                    switch (hash) {
                        case h_xsint: case h_xsinteger: // h_xsdecimal
                            p->currentOdfPath->flags = V_Int;
                            break;
                        case h_xsfloat:
                            p->currentOdfPath->flags = V_Float;
                            break;
                        case h_xsdouble:
                            p->currentOdfPath->flags = V_Double;
                            break;
                        case h_xsshort:
                            p->currentOdfPath->flags = V_Short;
                            break;
                        case h_xsbyte:
                            p->currentOdfPath->flags = V_Byte;
                            break;
                        case h_xsstring:
                            p->currentOdfPath->flags = V_String;
                            break;
                        case h_xsboolean:
                            p->currentOdfPath->flags = V_Boolean;
                            break;
                        case h_xsunsignedInt: // h_xsnonNegativeInteger h_xspositiveInteger
                            p->currentOdfPath->flags = V_UInt;
                            break;
                        case h_xsunsignedShort:
                            p->currentOdfPath->flags = V_UShort;
                            break;
                        case h_xsunsignedByte:
                            p->currentOdfPath->flags = V_UByte;
                            break;
                        case h_xslong:
                            p->currentOdfPath->flags = V_Long;
                            break;
                        default:
                            p->currentOdfPath->flags = V_String;
                            return p->odfCallback(p, p->currentOdfPath, OdfParserEvent(PE_ValueType, data));
                    }
                    free(data);
                    return 0;
                default:
                    break;
            }
            free(data);
            return Err_InvalidAttribute;
        case YXML_ELEMEND:
            p->st = OdfState_InfoItem;
            switch ((ValueType) p->currentOdfPath->flags & PF_ValueType) {
                case V_Int: case V_Short: case V_Byte: case V_UShort: case V_UByte:
                    p->currentOdfPath->value.i = parseInt(p->tempString);
                    data = NULL;
                    break;
                case V_UInt: case V_Long:
                    p->currentOdfPath->value.l = parseLong(p->tempString);
                    data = NULL;
                    break;
                case V_Float:
                    p->currentOdfPath->value.f = parseFloat(p->tempString);
                    data = NULL;
                    break;
                case V_Double:
                    p->currentOdfPath->value.d = parseDouble(p->tempString);
                    data = NULL;
                    break;
                case V_Boolean:
                    p->currentOdfPath->value.b = parseBoolean(p->stHash.hash);
                    data = NULL;
                    break;
                case V_String:
                    storeTempStringOrError(data);
                    p->currentOdfPath->flags |= PF_ValueMalloc;
                    p->currentOdfPath->value.str = data;
                    break;
            }
            return p->odfCallback(p, p->currentOdfPath, OdfParserEvent(PE_ValueData, data));
        case YXML_CONTENT:
            dataToTempString(p);
            break;
        default:
            return Err_InvalidElement;
    }
        //case YXML_ELEMSTART: // Moved to default
            //if (!elementEquals(value)) { // FIXME: Error? implement Call request?
    return 0;

}


#define StateMachineHandler(fun) \
    ret = fun(p, r); \
    if (ret) return ret; \
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
                return (ErrorResponse)r;
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
                    // commented because this state is used to recover errors
                    //requireTag(omiEnvelope, OmiState_OmiEnvelope);
                    // FIXME: will cause problems if data contains omiEnvelope after error
                    if (elementEquals(omiEnvelope)) {
                        p->st = OmiState_OmiEnvelope;
                    } else {
                        yxml_init(&p->xmlSt, &p->xmlBuffer, XmlParserBufferSize);
                    }
                }
                break;
            case OmiState_OmiEnvelope:
                StateMachineHandler(processOmiEnvelope);
            case OmiState_Verb:
                StateMachineHandler(processVerb);
            case OmiState_Response:
                StateMachineHandler(processResponse);
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
                //if (r == YXML_ELEMEND && elementEquals(omiEnvelope)) {
                //    yxml_ret_t r = yxml_eof(&p->xmlSt);
                //    if (r < 0) return (ErrorResponse)r;
                //    else return Err_End;
                //}
                if (r == YXML_ELEMSTART) return Err_InvalidElement;
                break;
        }
    }
    if (p->st == OdfState_End) {
        yxml_ret_t r = yxml_eof(&p->xmlSt);
        if (r < 0) return Err_OK; // don't know if there is more characters
        else return Err_End;
    }
    return 0; // TODO: ?
}

// Returns some interesting parser result, unless reaches the maxBytes processed
// **inputChunkP: pointer to pointer of current position in source string
yxml_ret_t runXmlParser(OmiParser * p, char ** inputChunkP, uint maxBytes) {
    char * doc = *inputChunkP;
    char * stop = doc + maxBytes;
    yxml_ret_t r = YXML_OK;
    for(;(r == YXML_OK) && *doc && (doc != stop); doc++) {
        r = yxml_parse(&p->xmlSt, *doc);
    }
    *inputChunkP = doc;
    return r;
}
