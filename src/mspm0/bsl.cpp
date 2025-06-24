#include <Arduino.h>
#include <main.h>


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_spiffs.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


#define TAG "BSL"

// GPIO 定義
#define NRST_GPIO    GPIO_NUM_18
#define BSL_GPIO     GPIO_NUM_19

// UART 定義
#define UART_PORT    UART_NUM_1
#define UART_TXD     17
#define UART_RXD     16
#define UART_BAUDRATE 9600

// SPIFFS 檔案路徑
#define FIRMWARE_PATH "/spiffs/app1.txt"

// BSL 指令定義
#define BSL_HEADER     0x80
#define CMD_CONNECTION 0x12
#define CMD_GET_ID     0x19
#define CMD_PASSWORD   0x21
#define CMD_MASS_ERASE 0x15
#define CMD_PROGRAM    0x20
#define CMD_START_APP  0x40

// CRC32 計算
uint32_t bsl_copilot_crc32(const uint8_t *data, size_t len) {
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
    uint32_t crc = bsl_copilot_crc32(&cmd, 1);
    memcpy(&out_buf[4], &crc, 4);

    uart_write_bytes(UART_PORT, (const char*)out_buf, 8);
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

    uint32_t crc = bsl_copilot_crc32(&out_buf[3], 33);
    memcpy(&out_buf[36], &crc, 4);  // CRC at offset 36

    uart_write_bytes(UART_PORT, (const char*)out_buf, 40);
    vTaskDelay(pdMS_TO_TICKS(500));
   
}
// 初始化 GPIO
void bsl_copilot_init_gpio() {
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
void bsl_copilot_init_uart() {
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

// 初始化 SPIFFS
void bsl_copilot_init_spiffs() {
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = true
    };
    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount SPIFFS");
    }
}

// 控制 BSL 模式進入
void enter_bsl_copilot_mode() {
    gpio_set_level(NRST_GPIO, 0);
    gpio_set_level(BSL_GPIO, 0);
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level(BSL_GPIO, 1);
    vTaskDelay(pdMS_TO_TICKS(100));
    vTaskDelay(pdMS_TO_TICKS(3300));
    gpio_set_level(NRST_GPIO, 1);
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level(BSL_GPIO, 0);
}

void exit_bsl_copilot_mode() {
    gpio_set_level(BSL_GPIO, 0);         // 確保離開 BSL 模式
    vTaskDelay(pdMS_TO_TICKS(50));

    gpio_set_level(NRST_GPIO, 0);        // 重置
    vTaskDelay(pdMS_TO_TICKS(100));

    gpio_set_level(NRST_GPIO, 1);        // 啟動
    vTaskDelay(pdMS_TO_TICKS(100));      // 等待 MCU 啟動
}
// 傳送資料到 MSPM0
void bsl_copilot_send_firmware() {
    FILE* f = fopen(FIRMWARE_PATH, "r");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open firmware file");
        return;
    }

    char line[256];
    uint32_t address = 0;
    uint8_t data_buf[128];
    int data_len = 0;

    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '@') {
            // 新區塊地址
            address = (uint32_t)strtol(line + 1, NULL, 16);
            data_len = 0;
        } else if (line[0] == 'q') {
            break;
        } else {
            // 資料行
            char *token = strtok(line, " ");
            while (token != NULL) {
                if (strlen(token) == 0 || token[0] == '\n') {
                    token = strtok(NULL, " ");
                    continue;
                }
                uint8_t byte = (uint8_t)strtol(token, NULL, 16);
                data_buf[data_len++] = byte;
                if (data_len == 128) {
                    // 分段傳送
                    // 產生寫入封包
                    uint8_t packet[140];
                    packet[0] = BSL_HEADER;
                    packet[1] = 0x85; // 長度低位
                    packet[2] = 0x00; // 長度高位
                    packet[3] = CMD_PROGRAM;
                    // 地址（小端）
                    packet[4] = (address & 0xFF);
                    packet[5] = (address >> 8) & 0xFF;
                    packet[6] = (address >> 16) & 0xFF;
                    packet[7] = (address >> 24) & 0xFF;
                    memcpy(&packet[8], data_buf, 128);
                    // CRC
                    uint8_t crc_data[133];
                    crc_data[0] = CMD_PROGRAM;
                    memcpy(&crc_data[1], &packet[4], 4 + 128);
                    uint32_t crc = bsl_copilot_crc32(crc_data, 133);//132
                    memcpy(&packet[136], &crc, 4);
                    uart_write_bytes(UART_PORT, (const char*)packet, 140);
                    vTaskDelay(pdMS_TO_TICKS(500));
                    address += 128;
                    data_len = 0;
                }
                token = strtok(NULL, " ");
            }
        }
    }
    // 傳送最後不足128 bytes的資料
    if (data_len > 0) {
        uint8_t packet[140] = {0};
        packet[0] = BSL_HEADER;
        packet[1] = data_len + 5;
        packet[2] = 0x00;
        packet[3] = CMD_PROGRAM;
        packet[4] = (address & 0xFF);
        packet[5] = (address >> 8) & 0xFF;
        packet[6] = (address >> 16) & 0xFF;
        packet[7] = (address >> 24) & 0xFF;
        memcpy(&packet[8], data_buf, data_len);
        uint8_t crc_data[5 + data_len];
        crc_data[0] = CMD_PROGRAM;
        memcpy(&crc_data[1], &packet[4], 4 + data_len);
        uint32_t crc = bsl_copilot_crc32(crc_data, 5 + data_len);
        memcpy(&packet[8 + data_len], &crc, 4);
        uart_write_bytes(UART_PORT, (const char*)packet, 8 + data_len + 4);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
    fclose(f);
    ESP_LOGI(TAG, "Firmware sent");
}


void bsl_copilot_func(void)
{
    bsl_copilot_init_gpio();
    bsl_copilot_init_uart();
    bsl_copilot_init_spiffs();

    enter_bsl_copilot_mode();

    build_simple_packet(CMD_CONNECTION);// receive 00
    build_bb_packet(0xBB);// receive 51
    build_simple_packet(CMD_GET_ID);// receive 00 08 19 00 31 00 01 00 01 00 00 00 ..........total 33 bytes
    build_password_packet(CMD_PASSWORD);// receive 00 08 02 00 3B 00 38 02 94 82
    build_simple_packet(CMD_MASS_ERASE);// receive 00 08 02 00 3B 00 38 02 94 82
    bsl_copilot_send_firmware();// receive 00 08 02 00 3B 00 38 02 94 82
    build_simple_packet(CMD_START_APP);// receive 00
    exit_bsl_copilot_mode();
    Serial.printf("\n bsl_end\n");
}