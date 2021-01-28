

#include "OMIParser.h"

// Parser memory pool
OmiParser parsers[ParserPoolSize] = {{0}};


OmiParser* initParser() {
    for (int i = 0; i < ParserPoolSize; ++i) {
        OmiParser * p = &parsers[i];
        if (p->st == OmiState_Ready) {
            p->bytesRead = 0;
            memset(&p->parameters, 0, sizeof(p->parameters));
            p->parameters.arrival = getTimestamp();
            p->st = OmiState_PreOmiEnvelope;

            //p->xmlst = 0;
            yxml_init(&p->xmlSt, &p->xmlBuffer, XmlParserBufferSize);

            return p;
        }
    }
    return NULL;
}


// Finite state machine here

int runParser(OmiParser * p) {
    char nameString[32];
    for (uint passLength = ParserSinglePassLength; passLength > 0; --passLength) {
        yxml_ret_t r = runXmlParser(p, ParserSinglePassLength - passLength, nameString, sizeof(nameString));
        // TODO 
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

// TODO how to pass characters
yxml_ret_t runXmlParser(OmiParser * p, uint *current, uint maxBytes, char *stringReturn, uint stringReturnLength) {
    for(; *doc; doc++) {
        yxml_ret_t r = yxml_parse(x, *doc);
        if(r != YXML_OK)
            return r
    }
}
