
#include "ScriptEngine.h"
#include <stdlib.h>
#include "OMIParser.h" // Error codes
#include "requestHandler.h" // Error codes
#include "OmiConstants.h"

#include "jerryscript-port.h"

jerry_context_t *scriptEngineContext;
Path * currentScriptCallbackPath = NULL;
OmiParser * currentParser = NULL;

#if SCRIPT_QUEUE_LEN > 126
#warning SCRIPT_QUEUE_LEN too large for implementation datatypes
#endif

// TODO: Better queue implementation
typedef struct {Path *path; HString script;} ScriptQueueElem;
struct ScriptQueue {
    uchar rear;
    ScriptQueueElem elems[SCRIPT_QUEUE_LEN];
} scriptQueue = {0, {0}};




/*
 * Write value to O-DF InfoItem.
 * Params: (value, path)
 * value: Number or String
 * path: String, optional. Script parent InfoItem is the default
 */
static jerry_value_t
odfWriteItem_handler(const jerry_call_info_t *call_info_p, /**< call information */
                     const jerry_value_t args_p[], /**< function arguments */
                     const jerry_length_t args_cnt) /**< number of function arguments */
{
    const jerry_char_t * TypeError = (const jerry_char_t *)"TypeError: args";
    (void) call_info_p;
    Path paths[OdfDepthLimit];
    OdfTree odf;
    OdfTree_init(&odf, paths, OdfDepthLimit);
    Path * targetPath;
    jerry_size_t length;
    if (args_cnt == 0) {
        return jerry_create_error(JERRY_ERROR_COMMON, TypeError); // TODO: better error?
    }
    if (args_cnt >= 2) {
        jerry_value_t pathArg = args_p[1];
        // extract the path string;
        if (!jerry_value_is_string(pathArg) || jerry_value_is_error(pathArg))
            return jerry_create_error(JERRY_ERROR_COMMON, TypeError);

        pathArg = jerry_acquire_value(pathArg);
        if (jerry_get_string_size(pathArg) > ParserMaxStringLength-1){
            jerry_release_value(pathArg);
            return jerry_create_error(JERRY_ERROR_COMMON, (const jerry_char_t *) "BufferOverflow");
        }

        length = jerry_string_to_utf8_char_buffer(
                pathArg, (jerry_char_t *) currentParser->tempString, ParserMaxStringLength-1);
        jerry_release_value(pathArg);
        currentParser->tempString[length] = '\0';
        currentParser->tempStringLength = length;

        // write path
        targetPath = addPath(&odf, currentParser->tempString, OdfInfoItem);
        for (int i = 0; i < odf.size; ++i) // odf should not contain anything else than parents
            handleWrite(currentParser, paths+i, OdfParserEvent(PE_Path, NULL));

    } else {
        targetPath = copyPath(currentScriptCallbackPath, &odf);
    } 

    // extract value
    jerry_value_t valueArg = jerry_acquire_value(args_p[0]);
    char * maybeStringValue = NULL;
    switch (jerry_value_get_type(valueArg)) {
        case JERRY_TYPE_BOOLEAN:
            targetPath->value.b = jerry_get_boolean_value(valueArg);
            targetPath->flags = V_Boolean;
            break;
        case JERRY_TYPE_NUMBER:
            targetPath->value.d = jerry_get_number_value(valueArg);
            targetPath->flags = V_Double;
            break;
        case JERRY_TYPE_STRING:
            length = jerry_get_utf8_string_size(valueArg);
            maybeStringValue = malloc(length+1);
            jerry_string_to_utf8_char_buffer(
                valueArg, (jerry_char_t *) maybeStringValue, length);
            maybeStringValue[length] = '\0';
            targetPath->flags = V_String | PF_ValueMalloc;
            break;
        case JERRY_TYPE_OBJECT:
            // TODO: good for specifying types like integers, example: {type: "xs:int", value: 5}, might allow custom timestamp also.
            // jerry_value_t prop_name = jerry_create_string ((const jerry_char_t *) "type");

            //jerry_value_t has_prop_js = jerry_has_property (valueArg, prop_name);
            //if (jerry_get_boolean_value (has_prop_js)) {
            //    jerry_value_t prop_value = jerry_get_property (valueArg, prop_name);
            //    ...
            //    jerry_release_value (prop_value);
            //}


            //jerry_release_value (has_prop_js);
            //jerry_release_value (prop_name);
            //break;
        default:
            jerry_release_value(valueArg);
            return jerry_create_error(JERRY_ERROR_COMMON, TypeError);
    }
    jerry_release_value(valueArg);


    // write the infoitem if missing and also ensure the correct type
    //handleWrite(currentParser, targetPath, OdfParserEvent(PE_Path, NULL));
    // write value
    handleWrite(currentParser, targetPath, OdfParserEvent(PE_ValueData, maybeStringValue));

    return jerry_create_undefined();
}

