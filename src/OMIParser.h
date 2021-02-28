#ifndef OMIPARSER_H
#define OMIPARSER_H

#include "OMI.h"
#include "yxml.h"
#include "utils.h"
#include "ODFTree.h"

#include <string.h> //memset

#ifndef ParserPoolSize
#define ParserPoolSize 2
#endif

#ifndef ParserSinglePassLength
#define ParserSinglePassLength 1024
#endif

#ifndef ParserMaxStringLength
#define ParserMaxStringLength 256
#endif

#ifndef XmlParserBufferSize
#define XmlParserBufferSize 512
#endif

// Something like finite state machine here

typedef enum OmiParserState {
    OmiState_Ready = 0,
    OmiState_PreOmiEnvelope,    // before "_<omiEnvelope "
    //OmiState_OmiEnvelopeAttr,   // inside "<omiEnvelope _>"
    OmiState_OmiEnvelope,   // inside "<omiEnvelope _>" or after
    //OmiState_PreVerb,           // after omeEnvelope open tag, before request verb "<omiEnvelope>_<"
    //OmiState_VerbAttr,          // inside omi request verb
    OmiState_Verb,          // inside omi request verb
    OmiState_Response,
    OmiState_Result,
    OmiState_RequestID,
    OmiState_Return,
    OmiState_Msg,
    OdfState_Objects,
    OdfState_Object,
    OdfState_Id,
    OdfState_ObjectChildren,
    OdfState_InfoItem,
    OdfState_Description,
    OdfState_MetaData,
    OdfState_Value,
} OmiParserState;

typedef enum ErrorResponse {
    Err_OK = 0,
    Err_NonMatchingCloseTag = YXML_ECLOSE,
    Err_StackOverflow = YXML_ESTACK,
    Err_XmlError = YXML_ESYN,
    Err_InvalidCharRef = YXML_EREF,
    Err_InvalidElement,
    Err_InvalidDataFormat,
    Err_InvalidAttribute,
    Err_InternalError,
    Err_OOM_String,
    Err_OOM,
} ErrorResponse;

typedef enum OdfElementType {
    OdfObject,
    OdfInfoItem,
    OdfMetaData,
    OdfType
} OdfElementType;

typedef struct OmiParser OmiParser;
typedef char* (*StringCallback)(OmiParser *); // store p->tempString
typedef int (*OdfPathCallback)(OmiParser *, Path);
typedef void (*ResponseCallback)(char *content);

struct OmiParser {
    uchar connectionId;
    uint bytesRead;
    uint stPosition; // tells the position in current state, used for comparison of tag names etc.
    uint tempStringLength;
    // functions
    StringCallback stringCallback;
    OdfPathCallback odfCallback;
    //ResponseCallback responseCallback; // not used in parser, but can be used in other callbacks

    
    PartialHash stHash; // hash state to construct hash for comparison of text content and attribute values
    OmiParserState st;
    OmiRequestParameters parameters;
    yxml_t xmlSt;
    char tempString[ParserMaxStringLength];
    char xmlBuffer[XmlParserBufferSize];
};
OmiParser* OmiParser_init(OmiParser* p, uchar connectionId);

struct OdfParser {
    OdfElementType stack[16];
    //OdfParserState st;
};
typedef struct OdfParser OdfParser;


//// Parser input character pull function with any pointer parameter, parameter is saved to parser state
//// scrapped: use connection ids push chunk strategy instead
//typedef schar (*CharInputF)(void *);
//typedef struct ParserSource {
//  CharInputF *getChar;
//  void * parameter;
//} ParserSource;

OmiParser* getParser(uchar connectionId);
int runParser(OmiParser * p, char * inputChunk);
yxml_ret_t runXmlParser(OmiParser * p, char ** inputChunkP, uint maxBytes);

//StringStorage stringStorage;



#endif
