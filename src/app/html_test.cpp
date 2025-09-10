// html_test.cpp - ESP32 Web Interface with OTA, WebSocket, SPIFFS File Manager

#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <Ticker.h>
#include <Update.h>
#include <NimBLEDevice.h>
#include <ota/simulation_Secure_Boot.h>

#include <app/sn_info.h>
#include <app/ble_ctrl.h>
#include <app/tcp_socket.h>
#include <app/debug.h>
#include <app/html_test.h>
#include <app/nvs.h>
#include <app/tcp_client.h>
#include <app/json.h>

#include <mspm0/bsl.h>
#include <mspm0/modbus.h>
#include <mspm0/mspm0_esp32_comm.h>

#include <bsp/tmr.h>
#include <bsp/uart.h>
#include <bsp/watchdog.h>
#include <bsp/wifi_ctrl.h>

#include <HTTPClient.h>

AsyncWebServer server(8080); // ✅ 使用原本指定的 8080 Port
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

// 建立 OTA 狀態結構
struct OTAStatus
{
  String status; // "Idle", "Queued", "Start", "Progress", "Success", "Fail"
  int percent;
  String error;
  String url;
};

OTAStatus otaStatus = {"Idle", 0, "", ""};
TaskHandle_t otaTaskHandle2 = nullptr;

// Arduino-friendly 餵 WDT
inline void feedWatchdog()
{
  yield(); // Arduino 內部會餵 WDT
}

// 改寫 ota_http_func 成為 Task（非阻塞）
void otaTask(void *param)
{
  String url = otaStatus.url;  // 直接從全域讀
  if (url.length() == 0) {
    otaStatus.status = "Fail";
    otaStatus.error = "Empty URL";
    vTaskDelete(NULL);
    return;
  }

  otaStatus.status = "Start";
  otaStatus.percent = 0;
  otaStatus.error = "";
  otaStatus.url = url;

  HTTPClient http;
  if (!http.begin(url)) {
    otaStatus.status = "Fail";
    otaStatus.error = "HTTP begin failed";
    vTaskDelete(NULL);
    return;
  }

  int httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK) {
    otaStatus.status = "Fail";
    otaStatus.error = "HTTP GET failed: " + String(httpCode);
    http.end();
    vTaskDelete(NULL);
    return;
  }

  int contentLength = http.getSize();
  if (contentLength <= 0) {
    otaStatus.status = "Fail";
    otaStatus.error = "Invalid content length";
    http.end();
    vTaskDelete(NULL);
    return;
  }

  WiFiClient &stream = http.getStream();
  const size_t bufferSize = 1024;
  uint8_t buffer[bufferSize];
  size_t totalWritten = 0;

  if (!Update.begin(contentLength)) {
    otaStatus.status = "Fail";
    otaStatus.error = "Not enough space for OTA";
    http.end();
    vTaskDelete(NULL);
    return;
  }

  int lastPercent = -1;
  unsigned long lastRead = millis();

  // 🔹 加 timeout 機制，避免無限卡住
  while (stream.connected() && totalWritten < (size_t)contentLength) {
    size_t available = stream.available();
    if (available > 0) {
      lastRead = millis(); // 更新最後讀取時間
      size_t toRead = min(available, bufferSize);
      int readBytes = stream.readBytes(buffer, toRead);
      if (readBytes > 0) {
        size_t written = Update.write(buffer, readBytes);
        if (written != (size_t)readBytes) {
          otaStatus.status = "Fail";
          otaStatus.error = "Update.write failed";
          Update.end();
          http.end();
          vTaskDelete(NULL);
          return;
        }
        totalWritten += written;
        int percent = (int)((totalWritten * 100) / contentLength);
        if (percent != lastPercent) {
          otaStatus.percent = percent;
          lastPercent = percent;
        }
      }
    }

    if (millis() - lastRead > 10000) { // 超過 10 秒沒收到資料
      otaStatus.status = "Fail";
      otaStatus.error = "Stream timeout";
      Update.end();
      http.end();
      vTaskDelete(NULL);
      return;
    }

    feedWatchdog(); // ✅ 餵 watchdog
    delay(1);       // ✅ 避免卡死 CPU
  }

  if (Update.end() && Update.isFinished()) {
    otaStatus.status = "Success";
    http.end();
    delay(100); // 等待 client poll
    ESP.restart();
  } else {
    otaStatus.status = "Fail";
    otaStatus.error = Update.errorString();
    http.end();
  }

  vTaskDelete(NULL);
}

// 設定命令對應的處理函式
#include <functional>
#include <map>

using CommandHandler = std::function<void(JsonObject &req, JsonObject &res)>;

