#include <stdio.h>
#include "OMIParser.h"
#include "requestHandler.h"
#include "StringStorage.h"
#include "ScriptEngine.h"

Printf getOutput(int c) {
    (void) c;
    return printf;
}

int main(int argc, char ** argv) {
    (void) argc, (void) argv;

    RequestHandler_init();
    StringStorage_init();
    int r = ScriptEngine_init();
    if (r != 0) return r;
    connectionHandler.getPrintfForConnection = getOutput;

    OmiParser * parser = getParser(0);

    char input[2] = " ";
    int c;
    while ((c = getchar()) != EOF) {
        input[0] = c;
        ErrorResponse result = runParser(parser, input);

        switch (result) {
            case Err_OK: break;
            default:
                responseFromErrorCode(parser, result);
                // return result;
                OmiParser_destroy(parser);
                OmiParser_init(parser, 0);
            case Err_End:
                printf("\r\n"); // Separator: Easier to pipe the result to other programs
                fflush(stdout);
                OmiParser_init(parser, 0);
                break;
        }
    }
}
