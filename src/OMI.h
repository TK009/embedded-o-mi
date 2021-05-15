#ifndef OMI_H
#define OMI_H

#include "utils.h"
#include "OdfTree.h"

#ifdef __cplusplus
extern "C" {
#endif

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
    OmiPayloadFormat format;
    OmiRequestType requestType;
    OmiVersion version;
    eomi_time arrival;
    eomi_time deadline;
    float interval;
    char* callbackAddr;
    int connectionId; // Opened connection for either immediate response or callback
    Path* lastPath; // Last path
} OmiRequestParameters;

//#define OmiRequestParameters_INITIALIZER {.requestType=OmiInvalid, .arrival=}


#ifdef __cplusplus
}
#endif
#endif
