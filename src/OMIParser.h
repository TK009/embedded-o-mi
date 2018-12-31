#ifndef OMIPARSER_H
#define OMIPARSER_H

#include "OMI.h"
#include "XML.h"


#ifndef ParserPoolSize
#define ParserPoolSize 5
#endif

// Something like finite state machine here

typedef enum OmiParserState {
    //OmiState_PreOmiEnvelope,    // before "_<omiEnvelope "
    OmiState_OmiEnvelopeAttr,   // inside "<omiEnvelope _>"
    OmiState_PreVerb,           // after omeEnvelope open tag, before request verb "<omiEnvelope>_<"
    OmiState_VerbAttr,          // inside omi request verb
    OmiState_PreMsg,
    OmiState_Msg,
} OmiParserState;

typedef enum OdfParserState {
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
    uint bytesRead;
    uint bytesToRead;
    OmiRequestParameters parameters;
    OmiParserState st;
    XmlState xmlst;
};
typedef struct OmiParser OmiParser;

struct OdfParser {
    OdfElementType stack[16];
    OdfParserState st;
};
typedef struct OdfParser OdfParser;


typedef schar (*CharInputF)(const void *, const void *);

#endif
