

#include "OMIParser.h"

// Parser memory pool
OmiParser parsers[ParserPoolSize] = {{0}};


bool initParser() {
    for (int i = 0; i < ParserPoolSize; ++i) {
        OmiParser * p = &parsers[i];
        if (p->st == OmiState_Ready) {
            p->xmlst = 0;
            p->bytesRead = 0;
            memset(&p->parameters, 0, sizeof(p->parameters));
            p->parameters.arrival = getTimestamp();
            p->st = OmiState_PreOmiEnvelope;
            p->xmlst = XmlState_PreDocument;
            return true;
        }
    }
    return false;
}


// Finite state machine here

int runParser(OmiParser * p) {
    char nameString[32];
    for (uint passLength = ParserSinglePassLength; passLength > 0; --passLength) {
        passLength += runXmlParser(p, ParserSinglePassLength - passLength, nameString, sizeof(nameString));
        switch (p->st) {
            case OmiState_Ready:
                break;
            case OmiState_PreOmiEnvelope:
                break;
            case OmiState_OmiEnvelopeAttr:
                break;
            case OmiState_PreVerb:
                break;
            case OmiState_VerbAttr:
                break;
            case OmiState_PreMsg:
                break;
            case OmiState_Msg:
                break;
        }
    }
}

#define NewXmlState(newst) \
    *lastXmlst = *xmlst; \
    *xmlst = newst;
#define SaveChar(x) \
    if (p->stPosition > stringReturnLength) {p->stPosition--;} \
    stringReturn[p->stPosition++] = x;

int runXmlParser(OmiParser * p, uint maxBytes, char *stringReturn, uint stringReturnLength) {
    // TODO: return bytes processed
    // XML parser
    // * where to store strings?
    //   - buffer from higher parser
    //   - statemachine from higher parser
    //
    XmlState originalXmlst = p->xmlst;
    uint passLength = maxBytes;
    for (; passLength > 0; --passLength) {

        char c = getChar(p);

        XmlState *xmlst = &p->xmlst;
        XmlState *lastXmlst = &p->lastXmlst;

parserSwitchStart:
        switch (*xmlst) {
            case XmlState_PreDocument:
                if (c == '<') { NewXmlState(XmlState_PreDocumentTag); }
                break;
            case XmlState_PreDocumentTag:
                if (c == '?') {
                    NewXmlState(XmlState_Prolog);
                } else {
                    NewXmlState(XmlState_Tag);
                    goto parserSwitchStart;
                }
                break;
            case XmlState_Prolog:
                if (c == '>') { NewXmlState(XmlState_PreDocument); }
                break;
            case XmlState_Tag: 
                if (isAlpha(c)) {
                    NewXmlState(XmlState_TagName);
                    goto parserSwitchStart;
                } else if (c == '!') {
                    NewXmlState(XmlState_SpecialTag);
                    p->stPosition = 0;
                } else if (c == '/') {
                    NewXmlState(XmlState_EndTag);
                } else {
                    // TODO: err
                }
                break;
            case XmlState_TagName:
                if (c == '>')  {
                    NewXmlState(XmlState_Body);
                } else if (isWhiteSpace(c)) {
                    NewXmlState(XmlState_TagAttributes);
                } else if (c == '/') {
                    NewXmlState(XmlState_EndTag);
                } else {
                    SaveChar(c);
                }
                SaveChar('\0');
                //break;
            case XmlState_TagAttributes:
                if (isAlpha(c)) {
                    NewXmlState(XmlState_AttributeName);
                    goto parserSwitchStart;
                } else if (isWhiteSpace(c)) { break;
                } else if (c == '/') {NewXmlState(XmlState_EndTag);
                } else if (c == '>') {NewXmlState(XmlState_Body);}
                break;
            case XmlState_AttributeName:
                // TODO: attribute name
                if (c == '=') {NewXmlState(XmlState_PreAttributeValue); }
                break;
            case XmlState_PreAttributeValue:
                if (c == '"') { NewXmlState(XmlState_AttributeValueQuot); }
                else if (c == '\'') { NewXmlState(XmlState_AttributeValueApos); }
                else if (isWhiteSpace(c)) { Noop; }
                else {
                    // TODO: err
                }
                break;
            case XmlState_AttributeValueQuot:
                if (c == '"') { NewXmlState(XmlState_TagAttributes); }
                if (c == '&') { NewXmlState(XmlState_Entity); }
                // TODO: attribute value
                break;
            case XmlState_AttributeValueApos:
                if (c == '\'') { NewXmlState(XmlState_TagAttributes); }
                // TODO: attribute value
                break;
            case XmlState_Body: break;
            case XmlState_EndTag:
                // TODO: tag name
                if (c == xmlTagEnd) { NewXmlState(XmlState_Body); }
                break;
            case XmlState_SpecialTag:
                if (c == '-' && p->stPosition == 1) {
                    NewXmlState(XmlState_Comment);
                }
                // TODO: CDATA?
                else {

                }
                p->stPosition++;
                break;
            case XmlState_Comment:
                if (c == '-') {
                    p->stPosition++;
                } else if (c == '>' && p->stPosition == 2) {
                    NewXmlState(XmlState_Body);// XXX: NewXmlState(p->lastXmlMajorState);
                }
                break;
            case XmlState_Entity:
                //if (c == '#') { NewXmlState(XmlState_CharRef); }
                if (c == 'a') { NewXmlState(XmlState_EntityA); }
                break;

            case XmlState_CharRef: // &#32;

                break;
            case XmlState_EntityA: // &a
                break;
            case XmlState_EntityApos: // &apos;
                break;
            case XmlState_EntityQuot: // &quot;
                break;
            case XmlState_EntityLt: // &lt;
                break;
            case XmlState_EntityGt: // &gt;
                break;
            case XmlState_EntityAmp: // &amp;
                break;
        }

        p->bytesRead++;

        if (originalXmlst != *xmlst) {
            return passLength;
        }
    }
    return passLength;
}
