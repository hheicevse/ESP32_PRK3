// 這個檔案是用來做 OTA 更新的,使用 HTTP 協定來下載更新檔案
#include <Arduino.h>

#include <WiFi.h>
#include <HTTPClient.h> // Ensure this library is installed in your environment
#include <Update.h>

#include <WiFiMulti.h>
#include <ota/ota_http.h>
#include <NimBLEDevice.h>
#include "esp_task_wdt.h"  // 若你有開啟 TWDT

HTTPClient ota_http;
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

#define USE_OTA_VERSION_2

#if defined(USE_OTA_VERSION_1)
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
#elif defined(USE_OTA_VERSION_2)
// watchdog-friendly
void ota_http_func(const char* url) {
  ota_http.begin(url);
  int httpCode = ota_http.GET();

  if (httpCode == HTTP_CODE_OK) {
    NimBLEDevice::deinit(false);  // 強制關掉藍芽,因為有藍芽會失敗
    int contentLength = ota_http.getSize();
    WiFiClient& stream = ota_http.getStream();

    const size_t bufferSize = 1024;
    uint8_t buffer[bufferSize];
    size_t totalWritten = 0;

    Serial.printf("OTA Start, size = %d bytes\n", contentLength);

    if (Update.begin(contentLength)) {
      while (ota_http.connected() && totalWritten < contentLength) {
        // 可用資料長度（避免卡死）
        size_t available = stream.available();
        if (available > 0) {
          size_t toRead = min(available, bufferSize);
          int readBytes = stream.readBytes(buffer, toRead);

          if (readBytes > 0) {
            size_t written = Update.write(buffer, readBytes);
            if (written != readBytes) {
              Serial.println("Update.write failed");
              Update.end();
              return;
            }
            totalWritten += written;
          }
        }

        // ✅ 餵 watchdog
        esp_task_wdt_reset();

        // 避免炸 CPU：讓其他任務有時間跑（也可避免 WDT 誤判）
        delay(1);
      }

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

#endif