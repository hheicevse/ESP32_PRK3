// ESP32 OTA Web Updater

//輸入此網址更新  http://192.168.3.175/update

void ota_web_init(void)
{
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  ota_web.on("/", handleRoot);
  ota_web.on("/update", HTTP_GET, handleUpdatePage);
  ota_web.on("/update", HTTP_POST, handleUpdateDone, handleUpdateUpload);

  ota_web.begin();
  Serial.println("ota_web started");
}

void ota_web_Task(void *pv) {
  while (1) {
    ota_web.handleClient();
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void handleRoot() {
  ota_web.send(200, "text/plain", "ESP32 OTA Web Updater\nVisit /update to upload firmware.");
}

void handleUpdateUpload() {
  HTTPUpload& upload = ota_web.upload();
  if (upload.status == UPLOAD_FILE_START) {
      NimBLEDevice::deinit(false);//強制關掉藍芽,因為有藍芽會失敗
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
