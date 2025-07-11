// ESP32 OTA Web Updater

//輸入此網址更新  http://192.168.3.175/update



// ESP32 OTA Web Updater + SPIFFS file browser & download

#include <Arduino.h>
#include <ota/ota_web.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Update.h>
#include <NimBLEDevice.h>
#include <SPIFFS.h>

WebServer ota_web(80);
// void handleRoot();
// void handleDownload();
// void handleUpdatePage();
// void handleUpdateDone();
// void handleUpdateUpload();


void ota_web_Task(void *pv) {
  while (1) {
    ota_web.handleClient();
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void handleRoot() {
  String html = "<html><head><title>SPIFFS File List</title></head><body>";
  html += "<h1>SPIFFS file list</h1><ul>";

  File root = SPIFFS.open("/");
  if (!root) {
    ota_web.send(500, "text/plain", "Failed to open SPIFFS root");
    return;
  }
  if (!root.isDirectory()) {
    ota_web.send(500, "text/plain", "SPIFFS root is not a directory");
    return;
  }

  File file = root.openNextFile();
  while (file) {
    String filename = file.name();
    html += "<li><a href=\"/download?file=" + filename + "\">" + filename + "</a></li>";
    file = root.openNextFile();
  }
  html += "</ul><hr>";
  html += "<a href='/update'>OTA Firmware Update</a>";
  html += "</body></html>";

  ota_web.send(200, "text/html", html);
}

void handleDownload() {
  if (!ota_web.hasArg("file")) {
    ota_web.send(400, "text/plain", "Missing file parameter");
    return;
  }

  String filename = "/"+ota_web.arg("file");

  if (!SPIFFS.exists(filename)) {
    ota_web.send(404, "text/plain", "File not found");
    return;
  }

  File file = SPIFFS.open(filename, "r");
  if (!file) {
    ota_web.send(500, "text/plain", "Failed to open file");
    return;
  }

  // 取得純檔名（去掉開頭斜線）
  String pureName = filename;
  if (pureName.startsWith("/")) {
    pureName = pureName.substring(1);
  }

  // 設定 Content-Disposition，告訴瀏覽器下載檔名
  ota_web.sendHeader("Content-Disposition", "attachment; filename=\"" + pureName + "\"");
  ota_web.streamFile(file, "application/octet-stream");
  file.close();
}

// OTA Upload handler (原本的)
void handleUpdateUpload() {
  HTTPUpload& upload = ota_web.upload();
  if (upload.status == UPLOAD_FILE_START) {
    NimBLEDevice::deinit(false); // 強制關閉藍牙，避免更新失敗
    Serial.printf("Update Start: %s\n", upload.filename.c_str());
    if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (Update.end(true)) {
      Serial.printf("Update Success: %u bytes\n", upload.totalSize);
    } else {
      Update.printError(Serial);
    }
  }
}

void handleUpdatePage() {
  ota_web.send(200, "text/html", R"rawliteral(
    <h1>ESP32 OTA Web Updater</h1>
    <form method='POST' action='/update' enctype='multipart/form-data'>
      <input type='file' name='update'>
      <input type='submit' value='Update'>
    </form>
  )rawliteral");
}

void handleUpdateDone() {
  ota_web.send(200, "text/plain", Update.hasError() ? "Update Failed" : "Update Success! Rebooting...");
  delay(1000);
  ESP.restart();
}
void ota_web_init(void)
{
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS Mount Failed");
    return;
  }
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  Serial.println("\nWiFi connected, IP: " + WiFi.localIP().toString());

  ota_web.on("/", handleRoot);                     // 根目錄：列出檔案清單
  ota_web.on("/download", HTTP_GET, handleDownload); // 下載檔案
  ota_web.on("/update", HTTP_GET, handleUpdatePage);   // OTA頁面
  ota_web.on("/update", HTTP_POST, handleUpdateDone, handleUpdateUpload);  // OTA上傳

  ota_web.begin();
  Serial.println("ota_web started");

  // 建立 RTOS task 處理網頁請求
  xTaskCreatePinnedToCore(ota_web_Task, "ota_web_Task", 4096, NULL, 1, NULL, 1);
}
