

#include "OMIParser.h"

// Parser memory pool
OmiParser parsers[ParserPoolSize] = {{0}};
// String storage for ids and string values
//StringStorage stringStorage // TODO

OmiParser* initParser(uchar connectionId) {
    for (int i = 0; i < ParserPoolSize; ++i) {
        OmiParser * p = &parsers[i];
        if (p->st == OmiState_Ready) {
            p->connectionId = connectionId; // use same as i?
            p->bytesRead = 0;
            memset(&p->parameters, 0, sizeof(p->parameters));
            p->parameters.arrival = getTimestamp();
            p->st = OmiState_PreOmiEnvelope;

            //p->xmlst = 0;
            yxml_init(&p->xmlSt, &p->xmlBuffer, XmlParserBufferSize);

            return p;
        }
    }
    // TODO: parse ttls of requests and check if any expired to make room for the new
    return NULL;
}


#define elementEquals(tag) (strcmp(p->xmlSt.elem, s_ ## tag) == 0)

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

static inline int processOmiEnvelope(OmiParser *p, yxml_ret_t r){
    switch (r) {
        case YXML_ATTRVAL:
        case YXML_ATTREND:
            switch (calcHashCode(p->xmlSt.attr)) {
                case h_xmlns:
                case h_version:
                    p->parameters.version = max(p->parameters.version, parseVersion(p->stHash.hash));
                    break;
                case h_ttl:
                    p->parameters.deadline = p->parameters.arrival + parseFloat(p->temp); // TODO unit ?
                    break;
            }
            break;
        case YXML_ELEMSTART:
            switch (calcHashCode(p->xmlSt.elem)) {
                case h_read:
                    p->parameters.requestType = OmiRead;
                    break;
                case h_write:
                    p->parameters.requestType = OmiWrite;
                    break;
                case h_cancel:
                    p->parameters.requestType = OmiCancel;
                    break;
                case h_response:
                    p->parameters.requestType = OmiResponse;
                    break;
                case h_call:
                    p->parameters.requestType = OmiCall;
                    break;
                default:
                    return 5; // ERR invalid element
            }
            break;
        default: break; 
    }
}
static inline int processVerb(OmiParser *p, yxml_ret_t r);
static inline int processPreMsg(OmiParser *p, yxml_ret_t r);
static inline int processMsg(OmiParser *p, yxml_ret_t r);

// Finite state machine here
// Pass only zero ended cstrings and initialized parser state
int runParser(OmiParser * p, char * inputChunk) {
    while (*inputChunk) {
        yxml_ret_t r = runXmlParser(p, &inputChunk, ParserSinglePassLength);
        yield();
        // Global effects like errors and non interesting ok case
        // TODO Error handling
        switch (r) {
            case YXML_OK: continue;
            case YXML_EREF:
                // Invalid character ref
                return 1;
            case YXML_ECLOSE:
                // Nonmatching close tag
                return 2;
            case YXML_ESTACK:
                // Stack overflow (OOM)
                return 3;
            case YXML_ESYN:
                // Miscellaneous error
                return 4;
            default: break;
        }
        // TODO Parser can return 8 byte strings or xml event
        // To handle strings, a string storage is made with a block of memory and hash table with a skip list; skip list needs memory pool
        // Buffer is needed for strings not yet fully received
        switch (p->st) {
            case OmiState_Ready:
                return 6; // ERR
            case OmiState_PreOmiEnvelope:
                if (r == YXML_ELEMSTART){
                    if (elementEquals(omiEnvelope))
                        p->st = OmiState_OmiEnvelope;
                    else
                        return 5; // ERR, invalid root element
                }
                break;
            case OmiState_OmiEnvelope:
                processOmiEnvelope(p, r);
                break;
            case OmiState_Verb:
                processVerb(p, r);
                break;
            case OmiState_PreMsg:
                processPreMsg(p, r);
                break;
            case OmiState_Msg:
                processMsg(p, r);
                break;
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
