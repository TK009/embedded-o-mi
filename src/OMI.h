#ifndef OMI_H
#define OMI_H

#include "utils.h"

typedef enum OmiVersion {
    OmiV1,   // xmlns= omi.xsd
    OmiV1_0, // xmlns= http://www.opengroup.org/xsd/omi/1.0/
    OmiV2    // xmlns= http://www.opengroup.org/xsd/omi/2.0/
} OmiVersion;

typedef enum OmiPayloadFormat {
    OmiOdf,
    OmiOdfJson,
    OmiJson
} OmiPayloadFormat;

typedef enum OmiRequestType {
    OmiInvalid,
    OmiRead,
    OmiWrite,
    OmiSubscribe,
    OmiCancel,
    OmiDelete
} OmiRequestType;

struct OmiRequestParameters {
    time arrival;
    time deadline;
    OmiVersion version;
    OmiPayloadFormat format;
};
typedef struct OmiRequestParameters OmiRequestParameters;

#endif
