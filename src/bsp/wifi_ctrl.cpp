#include <Arduino.h>
#include <bsp/wifi_ctrl.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <main.h>

// const char* ssid = "TP-Link_2.4g_CCBD";
// const char* password = "63504149";
// const char* ssid = "s23ji";
// const char* password = "0968671988";

// AP SSID 預設為 SN
String ap_sn_str = getSNInfo().sn;
const char* ap_ssid = ap_sn_str.c_str();

bool is_mdns_started = false;

void wifi_init(void)
{
  //STA
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();//斷線（初始化的意思）

  // wifi_sacn();

  WiFi.onEvent(ConnectedToAP_Handler, ARDUINO_EVENT_WIFI_STA_CONNECTED);
  WiFi.onEvent(DisConnectedToAP_Handler, ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
  
  WiFi.onEvent(GotIP_Handler, ARDUINO_EVENT_WIFI_STA_GOT_IP);
  // WiFi.begin(ssid, password);


  String ssid, pwd;
  loadWiFiCredentials(ssid, pwd);
  WiFi.begin(ssid.c_str(), pwd.c_str());

  Serial.println("[WIFI STA] Connecting to WiFi Network ..");


  // AP
  WiFi.softAP(ap_ssid, "12345678");
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

  is_mdns_started = false; // 等待重連後再重新 begin
}


void GotIP_Handler(WiFiEvent_t wifi_event, WiFiEventInfo_t wifi_info) {
  Serial.print("[WIFI STA] Local ESP32 IP: ");
  Serial.println(WiFi.localIP());

  if (!is_mdns_started) {
    if (MDNS.begin(ap_sn_str)) {
      // 延遲確保 mDNS 已啟動
      delay(100);

      Serial.println("mDNS responder started");
      is_mdns_started = true;

      // 可選：註冊服務
      MDNS.addService("http", "tcp", 8080);
    }
  }
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
