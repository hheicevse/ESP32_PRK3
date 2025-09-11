#pragma once
#include <Arduino.h>

struct SNInfo
{
  String sn;
  String chip_id;
  String mac;
};

inline SNInfo getSNInfo()
{
  uint8_t mac[6];
  // 讀取 Wi-Fi STA 用的 MAC，順序會跟外部看到一致
  esp_read_mac(mac, ESP_MAC_WIFI_STA);

  char macStr[18];
  sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X",
          mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  char chipIdStr[13];
  sprintf(chipIdStr, "%02X%02X%02X%02X%02X%02X",
          mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  // 取最後三個 byte 當序號
  char snStr[7];
  sprintf(snStr, "%02X%02X%02X", mac[3], mac[4], mac[5]);

  SNInfo info;
  info.sn = "PRK-" + String(snStr);
  info.chip_id = chipIdStr;
  info.mac = String(macStr); // 也可以用上面有冒號的 macStr
  return info;
}
