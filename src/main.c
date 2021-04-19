#include <stdio.h>
#include "OMIParser.h"
#include "requestHandler.h"
#include "StringStorage.h"

Printf getOutput(int c) {
    (void) c;
    return printf;
}

int main(int argc, char ** argv) {
    (void) argc, (void) argv;

    OdfTree_init(&tree);
    StringStorage_init();
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
            case Err_End:
                printf("\r\n"); // Separator: Easier to pipe the result to other programs
                fflush(stdout);
                break;
        }
    }
}
