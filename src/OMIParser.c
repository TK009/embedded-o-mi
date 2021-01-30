

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



// Finite state machine here
// Pass only zero ended cstrings and initialized parser state
int runParser(OmiParser * p, char * inputChunk) {
    while (*inputChunk) {
        yxml_ret_t r = runXmlParser(p, &inputChunk, ParserSinglePassLength);
        yield();
        if (r == YXML_OK) continue;
        // TODO Parser can return 8 byte strings or xml event
        // To handle strings, a string storage is made with a block of memory and hash table with a skip list; skip list needs memory pool
        switch (p->st) {
            case OmiState_Ready:
                break;
            case OmiState_PreOmiEnvelope:
                break;
            case OmiState_OmiEnvelopeAttr:
                break;
            case OmiState_PreVerb:
                break;
            case OmiState_VerbAttr:
                break;
            case OmiState_PreMsg:
                break;
            case OmiState_Msg:
                break;
        }
    }
}

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
