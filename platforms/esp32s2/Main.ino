// vim: set syntax=cpp:



#include <esp32-hal-psram.h>
#include <esp32-hal.h>
#include <Arduino.h>
#ifdef ESP32
#include <WiFi.h>
#include <AsyncTCP.h>
#include <Adafruit_NeoPixel.h>
#include <Ticker.h>
#include <ESPmDNS.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#endif
#include <ESPAsyncWebServer.h>
#include <WiFiMulti.h>

#include <buildinfo.h>

// Sensors
//#include "DFRobot_SHT20.h"

// use ESP32-ESP32S2-AnalogWrite for servo, official AnalogWrite does not exist so normally either ledc or delta sigma needs to be used directly
#include <analogWrite.h>


#include "OMIParser.h"
#include "requestHandler.h"
#include "StringStorage.h"
#include "ScriptEngine.h"
#include "utils.h"

#ifndef OUTPUT_BUF_SIZE
#define OUTPUT_BUF_SIZE 3000
#endif


// PINS
#define LedPin   18 
#define ServoPin 17


// TODO: Find AsyncWebServer compatible WiFi manager!?
#define STR(s) #s
const char* ssid = STR(WIFI_SSID);
const char* password = STR(WIFI_PASS);
const char* hostname = "tk-dippa";
#define XSTR(x) STR(x)
#pragma message "SSID is " XSTR(WIFI_SSID)
#pragma message "Pass is " XSTR(WIFI_PASS)


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





WiFiMulti wifiMulti;
AsyncWebServer server(80);
AsyncWebSocket ws("/");

Adafruit_NeoPixel pixel(1, LedPin, NEO_GRB + NEO_KHZ800);
void error() {
  Serial.println("Fail");
  pixel.setBrightness(20);
  pixel.setPixelColor(0, 255,0,0);
  pixel.show();
  delay(10000);
  ESP.restart();
}

int noConnection(const char* c, ...){
  (void) c;
  return 0;
}

// Each subscription requires own output buffer because of streaming
typedef struct ConnectionData {
  //AsyncWebSocketMessageBuffer * outputBuffer;
  AsyncWebSocketClient * client;
  int outputSize;
  OmiParser * parser;
  char inputBuffer[OUTPUT_BUF_SIZE];
  char outputBuffer[OUTPUT_BUF_SIZE];
} ConnectionData;
ConnectionData wsConnections[NumConnections] = {0};

ConnectionData * currentConn = NULL;
int printfBuffer(const char *format, ...) {
  if (!currentConn) return 0;
  char* buffer = currentConn->outputBuffer;
  va_list arg;
  va_start(arg, format);
  // FIXME: use longer buffer if not  fitting
  size_t len = vsnprintf(buffer + currentConn->outputSize, OUTPUT_BUF_SIZE - currentConn->outputSize, format, arg);
  currentConn->outputSize += len;
  va_end(arg);
  return len;
}


int initConnection(AsyncWebSocketClient * client){
  for (int i = 0; i < NumConnections; ++i) {
    auto & conn = wsConnections[i];
    if ((conn.client == NULL || conn.client->status() == WS_DISCONNECTED) && conn.inputBuffer[0] == 0) {
      conn.client = client;
      //conn.outputBuffer = new AsyncWebSocketMessageBuffer(OUTPUT_BUF_SIZE);
      conn.outputBuffer[0] = 0;
      conn.inputBuffer[0] = 0;
      conn.outputSize = 0;
      conn.parser = getParser(i);
      return i;
    }
  }
  return -1;
}

// Uses E-O-MI connection id instead of websocket library client id
Printf getConnectionParserId(int eomiConnId) {
  if (eomiConnId < 0) return noConnection;
  currentConn = &wsConnections[eomiConnId];
  if (currentConn->client && currentConn->client->status() != WS_CONNECTED)
    return noConnection;
  return printfBuffer;
}
int getParserIdFromWsId(int wsId) {
  for (int i = 0; i < NumConnections; ++i) {
    auto & conn = wsConnections[i];
    if (conn.client != NULL) {
      int connId = conn.client->id();
      if (connId == wsId) {
        return i;
      }
    }
  }
  return -1;
}
void endWsMessage(int eomiConnId) {
  if (eomiConnId < 0) return;
  auto & conn = wsConnections[eomiConnId];
  if (conn.outputSize == 0) return;
  //conn.outputBuffer[conn.outputSize] = '\0';
  if (conn.client == NULL || conn.client->status() != WS_CONNECTED) return;
  conn.client->text(conn.outputBuffer, conn.outputSize);
  conn.outputSize = 0;
}
void destroyConnection(int eomiConnId) {
  if (eomiConnId < 0) return;
  //OmiParser_destroy(wsConnections[eomiConnId].parser);
  
  // This might be needed, trying to stop crashes on disconnect (OMIParser.c:725 -> yxml.c:346)
  //wsConnections[eomiConnId].client = NULL;

  ws.cleanupClients(); // prob. not needed
}

