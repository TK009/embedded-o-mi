// vim: set syntax=cpp:



#include <esp32-hal-psram.h>
#include <Arduino.h>
#ifdef ESP32
#include <WiFi.h>
#include <AsyncTCP.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#endif
#include <ESPAsyncWebServer.h>

#include "OMIParser.h"
#include "requestHandler.h"
#include "StringStorage.h"
#include "ScriptEngine.h"
#include "utils.h"


// TODO: Find AsyncWebServer compatible WiFi manager
#define STR(s) #s
const char* ssid = STR(WIFI_SSID);
const char* password = STR(WIFI_PASS);
#define XSTR(x) STR(x)
//#pragma message "Pass is " XSTR(WIFI_PASS)


//External RAM use has the following restrictions:
//
//When flash cache is disabled (for example, if the flash is being written to),
//the external RAM also becomes inaccessible; any reads from or writes to it
//will lead to an illegal cache access exception. This is also the reason why
//ESP-IDF does not by default allocate any task stacks in external RAM (see
//below).
//
//External RAM cannot be used as a place to store DMA transaction descriptors
//or as a buffer for a DMA transfer to read from or write into. Any buffers
//that will be used in combination with DMA must be allocated using
//heap_caps_malloc(size, MALLOC_CAP_DMA) and can be freed using a standard
//free() call.
//
//External RAM uses the same cache region as the external flash. This means
//that frequently accessed variables in external RAM can be read and modified
//almost as quickly as in internal ram. However, when accessing large chunks of
//data (>32 KB), the cache can be insufficient, and speeds will fall back to
//the access speed of the external RAM. Moreover, accessing large chunks of
//data can “push out” cached flash, possibly making the execution of code
//slower afterwards.
//
//In general, external RAM cannot be used as task stack memory. Due to this,
//xTaskCreate() and similar functions will always allocate internal memory for
//stack and task TCBs, and functions such as xTaskCreateStatic() will check if
//the buffers passed are internal.
Allocator PsramAllocator = {
  .malloc = ps_malloc,
  .calloc = ps_calloc,
  .realloc = ps_realloc,
  .free   = free,
  .nullFree = stdNullFree,
};





AsyncWebServer server(80);
AsyncWebSocket ws("/");

#define os_printf Serial.printf
void onEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len){
  if(type == WS_EVT_CONNECT){
    //client connected
    os_printf("ws[%s][%u] connect\n", server->url(), client->id());
    client->ping();
  } else if(type == WS_EVT_DISCONNECT){
    //client disconnected
    os_printf("ws[%s][%u] disconnect: %u\n", server->url(), client->id());
  } else if(type == WS_EVT_ERROR){
    //error was received from the other end
    os_printf("ws[%s][%u] error(%u): %s\n", server->url(), client->id(), *((uint16_t*)arg), (char*)data);
  } else if(type == WS_EVT_PONG){
    //pong message was received (in response to a ping request maybe)
    os_printf("ws[%s][%u] pong[%u]: %s\n", server->url(), client->id(), len, (len)?(char*)data:"");
  } else if(type == WS_EVT_DATA){
    //data packet
    AwsFrameInfo * info = (AwsFrameInfo*)arg;
    if(info->final && info->index == 0 && info->len == len){
      //the whole message is in a single frame and we got all of it's data
      os_printf("ws[%s][%u] %s-message[%llu]: ", server->url(), client->id(), (info->opcode == WS_TEXT)?"text":"binary", info->len);
      if(info->opcode == WS_TEXT){
        data[len] = 0;
        os_printf("%s\n", (char*)data);
        client->text("I got your text message");
      }
    } else {
      //message is comprised of multiple frames or the frame is split into multiple packets
      if(info->index == 0){
        if(info->num == 0)
          os_printf("ws[%s][%u] %s-message start\n", server->url(), client->id(), (info->message_opcode == WS_TEXT)?"text":"binary");
        os_printf("ws[%s][%u] frame[%u] start[%llu]\n", server->url(), client->id(), info->num, info->len);
      }

      os_printf("ws[%s][%u] frame[%u] %s[%llu - %llu]: ", server->url(), client->id(), info->num, (info->message_opcode == WS_TEXT)?"text":"binary", info->index, info->index + len);
      if(info->message_opcode == WS_TEXT){
        data[len] = 0;
        os_printf("%s\n", (char*)data);
      } else {
        for(size_t i=0; i < len; i++){
          os_printf("%02x ", data[i]);
        }
        os_printf("\n");
      }

      if((info->index + len) == info->len){
        os_printf("ws[%s][%u] frame[%u] end[%llu]\n", server->url(), client->id(), info->num, info->len);
        if(info->final){
          os_printf("ws[%s][%u] %s-message end\n", server->url(), client->id(), (info->message_opcode == WS_TEXT)?"text":"binary");
          if(info->message_opcode == WS_TEXT)
            client->text("I got your text message");
        }
      }
    }
  }
}

// TODO call every second
void cleanup() {
  ws.cleanupClients();
}




void setup() {
    Serial.begin(115200);
    Serial.println("\nBOOT");

#ifdef CONFIG_SPIRAM
    if (psramFound()) {
      Serial.println("PSRAM");
      if (psramInit()) {
        StringStorageAllocator = PsramAllocator;
        // Anything else to psram?
      } else {
        Serial.println("Fail");
      }
    }
#endif

    Serial.println("Structs");
    RequestHandler_init();
    StringStorage_init();
    if (ScriptEngine_init())
      Serial.println("ScriptEngine failed!");
    //connectionHandler.getPrintfForConnection = getOutput;

    //OmiParser * parser = getParser(0);

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    if (WiFi.waitForConnectResult() != WL_CONNECTED) {
        Serial.println("WiFi Fail!");
        return;
    }
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());

    // attach AsyncWebSocket
    ws.onEvent(onEvent);
    server.addHandler(&ws);

    server.begin();
}

void loop() {

    //char input[2] = " ";
    //int c;
    //while ((c = getchar()) != EOF) {
    //    input[0] = c;
    //    ErrorResponse result = runParser(parser, input);

    //    switch (result) {
    //        case Err_OK: break;
    //        default:
    //            responseFromErrorCode(parser, result);
    //            // return result;
    //            OmiParser_destroy(parser);
    //            OmiParser_init(parser, 0);
    //        case Err_End:
    //            printf("\r\n"); // Separator: Easier to pipe the result to other programs
    //            fflush(stdout);
    //            OmiParser_init(parser, 0);
    //            break;
    //    }
    //}
}


