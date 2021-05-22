#ifndef JS_INTEGRATION
#define JS_INTEGRATION

#include "utils.h"
#include "OMIParser.h" // Error codes, OmiParser
#include "jerryscript.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef JS_MEMORY_KB
#define JS_MEMORY_KB 128
#endif
#ifndef SCRIPT_QUEUE_LEN
#define SCRIPT_QUEUE_LEN 10
#endif

extern Allocator ScriptEngineAllocator;
int ScriptEngine_init();
void ScriptEngine_destroy();
int ScriptEngine_testParse(OmiParser * p, const HString* script);
int ScriptEngine_run(OmiParser * p, Path * path, const HString * script);


#ifdef __cplusplus
}
#endif
#endif
