#include "ConnectionHandler.h"
#include "OmiConstants.h"

ConnectionHandler connectionHandler = {NULL, ""};

const char* omiVersionStr(OmiVersion v) {
    switch (v) {
        case OmiV2_ns: return s_xmlnsOmi2;
        case OmiV1_0_ns: return s_xmlnsOmi1_0;
        default:
        case OmiV1_ns: return s_xmlnsOmi1;
    }
}
const char* odfVersionStr(OmiVersion v) {
    switch (v) {
        case OmiV2_ns: return s_xmlnsOdf2;
        case OmiV1_0_ns: return s_xmlnsOdf1_0;
        default:
        case OmiV1_ns: return s_xmlnsOdf1;
    }
}
const char* omiVersionNumStr(OmiVersion v) {
    if (v >= OmiV2) return s_v2;
    else return s_v1;
}

// TODO: inline function cleanup: parameter for send function

void responseStart(const OmiRequestParameters * p, bool hasOdf){
    //snprintf("<%s %s=\"%s\" %s=\"%s\" %s=\"%f\">"
    //        , connectionHandler.stringBufferLen, connectionHandler.stringBuffer
    //        , s_omiEnvelope
    //        , s_xmlns
    //        , versionStr(p->version)
    //        , s_version
    //        , versionNumStr(p->version)
    //        , s_ttl
    //        , 
    //        );
    Printf send = connectionHandler.getPrintfForConnection(p->connectionId);
    send("<"); send(s_omiEnvelope);
    send(" ");
    send(s_xmlns); send("=\""); // TODO: if for skipping
    send(omiVersionStr(p->version));
    send("\" ");
    send(s_version); send("=\"");
    send(omiVersionNumStr(p->version));
    send("\" ");
    send(s_ttl); send("=\"%.1d\">", p->deadline - p->arrival);
    send("<"); send(s_response); send(">");
    send("<"); send(s_result);
    if (hasOdf) {
        send(" "); send(s_msgformat); send("=\"");
        send(s_odf); send("\"");
    }
    send(">");
}
//inline requestVerbStr(const OmiRequestParameters * p) {
//    switch (p->requestType) {
//        case 
//    OmiInvalid,
//    OmiRead,
//    OmiWrite,
//    OmiSubscribe,
//    OmiCancel,
//    OmiDelete,
//    OmiResponse,
//    OmiPoll,
//    OmiCall
//    }
//}

void responseOdfEnd(const OmiRequestParameters * p){
    Printf send = connectionHandler.getPrintfForConnection(p->connectionId);
    send("</"); send(s_Objects); send(">");
    send("</"); send(s_msg); send(">");
}
void responseEnd(const OmiRequestParameters * p){
    Printf send = connectionHandler.getPrintfForConnection(p->connectionId);
    send("</"); send(s_result); send(">");
    send("</"); send(s_response); send(">");
    send("</"); send(s_omiEnvelope); send(">");
}
void responseEndWithObjects(const OmiRequestParameters * p){
    responseOdfEnd(p);
    responseEnd(p);
}

void responseReturnCode(const OmiRequestParameters * p, int returnCode, const char * description) {
    Printf send = connectionHandler.getPrintfForConnection(p->connectionId);
    send("<"); send(s_return);
    send(" ");
    send(s_returnCode);
    send("=\"%d\"", returnCode);
    if (description) {
        send(" "); send(s_description); send("=\"");
        send(description); send("\"");
    }
    send("/>");
}

void responseStartWithObjects(const OmiRequestParameters * p, int returnCode){
    Printf send = connectionHandler.getPrintfForConnection(p->connectionId);
    responseStart(p, true);
    responseReturnCode(p, returnCode, NULL);
    send("<"); send(s_msg); send(">");
    send("<"); send(s_Objects); send(" ");
    send(s_xmlns); send("=\""); // TODO: if for skipping
    send(odfVersionStr(p->version));
    send("\" ");
    send(s_version); send("=\"");
    send(omiVersionNumStr(p->version));
    send("\">");
}

void responseFullSuccess(const OmiRequestParameters * p){
    responseStart(p, false);
    responseReturnCode(p, 200, NULL);
    responseEnd(p);
}
void responseFullFailure(const OmiRequestParameters * p, int returnCode, const char * description){
    responseStart(p, false);
    responseReturnCode(p, returnCode, description);
    responseEnd(p);
}
#define caseResponseValue(typeEnum, typeString, stringSpecifier, accessor) \
    case typeEnum: \
        send(typeString); \
        send("\">" stringSpecifier "</", node->value.latest->current.value.accessor);\
        send(s_value);\
        send(">"); \
        break

void responseTypeAndValue(Printf send, const Path * node){
    switch (node->flags & PF_ValueType) {
        caseResponseValue(V_String, s_xsstring, "%s", str);
        caseResponseValue(V_Int, s_xsint, "%d", i);
        caseResponseValue(V_Float, s_xsfloat, "%f", f);
        caseResponseValue(V_Long, s_xslong, "%ld", l);
        caseResponseValue(V_Double, s_xsdouble, "%f", d);
        caseResponseValue(V_Short, s_xsshort, "%d", i);
        caseResponseValue(V_Byte, s_xsbyte, "%d", b);
        caseResponseValue(V_Boolean, s_xsboolean, "%d", b);
        caseResponseValue(V_ULong, s_xsunsignedLong, "%lu", l);
        caseResponseValue(V_UInt, s_xsunsignedInt, "%u", i);
        caseResponseValue(V_UShort, s_xsunsignedShort, "%u", i);
        caseResponseValue(V_UByte, s_xsunsignedByte, "%u", b);
        caseResponseValue(V_ODF, s_odf, "%s", str);
    }
}
void responseStartOdfNode(const OmiRequestParameters * p, const Path *node){
    Printf send = connectionHandler.getPrintfForConnection(p->connectionId);
    switch (node->flags & (PF_IsMetaData | PF_IsInfoItem | PF_IsDescription)){
        case PF_IsDescription:
            send("<"); send(s_description); send(">");
            if (node->value.str) send(node->value.str);
            break;
        case PF_IsMetaData:
            send("<"); send(s_MetaData); send(">");
            break;
        case PF_IsInfoItem:
            send("<"); send(s_InfoItem);
            send(" "); send(s_name); send("=\"");
            send(node->odfId); send("\">");
            if (node->flags & PF_ValueMalloc && node->value.latest) {
                send("<"); send(s_value); send(" "); send(s_unixTime);
                send("=\"%d\" ", node->value.latest->current.timestamp);
                send(s_type); send("=\"");
                responseTypeAndValue(send, node);
            }
            break;
        default: // Object
            if (node->depth == 1) break; // handled elsewhere, in start with objects
            send("<"); send(s_Object); send("><"); send(s_id); send(">");
            send(node->odfId); send("</"); send(s_id); send(">");
            break;
    }
}
void responseCloseOdfNode(const OmiRequestParameters * p, const Path *node){
    Printf send = connectionHandler.getPrintfForConnection(p->connectionId);
    switch (node->flags & (PF_IsMetaData | PF_IsInfoItem | PF_IsDescription)){
        case PF_IsDescription:
            send("</"); send(s_description); send(">");
            break;
        case PF_IsMetaData:
            send("</"); send(s_MetaData); send(">");
            break;
        case PF_IsInfoItem:
            send("</"); send(s_InfoItem); send(">");
            break;
        default:
            if (node->depth == 1) break; // handled elsewhere, in start with objects
            send("</"); send(s_Object); send(">");
            break;
    }
}
