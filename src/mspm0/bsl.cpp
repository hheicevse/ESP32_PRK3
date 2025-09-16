#include <Arduino.h>
#include <main.h>


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_http_client.h"
#include <ArduinoJson.h>

#include <main.h>

#define TAG "BSL"

// GPIO 定義
#if CONFIG_IDF_TARGET_ESP32S3
#define NRST_GPIO    GPIO_NUM_18
#define BSL_GPIO     GPIO_NUM_19
#else
#define NRST_GPIO    GPIO_NUM_23//GPIO_NUM_18
#define BSL_GPIO     GPIO_NUM_12//GPIO_NUM_19
#endif

// UART 定義
#define UART_PORT    UART_NUM_1
#define UART_TXD     18
#define UART_RXD     5
#define UART_BAUDRATE 9600


// BSL 指令定義
#define BSL_HEADER     0x80
#define CMD_CONNECTION 0x12
#define CMD_GET_ID     0x19
#define CMD_PASSWORD   0x21
#define CMD_MASS_ERASE 0x15
#define CMD_PROGRAM    0x20
#define CMD_START_APP  0x40

// CRC32 計算
uint32_t bsl_crc32(const uint8_t *data, size_t len) {
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            uint32_t mask = -(crc & 1);
            crc = (crc >> 1) ^ (0xEDB88320 & mask);
        }
    }
    return crc;
}
// 產生簡單 BSL 封包
size_t build_simple_packet(uint8_t cmd) {
    uint8_t out_buf[8];
    out_buf[0] = BSL_HEADER;
    out_buf[1] = 0x01;
    out_buf[2] = 0x00;
    out_buf[3] = cmd;
    uint32_t crc = bsl_crc32(&cmd, 1);
    memcpy(&out_buf[4], &crc, 4);

    uart_write_bytes(UART_PORT, (const char*)out_buf, 8);
    uart_wait_tx_done(UART_PORT, pdMS_TO_TICKS(100)); 
    vTaskDelay(pdMS_TO_TICKS(500));
    return 8;
}
size_t build_bb_packet(uint8_t cmd) {
    uint8_t conn_bb[1]={cmd};// receive 51
    uart_write_bytes(UART_PORT, (const char*)conn_bb, 1);
    vTaskDelay(pdMS_TO_TICKS(500));
    return 8;
}
void build_password_packet(uint8_t cmd) {
    uint8_t out_buf[40]={0xFF};
    out_buf[0] = BSL_HEADER;
    out_buf[1] = 0x21;
    out_buf[2] = 0x00;
    out_buf[3] = cmd;
    for (size_t i = 0; i < 32; i++) {
        out_buf[i + 4] = 0xFF;
    }

    uint32_t crc = bsl_crc32(&out_buf[3], 33);
    memcpy(&out_buf[36], &crc, 4);  // CRC at offset 36

    uart_write_bytes(UART_PORT, (const char*)out_buf, 40);
    vTaskDelay(pdMS_TO_TICKS(500));
   
}
// 初始化 GPIO
void bsl_init_gpio() {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << NRST_GPIO) | (1ULL << BSL_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
}
// 初始化 UART
void bsl_init_uart() {
    uart_config_t uart_config = {
        .baud_rate = UART_BAUDRATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    uart_driver_install(UART_PORT, 2048, 2048, 0, NULL, 0);
    uart_param_config(UART_PORT, &uart_config);
    uart_set_pin(UART_PORT, UART_TXD, UART_RXD, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}

// 控制 BSL 模式進入
void enter_bsl_mode() {
    gpio_set_level(NRST_GPIO, 0);
    gpio_set_level(BSL_GPIO, 0);
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level(BSL_GPIO, 1);

    vTaskDelay(pdMS_TO_TICKS(2000));
    gpio_set_level(NRST_GPIO, 1);
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level(BSL_GPIO, 0);
}

void exit_bsl_mode() {
    gpio_set_level(BSL_GPIO, 0);         // 確保離開 BSL 模式
    vTaskDelay(pdMS_TO_TICKS(50));

    gpio_set_level(NRST_GPIO, 0);        // 重置
    vTaskDelay(pdMS_TO_TICKS(100));

    gpio_set_level(NRST_GPIO, 1);        // 啟動
    vTaskDelay(pdMS_TO_TICKS(100));      // 等待 MCU 啟動
}
static bool bsl_wait_ack(int timeout_ms = 500) {
    uint8_t rx_buf[64] = {0};
    int64_t start = esp_timer_get_time(); // us
    while ((esp_timer_get_time() - start) < timeout_ms * 1000) {
     
        int len = uart_read_bytes(UART_PORT, rx_buf, sizeof(rx_buf), pdMS_TO_TICKS(50));
        if (len > 0) {
            const uint8_t expected[] = {0x00, 0x08, 0x02, 0x00, 0x3B, 0x00, 0x38, 0x02, 0x94, 0x82};
            if (len >= sizeof(expected) && memcmp(rx_buf, expected, sizeof(expected)) == 0) {
                return true;
            }
        }


    }
    return false; // timeout
}

// 傳送一包資料（支援 1~128 bytes）
static bool bsl_send_packet(uint32_t address, const uint8_t *data, size_t len) {
    if (len == 0 || len > 128) return 0;
    uint8_t packet[140] = {0};
    packet[0] = BSL_HEADER;
    packet[1] = len + 5;
    packet[2] = 0x00;
    packet[3] = CMD_PROGRAM;
    packet[4] = (address & 0xFF);
    packet[5] = (address >> 8) & 0xFF;
    packet[6] = (address >> 16) & 0xFF;
    packet[7] = (address >> 24) & 0xFF;
    memcpy(&packet[8], data, len);

    uint8_t crc_data[5 + len];
    crc_data[0] = CMD_PROGRAM;
    memcpy(&crc_data[1], &packet[4], 4 + len);
    uint32_t crc = bsl_crc32(crc_data, 5 + len);
    memcpy(&packet[8 + len], &crc, 4);

    uart_write_bytes(UART_PORT, (const char*)packet, 8 + len + 4);
    uart_wait_tx_done(UART_PORT, pdMS_TO_TICKS(100));
    // vTaskDelay(pdMS_TO_TICKS(500));

    Serial.printf("[BSL] address : %08X\n", address);
    // 等待回應
    if (!bsl_wait_ack(500)) {
        // ESP_LOGE(TAG, "No ACK received for address %08X", address);
        Serial.printf("[BSL] ack timeout %08X\n", address);
        return false;
    }
    else
    {
        DynamicJsonDocument doc1(128);
        JsonObject res1 = doc1.to<JsonObject>();
        if (mspm0Comm.bsl_fd != 0){
        doc1.clear(); 
        res1["command"] = "get_ota_bsl";
        res1["progress"] = address;
        String jsonOut = toJson(res1);
        send(mspm0Comm.bsl_fd, jsonOut.c_str(), jsonOut.length(), 0);
        }
        return true;
    }


}

void bsl_send_firmware_http(const char* url, int fd) {
    DynamicJsonDocument doc(128);
    JsonObject res = doc.to<JsonObject>();
    if (fd != 0){
        doc.clear(); 
        res["command"] = "get_ota_bsl";
        res["status"] = "Start";
        String jsonOut = toJson(res);
        send(fd, jsonOut.c_str(), jsonOut.length(), 0);
    }

    HTTPClient http;
    http.begin(url);
    int httpCode = http.GET();
    bool ok = true;

    if (httpCode == HTTP_CODE_OK) {
        WiFiClient& stream = http.getStream();
        char line[256] = {0};
        int len = 0;
        uint8_t data_buf[128];
        uint32_t address = 0;
        int data_len = 0;



        while (http.connected()) {
            while (stream.available()) {
                char ch = stream.read();
                if (ch == '\n' || ch == '\r') {
                    if (len == 0) continue;
                    line[len] = '\0';

                    if (line[0] == '@') {
                        if (data_len > 0) {
                            ok &= bsl_send_packet(address, data_buf, data_len);
                            address += data_len;
                            data_len = 0;
                        }
                        address = (uint32_t)strtol(line + 1, NULL, 16);
                    } else if (line[0] == 'q') {
                        break;
                    } else {
                        char *token = strtok(line, " ");
                        while (token != NULL) {
                            if (strlen(token) > 0) {
                                uint8_t byte = (uint8_t)strtol(token, NULL, 16);
                                data_buf[data_len++] = byte;
                                if (data_len == 128) {
                                    ok &= bsl_send_packet(address, data_buf, 128);
                                    address += 128;
                                    data_len = 0;
                                }
                            }
                            token = strtok(NULL, " ");
                        }
                    }
                    len = 0;
                } else if (len < sizeof(line) - 1) {
                    line[len++] = ch;
                }
            }
            delay(1);
        }

        if (data_len > 0) {
            ok &= bsl_send_packet(address, data_buf, data_len);
        }
        http.end();
        Serial.println(ok ? "[BSL] Success" : "[BSL] Fail");
    } else {
        Serial.printf("[BSL] HTTP GET failed, code: %d\n", httpCode);
        ok = false;
    }
    
    if (fd != 0){
        doc.clear(); 
        res["command"] = "get_ota_bsl";
        res["status"] = ok ? "Success" : "Fail";
        String jsonOut = toJson(res);
        send(fd, jsonOut.c_str(), jsonOut.length(), 0);
    }
}





void bsl_func(const char* url,int fd)
{
    bsl_init_gpio();
    bsl_init_uart();

    enter_bsl_mode();

    build_simple_packet(CMD_CONNECTION);// receive 00
    build_bb_packet(0xBB);// receive 51
    build_simple_packet(CMD_GET_ID);// receive 00 08 19 00 31 00 01 00 01 00 00 00 ..........total 33 bytes
    build_password_packet(CMD_PASSWORD);// receive 00 08 02 00 3B 00 38 02 94 82
    build_simple_packet(CMD_MASS_ERASE);// receive 00 08 02 00 3B 00 38 02 94 82

    Serial.println("[BSL] bsl_start");

    bsl_send_firmware_http(url,fd);// receive 00 08 02 00 3B 00 38 02 94 82

    build_simple_packet(CMD_START_APP);// receive 00
    exit_bsl_mode();

    Serial.println("[BSL] bsl_end");
}