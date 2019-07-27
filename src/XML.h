#ifndef XML_H
#define XML_H

// Simplified xml state machine
typedef enum XmlState {
    XmlState_PreDocument,
    XmlState_PreDocumentTag, // Start of prolog or root element
    XmlState_Prolog,
    XmlState_Tag,
    XmlState_TagName,
    XmlState_TagAttributes,
    XmlState_AttributeName,
    XmlState_AttributeValueQuot,
    XmlState_AttributeValueApos,
    XmlState_PreAttributeValue,
    XmlState_Body,
    XmlState_EndTag,
    XmlState_SpecialTag, // <!
    XmlState_Comment,
    XmlState_Entity, // &
    XmlState_CharRef, // &#32;
    XmlState_EntityA, // &a
    XmlState_EntityApos, // &apos;
    XmlState_EntityQuot, // &quot;
    XmlState_EntityLt, // &lt;
    XmlState_EntityGt, // &gt;
    XmlState_EntityAmp, // &amp;
} XmlState;

// Tokens
const char xmlTagStart = '<';
const char xmlTagEnd = '>';
const char xmlQuote = '"';
const char xmlSlash = '/';

#define isAlpha(c) (((c) > 'A' && (c) < 'Z') || ((c) > 'a' && (c) < 'z'))
#define isWhiteSpace(c) ((c) == ' ' || (c) == '\n' || (c) == '\r' || (c) == '\t')

#endif
