#ifndef _WIFI_CTRL_H_
#define _WIFI_CTRL_H_
#pragma once

#include <WiFi.h>

void wifi_init(void);
void ConnectedToAP_Handler(WiFiEvent_t wifi_event, WiFiEventInfo_t wifi_info);
void DisConnectedToAP_Handler(WiFiEvent_t wifi_event, WiFiEventInfo_t wifi_info);
void GotIP_Handler(WiFiEvent_t wifi_event, WiFiEventInfo_t wifi_info);
// void wifi_sacn(void);


#endif /* _WIFI_CTRL_H_ */