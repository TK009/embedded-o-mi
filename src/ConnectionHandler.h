#ifndef CONNECTIONHANDLER_H
#define CONNECTIONHANDLER_H

#include "OMI.h"
#include "OdfTree.h"

typedef void (*Printf)(const char*, ...);
typedef struct ConnectionHandler {
    Printf (*getPrintfForConnection)(int);
    //size_t stringBufferLen;
    char stringBuffer[26];
} ConnectionHandler;

extern ConnectionHandler connectionHandler;

void responseStartWithObjects(const OmiRequestParameters * param, int returnCode);
void responseFullSuccess(const OmiRequestParameters * param);
void responseFullFailure(const OmiRequestParameters * param, int returnCode, const char * description);
void responseStartOdfNode(const OmiRequestParameters * param, const Path *node);
void responseCloseOdfNode(const OmiRequestParameters * param, const Path *node);
void responseEndWithObjects(const OmiRequestParameters * p);

#endif
