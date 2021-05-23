// vim: set syntax=cpp:


#ifndef OUTPUT_BUF_SIZE
#define OUTPUT_BUF_SIZE 3500
#define ParserSinglePassLength OUTPUT_BUF_SIZE
#endif

#include <esp32-hal-psram.h>
#include <esp32-hal.h>
#include <Arduino.h>
#include <ArduinoOTA.h>
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
#include <WebSocketsClient.h>


#include <buildinfo.h>

// Sensors
//#include "DFRobot_SHT20.h"



#include "OMIParser.h"
#include "requestHandler.h"
#include "StringStorage.h"
#include "ScriptEngine.h"
#include "utils.h"
#include "ParserUtils.h"

#define STR(s) #s
#define XSTR(x) STR(x)


#ifndef SERVO_FREQ
#define SERVO_FREQ 50 // Hz
#endif
#ifndef SERVO_RESOLUTION
#define SERVO_RESOLUTION 12 // bits
#endif

#ifndef HOSTNAME
#define HOSTNAME tk-dippa
#endif

// PINS
#ifndef LedPin
#define LedPin   18 
#endif
#ifdef ServoPin
// use ESP32-ESP32S2-AnalogWrite for servo, official AnalogWrite does not exist so normally either ledc or delta sigma needs to be used directly
#include <analogWrite.h>
//#define ServoPin 16
#define ServoEnable 1
#pragma message "Servo enabled on pin " XSTR(ServoPin) " "
#endif

#ifdef DhtPin
#define DhtEnable 1
//#include <Adafruit_Sensor.h>
//#include <DHT_U.h>
#include <DHT.h>
#define DHTTYPE DHT11
DHT dht(DhtPin, DHTTYPE);
#pragma message "DHT11 enabled on pin " XSTR(DhtPin) " "
#endif

// TODO: Find AsyncWebServer compatible WiFi manager!?
const char* ssid = XSTR(WIFI_SSID);
const char* password = XSTR(WIFI_PASS);
const char* hostname = XSTR(HOSTNAME);
#pragma message "WIFI_SSID is " XSTR(WIFI_SSID) " "
#pragma message "WIFI_PASS is " XSTR(WIFI_PASS) " "
#pragma message "HOSTNAME is ws://" XSTR(HOSTNAME) ".local"

#define println(...) Serial.println(__VA_ARGS__); Serial.flush()
#define os_printf(...) Serial.printf(__VA_ARGS__); Serial.flush()

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
  println("Fail");
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
  enum clientType {CT_AsyncServerClient, CT_StandaloneClient};
  AsyncWebSocketClient * client;
  int outputSize;
  int inputSize;
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
      conn.parser = getParser(i);
      if (conn.parser == NULL) {
        println("No parsers left!");
        return -1;
      }
      conn.client = client;
      //conn.outputBuffer = new AsyncWebSocketMessageBuffer(OUTPUT_BUF_SIZE);
      conn.outputBuffer[0] = 0;
      conn.inputBuffer[0] = 0;
      conn.outputSize = 0;
      conn.inputSize = 0;
      //OmiParser_init(conn.parser);
      return i;
    }
  }
  return -1;
}

// Uses E-O-MI connection id instead of websocket library client id
Printf getConnectionParserId(int eomiConnId) {
  //os_printf("R%d ", eomiConnId);
  if (eomiConnId < 0) return noConnection;
  currentConn = &wsConnections[eomiConnId];
  if (currentConn->client == NULL || currentConn->client->status() != WS_CONNECTED)
    return noConnection;
  return printfBuffer;
}

// TODO use Links ws client
//int openConnection(const char * url) {
//  int connectionId = initConnection(NULL);
//  if (connectionId < 0) return connectionId;
//
//  WebSocketsClient * client = new WebSocketsClient();
//  const char * hostStart = strstr(url, "ws://");
//  if (!hostStart) {
//    hostStart = url;
//  };
//  //char * hostEnd = strchr(hostStart, "/");
//  const char * hostEnd = hostStart + strcspn(hostStart, ":/");
//  const char * pathStart = hostEnd;
//  int port = 80;
//  if (*hostEnd == ':'){
//    int portlen = strcspn(hostEnd, "/");
//    char portStr[7] = {0};
//    strncpy(portStr, hostEnd+1, portlen-1);
//    port = parseInt(portStr);
//    pathStart = hostEnd+portlen;
//  }
//  
//  const char * end = hostStart + strlen(hostStart);
//  if (!hostEnd) {
//    hostEnd = end;
//  }
//  os_printf("ws connect: host %s port %d path ", hostStart, port);
//  if (hostEnd == end) {
//    println("/");
//    client->begin(hostStart, port, "/");
//  } else {
//    char host[200];
//    strncpy(host, hostStart, min(200,hostEnd-hostStart));
//    println(host);
//    client->begin(host, port, pathStart);
//  }
//  wsConnections[connectionId].type = client;
//  wsConnections[connectionId].client = client;
//  return connectionId;
//}


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
  if (wsConnections[eomiConnId].inputBuffer[0] == 0)
    OmiParser_destroy(wsConnections[eomiConnId].parser);
  
  // This might be needed, trying to stop crashes on disconnect (OMIParser.c:725 -> yxml.c:346)
  wsConnections[eomiConnId].client = NULL;

  ws.cleanupClients(); // prob. not needed
}