std::map<String, CommandHandler> commandMap;
void setupCommandMap()
{
  commandMap["get_wifi"] = [](JsonObject &req, JsonObject &res)
  {
    String ssid, pwd;
    loadWiFiCredentials(ssid, pwd);

    res["status"] = "ok";
    res["ssid"] = ssid;
    res["password"] = pwd;
  };

  commandMap["set_wifi"] = [](JsonObject &req, JsonObject &res)
  {
    String ssid = req["ssid"] | "";
    String password = req["pwd"] | "";
    saveWiFiCredentials(ssid, password);

    res["status"] = "ok";
  };

  commandMap["set_wifi_connect"] = [](JsonObject &req, JsonObject &res)
  {
    String ssid, pwd;
    loadWiFiCredentials(ssid, pwd);
    if (WiFi.status() != WL_CONNECTED)
    {
      WiFi.begin(ssid.c_str(), pwd.c_str());
    }
    res["status"] = "ok";
  };

  commandMap["set_wifi_disconnect"] = [](JsonObject &req, JsonObject &res)
  {
    WiFi.disconnect();
    res["status"] = "ok";
  };

  commandMap["get_wifi_status"] = [](JsonObject &req, JsonObject &res)
  {
    res["status"] = "ok";
    res["connection"] = (WiFi.status() == WL_CONNECTED) ? "connected" : "disconnected";
  };

  commandMap["get_wifi_ip"] = [](JsonObject &req, JsonObject &res)
  {
    res["ip"] = WiFi.localIP().toString();
    res["port"] = String(SERVER_PORT_STA);
  };

  commandMap["ota"] = [](JsonObject &req, JsonObject &res)
  {
    if (!req.containsKey("file"))
    {
      res["status"] = "error";
      res["error"] = "Missing file parameter";
      return;
    }

    // 傳遞參數給 FreeRTOS Task 時，不能傳遞區域變數指標，應該用 全域變數 或 動態配置
    String url = req["file"] | "";

    // 重置 OTA 狀態
    otaStatus = {"Queued", 0, "", url};

    // 建立 OTA task
    if (otaTaskHandle2 != nullptr)
    {
      vTaskDelete(otaTaskHandle2); // 停掉前一個任務
    }
    xTaskCreate(otaTask, "OTA_Task", 8192, NULL, 1, &otaTaskHandle2);

    res["status"] = "ok";
    res["message"] = "OTA started";
  };

  commandMap["ota_status"] = [](JsonObject &req, JsonObject &res)
  {
    res["status"] = "ok";
    res["ota_status"] = otaStatus.status;
    res["percent"] = otaStatus.percent;
    res["error"] = otaStatus.error;
    res["url"] = otaStatus.url;
  };
}

