#ifndef OMI_H
#define OMI_H

#include "utils.h"

typedef enum OmiVersion {
    OmiV_unknown,
    OmiV1,      // without xmlns
    OmiV1_ns,   // xmlns= omi.xsd
    OmiV1_0_ns, // xmlns= http://www.opengroup.org/xsd/omi/1.0/
    OmiV2,      // without xmlns
    OmiV2_ns,   // xmlns= http://www.opengroup.org/xsd/omi/2.0/
} OmiVersion;

typedef enum OmiPayloadFormat {
    OmiOdf,
    OmiOdfJson,
    OmiJson
} OmiPayloadFormat;

typedef enum OmiRequestType {
    // Non O-DF responses (no msg)
    OmiInvalid,
    OmiWrite,
    OmiSubscribe,
    OmiCancel,
    OmiDelete,
    OmiResponse,
    // O-DF responses (with msg)
    OmiRead,
    OmiPoll,
    OmiCall
} OmiRequestType;

typedef struct OmiRequestParameters {
    OmiRequestType requestType;
    eomi_time arrival;
    eomi_time deadline;
    OmiVersion version;
    OmiPayloadFormat format;
    float interval;
    char* callbackAddr;
    int connectionId; // Opened connection for either immediate response or callback
} OmiRequestParameters;

//#define OmiRequestParameters_INITIALIZER {.requestType=OmiInvalid, .arrival=}

#endif
