#include <Wire.h>
#include <RTClib.h>
#include <RBDdimmer.h>

// ==========================================
// DEVICE SELECTION (Commented out = Bluetooth Classic, Uncommented = BLE)
// ==========================================
#define USE_BLE

#ifdef USE_BLE
    #include <BLEDevice.h>
    #include <BLEUtils.h> 
    #include <BLEServer.h>
    #include <BLE2902.h>
    
    // UUIDs for BLE service and characteristics (Standard UART UUIDs)
    #define SERVICE_UUID        "6E400001-B5A3-F393-E0A9-E50e24dCCA9E"
    #define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50e24dCCA9E"
    #define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50e24dCCA9E"

    bool deviceConnected = false;
    String bleReceivedData = "";
    bool newDataReceived = false;

    class MyServerCallbacks: public BLEServerCallbacks {
        void onConnect(BLEServer* pServer) {
            deviceConnected = true;
        }
        void onDisconnect(BLEServer* pServer) {
            deviceConnected = false;
            // Restart advertising so the client can reconnect
            pServer->getAdvertising()->start();
        }
    };

    class MyCallbacks: public BLECharacteristicCallbacks {
        void onWrite(BLECharacteristic *pCharacteristic) {
            std::string value = pCharacteristic->getValue();
            if (value.length() > 0) {
                for (size_t i = 0; i < value.length(); i++) {
                    char c = value[i];
                    if (c == '\n' || c == '\r') {
                        if (bleReceivedData.length() > 0) {
                            newDataReceived = true;
                        }
                    } else {
                        bleReceivedData += c;
                    }
                }
            }
        }
    };

    BLECharacteristic *pTxCharacteristic;
#else
    #include <BluetoothSerial.h>
    // Check if Bluetooth is enabled in the board configuration for Classic BT
    #if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
    #error Bluetooth is not enabled! Run "make menuconfig" and enable it.
    #endif
    BluetoothSerial SerialBT; // Bluetooth Classic object
#endif

// Pin definitions for ESP32
#define DIMMER1_PIN 32
#define DIMMER2_PIN 33
#define ZEROCROSS_PIN 35 

RTC_DS1307 rtc;

// Dimmer initialization
dimmerLamp dimmer1(DIMMER1_PIN, ZEROCROSS_PIN);
dimmerLamp dimmer2(DIMMER2_PIN, ZEROCROSS_PIN);

struct LampConfig {
    int onTimeMinutes;      // Full turn-on time in minutes from midnight
    int sunriseDuration;    // Sunrise duration in minutes
    int offTimeMinutes;     // Turn-off start time in minutes from midnight
    int sunsetDuration;     // Sunset duration in minutes
};

LampConfig lamp1 = { 20 * 60 + 30, 15, 20 * 60 + 45, 15 };
LampConfig lamp2 = { 21 * 60 + 0,  15, 21 * 60 + 15, 15 };

uint8_t lastSecond = 255;

// Helper function for safe modulo
long safeMod(long val, long mod) {
    long res = val % mod;
    return (res < 0) ? (res + mod) : res;
}

void setup()
{
    Serial.begin(115200); // Standard baud rate for ESP32
    
#ifdef USE_BLE
    // BLE initialization
    BLEDevice::init("ESP32_Akvarijum");
    BLEServer *pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    BLEService *pService = pServer->createService(SERVICE_UUID);

    pTxCharacteristic = pService->createCharacteristic(
                        CHARACTERISTIC_UUID_TX,
                        BLECharacteristic::PROPERTY_NOTIFY
                    );
    pTxCharacteristic->addDescriptor(new BLE2902());

    BLECharacteristic *pRxCharacteristic = pService->createCharacteristic(
                        CHARACTERISTIC_UUID_RX,
                        BLECharacteristic::PROPERTY_WRITE
                    );
    pRxCharacteristic->setCallbacks(new MyCallbacks());

    pService->start();
    pServer->getAdvertising()->start();
    Serial.println("BLE device is ready for pairing!");
#else
    // Start Bluetooth Serial (Classic)
    SerialBT.begin("ESP32_Akvarijum"); 
    Serial.println("Bluetooth Classic device is ready for pairing!");
#endif

    if (!rtc.begin()) {
        Serial.println("RTC module not found!");
    }

    dimmer1.begin(NORMAL_MODE, ON);
    dimmer2.begin(NORMAL_MODE, ON);
    
    dimmer1.setPower(0);
    dimmer2.setPower(0);
}

int parseTimeToMinutes(String str) {
    if (str.length() != 4) return -1;
    int hh = str.substring(0, 2).toInt();
    int mm = str.substring(2, 4).toInt();
    if (hh >= 0 && hh < 24 && mm >= 0 && mm < 60) {
        return hh * 60 + mm;
    }
    return -1;
}

bool isTimeInInterval(long t, long start, long end) {
    if (start == end) return false;
    if (start < end) {
        return (t >= start && t < end);
    }
    return (t >= start || t < end);
}

long getElapsedSeconds(long t, long start) {
    long daySec = 86400L;
    return safeMod(t - start, daySec);
}

