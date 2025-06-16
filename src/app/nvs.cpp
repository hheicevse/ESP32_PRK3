#include <Arduino.h>
#include <Preferences.h>


#include <main.h>
Preferences prefs;

void saveWiFiCredentials(const String& ssid, const String& password) {
  prefs.begin("wifi", false);  // 命名空間叫 wifi
  prefs.putString("ssid", ssid);
  prefs.putString("password", password);
  prefs.end();
}

void loadWiFiCredentials(String& ssid, String& password) {
  prefs.begin("wifi", true);  // read-only 模式
  ssid = prefs.getString("ssid", "");
  password = prefs.getString("password", "");
  prefs.end();
}

void clearWiFiCredentials() {
  prefs.begin("wifi", false);  // false = read-write 模式
  prefs.clear();               // 清除該命名空間下所有 key
  // prefs.remove("ssid");//只想刪掉某個 key
  // prefs.remove("password");//只想刪掉某個 key
  prefs.end();
}