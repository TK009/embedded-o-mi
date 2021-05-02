#ifndef OMIPARSER_H
#define OMIPARSER_H

#include "OMI.h"
#include "yxml.h"
#include "utils.h"
#include "OdfTree.h"

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

#ifndef OdfDepthLimit
#define OdfDepthLimit 15
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
    OdfState_ObjectObjects,
    OdfState_ObjectInfoItems,
    OdfState_InfoItem,
    OdfState_Description,
    OdfState_MetaData,
    OdfState_Value,
    OdfState_End,
} OmiParserState;

typedef enum ErrorResponse {
    Err_NonMatchingCloseTag = YXML_ECLOSE,
    Err_StackOverflow       = YXML_ESTACK,
    Err_XmlError            = YXML_ESYN,
    Err_InvalidCharRef      = YXML_EREF,
    Err_OK                  = 0,
    Err_End                 = 1,
    Err_InvalidElement      = 2,
    Err_InvalidDataFormat   = 3,
    Err_InvalidAttribute    = 4,
    Err_TooDeepOdf          = 5,
    Err_InternalError       = 6,
    Err_OOM_String          = 7,
    Err_OOM                 = 8,
    Err_NotImplemented      = 9,
    Err_NotFound            = 10,
} ErrorResponse;

typedef enum OdfParserEventType {
    PE_Path,
    PE_ValueUnixTime,
    PE_ValueDateTime,
    PE_ValueType,
    PE_ValueData,
    PE_RequestID,
    PE_RequestEnd,
} OdfParserEventType;

typedef struct OdfParserEvent {
    OdfParserEventType type;
    char * data;
} OdfParserEvent;

#define OdfParserEvent(...) (OdfParserEvent){__VA_ARGS__}

typedef struct OmiParser OmiParser;
// Allocate & Store a string (often p->tempString), length given as the last parameter
typedef char* (*StringCallback)(OmiParser *, const char *, size_t);
// Called at every o-df path level, return ErrorResponse. Should handle freeing of ParserEvent data
typedef ErrorResponse (*OdfPathCallback)(OmiParser *, Path *, OdfParserEvent); 
//typedef void (*ResponseCallback)(char *content); // Send

struct OmiParser {
    OmiParserState st;
    uint bytesRead;
    uint stPosition; // tells the position in current state, used for comparison of tag names etc.
    uint tempStringLength;
    // functions
    StringCallback stringCallback;
    OdfPathCallback odfCallback;
    Path* currentOdfPath;
    //ResponseCallback responseCallback; // not used in parser, but can be used in other callbacks
    uint callbackOpenFlag; // conection id bit array For closing subscription notifications
    
    PartialHash stHash; // hash state to construct hash for comparison of text content and attribute values
    Allocator * stringAllocator;
    OmiRequestParameters parameters;
    yxml_t xmlSt;
    Path pathStack[OdfDepthLimit]; // for odfCallback
    char tempString[ParserMaxStringLength];
    char xmlBuffer[XmlParserBufferSize];
};
OmiParser* OmiParser_init(OmiParser* p, uchar connectionId);
void OmiParser_destroy(OmiParser* p);

// Return true on success, false on failure
ErrorResponse OmiParser_pushPath(OmiParser* p, OdfId id, PathFlags flags);
Path* OmiParser_popPath(OmiParser* p);

char* storeTempString(OmiParser *p, const char * str, size_t stringLength);

//struct OdfParser {
//    OdfElementType stack[16];
//    //OdfParserState st;
//};
//typedef struct OdfParser OdfParser;


//// Parser input character pull function with any pointer parameter, parameter is saved to parser state
//// scrapped: use connection ids push chunk strategy instead
//typedef schar (*CharInputF)(void *);
//typedef struct ParserSource {
//  CharInputF *getChar;
//  void * parameter;
//} ParserSource;

OmiParser* getParser(uchar connectionId);
ErrorResponse runParser(OmiParser * p, char * inputChunk);
yxml_ret_t runXmlParser(OmiParser * p, char ** inputChunkP, uint maxBytes);

//StringStorage stringStorage;


OmiVersion parseOmiVersion(strhash xmlns);

typedef enum HandlerType {
    HT_Subscription, HT_InternalFunction, HT_Script
} HandlerType;
typedef struct HandlerInfo HandlerInfo;
struct HandlerInfo {
    HandlerType handlerType;
    OdfPathCallback handler;
    Path * parentPath;
    HandlerInfo * another; // next same sub
    HandlerInfo * prevOther; // previous different sub for the same path
    HandlerInfo * nextOther; // next different sub for the same path
    OmiRequestParameters callbackInfo; // use union of OmiParameters/ScriptInfo?
};
//#define HandlerInfo(...) {}

struct LatestValue {
    SingleValue current;
    SingleValue upcoming;
    // event sub, internal function, script
    HandlerInfo * writeHandler;
    HandlerInfo * readHandler;
    //OdfPathCallback writeHandler;
    //OdfPathCallback readHandler;
};

#endif
