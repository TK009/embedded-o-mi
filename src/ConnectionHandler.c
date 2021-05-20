#include "ConnectionHandler.h"
#include "OmiConstants.h"
#include <stdio.h>

ConnectionHandler connectionHandler = {NULL, "", {0}};

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

void responseStart(const OmiRequestParameters * p, eomi_bool hasOdf){
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
    send(s_ttl); send("=\"%d\">", p->deadline - p->arrival);
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
    responseStart(p, eomi_true);
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
void responseRequestId(const OmiRequestParameters * p, uint requestId) {
    Printf send = connectionHandler.getPrintfForConnection(p->connectionId);
    responseStart(p, eomi_false);
    responseReturnCode(p, 200, NULL);
    send("<"); send(s_requestID);
    send(">%d</", requestId); send(s_requestID); send(">");
    responseEnd(p);
}

void responseFullSuccess(const OmiRequestParameters * p){
    responseStart(p, eomi_false);
    responseReturnCode(p, 200, NULL);
    responseEnd(p);
}
void responseEndWithFailure(const OmiRequestParameters * p, int returnCode, const char * description) {
    responseOdfEnd(p);
    Printf send = connectionHandler.getPrintfForConnection(p->connectionId);
    send("</"); send(s_result); send(">");
    send("<"); send(s_result); send(">");
    responseReturnCode(p, returnCode, description);
    responseEnd(p);
}
void responseFullFailure(const OmiRequestParameters * p, int returnCode,
        const char * description, OmiParser * parser){
    responseStart(p, eomi_false);
    char d[100];
    if (parser) {
        switch (parser->st) {
            case OmiState_PreOmiEnvelope:
                sprintf(d, "%s; Near %s", description, s_omiEnvelope);
                break;
            case OmiState_OmiEnvelope:
                sprintf(d, "%s; Near %s", description, s_omiEnvelope);
                break;
            case OmiState_Verb:
                sprintf(d, "%s; Near request element", description);
                break;
            case OmiState_Response:
                sprintf(d, "%s; Near %s", description, s_response);
                break;
            case OmiState_Result:
                sprintf(d, "%s; Near %s", description, s_result);
                break;
            case OmiState_RequestID:
                sprintf(d, "%s; Near %s", description, s_requestID);
                break;
            case OmiState_Return:
                sprintf(d, "%s; Near %s", description, s_return);
                break;
            case OmiState_Msg:
                sprintf(d, "%s; Near %s", description, s_msg);
                break;
            case OdfState_Objects:
                sprintf(d, "%s; Near %s", description, s_Objects);
                break;
            case OdfState_Object:
                sprintf(d, "%s; Near %s %s", description, s_Object, parser->currentOdfPath->odfId);
                break;
            case OdfState_Id:
                sprintf(d, "%s; Near %s in %s", description, s_id, parser->currentOdfPath->odfId);
                break;
            case OdfState_ObjectObjects:
                sprintf(d, "%s; Near %s, %s children", description, parser->currentOdfPath->odfId, s_Object);
                break;
            case OdfState_ObjectInfoItems:
                sprintf(d, "%s; Near %s, %s children", description, parser->currentOdfPath->odfId, s_InfoItem);
                break;
            case OdfState_InfoItem:
                sprintf(d, "%s; Near %s %s", description, s_InfoItem, parser->currentOdfPath->odfId);
                break;
            case OdfState_Description:
                sprintf(d, "%s; Near %s of %s", description, s_description, parser->currentOdfPath->odfId);
                break;
            case OdfState_MetaData:
                sprintf(d, "%s; Near %s of %s", description, s_MetaData, parser->currentOdfPath->parent->odfId);
                break;
            case OdfState_Value:
                sprintf(d, "%s; Near %s of %s", description, s_value, parser->currentOdfPath->odfId);
                break;
            case OmiState_Ready:
                sprintf(d, "%s; Invalid internal state", description);
                break;
            case OdfState_End:
                sprintf(d, "%s; Near the end, after %s", description, parser->currentOdfPath->odfId);
                break;
            //default:
            //    strcpy(d, description);
        }

    } else {
        strcpy(d, description);
    }
    
    responseReturnCode(p, returnCode, d);
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
        //caseResponseValue(V_String, s_xsstring, "%s", str);
    switch (node->flags & PF_ValueType) {
        case V_String:
            send(s_xsstring); send("\">");
            send(node->value.latest->current.value.str);
            send("</"); send(s_value); send(">");
            break;
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
    switch (PathGetNodeType(node)){
        case OdfDescription:
            send("<"); send(s_description); send(">");
            if (node->value.str) send(node->value.str);
            break;
        case OdfMetaData:
            send("<"); send(s_MetaData); send(">");
            break;
        case OdfInfoItem:
            send("<"); send(s_InfoItem);
            send(" "); send(s_name); send("=\"");
            send(node->odfId); send("\">");
            break;
        case OdfObject:
            if (node->depth == ObjectsDepth) break; // handled elsewhere, in start with objects
            send("<"); send(s_Object); send("><"); send(s_id); send(">");
            send("%.*s", node->odfIdLength, node->odfId); send("</"); send(s_id); send(">");
            break;
    }
}
void responseCloseOdfNode(const OmiRequestParameters * p, const Path *node){
    Printf send = connectionHandler.getPrintfForConnection(p->connectionId);
    switch (PathGetNodeType(node)){
        case OdfDescription:
            send("</"); send(s_description); send(">");
            break;
        case OdfMetaData:
            send("</"); send(s_MetaData); send(">");
            break;
        case OdfInfoItem:
            // Value comes after possible description and metadata
            if (node->flags & PF_ValueMalloc && node->value.latest) {
                send("<"); send(s_value); send(" "); send(s_unixTime);
                send("=\"%d\" ", node->value.latest->current.timestamp);
                send(s_type); send("=\"");
                responseTypeAndValue(send, node);
            }
            send("</"); send(s_InfoItem); send(">");
            break;
        case OdfObject:
            if (node->depth == ObjectsDepth) break; // handled elsewhere, in start with objects
            send("</"); send(s_Object); send(">");
            break;
    }
}

void responseFromErrorCode(OmiParser* parser, ErrorResponse err){
    const OmiRequestParameters * p = &parser->parameters;
    switch (err) {
        case Err_OK:
        case Err_End:
            break;
        case Err_NonMatchingCloseTag:
            responseFullFailure(p, 400, "Non matching close tag", parser);
            break;
        case Err_StackOverflow      :
            responseFullFailure(p, 500, "XML parser stack overflow, too deep structure", parser);
            break;
        case Err_XmlError           :
            responseFullFailure(p, 400, "XML error", parser);
            break;
        case Err_InvalidCharRef     :
            responseFullFailure(p, 400, "Invalid XML character reference", parser);
            break;
        case Err_InvalidElement     :
            responseFullFailure(p, 400, "Invalid O-MI/O-DF element", parser);
            break;
        case Err_InvalidDataFormat  :
            responseFullFailure(p, 400, "Invalid O-MI payload type or not implemented", parser);
            break;
        case Err_InvalidAttribute   :
            responseFullFailure(p, 400, "Invalid O-MI/O-DF element attribute", parser);
            break;
        case Err_TooDeepOdf         :
            responseFullFailure(p, 500, "O-DF parser stack overflow, too deep hierarchy", parser);
            break;
        case Err_InternalError      :
            responseFullFailure(p, 500, "Internal error", parser);
            break;
        case Err_OOM_String         :
            responseFullFailure(p, 500, "Out of text memory", parser);
            break;
        case Err_OOM                :
            responseFullFailure(p, 500, "Out of memory", parser);
            break;
        case Err_NotImplemented     :
            responseFullFailure(p, 501, "Not implemented", parser);
            break;
        case Err_ScriptParse        :
            responseFullFailure(p, 400, "Script parsing error", parser);
            break;
        case Err_ScriptRun          :
            responseFullFailure(p, 500, "Script runtime error", parser);
            break;
        case Err_NotFound :
            break; // response started already elsewhere, handle error there
    }
}
