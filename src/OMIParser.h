#ifndef OMIPARSER_H
#define OMIPARSER_H

#include "OMI.h"
#include "yxml.h"
#include "utils.h"

#include <string.h> //memset

#ifndef ParserPoolSize
#define ParserPoolSize 2
#endif

#ifndef ParserSinglePassLength
#define ParserSinglePassLength 256
#endif

#ifndef ParserMaxStringLength
#define ParserMaxStringLength 128
#endif

#ifndef XmlParserBufferSize
#define XmlParserBufferSize 512
#endif

// Something like finite state machine here

typedef enum OmiParserState {
    OmiState_Ready = 0,
    OmiState_PreOmiEnvelope,    // before "_<omiEnvelope "
    OmiState_OmiEnvelopeAttr,   // inside "<omiEnvelope _>"
    OmiState_PreVerb,           // after omeEnvelope open tag, before request verb "<omiEnvelope>_<"
    OmiState_VerbAttr,          // inside omi request verb
    OmiState_PreMsg,
    OmiState_Msg,
} OmiParserState;

typedef enum OdfParserState {
    OdfState_Ready = 0,
    OdfState_PreObjects,
    OdfState_ObjectsAttr,
    OdfState_PreObject,
    OdfState_ObjectAttr,
    OdfState_PreId,
    OdfState_Id,
    OdfState_ObjectChildren,
    OdfState_InfoItemAttr,
    OdfState_InfoItemChildren,
    OdfState_InfoItemMeta,
    OdfState_ValueAttrs,
    OdfState_Value,
} OdfParserState;

typedef enum OdfElementType {
    OdfObject,
    OdfInfoItem,
    OdfMetaData,
    OdfType
} OdfElementType;

struct OmiParser {
    OmiRequestParameters parameters;
    uint bytesRead;
    uint stPosition; // tells the position in current state, used for comparison of tag names etc.
    PartialHash stHash; // hash state to construct hash for comparison of tag and attribute names
    OmiParserState st;
    yxml_t xmlSt;
    char xmlBuffer[XmlParserBufferSize];
};
typedef struct OmiParser OmiParser;

struct OdfParser {
    OdfElementType stack[16];
    OdfParserState st;
};
typedef struct OdfParser OdfParser;


typedef schar (*CharInputF)(const void *, const void *);

#endif
