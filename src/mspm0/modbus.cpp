// 預設 ModbusMaster 的傳送 buffer 大小是 64 bytes，約可容納 32 筆 16-bit register。若你要寫入很多筆，要在 .h 裡改：
// #define ku8MaxBufferSize 64  // 預設值

#include <Arduino.h>

#include <ModbusMaster.h>

#include <main.h>

// 宣告 ModbusMaster 物件
ModbusMaster node;

// 將 Serial1 設定為 UART1
#define MODBUS_SERIAL Serial1
// UART 腳位設定（可依需求修改）
#define MODBUS_RX_PIN 16  // RX1
#define MODBUS_TX_PIN 17  // TX1
void preTransmission() {
  // RS485 驅動腳（如果你有用 RS485 Transceiver，這裡要切換方向）
  // digitalWrite(DE_RE_PIN, HIGH);  // 啟用傳送
}

void postTransmission() {
  // digitalWrite(DE_RE_PIN, LOW);   // 結束傳送
}

void printResult(uint8_t result) {
  if (result == node.ku8MBSuccess) {
    Serial.println("[Modbus] Success");
  } else {
    Serial.print("Error: ");
    Serial.println(result, HEX);
  }
}

void modbus_init() {
  MODBUS_SERIAL.end();
  MODBUS_SERIAL.begin(115200, SERIAL_8N1, MODBUS_RX_PIN, MODBUS_TX_PIN);
  // 初始化 modbus node, slave ID 1
  node.begin(1, MODBUS_SERIAL);
  Serial.println("[Modbus] Master Start");
  // 可選 RS485 傳送控制
  // node.preTransmission(preTransmission);
  // node.postTransmission(postTransmission);

  uint8_t result;
  // === 0x06: 寫入單一暫存器 ===
  Serial.println("[Modbus][0x06] Write 0x1234 to register 0x0001");
  result = node.writeSingleRegister(0x0001, 0x1234);
  printResult(result);

  delay(1000);

  // === 0x10: 寫入多個暫存器 ===
  Serial.println("[Modbus][0x10] Write 3 registers starting from 0x0010");

  node.clearTransmitBuffer();  // 清除之前的資料
  node.setTransmitBuffer(0, 0x1111);  // 寫入 index=0 的值
  node.setTransmitBuffer(1, 0x2222);  // index=1
  node.setTransmitBuffer(2, 0x3333);  // index=2

  // 寫入起始暫存器地址（這是寫入目的地）
  result = node.writeMultipleRegisters(0x0010, 3);  // 舊版寫法（也支援）

  printResult(result);

  delay(1000);

  // === 0x03: 讀取暫存器 ===
  Serial.println("[Modbus][0x03] Read 5 registers starting from 0x0010");
  result = node.readHoldingRegisters(0x0010, 5);
  if (result == node.ku8MBSuccess) {
    for (uint8_t i = 0; i < 5; i++) {
      Serial.print("[Modbus] Reg ");
      Serial.print(0x0010 + i, HEX);
      Serial.print(": ");
      Serial.println(node.getResponseBuffer(i), HEX);
    }
  } else {
    printResult(result);
  }

  MODBUS_SERIAL.end();
}