int ScriptEngine_testParse(const HString* script) {
    jerry_value_t parsed_code =
        jerry_parse((jerry_char_t*) script->value,
                script->length - 1, NULL);
    bool notOk = jerry_value_is_error(parsed_code);
    jerry_release_value (parsed_code);

    return notOk ? Err_ScriptParse: Err_OK;
}

jerry_value_t jerry_value_from_infoitem(Path * path){

    // Not possible atm unless badly written test
    //if (! path->flags & PF_ValueMalloc)
    //    return jerry_create_null();

    //bool isNumber = false;
    //double number;
    switch ((ValueType) path->flags & PF_ValueType) {
        case V_Int:
        case V_Byte:
        case V_Short:
        case V_Long: // FIXME: use bigint?
            return jerry_create_number(path->value.latest->current.value.l);
        case V_UInt:
        case V_UShort:
        case V_UByte:
        case V_ULong: // FIXME: use bigint?
            return jerry_create_number((unsigned long) path->value.latest->current.value.l);
        case V_Float:
            return jerry_create_number(path->value.latest->current.value.f);
        case V_Double:
            return jerry_create_number(path->value.latest->current.value.d);
        case V_String:
            return jerry_create_string((const jerry_char_t *)path->value.latest->current.value.str);
        case V_Boolean:
            return jerry_create_boolean(path->value.latest->current.value.b);
        // not possible atm
        default:
            return jerry_create_null();
    }
}


