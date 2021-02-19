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
    OmiInvalid,
    OmiRead,
    OmiWrite,
    OmiSubscribe,
    OmiCancel,
    OmiDelete,
    OmiResponse,
    OmiCall
} OmiRequestType;

struct OmiRequestParameters {
    OmiRequestType requestType;
    eomi_time arrival;
    eomi_time deadline;
    OmiVersion version;
    OmiPayloadFormat format;
};
typedef struct OmiRequestParameters OmiRequestParameters;

#define StringData(str, hash) \
    static const char* s_ ## str = #str; \
    static const strhash h_ ## str = hash;
#define NamedStringData(name, str, hash) \
    static const char* s_ ## name = str; \
    static const strhash h_ ## name = hash;

StringData(omiEnvelope, 3735718108)
StringData(version, 1181855383)
StringData(xmlns, 2376145825)
StringData(ttl, 3173728859)
StringData(read, 3470762949)
StringData(write, 3190202204)
StringData(cancel, 107912219)
StringData(response, 1499316702)
StringData(call, 3018949801)
NamedStringData(xmlnsOmi2, "http://www.opengroup.org/xsd/omi/2.0/", 1578010721)
NamedStringData(xmlnsOmi1_0, "http://www.opengroup.org/xsd/omi/1.0/", 3518628538)
NamedStringData(xmlnsOmi1, "omi.xsd", 2350025953)
NamedStringData(xmlnsOdf2, "http://www.opengroup.org/xsd/odf/2.0/", 177890333)
NamedStringData(v1, "1.0", 3946978290)
NamedStringData(v2, "2.0", 673058499)

#endif