void handleParsing(OmiParser * parser, char * data) {
  if (!data[0]) return;
  ErrorResponse result = runParser(parser, data);
  Serial.printf("CID: %d PARSE RESULT %d \n", parser->parameters.connectionId, result);
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

void onEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len){
  if(type == WS_EVT_CONNECT){ //client connected

    os_printf("ws[%s][%u] connect ", server->url(), client->id());
    //client->ping();
    int id = initConnection(client);
    os_printf("Cid:%d\n", id);
    if (id == -1) {
      os_printf("initConnection error!\n");
      ws.pingAll(); // try to catch broken connections faster
      return;
    }

  } else if(type == WS_EVT_DISCONNECT){ //client disconnected

    int id = getParserIdFromWsId(client->id());
    os_printf("ws[%s][%u] disconnect: %d\n", server->url(), client->id(), id);
    destroyConnection(id);

  } else if(type == WS_EVT_ERROR){ //error was received from the other end

    os_printf("ws[%s][%u] error(%u): %s\n", server->url(), client->id(), *((uint16_t*)arg), (char*)data);

  } else if(type == WS_EVT_PONG){ //pong message was received (in response to a ping request maybe)

    os_printf("ws[%s][%u] pong[%u]: %s\n", server->url(), client->id(), len, (len)?(char*)data:"");

  } else if(type == WS_EVT_DATA){ //data packet

    AwsFrameInfo * info = (AwsFrameInfo*)arg;

    if (len == 0) {
      os_printf("ws[%s][%u] message ping\n", server->url(), client->id());
      return;
    }
    if (len+1 > OUTPUT_BUF_SIZE) error();

    int id = getParserIdFromWsId(client->id());
    if (id == -1) {
      os_printf("id error!\n");
      return;
    }
    auto & conn = wsConnections[id];

    if(info->final && info->index == 0 && info->len == len){
      //the whole message is in a single frame and we got all of it's data
      os_printf("ws[%s][%u] %s-message[%llu]: ", server->url(), client->id(), (info->opcode == WS_TEXT)?"text":"binary", info->len);

      if(info->opcode == WS_TEXT){
        //os_printf("%s\n", (char*)data);
        char * buffer = conn.inputBuffer;
        if (buffer[0] != '\0') {
          os_printf("unhandled data Cid:%d\n", id);
          return;
        }
        memcpy(buffer, data, len);
        buffer[len] = 0;
        conn.inputSize = len;
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
        char * buffer = conn.inputBuffer;
        if (buffer[0] != '\0') {
          if (conn.inputSize + len < OUTPUT_BUF_SIZE ) {
            buffer += conn.inputSize;
          }else {
            os_printf("unhandled data Cid:%d\n", id);
            return;
          }
        }
        memcpy(buffer, data, len);
        buffer[len] = 0;
        conn.inputSize += len;
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
    connectionHandler.endResponse = endWsMessage;
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
    for (int i = 0; wifiMulti.run(8000) != WL_CONNECTED && i < 4; ++i) {
      delay(2000);
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
    wifiMulti.addAP(ssid, password);
    //wifiMulti.addAP("***REMOVED***", "***REMOVED***");
    //wifiMulti.addAP("***REMOVED***", "***REMOVED***");

    //WiFi.mode(WIFI_STA);
    //WiFi.begin(ssid, password);
    connectWiFi();
}

void setupServer() {
    // attach AsyncWebSocket
    ws.onEvent(onEvent);

    server.addHandler(&ws);

    // CORS
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
    server.onNotFound([](AsyncWebServerRequest *request) {
      if (request->method() == HTTP_OPTIONS) {
        request->send(200);
      } else {
        request->send(404);
      }
    });


    server.begin();
}


void setupLED() {
  pixel.setBrightness(5); // max 255
  pixel.begin();
  pixel.setPixelColor(0,255,200,0); pixel.show();
}
void printLocalTime()
{
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    println("Failed to obtain time");
    return;
  }
  println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}

void setupNTP() {
  //configTime(long gmtOffset_sec, int daylightOffset_sec, const char* server1, const char* server2, const char* server3)
  configTime(TIMEZONE_SECS, TIMEZONE_DST_SECS, STR(NTP_SERVER0));//, STR(NTP_SERVER1), STR(NTP_SERVER2));
  printLocalTime();
}

Path * writeStringItem(OmiParser * p, const char * strPath, const char * value, bool noPop=false) {
    OmiParser_pushPath(p, strPath, OdfInfoItem);
    Path * path = p->currentOdfPath;
    path->flags = V_String;
    handleWrite(p, path, OdfParserEvent(PE_ValueData, strdup(value)));
    if (noPop) return path;
    OmiParser_popPath(p);
    return NULL;
}
Path * writeIntItem(OmiParser * p, const char * strPath, int value, bool noPop=false) {
    OmiParser_pushPath(p, strPath, OdfInfoItem);
    Path * path = p->currentOdfPath;
    path->flags = V_Int;
    path->value.i = value;
    handleWrite(p, path, OdfParserEvent(PE_ValueData,NULL));
    if (noPop) return path;
    OmiParser_popPath(p);
    return NULL;
}
Path * writeFloatItem(OmiParser * p, const char * strPath, float value, bool noPop=false) {
    OmiParser_pushPath(p, strPath, OdfInfoItem);
    Path * path = p->currentOdfPath;
    path->flags = V_Float;
    path->value.f = value;
    handleWrite(p, path, OdfParserEvent(PE_ValueData,NULL));
    if (noPop) return path;
    OmiParser_popPath(p);
    return NULL;
}


void addInternalSubscription(OmiParser *p, Path * path, OdfPathCallback handler) {
    HandlerInfo * subInfo = (HandlerInfo*) poolAlloc(&HandlerInfoPool);
    *subInfo = (HandlerInfo){
        .handlerType = HT_Script, 
        .handler = handler,
        .another = NULL,
        .prevOther = NULL,
        .nextOther = NULL,
        .callbackInfo = p->parameters, 
    };
    subInfo->callbackInfo.connectionId = -1; // do not try to send sub responses
    int resultIndex = -1;
    if (odfBinarySearch(&tree, path, &resultIndex)) {
      Path * subscriptionTarget = &tree.sortedPaths[resultIndex];
      addSubscription(NULL, subscriptionTarget, &subInfo);
    }
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
    //os_printf("Actuator Color %d\n", color);
    pixel.setBrightness(color >> 24);
    pixel.setPixelColor(0,color & 0x00FFFFFF);
    pixel.show();
    return Err_OK;
}

#ifdef ServoEnable
Ticker servoPowersaveTimer;
void servoPowerOff(){
  analogWrite(ServoPin, 0);
}
ErrorResponse handleServo(OmiParser *p, Path *path, OdfParserEvent event) {
    (void) event;
    if (PathGetNodeType(path) != OdfInfoItem) return Err_OK;
    AnyValue value = path->value.latest->current.value;

    float angle;
    switch (path->flags & PF_ValueType) {
      case V_Int:
      case V_UInt:
        angle = value.l; break;
      case V_Double:
        angle = value.d; break;
      case V_Float:
        angle = value.f; break;
      default:
        return Err_InvalidAttribute;
    }
    servoPowersaveTimer.detach();
    //os_printf("Actuator Servo angle %f ", angle);

    constexpr float in_min=0;
    constexpr float in_max=180;
    constexpr float out_min=(0.9/20.0 * (1<<(SERVO_RESOLUTION)));
    constexpr float out_max=(2.1/20.0 * (1<<(SERVO_RESOLUTION)));
    int32_t pwm = constrain((angle - in_min) * (out_max - out_min) / (in_max - in_min) + out_min,
        out_min, out_max);
    //os_printf("pwm %d / %f \n", pwm, out_max);
    analogWrite(ServoPin, pwm, SERVO_FREQ, SERVO_RESOLUTION);
    servoPowersaveTimer.once(3, servoPowerOff);
    return Err_OK;
}
#endif

#ifdef DhtEnable
//Path dhtParent; // an idea to reduce path constructing
//Path dhtTempPath;
//Path dhtHumiPath;
float dhtTemp = 0;
float dhtHumi = 0;

void readDht(OmiParser * p) {
  OmiParser_pushPath(p, "Dht11", OdfObject);
  float newTemp = dht.readTemperature();
  if (isnan(newTemp)) {
    Serial.print(F("Error reading temperature! "));
  } else {
    Serial.print(F("Temperature: ")); Serial.print(newTemp); Serial.print("°C ");
    dhtTemp = dhtTemp * 0.75 + newTemp * 0.25;
    writeFloatItem(p, "Temperature", dhtTemp);
  }

  float newHumi = dht.readHumidity();
  if (isnan(newHumi)) {
    println(F("Error reading humidity!"));
  } else {
    Serial.print(F("Humidity: ")); Serial.print(newHumi); println("%");
    dhtHumi = dhtHumi * 0.75 + newHumi * 0.25;
    writeFloatItem(p, "RelativeHumidity", dhtHumi);
  }
  OmiParser_popPath(p);
}
#endif


bool g_initial=true;
int counter = 6;
void writeInternalItems() {
  bool quick = true;
  if (counter++ >= 6) {
    counter = 0;
    quick = false;
  }
  OmiParser pa;
  OmiParser *p = &pa;
  OmiParser_init(p, -1); // negative connectionId prevents subscription responses to handlers
  p->parameters.requestType = OmiWrite;

  if (g_initial) {
    g_initial=false;


    OmiParser_pushPath(p, "Device", OdfObject);

    // Items
    writeStringItem(p, "SoftwareBuildTime", _BuildInfo.time);
    writeStringItem(p, "SoftwareBuildDate", _BuildInfo.date);
    writeStringItem(p, "SoftwareBuildVersion", _BuildInfo.src_version);
    writeStringItem(p, "SoftwareKernelVersion", _BuildInfo.env_version);
    writeStringItem(p, "ChipModel", ESP.getChipModel());
    writeStringItem(p, "SdkVersion", ESP.getSdkVersion());
    //TODO: writeStringItem(p, "SoftwareHashMD5", ESP.getSketchMD5().c_str());

    // Constant
    //uint32_t getCpuFreqMHz()
    writeIntItem(p, "ChipRevision", ESP.getChipRevision());
    OmiParser_pushPath(p, "Memory", OdfObject);
    //TODO: writeIntItem(p, "SoftwareSize", ESP.getSketchSize());
    //TODO: writeIntItem(p, "SoftwareSizeFree", ESP.getFreeSketchSpace());
    writeIntItem(p, "HeapTotal", ESP.getHeapSize());
    writeIntItem(p, "PSRAMTotal", ESP.getPsramSize());
    OmiParser_popPath(p); // Memory
    OmiParser_popPath(p); // Device

    // Writable items
    OmiParser_pushPath(p, "RgbLed", OdfObject);
    Path * path;
    path = writeIntItem(p, "ColorRGB", 0, true);
    addInternalSubscription(p, path, handleRgbLed);
    OmiParser_popPath(p); // InfoItem
    OmiParser_popPath(p);

#ifdef ServoEnable
    OmiParser_pushPath(p, "Servo", OdfObject);
    path = writeFloatItem(p, "SetDegrees", 0, true);
    addInternalSubscription(p, path, handleServo);
    // TODO: add items to modify the max and min
    OmiParser_popPath(p); // InfoItem
    OmiParser_popPath(p);
#endif

#ifdef DhtEnable
    dht.begin();
    dhtTemp = dht.readTemperature();
    dhtHumi = dht.readHumidity();
#endif
  }

  // Non-constant variables

#ifdef DhtEnable
  readDht(p);
#endif

  OmiParser_pushPath(p, "Device", OdfObject);
  writeFloatItem(p, "ChipTemperature", temperatureRead());
  if (!quick) {
    // Memory information
    //Internal RAM
    OmiParser_pushPath(p, "Memory", OdfObject);
    writeIntItem(p, "HeapFree", ESP.getFreeHeap());
    writeIntItem(p, "HeapMinFree", ESP.getMinFreeHeap());
    //largest block of heap that can be allocated at once
    writeIntItem(p, "HeapLargestFreeBlock", ESP.getMaxAllocHeap());

    //SPI RAM
    writeIntItem(p, "PSRAMFree", ESP.getFreePsram());
    writeIntItem(p, "PSRAMMinFree", ESP.getMinFreePsram());
    //largest block of heap that can be allocated at once
    writeIntItem(p, "PSRAMLargestFreeBlock", ESP.getMaxAllocPsram());

    OmiParser_popPath(p); // Memory
  }
  OmiParser_popPath(p); // Device

  // End any subscription responses
  handleWrite(p, NULL, OdfParserEvent(PE_RequestEnd, NULL));

  OmiParser_destroy(p);
}

Ticker cleanupTimer;
Ticker internalWriteTimer;
void setupTimers() {
  cleanupTimer.attach(1, cleanup);
  internalWriteTimer.attach(10, writeInternalItems);
}

void setupOTA() {
// OTA callbacks
  ArduinoOTA.setPassword("admin");
  ArduinoOTA.onStart([]() {
    // Clean SPIFFS
    //SPIFFS.end();

    // Disable client connections    
    ws.enable(false);

    // Close them
    ws.closeAll();
  });
  ArduinoOTA.begin();
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
      if (conn.client && conn.client->status() != WS_CONNECTED) {
        OmiParser_destroy(conn.parser);
      }
    }
  }
  if (WiFi.status() != WL_CONNECTED) connectWiFi();
  ArduinoOTA.handle();
}

// if (typeof triggered !== "undefined") triggered = false;

