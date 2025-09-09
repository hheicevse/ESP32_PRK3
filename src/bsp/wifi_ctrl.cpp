#include <Arduino.h>
#include <bsp/wifi_ctrl.h>
#include <WiFi.h>
#include <main.h>

const char* ssid = "TP-Link_2.4g_CCBD";
const char* password = "63504149";
// const char* ssid = "s23ji";
// const char* password = "0968671988";
void wifi_init(void)
{
  //STA
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();//斷線（初始化的意思）

  // wifi_sacn();

  WiFi.onEvent(ConnectedToAP_Handler, ARDUINO_EVENT_WIFI_STA_CONNECTED);
  WiFi.onEvent(DisConnectedToAP_Handler, ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
  
  WiFi.onEvent(GotIP_Handler, ARDUINO_EVENT_WIFI_STA_GOT_IP);
  WiFi.begin(ssid, password);
  Serial.println("[WIFI STA] Connecting to WiFi Network ..");


  // AP
  WiFi.softAP("ESP32_AP", "12345678");
  IPAddress ap_ip = WiFi.softAPIP();
  Serial.printf("[WIFI AP] IP: %s\n", ap_ip.toString().c_str());
  // 開 AP server (502)
  tcp_ap_init((uint32_t)ap_ip, SERVER_PORT_AP);

}

void ConnectedToAP_Handler(WiFiEvent_t wifi_event, WiFiEventInfo_t wifi_info) {
  Serial.println("[WIFI STA] Connected To The WiFi Network");
}

void DisConnectedToAP_Handler(WiFiEvent_t wifi_event, WiFiEventInfo_t wifi_info) {
  Serial.println("[WIFI STA] DisConnected To The WiFi Network");
} 


void GotIP_Handler(WiFiEvent_t wifi_event, WiFiEventInfo_t wifi_info) {
  Serial.print("[WIFI STA] Local ESP32 IP: ");
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
