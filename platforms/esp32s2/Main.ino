
#include <Arduino.h>
#include "OMIParser.h"
#include "requestHandler.h"
#include "StringStorage.h"
#include "ScriptEngine.h"

void setup() {
    RequestHandler_init();
    StringStorage_init();
    int r = ScriptEngine_init();
}

void loop() {

}
