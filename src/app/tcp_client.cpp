#include <Arduino.h>
#include <main.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "esp_log.h"
#include <sys/ioctl.h>  // for FIONREAD

#define TAG         "TCP"

static int tcp_socket = -1;
int tcp_available()
{
    if (tcp_socket < 0) return 0;

    int count = 0;
    int result = ioctl(tcp_socket, FIONREAD, &count);
    if (result < 0) {
        Serial.print("ioctl failed: ");
        Serial.println(errno);
        return 0;
    }
    return count;
}
// 十六進制列印函式，類似 ESP_LOG_BUFFER_HEXDUMP
void printHexBuffer(const uint8_t *buf, size_t len) {
  Serial.print(TAG);
  Serial.print(": ");
  for (size_t i = 0; i < len; i++) {
    if (buf[i] < 0x10) Serial.print('0');
    Serial.print(buf[i], HEX);
    Serial.print(' ');
  }
  Serial.println();
}

// 初始化連線（只需呼叫一次）
bool tcp_connect(const char* url, int port)
{
    struct sockaddr_in dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(port);
    dest_addr.sin_addr.s_addr = inet_addr(url);
    tcp_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (tcp_socket < 0) {
        Serial.print(TAG);
        Serial.print(": socket() failed: ");
        Serial.println(errno);
        return false;
    }
    if (connect(tcp_socket, (struct sockaddr*)&dest_addr, sizeof(dest_addr)) != 0) {
        Serial.print(TAG);
        Serial.print(": connect() failed: ");
        Serial.println(errno);
        close(tcp_socket);
        tcp_socket = -1;
        return false;
    }
    Serial.print(TAG);
    Serial.print(": TCP connected to ");
    Serial.print(url);
    Serial.print(":");
    Serial.println(port);
    return true;
}

// 傳送資料（呼叫前請先 tcp_connect）
bool tcp_send_data(const uint8_t *data, size_t len)
{
    if (tcp_socket < 0) return false;
    int sent = send(tcp_socket, data, len, 0);
    if (sent < 0) {
        Serial.print(TAG);
        Serial.print(": send() failed: ");
        Serial.println(errno);
        return false;
    }
    Serial.print(TAG);
    Serial.print(": Sent ");
    Serial.print(sent);
    Serial.println(" bytes");
    return true;
}

// 接收資料（阻塞，最多 bufsize bytes）
int tcp_receive_data(uint8_t *buf, size_t bufsize)
{
    if (tcp_socket < 0) return -1;

    int len = recv(tcp_socket, buf, bufsize, 0);
    if (len < 0) {
        Serial.print(TAG);
        Serial.print(": recv() failed: ");
        Serial.println(errno);
    } else if (len == 0) {
        Serial.print(TAG);
        Serial.println(": Connection closed by peer");
    } else {
        Serial.print(TAG);
        Serial.print(": Received ");
        Serial.print(len);
        Serial.println(" bytes");
        printHexBuffer(buf, len);
    }
    return len;
}

// 關閉連線（可選）
void tcp_disconnect()
{
    if (tcp_socket >= 0) {
        shutdown(tcp_socket, 0);
        close(tcp_socket);
        tcp_socket = -1;
        Serial.print(TAG);
        Serial.println(": TCP disconnected");
    }
}