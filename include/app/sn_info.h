#pragma once
#include <Arduino.h>

struct SNInfo {
  String sn;
  String chip_id;
  String mac;
};

inline SNInfo getSNInfo() {
  uint64_t mac = ESP.getEfuseMac();
  char macStr[18];
  char chipIdStr[13];
  sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X",
    (uint8_t)(mac >> 40), (uint8_t)(mac >> 32), (uint8_t)(mac >> 24),
    (uint8_t)(mac >> 16), (uint8_t)(mac >> 8), (uint8_t)mac);
  sprintf(chipIdStr, "%02X%02X%02X%02X%02X%02X",
    (uint8_t)(mac >> 40), (uint8_t)(mac >> 32), (uint8_t)(mac >> 24),
    (uint8_t)(mac >> 16), (uint8_t)(mac >> 8), (uint8_t)mac);
  SNInfo info;
  info.sn = "ESP-" + String(chipIdStr).substring(6, 12);
  info.chip_id = chipIdStr;
  info.mac = macStr;
  return info;
}
