#include <Arduino.h>


const char* ssid = "TP-Link_2.4g_CCBD";
const char* password = "63504149";

void wifi_init(void)
{
  
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();//斷線（初始化的意思）

  // wifi_sacn();

  WiFi.onEvent(ConnectedToAP_Handler, ARDUINO_EVENT_WIFI_STA_CONNECTED);
  WiFi.onEvent(DisConnectedToAP_Handler, ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
  
  WiFi.onEvent(GotIP_Handler, ARDUINO_EVENT_WIFI_STA_GOT_IP);
  WiFi.begin(ssid, password);
  Serial.println("\nConnecting to WiFi Network ..");
}

void ConnectedToAP_Handler(WiFiEvent_t wifi_event, WiFiEventInfo_t wifi_info) {
  Serial.println("Connected To The WiFi Network");
}

void DisConnectedToAP_Handler(WiFiEvent_t wifi_event, WiFiEventInfo_t wifi_info) {
  Serial.println("DisConnected To The WiFi Network");
} 


void GotIP_Handler(WiFiEvent_t wifi_event, WiFiEventInfo_t wifi_info) {
  Serial.print("Local ESP32 IP: ");
  Serial.println(WiFi.localIP());
}

// void wifi_sacn(void)
// {
//   Serial.println("scan start");
//   int n = WiFi.scanNetworks();//掃描網路，並將掃描到的網路數量存入n
//   Serial.println("scan done");
//   if (n == 0) {
//       Serial.println("no networks found");
//   } else {
//       Serial.print(n);
//       Serial.println(" networks found");
//       for (int i = 0; i < n; ++i) {
//           //顯示無線網路SSID, RSSI, 加密
//           Serial.print(i + 1);
//           Serial.print(": ");
//           Serial.print(WiFi.SSID(i));
//           Serial.print(" (");
//           Serial.print(WiFi.RSSI(i));
//           Serial.print(")");
//           Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN)?" ":"*");
//           delay(10);
//       }
//   }
//   Serial.println("");
// }
