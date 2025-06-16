#include <Arduino.h>

#include <Update.h>
#include <NimBLEDevice.h>

#include <WiFiMulti.h>


#include <main.h>

#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

NimBLECharacteristic* pCharacteristic = nullptr;
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
    void onRead(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override {

        std::string value = pCharacteristic->getValue();

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
        // Serial.printf("%s : onRead(), value: %s\n",
        //               pCharacteristic->getUUID().toString().c_str(),
        //               pCharacteristic->getValue().c_str());
       
    }

    void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override {
        Serial.printf("%s : onWrite(), value: %s\n",
                      pCharacteristic->getUUID().toString().c_str(),
                      pCharacteristic->getValue().c_str());
    }

    /**
     *  The value returned in code is the NimBLE host return code.
     */
    void onStatus(NimBLECharacteristic* pCharacteristic, int code) override {
        Serial.printf("Notification/Indication return code: %d, %s\n", code, NimBLEUtils::returnCodeToString(code));
    }

    /** Peer subscribed to notifications/indications */
    void onSubscribe(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo, uint16_t subValue) override {
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
        str += std::string(pCharacteristic->getUUID());

        Serial.printf("%s\n", str.c_str());
    }
} chrCallbacks;


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
  NimBLEAdvertisementData advData;
  advData.setName(ble_ssid); //for android,local name
  
  NimBLEServer* pServer = NimBLEDevice::createServer();
  pServer->setCallbacks(&serverCallbacks);
  pServer->advertiseOnDisconnect(false);

  NimBLEService* pService = pServer->createService(SERVICE_UUID);

  pCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::NOTIFY);

  pCharacteristic->setCallbacks(&chrCallbacks);

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