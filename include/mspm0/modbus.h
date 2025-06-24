#ifndef _MODBUS_H_
#define _MODBUS_H_
#pragma once

void modbus_init(void);

#endif /* _MODBUS_H_ */

/* 錯誤回傳方式
ku8MBSuccess	0x00	成功
ku8MBInvalidSlaveID	0xE0	回應的 slave ID 不符
ku8MBInvalidFunction	0xE1	功能碼錯誤或未實作
ku8MBResponseTimedOut	0xE2	超時無回應
ku8MBInvalidCRC	0xE3	CRC 錯誤
ku8MBInvalidData	0xE4	資料長度錯誤或不完整


常見 Exception Code（根據 MODBUS 協定）：

Exception Code	說明
0x01	Illegal Function（不支援該功能碼）
0x02	Illegal Data Address（資料位址錯誤）
0x03	Illegal Data Value（資料值不合法）
0x04	Slave Device Failure（內部錯誤）

*/