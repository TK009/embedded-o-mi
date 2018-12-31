#ifndef XML_H
#define XML_H

// Simplified xml state machine
typedef enum XmlState {
    XmlState_PreDocument,
    //XmlState_Prolog,
    XmlState_Tag,
    XmlState_TagAttributes,
    XmlState_AttributeName,
    XmlState_AttributeValue,
    XmlState_Body,
    XmlState_EndTag,
    XmlState_Comment,
} XmlState;

#endif
