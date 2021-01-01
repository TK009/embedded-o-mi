

#include "OMIParser.h"

// Parser memory pool
OmiParser parsers[ParserPoolSize] = {{0}};


bool initParser() {
    for (int i = 0; i < ParserPoolSize; ++i) {
        OmiParser * p = &parsers[i];
        if (p->st == OmiState_Ready) {
            p->bytesRead = 0;
            memset(&p->parameters, 0, sizeof(p->parameters));
            p->parameters.arrival = getTimestamp();
            p->st = OmiState_PreOmiEnvelope;

            //p->xmlst = 0;
            yxml_init(&p->xmlSt, &p->xmlBuffer, XmlParserBufferSize);

            return true;
        }
    }
    return false;
}


// Finite state machine here

int runParser(OmiParser * p) {
    char nameString[32];
    for (uint passLength = ParserSinglePassLength; passLength > 0; --passLength) {
        passLength += runXmlParser(p, ParserSinglePassLength - passLength, nameString, sizeof(nameString));
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


int runXmlParser(OmiParser * p, uint maxBytes, char *stringReturn, uint stringReturnLength) {
}
