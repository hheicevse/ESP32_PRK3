#include <Arduino.h>
#include <Update.h>
#include <WiFiClientSecure.h>
#include "esp_ota_ops.h"
#include "mbedtls/md.h"
#include "mbedtls/rsa.h"
#include "mbedtls/pk.h"
#include "mbedtls/base64.h"
#include <vector>
#include <HTTPClient.h>
#include <NimBLEDevice.h>

const char* PUBLIC_KEY_PEM = R"(-----BEGIN PUBLIC KEY-----
MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAzCvoCusGHoz7wFAbXfFb
lbJe2BecAs914Jk2DRS1Yntn4HYCglliNAh3uV7CbalMs6jSEiOVTbd1FnXiakOV
ynzaraLESzBlJuCIYiXuh+vqxZA4hRtODHBEuD1MOkC/z1nVeBj2MQwZgpocoiiX
VWsdoEXoGKB6wuxj3i33c55kM7ojOIc21sNYZ3RFkvE5iOcFFYYsUqaNE5yZcxik
9/XaMoz10nezM757zJxUYdE7CLs3WpuFFocZmoIhIy1Ug53wVR9eO8epxeLHzqxZ
wkrT6R5o+G2keZNR98iDmvl/roYM+8VYvQ3KfAGfsduU+jaMEtxHODgBTEIhFlUw
OQIDAQAB
-----END PUBLIC KEY-----)";

// bool verifySignature(uint8_t* firmware_bin, size_t firmware_len,
//                      uint8_t* signature, size_t sig_len) {
//   mbedtls_pk_context pk;
//   mbedtls_pk_init(&pk);

//   int ret = mbedtls_pk_parse_public_key(&pk, (const uint8_t*)PUBLIC_KEY_PEM, strlen(PUBLIC_KEY_PEM) + 1);
//   if (ret != 0) return false;

//   // Hash firmware
//   uint8_t sha256[32];
//   mbedtls_md(mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), firmware_bin, firmware_len, sha256);

//   // 驗證 RSA SHA256 簽章
//   ret = mbedtls_pk_verify(&pk, MBEDTLS_MD_SHA256, sha256, sizeof(sha256), signature, sig_len);

//   mbedtls_pk_free(&pk);
//   return ret == 0;
// }
bool verifySignature(uint8_t* hash, uint8_t* signature, size_t sig_len) {
  mbedtls_pk_context pk;
  mbedtls_pk_init(&pk);

  if (mbedtls_pk_parse_public_key(&pk, (const uint8_t*)PUBLIC_KEY_PEM, strlen(PUBLIC_KEY_PEM) + 1) != 0) {
    mbedtls_pk_free(&pk);
    return false;
  }

  int ret = mbedtls_pk_verify(&pk, MBEDTLS_MD_SHA256, hash, 32, signature, sig_len);
  mbedtls_pk_free(&pk);
  return ret == 0;
}
bool streamFirmwareAndHash(WiFiClient& stream, size_t totalLength, uint8_t* out_sha256) {
  const size_t bufSize = 1024;
  uint8_t buffer[bufSize];
  size_t total_read = 0;

  // init SHA256 context
  mbedtls_md_context_t ctx;
  const mbedtls_md_info_t* md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
  mbedtls_md_init(&ctx);
  mbedtls_md_setup(&ctx, md_info, 0);
  mbedtls_md_starts(&ctx);

  if (!Update.begin(totalLength)) {
    Serial.println("[SIG] Not enough space for OTA");
    return false;
  }

  while (total_read < totalLength) {
    size_t len = stream.readBytes(buffer, std::min(bufSize, totalLength - total_read));
    if (len <= 0) continue;

    Update.write(buffer, len);                // OTA 寫入
    mbedtls_md_update(&ctx, buffer, len);     // SHA256 累積
    total_read += len;
  }

  mbedtls_md_finish(&ctx, out_sha256);        // 完成 SHA256
  mbedtls_md_free(&ctx);

  if (!Update.end()) {
    Serial.printf("OTA failed: %s\n", Update.errorString());
    return false;
  }

  return Update.isFinished();
}
bool simulation_Secure_Boot_func(const char* bin_url, const char* sig_url) {

  bool is_ok = false;
  HTTPClient my_http;

  my_http.begin(sig_url);
  int sigCode = my_http.GET();

  std::vector<uint8_t> sig_data;

  if (sigCode == HTTP_CODE_OK) {
    NimBLEDevice::deinit(false);
    WiFiClient& sig_stream = my_http.getStream();
    int sig_length = my_http.getSize();
    sig_data.reserve(sig_length);
    Serial.printf("sig_length: %d bytes\n", sig_length);

    size_t total_read = 0;
    while (total_read < sig_length) {
      if (sig_stream.available()) {
        int c = sig_stream.read();
        if (c >= 0) {
          sig_data.push_back((uint8_t)c);
          total_read++;
        }
      } else {
        delay(1); // 等待資料到達
      }
    }

    Serial.printf("Signature downloaded: %d bytes\n", sig_data.size());
  } else {
    Serial.printf("Failed to download signature. HTTP error: %s\n", my_http.errorToString(sigCode).c_str());
    my_http.end();
    return 0;
  }
  my_http.end();






// Step 2: 開始 OTA 寫入與 SHA256 hash
my_http.begin(bin_url);
int httpCode = my_http.GET();

if (httpCode == HTTP_CODE_OK) {
  WiFiClient& bin_stream = my_http.getStream();
  size_t bin_len = my_http.getSize();
  uint8_t firmware_hash[32];

  if (streamFirmwareAndHash(bin_stream, bin_len, firmware_hash)) {
    if (verifySignature(firmware_hash, sig_data.data(), sig_data.size())) {
      Serial.println("[SIG] Signature OK! Rebooting...");
      ESP.restart();
    } else {
      Serial.println("[SIG] Signature verification failed");
    }
  }
}
my_http.end();

  return is_ok;
}