int calculateLampPower(DateTime now, LampConfig config) {
    long daySec = 86400L;
    long currentTimeSec = now.hour() * 3600L + now.minute() * 60L + now.second();

    long onTimeSec = safeMod(config.onTimeMinutes * 60L, daySec);
    long offTimeSec = safeMod(config.offTimeMinutes * 60L, daySec);
    
    long sunriseStartSec = safeMod(onTimeSec - (config.sunriseDuration * 60L), daySec);
    long sunsetEndSec = safeMod(offTimeSec + (config.sunsetDuration * 60L), daySec);

    // 1. Sunrise Phase
    if (isTimeInInterval(currentTimeSec, sunriseStartSec, onTimeSec)) {
        long elapsed = getElapsedSeconds(currentTimeSec, sunriseStartSec);
        long total = getElapsedSeconds(onTimeSec, sunriseStartSec);
        if (total <= 0) return 100;
        return (int)((elapsed * 100L) / total);
    }

    // 2. Full Power Phase
    if (isTimeInInterval(currentTimeSec, onTimeSec, offTimeSec)) {
        return 100;
    }

    // 3. Sunset Phase
    if (isTimeInInterval(currentTimeSec, offTimeSec, sunsetEndSec)) {
        long elapsed = getElapsedSeconds(currentTimeSec, offTimeSec);
        long total = getElapsedSeconds(sunsetEndSec, offTimeSec);
        if (total <= 0) return 0;
        return (int)(100 - ((elapsed * 100L) / total));
    }

    // Outside all ranges
    return 0;
}

void applyDimmerPower(dimmerLamp &dimmer, int power) {
    if (power <= 0) {
        dimmer.setPower(0);
        dimmer.setState(OFF);
    } else {
        dimmer.setState(ON);
        dimmer.setPower(power);
    }
}

// Universal function for sending responses (via BLE or SerialBT)
void sendResponse(String message) {
    Serial.println(message);
#ifdef USE_BLE
    if (deviceConnected) {
        pTxCharacteristic->setValue((uint8_t*)message.c_str(), message.length());
        pTxCharacteristic->notify();
    }
#else
    SerialBT.println(message);
#endif
}

void processIncomingCommand(String msg) {
    msg.trim();
    if (msg.length() == 0) return;

    // RTC time adjustment (format: HHMM, e.g. "1430")
    if (msg.length() == 4 && msg.indexOf(';') == -1) {
        int hh = msg.substring(0, 2).toInt();
        int mm = msg.substring(2, 4).toInt();

        if (hh >= 0 && hh < 24 && mm >= 0 && mm < 60) {
            DateTime current = rtc.now();
            rtc.adjust(DateTime(current.year(), current.month(), current.day(), hh, mm, 0));
            
            String response = "RTC set to: " + String(hh) + ":" + String(mm);
            sendResponse(response);
        }
        return;
    }

    // Light parameters adjustment (format: L12030;15;2045;15 or L22100;15;2115;15)
    if ((msg.startsWith("L1") || msg.startsWith("L2")) && msg.indexOf(';') != -1) {
        int id = msg.substring(1, 2).toInt();
        String data = msg.substring(2);

        int p1 = data.indexOf(';');
        int p2 = data.indexOf(';', p1 + 1);
        int p3 = data.indexOf(';', p2 + 1);

        if (p1 != -1 && p2 != -1 && p3 != -1) {
            String onTimeStr = data.substring(0, p1);
            int sunriseDur = data.substring(p1 + 1, p2).toInt();
            String offTimeStr = data.substring(p2 + 1, p3);
            int sunsetDur = data.substring(p3 + 1).toInt();

            int onMin = parseTimeToMinutes(onTimeStr);
            int offMin = parseTimeToMinutes(offTimeStr);

            if (onMin != -1 && offMin != -1 && sunriseDur >= 0 && sunsetDur >= 0) {
                LampConfig *targetLamp = (id == 1) ? &lamp1 : &lamp2;
                
                targetLamp->onTimeMinutes = onMin;
                targetLamp->sunriseDuration = sunriseDur;
                targetLamp->offTimeMinutes = offMin;
                targetLamp->sunsetDuration = sunsetDur;

                String response = "Lamp " + String(id) + " successfully configured!";
                sendResponse(response);
            } else {
                String err = "Error in time values!";
                sendResponse(err);
            }
        }
    }
}

void handleBluetoothCommands()
{
#ifdef USE_BLE
    if (newDataReceived) {
        newDataReceived = false;
        String cmd = bleReceivedData;
        bleReceivedData = "";
        processIncomingCommand(cmd);
    }
#else
    if (!SerialBT.available()) return;
    String msg = SerialBT.readStringUntil('\n');
    processIncomingCommand(msg);
#endif
}

void loop()
{
    handleBluetoothCommands();

    DateTime now = rtc.now();

    if (now.second() != lastSecond)
    {
        lastSecond = now.second();

        int p1 = calculateLampPower(now, lamp1);
        int p2 = calculateLampPower(now, lamp2);

        applyDimmerPower(dimmer1, p1);
        applyDimmerPower(dimmer2, p2);

        char timeStr[9];
        sprintf(timeStr, "%02d:%02d:%02d", now.hour(), now.minute(), now.second());

        // Print to Serial Monitor (USB)
        Serial.print("["); Serial.print(timeStr); Serial.print("] ");
        Serial.print("L1: "); Serial.print(p1); Serial.print("% | ");
        Serial.print("L2: "); Serial.print(p2); Serial.println("%");
    }
}