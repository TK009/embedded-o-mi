#ifndef CONNECTIONHANDLER_H
#define CONNECTIONHANDLER_H

#include "OMI.h"
#include "OdfTree.h"
#include "OMIParser.h"

#ifndef NumConnections
#define NumConnections 10
#endif

typedef int (*Printf)(const char*, ...);

typedef struct ConnectionState {
    eomi_bool started;
    char *address;
    HandlerInfo *responsibleHandler;
} ConnectionState;


typedef struct ConnectionHandler {
    Printf (*getPrintfForConnection)(int);
    //size_t stringBufferLen;
    char stringBuffer[26];
    ConnectionState connections[NumConnections];
} ConnectionHandler;

extern ConnectionHandler connectionHandler;

void responseStartWithObjects(const OmiRequestParameters * param, int returnCode);
void responseStart(const OmiRequestParameters * p, eomi_bool hasOdf);
void responseFullSuccess(const OmiRequestParameters * param);
void responseFullFailure(const OmiRequestParameters * p, int returnCode,
        const char * description, OmiParser * parser);
void responseRequestId(const OmiRequestParameters * p, uint requestId);
void responseStartOdfNode(const OmiRequestParameters * param, const Path *node);
void responseCloseOdfNode(const OmiRequestParameters * param, const Path *node);
void responseEndWithObjects(const OmiRequestParameters * p);
void responseFromErrorCode(OmiParser* parser, ErrorResponse err);
void responseEndWithFailure(const OmiRequestParameters * p, int returnCode, const char * description);

#endif
