#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include <main.h>
#define MAX_CLIENTS 5
#define BUFFER_SIZE 1024

// const char* ssid = "你的SSID";
// const char* password = "你的密碼";

int server_fd = -1;
int client_fds[MAX_CLIENTS] = {-1, -1, -1, -1, -1};
std::string client_buffers[MAX_CLIENTS];  // 每個 client 一個接收緩衝區
int mockvoltage = 0;

void send_json(int fd, JsonDocument& doc) {
  String out;
  serializeJson(doc, out);
  send(fd, out.c_str(), out.length(), 0);
  Serial.print("Sent: ");
  Serial.println(out);
}

void handle_json(int client_fd, const char* data) {
  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, data);
  if (error) {
    Serial.print("JSON parse error: ");
    Serial.println(error.f_str());
    return;
  }

  if (!doc.is<JsonArray>()) {
    Serial.println("Expected JSON array.");
    return;
  }

  JsonArray arr = doc.as<JsonArray>();
  StaticJsonDocument<512> response;

  for (JsonObject obj : arr) {
    const char* cmd = obj["command"];
    if (!cmd) continue;

    if (strcmp(cmd, "get_status") == 0) {
     float mockCurrent = 1.0 + 2.0 * sin(millis() / 1000.0);
      mockvoltage = (mockvoltage + 1) % 10;
      JsonObject res = response.createNestedObject();
      res["command"] = "get_status";
      res["voltage"] = 12.5 + mockvoltage;
      res["current"] = roundf(mockCurrent * 100) / 100.0;  // ✅ 保留兩位小數但是數字
    } else if (strcmp(cmd, "set_level") == 0) {
      float l1 = obj["level1"] | 0.0;
      float l2 = obj["level2"] | 0.0;
      Serial.printf("Level1: %.2f, Level2: %.2f\n", l1, l2);
    } else if (strcmp(cmd, "set_ssid_pwd") == 0) {
      const char* s = obj["ssid"] | "";
      const char* p = obj["password"] | "";
      Serial.printf("SSID: %s, PASSWORD: %s\n", s, p);
    }
  }

  if (!response.isNull()) {
    send_json(client_fd, response);
  }
}



int bsd_socket_init() {
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected.");
  Serial.println(WiFi.localIP());

  server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
    Serial.println("socket() failed");
    return 0;
  }

  struct sockaddr_in server_addr;
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(SERVER_PORT);
  server_addr.sin_addr.s_addr = INADDR_ANY;

  if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
    Serial.println("bind() failed");
    return 0;
  }

  listen(server_fd, MAX_CLIENTS);
  Serial.printf("TCP server listening on port %d...\n", SERVER_PORT);
  return 1;
}

void bsd_socket_func() {
  fd_set readfds;
  FD_ZERO(&readfds);
  FD_SET(server_fd, &readfds);
  int max_fd = server_fd;

  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (client_fds[i] != -1) {
      FD_SET(client_fds[i], &readfds);
      if (client_fds[i] > max_fd) max_fd = client_fds[i];
    }
  }

  struct timeval timeout = {0, 100000};  // 100ms timeout
  int activity = select(max_fd + 1, &readfds, NULL, NULL, &timeout);
  if (activity < 0) return;

  // 接受新連線
  if (FD_ISSET(server_fd, &readfds)) {
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    int new_fd = accept(server_fd, (struct sockaddr*)&client_addr, &addr_len);
    if (new_fd >= 0) {
      Serial.print("New client: ");
      Serial.println(inet_ntoa(client_addr.sin_addr));

      bool added = false;
      for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_fds[i] == -1) {
          client_fds[i] = new_fd;
          client_buffers[i].clear();  // 初始化 buffer
          added = true;
          break;
        }
      }

      if (!added) {
        Serial.println("Max clients reached. Connection refused.");
        close(new_fd);
      }
    }
  }

  // 處理已連線 client
  for (int i = 0; i < MAX_CLIENTS; i++) {
    int fd = client_fds[i];
    if (fd == -1 || !FD_ISSET(fd, &readfds)) continue;

    char buffer[BUFFER_SIZE] = {0};
    int len = recv(fd, buffer, sizeof(buffer) - 1, 0);
    if (len <= 0) {
      Serial.printf("Client %d disconnected\n", fd);
      close(fd);
      client_fds[i] = -1;
      client_buffers[i].clear();
      continue;
    }

    buffer[len] = '\0';
    client_buffers[i] += buffer;  // append 到 client 專屬 buffer

    // 嘗試解析完整 JSON 陣列（用中括號對應）
    std::string& buf = client_buffers[i];
    while (true) {
      int open = 0;
      size_t jsonEnd = std::string::npos;

      for (size_t j = 0; j < buf.length(); j++) {
        if (buf[j] == '[') open++;
        else if (buf[j] == ']') open--;

        if (open == 0 && buf[j] == ']') {
          jsonEnd = j + 1;  // 拿到結尾位置
          break;
        }
      }

      if (jsonEnd != std::string::npos) {
        std::string oneJson = buf.substr(0, jsonEnd);
        buf.erase(0, jsonEnd);  // 移除已處理部分
        Serial.printf("Client %d parsed JSON: %s\n", fd, oneJson.c_str());
        handle_json(fd, oneJson.c_str());
      } else {
        // 未完成，等待下次補齊
        break;
      }
    }
  }
}