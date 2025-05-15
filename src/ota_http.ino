// 這個檔案是用來做 OTA 更新的,使用 HTTP 協定來下載更新檔案
#include <Arduino.h>

#include <WiFi.h>
#include <HTTPClient.h> // Ensure this library is installed in your environment
#include <Update.h>

#include <WiFiMulti.h>



/*
使用 Python 在想要的bin檔,資料夾中啟動 HTTP 伺服器：
python -m http.server 8000
*/

// const char* firmware_url = "http://192.168.3.153:8000/firmware.bin";//先用本地的bin就好

void ota_http_init() {

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

 
}



void ota_http_func(const char* url) {
  // HTTPClient http;
  ota_http.begin(url);
  int httpCode = ota_http.GET();

  if (httpCode == HTTP_CODE_OK) {
    NimBLEDevice::deinit(false);//強制關掉藍芽,因為有藍芽會失敗
    int contentLength = ota_http.getSize();
    WiFiClient& stream = ota_http.getStream();

    if (Update.begin(contentLength)) {
      size_t written = Update.writeStream(stream);
      if (Update.end() && Update.isFinished()) {
        Serial.println("OTA success. Rebooting...");
        ESP.restart();
      } else {
        Serial.println("OTA failed. Error: " + String(Update.errorString()));
      }
    } else {
      Serial.println("Not enough space for OTA");
    }
  } else {
    Serial.printf("HTTP GET failed. Error: %s\n", ota_http.errorToString(httpCode).c_str());
  }

  ota_http.end();
}

