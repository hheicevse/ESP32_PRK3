#ifndef _NVS_H_
#define _NVS_H_
#pragma once

void saveWiFiCredentials(const String& ssid, const String& password);
void loadWiFiCredentials(String& ssid, String& password);
void clearWiFiCredentials();
void saveSettingsKeyValue(const String& key, const String& value);
String loadSettingsKeyValue(const String& key);
void clearSettings();

#endif /* _NVS_H_ */