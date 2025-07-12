// html_test.cpp - ESP32 Web Interface with OTA, WebSocket, SPIFFS File Manager

#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <Ticker.h>
#include <Update.h>
#include <NimBLEDevice.h>
#include <ota/simulation_Secure_Boot.h>

AsyncWebServer server(8080);  // ✅ 使用原本指定的 8080 Port
AsyncWebSocket ws("/ws");
Ticker ticker;
float mockCurrent = 0.0;
bool ledState = false;

// 輸入此網址 http://192.168.3.165:8080/
// 即時電流顯示、LED 控制、OTA 韌體更新、SPIFFS 檔案上傳/刪除整合版
TaskHandle_t otaTaskHandle = NULL;

// 用於儲存參數，簡單示範
String g_bin_url = "";
String g_sig_url = "";

void ota_ca_task(void* param) {
  while (true) {
    // 等待通知，永遠阻塞直到被喚醒
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    // 開始 OTA 模擬作業
    bool result = simulation_Secure_Boot_func(g_bin_url.c_str(), g_sig_url.c_str());

    if (result) {
      // OTA 成功，重啟
      // esp_restart();
    } else {
      // OTA 失敗，繼續等待下一次通知（回到阻塞）
      // 你也可以加 log 或其他處理
      Serial.println("OTA Fail");
    }
  }
}
// ===== WebSocket 控制 & 推送資料 =====
void notifyClients() {
  mockCurrent = 1.0 + 2.0 * sin(millis() / 1000.0);
  String json = "{\"current\":" + String(mockCurrent, 2) + ",\"led\":" + (ledState ? "true" : "false") + "}";
  ws.textAll(json);
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    String msg = String((char*)data, len);
    if (msg == "toggleLED") {
      ledState = !ledState;
      digitalWrite(2, ledState ? HIGH : LOW);
      notifyClients();
    }
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
             AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    notifyClients();
  } else if (type == WS_EVT_DATA) {
    handleWebSocketMessage(arg, data, len);
  }
}

// OTA 韌體上傳處理器
void handleFirmwareUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
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
    request->send(200, "text/plain", Update.hasError() ? "FAIL" : "OK, Rebooting...");
    delay(1000);
    ESP.restart();
  }
}

// 列出 SPIFFS 所有檔案
void handleListFiles(AsyncWebServerRequest *request) {
  String json = "[";
  File root = SPIFFS.open("/");
  File file = root.openNextFile();
  bool first = true;
  while (file) {
    if (!first) json += ",";
    json += "\"" + String(file.name()) + "\"";
    first = false;
    file = root.openNextFile();
  }
  json += "]";
  request->send(200, "application/json", json);
}

// 上傳檔案到 SPIFFS
File uploadFile2;
void handleFileUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
  if (!index) {
    if (!filename.startsWith("/")) filename = "/" + filename;
    if (SPIFFS.exists(filename)) SPIFFS.remove(filename);
    uploadFile2 = SPIFFS.open(filename, "w");
  }
  if (uploadFile2) uploadFile2.write(data, len);
  if (final) uploadFile2.close();
}

// 刪除 SPIFFS 檔案
void handleDelete(AsyncWebServerRequest *request) {
  if (!request->hasParam("file", true)) {
    request->send(400, "text/plain", "Missing file parameter");
    return;
  }
  String filename = request->getParam("file", true)->value();
  if (!filename.startsWith("/")) filename = "/" + filename;
  if (SPIFFS.exists(filename)) {
    SPIFFS.remove(filename);
    request->send(200, "text/plain", "Deleted");
  } else {
    request->send(404, "text/plain", "File not found");
  }
}

// 系統初始化函式 (使用者指定名稱與設定)
void html_test_init() {
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS mount failed");
    return;
  }

  xTaskCreate(ota_ca_task, "ota_task", 8192, NULL, 1, &otaTaskHandle);

  ws.onEvent(onEvent);
  server.addHandler(&ws);
  server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");

  server.on("/upload", HTTP_POST, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Upload complete");
  }, handleFirmwareUpload);

  server.on("/uploadFile", HTTP_POST, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Upload OK");
  }, handleFileUpload);

  server.on("/delete", HTTP_POST, handleDelete);
  server.on("/files", HTTP_GET, handleListFiles);





  // 初始化 server 路由
  server.on("/secure_ota", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!request->hasParam("bin") || !request->hasParam("sig")) {
      request->send(400, "text/plain", "缺少 bin 或 sig 參數");
      return;
    }

    // 儲存參數給 task 使用
    g_bin_url = request->getParam("bin")->value();
    g_sig_url = request->getParam("sig")->value();

    // 回應客戶端立即回應，避免阻塞
    request->send(200, "text/plain", "OTA 啟動中...");

    // 喚醒 task 執行 OTA
    xTaskNotifyGive(otaTaskHandle);
  });







  server.begin();
  ticker.attach(1, notifyClients);
  Serial.println("Web server started on port 8080");
}
