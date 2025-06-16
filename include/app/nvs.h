#ifndef _NVS_H_
#define _NVS_H_
#pragma once

void saveWiFiCredentials(const String& ssid, const String& password);
void loadWiFiCredentials(String& ssid, String& password);
void clearWiFiCredentials();

#endif /* _NVS_H_ */