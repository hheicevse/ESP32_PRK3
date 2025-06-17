#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <app/html_test.h>
#include <math.h>
#include <SPIFFS.h> 
#include <Ticker.h>  // 別忘記這個 include
//輸入此網址 http://192.168.3.165:8080
#include <Update.h>
#include <NimBLEDevice.h>

AsyncWebServer server(8080);
AsyncWebSocket ws("/ws");
Ticker ticker; // 每秒呼叫 notifyClients()
float mockCurrent = 0.0;


bool ledState = false;



// OTA 韌體上傳處理器
void handleFirmwareUpload(AsyncWebServerRequest *request,
                          String filename, size_t index,
                          uint8_t *data, size_t len, bool final) {
  if (!index) {
    Serial.printf("OTA Start: %s\n", filename.c_str());
    NimBLEDevice::deinit(false); // 關閉藍牙，避免 Update 失敗
    if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
      Update.printError(Serial);
    }
  }

  if (Update.write(data, len) != len) {
    Update.printError(Serial);
  }

  if (final) {
    if (Update.end(true)) {
      Serial.printf("OTA Success: %u bytes\n", index + len);
    } else {
      Update.printError(Serial);
    }

    request->send(200, "text/plain",
                  Update.hasError() ? "FAIL" : "OK, Rebooting...");
    delay(1000);
    ESP.restart();
  }
}




void notifyClients() {
  mockCurrent = 1.0 + 2.0 * sin(millis() / 1000.0);
  String json = "{\"current\":" + String(mockCurrent, 2) + ",\"led\":" + String(ledState ? "true" : "false") + "}";
  ws.textAll(json);
}

// #include <ArduinoJson.h>
// void notifyClients() {
//   StaticJsonDocument<100> doc;
//   doc["current"] =  String(mockCurrent, 2); // 你目前測到的電流值
//   doc["led"] = ledState ? "true" : "false";

//   String jsonStr;
//   serializeJson(doc, jsonStr);
//   ws.textAll(jsonStr); // 對所有連線送資料
// }
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    String msg = String((char*)data, len); 
    if (msg == "toggleLED") {
      ledState = !ledState;
      digitalWrite(2, ledState ? HIGH : LOW);
      // notifyClients();

      String json = "{\"current\":" + String(mockCurrent, 2) + ",\"led\":" + String(ledState ? "true" : "false") + "}";
      ws.textAll(json);

    }
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
             AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    Serial.println("WebSocket client connected");
  } else if (type == WS_EVT_DISCONNECT) {
    Serial.println("WebSocket client disconnected");
  } else if (type == WS_EVT_DATA) {
    handleWebSocketMessage(arg, data, len);
  }
}
void html_test_init(void){
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS Mount Failed");
    return;
  }
  ws.onEvent(onEvent);
  server.addHandler(&ws);
  server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");
  
  server.on(
    "/upload", HTTP_POST,
    [](AsyncWebServerRequest *request) {},
    handleFirmwareUpload
  );


  server.begin();
  ticker.attach(1.0, notifyClients);
  
}
