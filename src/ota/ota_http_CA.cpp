/**
   httpUpdate.ino

    Created on: 27.11.2015

*/

#include <Arduino.h>

#include <WiFi.h>
#include <WiFiMulti.h>

#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <WiFiClientSecure.h>
#include <NimBLEDevice.h>

#include <Update.h>
#include <FS.h>
// #include <LittleFS.h>
#include <SPIFFS.h>  // 改成 SPIFFS
#include <ota/ota_http_CA.h>
String caCertStr;
WiFiClientSecure client;
// const char *rootCACertificate = "-----BEGIN CERTIFICATE-----\n"
// "MIIEBTCCAu2gAwIBAgIUBSxn2DDuaGXvANZBA68vIk0v4vgwDQYJKoZIhvcNAQEL\n"
// "BQAwgZExCzAJBgNVBAYTAlRXMQ8wDQYDVQQIDAZUYWlwZWkxDzANBgNVBAcMBlRh\n"
// "aXBlaTESMBAGA1UECgwJTXlJT1QgSW5jMQwwCgYDVQQLDANEZXYxFDASBgNVBAMM\n"
// "C215aW90LmxvY2FsMSgwJgYJKoZIhvcNAQkBFhlhMTI2MDE3NzM5MDFAeWFob28u\n"
// "Y29tLnR3MB4XDTI1MDQyNDA1MzM0MloXDTM1MDQyMjA1MzM0MlowgZExCzAJBgNV\n"
// "BAYTAlRXMQ8wDQYDVQQIDAZUYWlwZWkxDzANBgNVBAcMBlRhaXBlaTESMBAGA1UE\n"
// "CgwJTXlJT1QgSW5jMQwwCgYDVQQLDANEZXYxFDASBgNVBAMMC215aW90LmxvY2Fs\n"
// "MSgwJgYJKoZIhvcNAQkBFhlhMTI2MDE3NzM5MDFAeWFob28uY29tLnR3MIIBIjAN\n"
// "BgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA+bOO3Vb38M8AFoCZH8tys8s9f2mH\n"
// "nYs6Ff2k+zbBNloATZQrz+op2Dnlks0D8LX6vKsRh21Zu2pgM18KjiD3zQ3jCtdp\n"
// "Da7LS4JdLHkAJyK8tHYGz7I5+fmdhHYkHIC1/Q43bukHCpEyFblW9Rnn2VOu947x\n"
// "+PZuLO8jqyor9GS8v95Na2A469b6m1NRa9KbD8ck/mfsGNvBhayZ0CynrCE2ZVwg\n"
// "5SYLZpZX9JDign34V6plodAUHLiQuNmybvOgaIw12Ikm+rBK87byEigxIlaSjZZ2\n"
// "+MZDRJqSLO0xCSMBgt3EqKINyBTOZUyVtzBd74js+DS1DEUyMkWBChgvVQIDAQAB\n"
// "o1MwUTAdBgNVHQ4EFgQUNrmdR6HrWUp/mbED7zwqCgTJGmwwHwYDVR0jBBgwFoAU\n"
// "NrmdR6HrWUp/mbED7zwqCgTJGmwwDwYDVR0TAQH/BAUwAwEB/zANBgkqhkiG9w0B\n"
// "AQsFAAOCAQEAf9WIeA2xJeuNNOaiPQI3LouRNiQpK5HtH0wnbOsSfAacVaWZuD2g\n"
// "SzOHU7btMLRSYa6w/yoEVuDsfuXlSD4pm+//iflYa0K6gL8FunWoEpvgbX2mxhsK\n"
// "/QmAZsKabxelFUp4oiEhKnHSLlgGKRSZTEk0bjDJEGMceYA6+q4J7JA5StTWOExZ\n"
// "dkqaZhkOhinNN9EWwkSYJENLEltfUEoETdTLeUadJnI3VhjpQ1LnZ12YclPli9CL\n"
// "8T2joK0/593VlZOX1HqMP87diJC29UgKHRbSmXYswl0aqok08aKIymiDGT3qemNv\n"
// "FqXKz20e9Tgwqfg9w420dGaht3KxLOPZsg==\n"
// "-----END CERTIFICATE-----\n";

