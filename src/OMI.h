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
} OmiRequestParameters;

//#define OmiRequestParameters_INITIALIZER {.requestType=OmiInvalid, .arrival=}

#define StringData(str, hash) \
    static const char* s_ ## str = #str; \
    static const strhash h_ ## str = hash;
#define NamedStringData(name, str, hash) \
    static const char* s_ ## name = str; \
    static const strhash h_ ## name = hash;
#define xsStringData(str, hash) \
    static const char* s_xs ## str = "xs:"#str; \
    static const strhash h_xs ## str = hash;

StringData(omiEnvelope, 3735718108)
StringData(version, 1181855383)
StringData(xmlns, 2376145825)
StringData(ttl, 3173728859)
StringData(read, 3470762949)
StringData(write, 3190202204)
StringData(delete, 1740784714)
StringData(cancel, 107912219)
StringData(response, 1499316702)
StringData(call, 3018949801)
NamedStringData(xmlnsOmi2, "http://www.opengroup.org/xsd/omi/2.0/", 1578010721)
NamedStringData(xmlnsOmi1_0, "http://www.opengroup.org/xsd/omi/1.0/", 3518628538)
NamedStringData(xmlnsOmi1, "omi.xsd", 2350025953)
NamedStringData(xmlnsOdf2, "http://www.opengroup.org/xsd/odf/2.0/", 177890333)
NamedStringData(v1, "1.0", 3946978290)
NamedStringData(v2, "2.0", 673058499)

StringData(msgformat, 442031329)
StringData(interval, 3470624120)
StringData(callback, 2280666118)
StringData(odf, 2873137720)
StringData(result, 171406884)
StringData(return, 2246981567)
StringData(msg, 3766509314)
StringData(requestID, 1450582271)
StringData(returnCode, 1808323374)
StringData(description, 879704937)

StringData(Objects, 136869387)
StringData(Object, 3851314394) 
StringData(id, 926444256)
StringData(InfoItem, 2589257766)
StringData(name, 2369371622)
StringData(type, 1361572173)
StringData(MetaData, 891574848)
StringData(value, 1113510858)
StringData(unixTime, 3907425434)
StringData(dateTime, 1293334960)

xsStringData(int, 1278669515)
xsStringData(integer, 2692559660)
xsStringData(float, 1349277360)
xsStringData(double, 2054327359)
xsStringData(long, 2086416348)
xsStringData(short, 3489489204)
xsStringData(byte, 1641634136)
xsStringData(string, 1051773867)
xsStringData(boolean, 2009594314)
xsStringData(unsignedInt, 1584946050)
xsStringData(unsignedShort, 3143947753)
xsStringData(unsignedByte, 624143547)

#endif