void handleParsing(OmiParser * parser, char * data) {
  if (!data[0]) return;
  ErrorResponse result = runParser(parser, data);
  Serial.printf("PARSE RESULT %d\n", result);
  int id = parser->parameters.connectionId;
  switch (result) {
    case Err_OK: break;
    default:
      responseFromErrorCode(parser, result);
    case Err_End:
      endWsMessage(id);
      OmiParser_destroy(parser);
      data[0] = 0; // mark as processed
      OmiParser_init(parser, id);
      break;
  }
}

#define println(...) Serial.println(__VA_ARGS__); Serial.flush()
#define os_printf(...) Serial.printf(__VA_ARGS__); Serial.flush()
void onEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len){
  if(type == WS_EVT_CONNECT){ //client connected

    os_printf("ws[%s][%u] connect", server->url(), client->id());
    //client->ping();
    int id = initConnection(client);
    os_printf("Cid:%d\n", id);
    if (id == -1) {
      os_printf("initConnection error!\n");
      return;
    }

  } else if(type == WS_EVT_DISCONNECT){ //client disconnected

    os_printf("ws[%s][%u] disconnect: %u\n", server->url(), client->id());
    destroyConnection(getParserIdFromWsId(client->id()));

  } else if(type == WS_EVT_ERROR){ //error was received from the other end

    os_printf("ws[%s][%u] error(%u): %s\n", server->url(), client->id(), *((uint16_t*)arg), (char*)data);

  } else if(type == WS_EVT_PONG){ //pong message was received (in response to a ping request maybe)

    os_printf("ws[%s][%u] pong[%u]: %s\n", server->url(), client->id(), len, (len)?(char*)data:"");

  } else if(type == WS_EVT_DATA){ //data packet

    AwsFrameInfo * info = (AwsFrameInfo*)arg;

    if (len+1 > OUTPUT_BUF_SIZE) error();

    if(info->final && info->index == 0 && info->len == len){
      //the whole message is in a single frame and we got all of it's data
      os_printf("ws[%s][%u] %s-message[%llu]: ", server->url(), client->id(), (info->opcode == WS_TEXT)?"text":"binary", info->len);

      if(info->opcode == WS_TEXT){
        //os_printf("%s\n", (char*)data);
        int id = getParserIdFromWsId(client->id());
        if (id == -1) {
          os_printf("id error!\n");
          return;
        }
        char * buffer = wsConnections[id].inputBuffer;
        if (buffer[0] != '\0') {
          os_printf("unhandled data Cid:%d\n", id);
          return;
        }
        memcpy(buffer, data, len);
        buffer[len] = 0;
        os_printf("data copied Cid:%d\n", id);
        //OmiParser * parser = wsConnections[id].parser;
        //handleParsing(parser, data);
      }
    } else {
      //message is comprised of multiple frames or the frame is split into multiple packets
      if(info->index == 0){
        if(info->num == 0) {
          os_printf("ws[%s][%u] %s-message start\n", server->url(), client->id(), (info->message_opcode == WS_TEXT)?"text":"binary");
        }
        os_printf("ws[%s][%u] frame[%u] start[%llu]\n", server->url(), client->id(), info->num, info->len);
      }

      os_printf("ws[%s][%u] frame[%u] %s[%llu - %llu]: ", server->url(), client->id(), info->num, (info->message_opcode == WS_TEXT)?"text":"binary", info->index, info->index + len);
      int id = -1;
      if(info->message_opcode == WS_TEXT){
        //os_printf("%s\n", (char*)data);
        //if (info->index == 0 && info->num == 0) {
        //  id = initConnection(client);
        //} else {
        id = getParserIdFromWsId(client->id());
        //}
        if (id == -1) {
          os_printf("id error!\n");
          return;
        }
        char * buffer = wsConnections[id].inputBuffer;
        if (buffer[0] != '\0') {
          os_printf("unhandled data Cid:%d\n", id);
          return;
        }
        memcpy(buffer, data, len);
        buffer[len] = 0;
        os_printf("data copied Cid:%d\n", id);
        //OmiParser * parser = wsConnections[id].parser;
        //handleParsing(parser, data);

      }

      if((info->index + len) == info->len){
        os_printf("ws[%s][%u] frame[%u] end[%llu]\n", server->url(), client->id(), info->num, info->len);
        if(info->final){
          os_printf("ws[%s][%u] %s-message end\n", server->url(), client->id(), (info->message_opcode == WS_TEXT)?"text":"binary");
          if(info->message_opcode == WS_TEXT)
            endWsMessage(id);
            //destroyConnection(id);
        }
      }
    }
  }
}

// TODO call every second
void cleanup() {
  ws.cleanupClients();
}