// const char* rootCACertificate = R"EOF(
// -----BEGIN CERTIFICATE-----
// MIIEBTCCAu2gAwIBAgIUBSxn2DDuaGXvANZBA68vIk0v4vgwDQYJKoZIhvcNAQEL
// BQAwgZExCzAJBgNVBAYTAlRXMQ8wDQYDVQQIDAZUYWlwZWkxDzANBgNVBAcMBlRh
// aXBlaTESMBAGA1UECgwJTXlJT1QgSW5jMQwwCgYDVQQLDANEZXYxFDASBgNVBAMM
// C215aW90LmxvY2FsMSgwJgYJKoZIhvcNAQkBFhlhMTI2MDE3NzM5MDFAeWFob28u
// Y29tLnR3MB4XDTI1MDQyNDA1MzM0MloXDTM1MDQyMjA1MzM0MlowgZExCzAJBgNV
// BAYTAlRXMQ8wDQYDVQQIDAZUYWlwZWkxDzANBgNVBAcMBlRhaXBlaTESMBAGA1UE
// CgwJTXlJT1QgSW5jMQwwCgYDVQQLDANEZXYxFDASBgNVBAMMC215aW90LmxvY2Fs
// MSgwJgYJKoZIhvcNAQkBFhlhMTI2MDE3NzM5MDFAeWFob28uY29tLnR3MIIBIjAN
// BgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA+bOO3Vb38M8AFoCZH8tys8s9f2mH
// nYs6Ff2k+zbBNloATZQrz+op2Dnlks0D8LX6vKsRh21Zu2pgM18KjiD3zQ3jCtdp
// Da7LS4JdLHkAJyK8tHYGz7I5+fmdhHYkHIC1/Q43bukHCpEyFblW9Rnn2VOu947x
// +PZuLO8jqyor9GS8v95Na2A469b6m1NRa9KbD8ck/mfsGNvBhayZ0CynrCE2ZVwg
// 5SYLZpZX9JDign34V6plodAUHLiQuNmybvOgaIw12Ikm+rBK87byEigxIlaSjZZ2
// +MZDRJqSLO0xCSMBgt3EqKINyBTOZUyVtzBd74js+DS1DEUyMkWBChgvVQIDAQAB
// o1MwUTAdBgNVHQ4EFgQUNrmdR6HrWUp/mbED7zwqCgTJGmwwHwYDVR0jBBgwFoAU
// NrmdR6HrWUp/mbED7zwqCgTJGmwwDwYDVR0TAQH/BAUwAwEB/zANBgkqhkiG9w0B
// AQsFAAOCAQEAf9WIeA2xJeuNNOaiPQI3LouRNiQpK5HtH0wnbOsSfAacVaWZuD2g
// SzOHU7btMLRSYa6w/yoEVuDsfuXlSD4pm+//iflYa0K6gL8FunWoEpvgbX2mxhsK
// /QmAZsKabxelFUp4oiEhKnHSLlgGKRSZTEk0bjDJEGMceYA6+q4J7JA5StTWOExZ
// dkqaZhkOhinNN9EWwkSYJENLEltfUEoETdTLeUadJnI3VhjpQ1LnZ12YclPli9CL
// 8T2joK0/593VlZOX1HqMP87diJC29UgKHRbSmXYswl0aqok08aKIymiDGT3qemNv
// FqXKz20e9Tgwqfg9w420dGaht3KxLOPZsg==
// -----END CERTIFICATE-----
// )EOF";


void ota_http_ca_init(){
    // 初始化檔案系統
  // if (!LittleFS.begin()) {
  if (!SPIFFS.begin()) {
    Serial.println("[OTA CA]LittleFS init failed!");
    return;
  }

  // 讀取 CA 憑證
  // File caFile = LittleFS.open("/myCA.pem", "r");
  File caFile = SPIFFS.open("/myCA.pem", "r");  // 改成 SPIFFS
  if (!caFile) {
    Serial.println("[OTA CA]Failed to open CA cert file");
    return;
  }

  caCertStr = "";
  while (caFile.available()) {
    caCertStr += (char)caFile.read();
  }
  caFile.close();

  // client.setCACert(rootCACertificate);
  client.setCACert(caCertStr.c_str());
  // client.setInsecure();  // 暫時跳過憑證檢查

  httpUpdate.rebootOnUpdate(false);//關閉自動重啟

}


void update_started() {
  Serial.println("[OTA CA]CALLBACK:  HTTP update process started");
}

void update_finished() {
  Serial.println("[OTA CA]CALLBACK:  HTTP update process finished");
}

void update_progress(int cur, int total) {
  Serial.printf("[OTA CA]CALLBACK:  HTTP update process at %d of %d bytes...\n", cur, total);
}

void update_error(int err) {
  Serial.printf("[OTA CA]CALLBACK:  HTTP update fatal error code %d\n", err);
}

bool ota_http_CA_func(const char* url) {
  bool is_ok = false;
  if ((WiFi.status() == WL_CONNECTED)) {
    // if (!client.connect("192.168.3.153", 443)) {
    //   Serial.println(" Cannot connect to OTA  CA server.");
  
    // }
    // else{
      httpUpdate.onStart(update_started);
      httpUpdate.onEnd(update_finished);
      httpUpdate.onProgress(update_progress);
      httpUpdate.onError(update_error);
      // Serial.println(" connect to OTA  CA server.");
      NimBLEDevice::deinit(false);//強制關掉藍芽,因為有藍芽會失敗
      
      // t_httpUpdate_return ret = httpUpdate.update(client, "https://192.168.3.153:443/firmware.bin");
      t_httpUpdate_return ret = httpUpdate.update(client, url);

      switch (ret) {
      case HTTP_UPDATE_FAILED:
        Serial.printf("[OTA CA]HTTP_UPDATE_FAILED Error (%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
        break;
      case HTTP_UPDATE_NO_UPDATES:
        Serial.println("[OTA CA]HTTP_UPDATE_NO_UPDATES");
        break;
      case HTTP_UPDATE_OK:
        Serial.println("[OTA CA]HTTP_UPDATE_OK");
        is_ok = true;
        vTaskDelay(pdMS_TO_TICKS(1000));
        ESP.restart();
        break;
      }
    // }
    
  }

  return is_ok;

}
