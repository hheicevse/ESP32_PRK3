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
File uploadFile; // 全域變數 for upload

// ---------- 網頁首頁 ----------
void handleRoot() {
  String html = "<html><head><meta charset='utf-8'><title>SPIFFS File Manager</title></head><body>";
  html += "<h1>SPIFFS 檔案總管</h1>";

  // 刪除表單
  html += "<form method='POST' action='/delete'>";
  html += "<ul>";

  File root = SPIFFS.open("/");
  File file = root.openNextFile();
  while (file) {
    String filename = file.name();
    html += "<li>";
    html += "<input type='checkbox' name='delete' value='" + filename + "'>";
    html += "<a href='/download?file=" + filename + "'>" + filename + "</a>";
    html += "</li>";
    file = root.openNextFile();
  }

  html += "</ul>";
  html += "<input type='submit' value='刪除選取檔案'>";
  html += "</form>";
  html += "<hr>";

  // 上傳表單
  html += R"rawliteral(
    <h2>上傳檔案</h2>
    <form method="POST" action="/upload" enctype="multipart/form-data">
      <input type="file" name="file">
      <input type="submit" value="上傳">
    </form>
  )rawliteral";

  html += "<hr><a href='/update'>OTA 韌體更新</a>";
  html += "</body></html>";

  ota_web.send(200, "text/html", html);
}

// ---------- 檔案下載 ----------
void handleDownload() {
  if (!ota_web.hasArg("file")) {
    ota_web.send(400, "text/plain", "Missing file parameter");
    return;
  }

  String filename = ota_web.arg("file");
  // ✅ 如果沒有 / 開頭，自動補上
  if (!filename.startsWith("/")) {
    filename = "/" + filename;
  }
  if (!SPIFFS.exists(filename)) {
    ota_web.send(404, "text/plain", "File not found");
    return;
  }

  File file = SPIFFS.open(filename, "r");
  if (!file) {
    ota_web.send(500, "text/plain", "Failed to open file");
    return;
  }

  String pureName = filename;
  if (pureName.startsWith("/")) pureName = pureName.substring(1);
  ota_web.sendHeader("Content-Disposition", "attachment; filename=\"" + pureName + "\"");
  ota_web.streamFile(file, "application/octet-stream");
  file.close();
}

// ---------- 檔案上傳 ----------
void handleFileUpload() {
  HTTPUpload& upload = ota_web.upload();

  if (upload.status == UPLOAD_FILE_START) {
    String path = "/" + upload.filename;
    if (SPIFFS.exists(path)) SPIFFS.remove(path);
    uploadFile = SPIFFS.open(path, FILE_WRITE);
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (uploadFile) uploadFile.write(upload.buf, upload.currentSize);
  } else if (upload.status == UPLOAD_FILE_END) {
    if (uploadFile) uploadFile.close();
  }
}
void handleUploadDone() {
  ota_web.sendHeader("Location", "/");
  ota_web.send(303); // 303 See Other -> 回首頁
}

// ---------- 檔案刪除 ----------
void handleDelete() {
  if (!ota_web.hasArg("delete")) {
    ota_web.send(400, "text/plain", "No files selected for deletion");
    return;
  }

  // 支援單一或多選
  int count = ota_web.arg("delete").indexOf(',') >= 0 ? ota_web.args() : 1;

  for (int i = 0; i < ota_web.args(); i++) {
    if (ota_web.argName(i) == "delete") {
      String filename = ota_web.arg(i);
      // ✅ 如果沒有 / 開頭，自動補上
      if (!filename.startsWith("/")) {
        filename = "/" + filename;
      }
      if (SPIFFS.exists(filename)) {
        SPIFFS.remove(filename);
        Serial.println("Deleted: " + filename);
      }
    }
  }

  ota_web.sendHeader("Location", "/");
  ota_web.send(303); // Redirect
}

// ---------- OTA ----------
void handleUpdateUpload() {
  HTTPUpload& upload = ota_web.upload();
  if (upload.status == UPLOAD_FILE_START) {
    NimBLEDevice::deinit(false);
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

// ---------- 啟動 ----------
void ota_web_Task(void *pv) {
  while (1) {
    ota_web.handleClient();
    vTaskDelay(pdMS_TO_TICKS(10));
  }
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

  ota_web.on("/", handleRoot);
  ota_web.on("/download", HTTP_GET, handleDownload);
  ota_web.on("/upload", HTTP_POST, handleUploadDone, handleFileUpload);
  ota_web.on("/delete", HTTP_POST, handleDelete);
  ota_web.on("/update", HTTP_GET, handleUpdatePage);
  ota_web.on("/update", HTTP_POST, handleUpdateDone, handleUpdateUpload);

  ota_web.begin();
  Serial.println("ota_web started");

  xTaskCreatePinnedToCore(ota_web_Task, "ota_web_Task", 4096, NULL, 1, NULL, 1);
}