void setupPSRAM() {
#ifdef CONFIG_SPIRAM
    if (psramFound()) {
      println("PSRAM");
      if (psramInit()) {
        StringStorageAllocator = PsramAllocator;
        // Anything else to psram?
      } else {
        error();
      }
    }
#endif
}

void setupEOMI() {
    println("EOMI");
    RequestHandler_init();
    StringStorage_init();
    if (ScriptEngine_init()){
      println("ScriptEngine failed!");
      error();
    }
    connectionHandler.getPrintfForConnection = getConnectionParserId;
}

void connectWiFi(){
    //Serial.print(ssid); Serial.print(" "); Serial.print(password);
    //if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    //  // Had problem with connecting; Double try recommended on forums
    //  WiFi.reconnect();
    //  for (int i = 0; WiFi.status() != WL_CONNECTED && i < 20*12; ++i) {
    //      delay(500);
    //      Serial.print(".");
    //  }
    //  if (WiFi.status() != WL_CONNECTED) error();
    //}
    for (int i = 0; wifiMulti.run(8000) != WL_CONNECTED && i < 5; ++i) {
      delay(1000);
      Serial.print(".");
    }
    if (WiFi.status() != WL_CONNECTED) {
      delay(5000);
      if (WiFi.status() != WL_CONNECTED)
        error();
    }

    Serial.print("IP: ");
    println(WiFi.localIP());
    pixel.setPixelColor(0, 0,255,255); pixel.show();
    println("mDNS");
    if (!MDNS.begin(hostname)) error();
}
void setupWiFi() {
    println("WIFI");
    WiFi.setHostname(hostname);
    //wifiMulti.addAP(ssid, password);
    wifiMulti.addAP("***REMOVED***", "***REMOVED***");
    wifiMulti.addAP("***REMOVED***", "***REMOVED***");
    //wifiMulti.addAP("***REMOVED***", "***REMOVED***");

    //WiFi.mode(WIFI_STA);
    //WiFi.begin(ssid, password);
    connectWiFi();
}

void setupServer() {
    // attach AsyncWebSocket
    ws.onEvent(onEvent);
    server.addHandler(&ws);

    server.begin();
}


void setupLED() {
  pixel.setBrightness(5); // max 255
  pixel.begin();
  pixel.setPixelColor(0,255,200,0); pixel.show();
}

void setupNTP() {
  //configTime(long gmtOffset_sec, int daylightOffset_sec, const char* server1, const char* server2, const char* server3)
  configTime(TIMEZONE_SECS, TIMEZONE_DST_SECS, STR(NTP_SERVER0), STR(NTP_SERVER1), STR(NTP_SERVER2));
}

Path * writeObject(OmiParser * p, OdfTree & odf, const char * strPath) {
    Path * path = addPath(&odf, strPath, OdfObject);
    handleWrite(p, path, OdfParserEvent(PE_Path, NULL));
    return path;
}
Path * writeStringItem(OmiParser * p, OdfTree & odf, const char * strPath, const char * value) {
    Path * path = addPath(&odf, strPath, OdfInfoItem);
    handleWrite(p, path, OdfParserEvent(PE_Path, NULL));
    path->flags = V_String;
    handleWrite(p, path, OdfParserEvent(PE_ValueData, strdup(value)));
    return path;
}
Path * writeIntItem(OmiParser * p, OdfTree & odf, const char * strPath, int value) {
    Path * path = addPath(&odf, strPath, OdfInfoItem);
    handleWrite(p, path, OdfParserEvent(PE_Path, NULL));
    path->flags = V_Int;
    path->value.i = value;
    handleWrite(p, path, OdfParserEvent(PE_ValueData,NULL));
    return path;
}
Path * writeFloatItem(OmiParser * p, OdfTree & odf, const char * strPath, float value) {
    Path * path = addPath(&odf, strPath, OdfInfoItem);
    handleWrite(p, path, OdfParserEvent(PE_Path, NULL));
    path->flags = V_Float;
    path->value.f = value;
    handleWrite(p, path, OdfParserEvent(PE_ValueData,NULL));
    return path;
}


ErrorResponse handleRgbLed(OmiParser *p, Path *path, OdfParserEvent event) {
    (void) event;
    if (PathGetNodeType(path) != OdfInfoItem) return Err_OK;
    AnyValue value = path->value.latest->current.value;
    uint32_t color = 0;
    switch (path->flags & PF_ValueType) {
      case V_Int:
        color = value.i;
        break;
      case V_UInt:
        color = value.l;
        break;
      default:
        return Err_InvalidAttribute;
    }
    os_printf("Actuator color %d\n", color);
    pixel.setBrightness(color >> 24);
    pixel.setPixelColor(0,color & 0x00FFFFFF);
    pixel.show();
    return Err_OK;
}