int ScriptEngine_run(OmiParser * p, Path * path, const HString * firstScript) {

    if (currentScriptCallbackPath) {
        // Stop possible infinite loop if the script tries to run itself
        if (currentScriptCallbackPath == path) return Err_OK;
    }
    // add to queue
    if (scriptQueue.rear < SCRIPT_QUEUE_LEN) {
        scriptQueue.elems[scriptQueue.rear++] = (ScriptQueueElem){path, *firstScript}; //p->currentOdfPath;
    }
    // already running other script, run this from the queue later
    if (currentScriptCallbackPath) return Err_OK;


    bool runError = true;
    currentParser = p;
    // run queue
    for (uchar front = 0; front < scriptQueue.rear; ++front) {
        currentScriptCallbackPath = scriptQueue.elems[front].path; 
        HString script = scriptQueue.elems[front].script;

        jerry_value_t parsed_code =
            jerry_parse((jerry_char_t*) script.value,
                    script.length - 1, NULL);

        if (!jerry_value_is_error(parsed_code)) {
            // Prepare: arguments = [{'value': <value>, 'unixTime': <timestamp>}]
            
            jerry_value_t parent, prop, propName, setResult, global;

            // arguments[0].value
            parent    = jerry_create_object();
            prop      = jerry_value_from_infoitem(currentScriptCallbackPath);
            propName  = jerry_create_string((const jerry_char_t *) s_value);
            setResult = jerry_set_property(parent, propName, prop);
            //if (jerry_value_is_error(setResult)) ++errors; // TODO: error handling (log?)
            jerry_release_value(prop);
            jerry_release_value(propName);
            jerry_release_value(setResult);
            //jerry_release_value(parent); reused

            // arguments[0].unixTime
            prop      = jerry_create_number(currentScriptCallbackPath->value.latest->current.timestamp);
            propName  = jerry_create_string((const jerry_char_t *) s_unixTime);
            setResult = jerry_set_property(parent, propName, prop);
            //if (jerry_value_is_error(setResult)) ++errors; // TODO: error handling (log?)
            jerry_release_value(prop);
            jerry_release_value(propName);
            jerry_release_value(setResult);


            global    = jerry_get_global_object();

            // arguments array
            prop      = parent;
            parent    = jerry_create_array(1);
            setResult = jerry_set_property_by_index(parent, 0, prop);
            //if (jerry_value_is_error(setResult)) ++errors; // TODO: error handling (log?)
            //jerry_release_value(prop);
            //jerry_release_value(parent); reused
            jerry_release_value(setResult);

            // event global
            propName  = jerry_create_string((const jerry_char_t *) "event");
            setResult = jerry_set_property(global, propName, prop);
            //if (jerry_value_is_error(setResult)) ++errors; // TODO: error handling (log?)
            jerry_release_value(propName);
            jerry_release_value(setResult);
            jerry_release_value(prop);


            // arguments global
            prop      = parent;
            propName  = jerry_create_string((const jerry_char_t *) "arguments");
            setResult = jerry_set_property(global, propName, prop);
            //if (jerry_value_is_error(setResult)) ++errors; // TODO: error handling (log?)
            jerry_release_value(prop);
            jerry_release_value(propName);
            jerry_release_value(setResult);
            jerry_release_value(global);



            // run
            jerry_value_t ret_value = jerry_run(parsed_code);

            runError = jerry_value_is_error(ret_value);

            jerry_release_value (ret_value);
        }

        jerry_release_value (parsed_code);

    }

    currentScriptCallbackPath = NULL;
    scriptQueue.rear = 0;

    return (runError ? Err_ScriptRun : Err_OK);
}


jerry_context_t * jerry_port_get_current_context(void) {
  return scriptEngineContext;
}
static void * contextAlloc(size_t size, void *cb_data) {
  (void) cb_data;
  return malloc (size);
}

int ScriptEngine_init() {
    scriptEngineContext = 
        jerry_create_context(JS_MEMORY_KB * 1024, contextAlloc, NULL);
    if (!scriptEngineContext) return -1;
    jerry_init(JERRY_INIT_EMPTY);

    int errors = 0;
    jerry_value_t parent, prop, propName, setResult;
    

    // odf.writeItem function
    parent    = jerry_create_object();
    propName  = jerry_create_string((const jerry_char_t *) "writeItem");
    prop      = jerry_create_external_function(odfWriteItem_handler);
    setResult = jerry_set_property(parent, propName, prop);
    if (jerry_value_is_error (setResult)) ++errors; // TODO: error handling (log?)
    jerry_release_value(prop);
    jerry_release_value(propName);
    jerry_release_value(setResult);
    //jerry_release_value(parent); reused
    

    // global.odf object
    prop      = parent;
    parent    = jerry_get_global_object();
    propName  = jerry_create_string((const jerry_char_t *) "odf");
    setResult = jerry_set_property(parent, propName, prop);
    if (jerry_value_is_error(setResult)) ++errors; // TODO: error handling (log?)
    jerry_release_value(prop);
    jerry_release_value(propName);
    jerry_release_value(setResult);
    jerry_release_value(parent);

    
    currentScriptCallbackPath = NULL;
    currentParser = NULL;

    return errors;
}
void ScriptEngine_destroy() {
    jerry_cleanup();
    free(scriptEngineContext);
}

