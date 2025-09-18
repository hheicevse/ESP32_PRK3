#include <Arduino.h>
#include "driver/uart.h"
#include <main.h>


void debug_read(const char * rx)
{
    // ✅ 把 rx buffer 轉成 String
    String input = String((char*)uart0.rx);
    input.trim();  // 去掉前後空白和換行符

    // ✅ 拆欄位
    int firstComma = input.indexOf(',');
    int secondComma = input.indexOf(',', firstComma + 1);
    int thirdComma = input.indexOf(',', secondComma + 1);  // optional


    String cmd = input.substring(0, firstComma);
    String cmd1 = input.substring(firstComma + 1, secondComma);  // 第2欄 IP
    String cmd2 = input.substring(secondComma + 1,thirdComma);   // 第3欄

    // uart傳送  ota_ca,https://192.168.3.152:443/firmware.bin
    // PC端要執行 ota_server_ca.py
    if (cmd == "ota_ca" && firstComma != -1) {
        Serial.println("[RX0] ota_ca start with URL: " + cmd1);
        ota_http_CA_func(cmd1.c_str());  // 把 URL 當參數傳給 ota_http_CA_func
    }

    // uart傳送  ota,http://192.168.3.152:8000/firmware.bin
    // 然後 PC 端要打 python -m http.server 8000
    else if (cmd == "ota" && firstComma != -1) {
        Serial.println("[RX0] ota start with URL: " + cmd1);
        ota_http_func(cmd1.c_str(),0);  // 把 URL 當參數傳給 ota_http_func
    }

    // uart傳送 SB_V2,http://192.168.3.152:8000/firmware.bin,http://192.168.3.152:8000/firmware.sig
    // 然後 PC 端要打 python -m http.server 8000
    else if (cmd == "SB_V2" && secondComma != -1) {
        Serial.println("[RX0] uart simulation_Secure_Boot V2 start");
        Serial.println("[RX0] firmware URL: " + cmd1);
        Serial.println("[RX0] signature URL: " + cmd2);
        simulation_Secure_Boot_func(cmd1.c_str(), cmd2.c_str());
    }

    // uart傳送  bsl_mspm0,http://192.168.3.153:8000/app2.txt
    // 然後 PC 端要打 python -m http.server 8000
    else if (cmd == "bsl_mspm0" && firstComma != -1) {
        if (mspm0Comm.bsl_triggered == true){
            Serial.println("[RX0] bsl_mspm0 is Updating ");
        }
        else{
            Serial.println("[RX0] bsl_mspm0 start with URL: " + cmd1);
            mspm0Comm.bsl_url = cmd1;
            mspm0Comm.bsl_triggered = true;
        }

    }

    // uart傳送  tcp_connect,192.168.3.153,502
    else if (cmd == "tcp_connect" && secondComma != -1) {
        tcp_connect(cmd1.c_str(), cmd2.toInt());
    }
    // uart傳送  tcp_disconnect
    else if (cmd == "tcp_disconnect") {
        tcp_disconnect();
    }
    // uart傳送  tcp_sent
    else if (cmd == "tcp_sent") {
    const uint8_t tx[] = {0x01, 0x03, 0x35, 0x00, 0x00, 0x01};  // 你自定封包
    tcp_send_data(tx, sizeof(tx));
    }
    // uart傳送  tcp_read
    else if (cmd == "tcp_read") {
    // 改用 non-blocking tcp_available() 檢查
        int len = tcp_available();
        if (len > 0) {
            uint8_t buf[128];
            int r = tcp_receive_data(buf, len);
        }
    }
    // uart傳送 wifi_connect
    else if (cmd == "wifi_connect") {
        String ssid, pwd;
        loadWiFiCredentials(ssid, pwd);
        if (WiFi.status() != WL_CONNECTED) {
            WiFi.begin(ssid.c_str(), pwd.c_str());
            Serial.printf("[RX0] wifi_connect \n");
        }
        else {
            Serial.printf("[RX0] wifi already connected \n");
        }
    }
    // uart傳送 wifi_disconnect
    else if (cmd == "wifi_disconnect") {
        Serial.printf("[RX0] wifi_disconnect \n");
        WiFi.disconnect();
    }
    // uart傳送 wifi_status
    else if (cmd == "wifi_status") {
        Serial.printf("[RX0] wifi %s \n",(WiFi.status() == WL_CONNECTED) ? "connected" : "disconnected");
    }
    // uart傳送 set_ssid,TP-Link_2.4g_CCBD,63504149
    else if (cmd == "set_ssid") {
        saveWiFiCredentials(cmd1, cmd2);
        Serial.println("[RX0] set_ssid_ok," + cmd1 + "," + cmd2);
    }
    // uart傳送 get_ssid
    else if (cmd == "get_ssid") {
        String ssid, pwd;
        loadWiFiCredentials(ssid, pwd);
        Serial.println("[RX0] get_ssid," + ssid + "," + pwd);
    }
    // uart傳送 clear_ssid
    else if (cmd == "clear_ssid") {
        Serial.printf("[RX0] clear_ssid \n");
        clearWiFiCredentials();
    }


    else {
        Serial.println("[RX0] Unknown or malformed command: " + input);
    }
}