bool g_initial=true;
void writeInternalItems() {
  OmiParser pa;
  OmiParser *p = &pa;
  OmiParser_init(p, -1); // negative connectionId prevents subscription responses to handlers
  p->parameters.requestType = OmiWrite;
  const char maxPaths = 30;
  Path paths[maxPaths]; 
  OdfTree odf;
  OdfTree_init(&odf, paths, maxPaths);
  //Path * path;

  if (g_initial) {
    g_initial=false;
    // Objects; NOTE: Each must have its parent defined before
    writeObject(p, odf, "Objects/Device");
    writeObject(p, odf, "Objects/Device/Memory");
    writeObject(p, odf, "Objects/RgbLed");

    // Items
    writeStringItem(p, odf, "Objects/Device/SoftwareBuildTime", _BuildInfo.time);
    writeStringItem(p, odf, "Objects/Device/SoftwareBuildDate", _BuildInfo.date);
    writeStringItem(p, odf, "Objects/Device/SoftwareBuildVersion", _BuildInfo.src_version);
    writeStringItem(p, odf, "Objects/Device/SoftwareKernelVersion", _BuildInfo.env_version);
    writeStringItem(p, odf, "Objects/Device/ChipModel", ESP.getChipModel());
    writeStringItem(p, odf, "Objects/Device/SdkVersion", ESP.getSdkVersion());
    writeStringItem(p, odf, "Objects/Device/SoftwareHashMD5", ESP.getSketchMD5().c_str());

    // Constant
    //uint32_t getCpuFreqMHz()
    writeIntItem(p, odf, "Objects/Device/ChipRevision", ESP.getChipRevision());
    writeIntItem(p, odf, "Objects/Device/Memory/SoftwareSize", ESP.getSketchSize());
    writeIntItem(p, odf, "Objects/Device/Memory/SoftwareSizeFree", ESP.getFreeSketchSpace());
    writeIntItem(p, odf, "Objects/Device/Memory/HeapTotal", ESP.getHeapSize());
    writeIntItem(p, odf, "Objects/Device/Memory/PSRAMTotal", ESP.getPsramSize());

    // Writable items
    HandlerInfo * subInfo = (HandlerInfo*) poolAlloc(&HandlerInfoPool);
    *subInfo = (HandlerInfo){
        .handlerType = HT_Script, 
        .handler = handleRgbLed,
        .another = NULL,
        .prevOther = NULL,
        .nextOther = NULL,
        .callbackInfo = p->parameters, 
    };
    Path * path = writeIntItem(p, odf, "Objects/RgbLed/ColorRGB", 0);
    int resultIndex = -1;
    if (odfBinarySearch(&tree, path, &resultIndex)) {
      Path * subscriptionTarget = &tree.sortedPaths[resultIndex];
      addSubscription(NULL, subscriptionTarget, &subInfo);
    }
  }

  // Non-constant variables

  // Memory information
  //Internal RAM
  writeIntItem(p, odf, "Objects/Device/Memory/HeapFree", ESP.getFreeHeap());
  writeIntItem(p, odf, "Objects/Device/Memory/HeapMinFree", ESP.getMinFreeHeap());
  //largest block of heap that can be allocated at once
  writeIntItem(p, odf, "Objects/Device/Memory/HeapLargestFreeBlock", ESP.getMaxAllocHeap());

  //SPI RAM
  writeIntItem(p, odf, "Objects/Device/Memory/PSRAMFree", ESP.getFreePsram());
  writeIntItem(p, odf, "Objects/Device/Memory/PSRAMMinFree", ESP.getMinFreePsram());
  //largest block of heap that can be allocated at once
  writeIntItem(p, odf, "Objects/Device/Memory/PSRAMLargestFreeBlock", ESP.getMaxAllocPsram());

  writeFloatItem(p, odf, "Objects/Device/ChipTemperature", temperatureRead());


  OmiParser_destroy(p);
}

Ticker cleanupTimer;
Ticker internalWriteTimer;
void setupTimers() {
  cleanupTimer.attach(1, cleanup);
  internalWriteTimer.attach(60, writeInternalItems);
}

void setup() {
    Serial.begin(115200);
    println("\nBOOT");

    setupLED();

    setupWiFi();

    setupNTP();

    setupPSRAM();

    setupEOMI();

    setupServer();

    setupTimers();

    writeInternalItems();

    pixel.setPixelColor(0, 0,0,255); pixel.show();
    println("BOOT OK");
}

void loop() {
  for (int i = 0; i < NumConnections; ++i) {
    auto & conn = wsConnections[i];
    if (conn.inputBuffer[0] != '\0') {
      os_printf("PARSE %s\n", conn.inputBuffer);
      handleParsing(conn.parser, conn.inputBuffer);
    }
  }
  if (WiFi.status() != WL_CONNECTED) connectWiFi();
}


