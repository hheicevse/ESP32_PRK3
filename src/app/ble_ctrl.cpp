#include <Arduino.h>

#include <Update.h>
#include <NimBLEDevice.h>

#include <WiFiMulti.h>


#include <main.h>

#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID_1 "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define CHARACTERISTIC_UUID_2 "482fcad3-fbe6-49a9-9b91-b291b24d17b2"
NimBLECharacteristic* pCharacteristic1 = nullptr;
NimBLECharacteristic* pCharacteristic2 = nullptr;

bool deviceConnected = false;
bool oldDeviceConnected = false;
uint32_t value = 0;

const char* ble_ssid = "FanyuIsGood";

class ServerCallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) override {
    deviceConnected = true;
  };

  void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) override {
    // Peer disconnected, add them to the whitelist
    // This allows us to use the whitelist to filter connection attempts
    // which will minimize reconnection time.
    NimBLEDevice::whiteListAdd(connInfo.getAddress());
    deviceConnected = false;
  }
} serverCallbacks;

/** Handler class for characteristic actions */
class CharacteristicCallbacks : public NimBLECharacteristicCallbacks {
    void onRead(NimBLECharacteristic* pCharacteristic1, NimBLEConnInfo& connInfo) override {

    }

    void onWrite(NimBLECharacteristic* pCharacteristic1, NimBLEConnInfo& connInfo) override {
        std::string value = pCharacteristic1->getValue();
        Serial.printf("%s : onWrite(), value: %s\n", pCharacteristic1->getUUID().toString().c_str(), value.c_str());
        
        //fanyu 測試ble控制wifi連線斷線
        if (value.length() == 1 && value[0] == 0x31) {
          if(WiFi.status() != WL_CONNECTED)//防止已連線又再連線
          {
            Serial.printf("start fanyu \n");
            WiFi.begin("TP-Link_2.4g_CCBD", "63504149");
            ota_web_init();
            ota_http_init();
            digitalWrite(2, HIGH);
          }

        }
        else if (value.length() == 1 && value[0] == 0x32) {
          Serial.printf("stop fanyu \n");
          ota_web.stop();
          ota_http.end();
          WiFi.disconnect();
          digitalWrite(2, LOW);
        }
        else if (value.length() == 1 && value[0] == 0x33) {
          saveWiFiCredentials("DCDStyut", "q3ef");
        }
        else if (value.length() == 1 && value[0] == 0x34) {
          saveWiFiCredentials("TP-Lig_CCBD", "6149");
        }
        else if (value.length() == 1 && value[0] == 0x35) {
          // 讀取帳密
          String ssid, password;
          loadWiFiCredentials(ssid, password);
          Serial.println("SSID: " + ssid);
          Serial.println("PWD: " + password);
        }
        else if (value == "IP?") {
          // 回傳 IP
          String ip = "IP=" + WiFi.localIP().toString();
          Serial.println(ip);
          pCharacteristic1->setValue(ip.c_str()); // 或儲存起來待讀取
        }
        else if (value == "PORT?") {
          // 回傳 PORT
          String port =  "PORT=" + String(SERVER_PORT);
          Serial.println(port);
          pCharacteristic1->setValue(port.c_str());
        }

        // Serial.printf("%s : onRead(), value: %s\n",
        //               pCharacteristic1->getUUID().toString().c_str(),
        //               pCharacteristic1->getValue().c_str());


    }

    /**
     *  The value returned in code is the NimBLE host return code.
     */
    void onStatus(NimBLECharacteristic* pCharacteristic1, int code) override {
        Serial.printf("Notification/Indication return code: %d, %s\n", code, NimBLEUtils::returnCodeToString(code));
    }

    /** Peer subscribed to notifications/indications */
    void onSubscribe(NimBLECharacteristic* pCharacteristic1, NimBLEConnInfo& connInfo, uint16_t subValue) override {
        std::string str  = "Client ID: ";
        str             += connInfo.getConnHandle();
        str             += " Address: ";
        str             += connInfo.getAddress().toString();
        if (subValue == 0) {
            str += " Unsubscribed to ";
        } else if (subValue == 1) {
            str += " Subscribed to notifications for ";
        } else if (subValue == 2) {
            str += " Subscribed to indications for ";
        } else if (subValue == 3) {
            str += " Subscribed to notifications and indications for ";
        }
        str += std::string(pCharacteristic1->getUUID());

        Serial.printf("%s\n", str.c_str());
    }
} chrCallbacks1;

// 第二個 Characteristic 的 callback 類別
class CharacteristicCallbacks2 : public NimBLECharacteristicCallbacks {
    void onRead(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override {
        // 處理第二個 characteristic 的讀取邏輯
        Serial.println("[CHAR 2] onRead");
    }

    void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override {
        std::string value = pCharacteristic->getValue();
        Serial.printf("[CHAR 2] %s : onWrite(), value: %s\n", pCharacteristic->getUUID().toString().c_str(), value.c_str());

        // 你可以依照 value 寫你自己的邏輯
        if (value == "hello") {
            Serial.println("[CHAR 2] Received hello");
        }
    }
};
CharacteristicCallbacks2 chrCallbacks2; // callback 實例

void onAdvComplete(NimBLEAdvertising* pAdvertising) {
  Serial.println("Advertising stopped");
  if (deviceConnected) {
    return;
  }
  // If advertising timed out without connection start advertising without whitelist filter
  pAdvertising->setScanFilter(false, false);
  pAdvertising->start();
}


void ble_init(void) {

  NimBLEDevice::init(ble_ssid);//for ios
  // NimBLEDevice::setSecurityAuth(false, true, true); /** bonding, MITM, don't need BLE secure connections as we are using passkey pairing */
  // NimBLEDevice::setSecurityPasskey(666666);
  // NimBLEDevice::setSecurityIOCap(BLE_HS_IO_DISPLAY_ONLY); /** Display only passkey */

  NimBLEAdvertisementData advData;
  advData.setName(ble_ssid); //for android,local name
  
  NimBLEServer* pServer = NimBLEDevice::createServer();
  pServer->setCallbacks(&serverCallbacks);
  pServer->advertiseOnDisconnect(false);

  NimBLEService* pService = pServer->createService(SERVICE_UUID);
 // 第一個 characteristic
  pCharacteristic1 = pService->createCharacteristic(CHARACTERISTIC_UUID_1,  NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::NOTIFY);
  pCharacteristic1->setCallbacks(&chrCallbacks1);

  // 第二個 characteristic
  pCharacteristic2 = pService->createCharacteristic(CHARACTERISTIC_UUID_2, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::NOTIFY);
  pCharacteristic2->setCallbacks(&chrCallbacks2);

  pService->start();

  NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->enableScanResponse(false);
  pAdvertising->setAdvertisingCompleteCallback(onAdvComplete);

  pAdvertising->setAdvertisementData(advData);

  pAdvertising->start();
  Serial.println("Waiting a client connection to notify...");
}


void ble_notify(void) {

  if (!deviceConnected && oldDeviceConnected) {
    NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
    if (NimBLEDevice::getWhiteListCount() > 0) {
      // Allow anyone to scan but only whitelisted can connect.
      pAdvertising->setScanFilter(false, true);
    }
    // advertise with whitelist for 5 seconds
    pAdvertising->start(5 * 1000);
    Serial.println("start advertising");
    oldDeviceConnected = deviceConnected;
  }

  if (deviceConnected && !oldDeviceConnected) {
    // do stuff here on connecting
    oldDeviceConnected = deviceConnected;
  }
}