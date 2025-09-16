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

void saveSettingsKeyValue(const String& key, const String& value) {
  prefs.begin("settings", false);  // 命名空間叫 settings
  prefs.putString(key.c_str(), value);
  prefs.end();
}
String loadSettingsKeyValue(const String& key) {
  prefs.begin("settings", true);  // read-only 模式
  String value = prefs.getString(key.c_str(), "");
  prefs.end();
  return value;
}
void clearSettings() {
  prefs.begin("settings", false);  // false = read-write 模式
  prefs.clear();                   // 清除該命名空間下所有 key
  prefs.end();
}