#include <Arduino.h>

#include <Update.h>
#include <NimBLEDevice.h>

#include <WiFiMulti.h>
#include <ArduinoJson.h>

#include <main.h>

#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID_1 "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define CHARACTERISTIC_UUID_2 "482fcad3-fbe6-49a9-9b91-b291b24d17b2"
NimBLECharacteristic* pCharacteristic1 = nullptr;
NimBLECharacteristic* pCharacteristic2 = nullptr;

bool deviceConnected = false;
bool oldDeviceConnected = false;
uint32_t value = 0;

// BLE SSID 預設為 SN
String ble_sn_str = getSNInfo().sn;
const char* ble_ssid = ble_sn_str.c_str();
// const char* ble_ssid = "PRK_DEMO";

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
            std::string rxValue = pCharacteristic1->getValue();
            if (rxValue.length() == 0) return;

            Serial.printf("[BLE] RX: %s\n", rxValue.c_str());

            // 解析 JSON 陣列
            DynamicJsonDocument doc(512);
            DeserializationError error = deserializeJson(doc, rxValue);
            if (error) {
                Serial.printf("[BLE] JSON Parse Error: %s\n", error.c_str());
                return;
            }

            JsonArray arr = doc.as<JsonArray>();
            DynamicJsonDocument response(512);
            JsonArray resArr = response.to<JsonArray>();

            for (JsonObject obj : arr) {
                const char* command = obj["command"];
                if (!command) continue;

                JsonObject res = resArr.createNestedObject();
                res["command"] = command;

                if (strcmp(command, "set_wifi") == 0) {
                    const char* ssid = obj["ssid"] | "";
                    const char* pwd  = obj["pwd"]  | "";
                    saveWiFiCredentials(ssid, pwd);
                    res["status"] = "ok";

                } else if (strcmp(command, "get_wifi") == 0) {
                    String ssid, pwd;
                    loadWiFiCredentials(ssid, pwd);
                    res["ssid"] = ssid;
                    res["pwd"]  = pwd;

                } else if (strcmp(command, "set_wifi_connect") == 0) {
                    String ssid, pwd;
                    loadWiFiCredentials(ssid, pwd);
                    if (WiFi.status() != WL_CONNECTED) {
                        WiFi.begin(ssid.c_str(), pwd.c_str());
                    }
                    res["status"] = "ok";

                } else if (strcmp(command, "set_wifi_disconnect") == 0) {
                    WiFi.disconnect();
                    res["status"] = "ok";

                } else if (strcmp(command, "get_wifi_status") == 0) {
                    res["status"] = (WiFi.status() == WL_CONNECTED) ? "connected" : "disconnected";

                } else if (strcmp(command, "get_wifi_ip") == 0) {
                    res["ip"]   = WiFi.localIP().toString();
                    res["port"] = String(SERVER_PORT_STA);
                }
            }

            // 序列化回傳 JSON
            String jsonOut;
            serializeJson(resArr, jsonOut);
            Serial.printf("[BLE] TX: %s\n", jsonOut.c_str());

            pCharacteristic1->setValue(jsonOut.c_str());
            pCharacteristic1->notify(); // 立即推送給 Client
        

        // Serial.printf("%s : onRead(), value: %s\n",
        //               pCharacteristic1->getUUID().toString().c_str(),
        //               pCharacteristic1->getValue().c_str());


    }

    /**
     *  The value returned in code is the NimBLE host return code.
     */
    void onStatus(NimBLECharacteristic* pCharacteristic1, int code) override {
        Serial.printf("[BLE] Notification/Indication return code: %d, %s\n", code, NimBLEUtils::returnCodeToString(code));
    }

    /** Peer subscribed to notifications/indications */
    void onSubscribe(NimBLECharacteristic* pCharacteristic1, NimBLEConnInfo& connInfo, uint16_t subValue) override {
        std::string str  = "[BLE] Client ID: ";
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
        Serial.println("[BLE][CHAR 2] onRead");
    }

    void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override {
        std::string value = pCharacteristic->getValue();
        Serial.printf("[BLE][CHAR 2] %s : onWrite(), value: %s\n", pCharacteristic->getUUID().toString().c_str(), value.c_str());

        // 你可以依照 value 寫你自己的邏輯
        if (value == "hello") {
            Serial.println("[BLE][CHAR 2] Received hello");
        }
    }
};
CharacteristicCallbacks2 chrCallbacks2; // callback 實例

void onAdvComplete(NimBLEAdvertising* pAdvertising) {
  Serial.println("[BLE] Advertising stopped");
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

  // 自己塞 advData（尤其是 Local Name）時，覆蓋了 SDK 自動產生的 Service UUID 廣播資料，
  // 導致掃描端無法再用「Service UUID filter」去找到裝置
  // 因為一旦你呼叫 setAdvertisementData(advData)，原本的 Service UUID 廣播資料就被清空，只廣播了名字。
  // 想要 同時廣播名字 + Service UUID，需要自己手動把兩個都塞進去
  NimBLEAdvertisementData advData;
  advData.setName(ble_ssid); //for android,local name
  advData.addServiceUUID(SERVICE_UUID); // 把 Service UUID 加回去
  pAdvertising->setAdvertisementData(advData);

  pAdvertising->start();
  Serial.println("[BLE] Waiting a client connection to notify...");
}

// 停用 BLE 廣播與服務
void ble_deinit(void) {
    NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
    if (pAdvertising) {
        pAdvertising->stop();
    }
    NimBLEDevice::deinit(true);
    Serial.println("[BLE] Disabled");
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
    Serial.println("[BLE] start advertising");
    oldDeviceConnected = deviceConnected;
  }

  if (deviceConnected && !oldDeviceConnected) {
    // do stuff here on connecting
    oldDeviceConnected = deviceConnected;
  }
}