// ===== 處理 /command POST API =====
void handleCommand(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
{
  static String bodyBuffer = "";

  // 每次 chunk 都累積
  bodyBuffer += String((char *)data, len);

  // 最後一個 chunk
  if (index + len == total)
  {
    DynamicJsonDocument response(1024);
    JsonObject res = response.to<JsonObject>();

    DynamicJsonDocument bodyDoc(1024);
    DeserializationError err = deserializeJson(bodyDoc, bodyBuffer);
    if (err)
    {
      res["status"] = "error";
      res["error"] = "Invalid JSON: " + String(err.c_str());
      request->send(400, "application/json", response.as<String>());
      bodyBuffer = "";
      return;
    }

    String command = bodyDoc["command"] | "";
    if (command == "" || commandMap.find(command) == commandMap.end())
    {
      res["status"] = "error";
      res["error"] = "Unknown or missing command";
      request->send(400, "application/json", response.as<String>());
      bodyBuffer = "";
      return;
    }

    res["command"] = command;
    JsonObject req = bodyDoc.as<JsonObject>();
    commandMap[command](req, res);

    request->send(200, "application/json", response.as<String>());

    bodyBuffer = ""; // 重置
  }
}

void ota_ca_task(void *param)
{
  while (true)
  {
    // 等待通知，永遠阻塞直到被喚醒
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    // 開始 OTA 模擬作業
    bool result = simulation_Secure_Boot_func(g_bin_url.c_str(), g_sig_url.c_str());

    if (result)
    {
      // OTA 成功，重啟
      // esp_restart();
    }
    else
    {
      // OTA 失敗，繼續等待下一次通知（回到阻塞）
      // 你也可以加 log 或其他處理
      Serial.println("[HTML] OTA Fail");
    }
  }
}
// ===== WebSocket 控制 & 推送資料 =====
void notifyClients()
{
  mockCurrent = 1.0 + 2.0 * sin(millis() / 1000.0);
  String json = "{\"current\":" + String(mockCurrent, 2) + ",\"led\":" + (ledState ? "true" : "false") + "}";
  ws.textAll(json);
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len)
{
  AwsFrameInfo *info = (AwsFrameInfo *)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)
  {
    String msg = String((char *)data, len);
    if (msg == "toggleLED")
    {
      ledState = !ledState;
      digitalWrite(2, ledState ? HIGH : LOW);
      notifyClients();
    }
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
             AwsEventType type, void *arg, uint8_t *data, size_t len)
{
  if (type == WS_EVT_CONNECT)
  {
    notifyClients();
  }
  else if (type == WS_EVT_DATA)
  {
    handleWebSocketMessage(arg, data, len);
  }
}

// OTA 韌體上傳處理器
void handleFirmwareUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
{
  if (!index)
  {
    Serial.printf("[HTML] OTA Start: %s\n", filename.c_str());
    NimBLEDevice::deinit(false); // 關閉藍牙，避免 Update 失敗
    if (!Update.begin(UPDATE_SIZE_UNKNOWN))
    {
      Update.printError(Serial);
    }
  }
  if (Update.write(data, len) != len)
  {
    Update.printError(Serial);
  }
  if (final)
  {
    if (Update.end(true))
    {
      Serial.printf("[HTML] OTA Success: %u bytes\n", index + len);
    }
    else
    {
      Update.printError(Serial);
    }
    request->send(200, "text/plain", Update.hasError() ? "FAIL" : "OK, Rebooting...");
    delay(1000);
    ESP.restart();
  }
}

// 列出 SPIFFS 所有檔案
void handleListFiles(AsyncWebServerRequest *request)
{
  String json = "[";
  File root = SPIFFS.open("/");
  File file = root.openNextFile();
  bool first = true;
  while (file)
  {
    if (!first)
      json += ",";
    json += "\"" + String(file.name()) + "\"";
    first = false;
    file = root.openNextFile();
  }
  json += "]";
  request->send(200, "application/json", json);
}

// 上傳檔案到 SPIFFS
File uploadFile2;
void handleFileUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
{
  if (!index)
  {
    if (!filename.startsWith("/"))
      filename = "/" + filename;
    if (SPIFFS.exists(filename))
      SPIFFS.remove(filename);
    uploadFile2 = SPIFFS.open(filename, "w");
  }
  if (uploadFile2)
    uploadFile2.write(data, len);
  if (final)
    uploadFile2.close();
}

// 刪除 SPIFFS 檔案
void handleDelete(AsyncWebServerRequest *request)
{
  if (!request->hasParam("file", true))
  {
    request->send(400, "text/plain", "Missing file parameter");
    return;
  }
  String filename = request->getParam("file", true)->value();
  if (!filename.startsWith("/"))
    filename = "/" + filename;
  if (SPIFFS.exists(filename))
  {
    SPIFFS.remove(filename);
    request->send(200, "text/plain", "Deleted");
  }
  else
  {
    request->send(404, "text/plain", "File not found");
  }
}
// 傳回 SPIFFS 使用空間資訊
void handleSPIFFSUsage(AsyncWebServerRequest *request)
{
  size_t total = SPIFFS.totalBytes();
  size_t used = SPIFFS.usedBytes();
  String json = "{\"total\":" + String(total) + ",\"used\":" + String(used) + "}";
  request->send(200, "application/json", json);
}

// 系統初始化函式 (使用者指定名稱與設定)
void html_test_init()
{
  if (!SPIFFS.begin(true))
  {
    Serial.println("[HTML] SPIFFS mount failed");
    return;
  }

  xTaskCreate(ota_ca_task, "ota_task", 8192, NULL, 1, &otaTaskHandle);

  ws.onEvent(onEvent);
  server.addHandler(&ws);
  server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");

  server.on("/upload", HTTP_POST, [](AsyncWebServerRequest *request)
            { request->send(200, "text/plain", "Upload complete"); }, handleFirmwareUpload);

  server.on("/uploadFile", HTTP_POST, [](AsyncWebServerRequest *request)
            { request->send(200, "text/plain", "Upload OK"); }, handleFileUpload);

  server.on("/delete", HTTP_POST, handleDelete);
  server.on("/files", HTTP_GET, handleListFiles);
  server.on("/spiffs_usage", HTTP_GET, handleSPIFFSUsage);

  // 新增取得 ESP 虛列號、chip_id、mac API
  server.on("/sn", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    SNInfo info = getSNInfo();
    String json = String("{\"sn\":\"") + info.sn +
      "\",\"chip_id\":\"" + info.chip_id +
      "\",\"mac\":\"" + info.mac + "\"}";
    request->send(200, "application/json", json); });

  // BLE 控制 API
  server.on("/ble_enable", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    ble_init();
    request->send(200, "text/plain", "BLE enabled"); });

  server.on("/ble_disable", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    ble_deinit();
    request->send(200, "text/plain", "BLE disabled"); });

  setupCommandMap();
  server.on(
      "/command",
      HTTP_POST,
      [](AsyncWebServerRequest *request) {}, // onRequest 可以留空
      NULL,                                   // 不處理檔案上傳
      handleCommand                           // body handler
  );

  // 初始化 server 路由
  server.on("/secure_ota", HTTP_GET, [](AsyncWebServerRequest *request)
            {
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
    xTaskNotifyGive(otaTaskHandle); });

  server.begin();
  ticker.attach(1, notifyClients);
  Serial.println("[HTML] Web server started on port 8080");
}
