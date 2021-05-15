#!/usr/bin/env python3
# Non-optimized hash calculation for generating hard-coded hashes for the source code
import sys
import numpy

def hashUnsigned(string):
    h = 2166136261
    for x in string:
        h ^= ord(x)
        h *= 16777619
        h &= 0xFFFFFFFF # simulate overflow
    return h
def hashSigned(string):
    return numpy.int32(hashUnsigned(string))

if sys.argv[1] == "hashUnsigned" or sys.argv[1] == "hash":
    print(hashUnsigned(sys.argv[2]), end='')
if sys.argv[1] == "hashSigned":
    print(hashSigned(sys.argv[2]), end='')

class Hashed:
    name = ""
    strPrefix = "s_"
    hashPrefix = "h_"
    @property
    def stringVar(s): return s.strPrefix + s.name
    @property
    def hashVar(s): return s.hashPrefix + s.name
    @property
    def hash(s): return hashSigned(s.string)
class StringData(Hashed):
    def __init__(s, string):
        s.name = string
        s.string = string
class NamedStringData(Hashed):
    def __init__(s, name, string):
        s.name = name
        s.string = string
class xsStringData(StringData):
    strPrefix = "s_xs"
    hashPrefix = "h_xs"
    def __init__(s, name):
        super().__init__(name)
        s.string = "xs:" + s.string

def noprint(*x, **xs): pass
hprint = print if sys.argv[1] == "h" else noprint
cprint = print if sys.argv[1] == "c" else noprint

hprint("#ifndef OMICONSTANTS_H")
hprint("#define OMICONSTANTS_H")
hprint("#ifdef __cplusplus")
hprint('extern "C" {')
hprint("#endif")

cprint('#include "OmiConstants.h"')
print()

allStrings = []
def enum(name, *entries):
    hprint(f"typedef enum {name} {{")
    for e in entries:
        hprint(f"    {e.hashVar} = {e.hash},")
        allStrings.append(e)
    hprint(f"}} {name};")
    hprint()

enum("OmiEnvelopeHash"
    , StringData("omiEnvelope")
    , StringData("version")
    , StringData("xmlns")
    , StringData("ttl")
    , NamedStringData("xmlnsOmi2", "http://www.opengroup.org/xsd/omi/2.0/")
    , NamedStringData("xmlnsOmi1_0", "http://www.opengroup.org/xsd/omi/1.0/")
    , NamedStringData("xmlnsOmi1", "omi.xsd")
    , NamedStringData("xmlnsOdf2", "http://www.opengroup.org/xsd/odf/2.0/")
    , NamedStringData("xmlnsOdf1_0", "http://www.opengroup.org/xsd/odf/1.0/")
    , NamedStringData("xmlnsOdf1", "odf.xsd")
    , NamedStringData("v1", "1.0")
    , NamedStringData("v2", "2.0")
    )

enum("OmiVerbHash"
    , StringData("read")
    , StringData("write")
    , StringData("delete")
    , StringData("cancel")
    , StringData("response")
    , StringData("call")
    )

enum("OmiVerbContentHash"
    , StringData("msgformat")
    , StringData("interval")
    , StringData("callback")
    , StringData("odf")
    , StringData("result")
    , StringData("return")
    , StringData("msg")
    , StringData("requestID")
    , StringData("returnCode")
    , StringData("description")
    )

enum("OdfHash"
    , StringData("Objects")
    , StringData("Object") 
    , StringData("id")
    , StringData("InfoItem")
    , StringData("name")
    , StringData("type")
    , StringData("MetaData")
    , StringData("value")
    , StringData("unixTime")
    , StringData("dateTime")
    )

enum("OdfTypeHash"
    , xsStringData("int")
    , xsStringData("integer")
    , xsStringData("float")
    , xsStringData("double")
    , xsStringData("long")
    , xsStringData("short")
    , xsStringData("byte")
    , xsStringData("string")
    , xsStringData("boolean")
    , xsStringData("unsignedInt")
    , xsStringData("unsignedShort")
    , xsStringData("unsignedLong")
    , xsStringData("unsignedByte")
    )
enum("KeywordHash"
    , StringData("onwrite")
    )

enum("BooleanHash"
    , StringData("true")
    , StringData("True")
    , StringData("TRUE")
    , StringData("on")
    , StringData("ON")
    , StringData("false")
    , StringData("False")
    , StringData("FALSE")
    , StringData("off")
    , StringData("OFF")
    , StringData("1")
    , StringData("0")
    )

for s in allStrings:
    hprint("extern ", end='')
    print(f'const char* {s.stringVar}', end='')
    cprint(f' = "{s.string}"', end='')
    print(';')

print()

hprint("#ifdef __cplusplus")
hprint("}")
hprint("#endif")
hprint("#endif")
