/*
Elation Sports Technologies LLC
Austin Allen
24 Jun 2024

BaseCam ESP32-S3 BLE Client/Master Code — modified project version.
*/

#include <Arduino.h>
#include <BLEDevice.h>
#include <string.h>
#include <stdlib.h>
#include "config.h"

#if SERIAL_DEBUG_ENABLED
#define DBG_PRINT(...) Serial.print(__VA_ARGS__)
#define DBG_PRINTLN(...) Serial.println(__VA_ARGS__)
#else
#define DBG_PRINT(...)
#define DBG_PRINTLN(...)
#endif

static BLEUUID serviceUUID(BLE_SERVICE_UUID);
static BLEUUID charUUID(BLE_CHAR_UUID);
static BLEClient* bleClient = nullptr;
static BLERemoteCharacteristic* txCharacteristic = nullptr;
static bool bleConnected = false;
static uint32_t lastReconnectAttemptMs = 0;
static int joystickCenterX = 0, joystickCenterY = 0, joystickCenterZ = 0;
static int filterX = 0, filterY = 0, filterZ = 0;
static uint32_t lastSendMs = 0;

class ClientCallbacks : public BLEClientCallbacks {
    void onConnect(BLEClient*) override { bleConnected = true; DBG_PRINTLN("BLE connected to gimbal receiver."); }
    void onDisconnect(BLEClient*) override { bleConnected = false; txCharacteristic = nullptr; DBG_PRINTLN("BLE disconnected. Will retry."); }
};

static int readAdcAverage(int pin, int samples) {
    int32_t acc = 0;
    for (int i = 0; i < samples; i++) { acc += analogRead(pin); delay(2); }
    return (int)(acc / samples);
}

static int applyDeadzone(int value, int deadzone) { return abs(value) < deadzone ? 0 : value; }

static int applyLightFilter(int* state, int sample) {
#if JOYSTICK_FILTER_SHIFT <= 0
    (void)state; return sample;
#else
    *state += sample - (*state >> JOYSTICK_FILTER_SHIFT);
    return (*state >> JOYSTICK_FILTER_SHIFT);
#endif
}

static int processAxis(int pin, int center, int* filterState) {
    int value = analogRead(pin) - center;
    value = applyDeadzone(value, JOYSTICK_DEADZONE);
    value = applyLightFilter(filterState, value);
    if (value > ADC_MAX_VALUE) value = ADC_MAX_VALUE;
    else if (value < -ADC_MAX_VALUE) value = -ADC_MAX_VALUE;
    return value;
}

static bool connectToGimbal() {
    BLEScan* scan = BLEDevice::getScan();
    scan->setActiveScan(true);
    BLEScanResults found = scan->start(BLE_SCAN_TIME_SECONDS, false);
    bool foundTarget = false;
    BLEAdvertisedDevice target;
    for (int i = 0; i < found.getCount(); i++) {
        BLEAdvertisedDevice device = found.getDevice(i);
        if (device.haveName() && device.getName() == BLE_TARGET_DEVICE_NAME) { target = device; foundTarget = true; break; }
    }
    scan->clearResults();
    if (!foundTarget) return false;
    if (bleClient == nullptr) { bleClient = BLEDevice::createClient(); bleClient->setClientCallbacks(new ClientCallbacks()); }
    else if (bleClient->isConnected()) bleClient->disconnect();
    if (!bleClient->connect(&target)) return false;
    BLERemoteService* remoteService = bleClient->getService(serviceUUID);
    if (remoteService == nullptr) { bleClient->disconnect(); return false; }
    txCharacteristic = remoteService->getCharacteristic(charUUID);
    if (txCharacteristic == nullptr || !txCharacteristic->canWrite()) { txCharacteristic = nullptr; bleClient->disconnect(); return false; }
    bleConnected = true;
    return true;
}

void setup() {
    Serial.begin(DEBUG_SERIAL_BAUD);
    uint32_t serialWaitStart = millis();
    while (!Serial && (millis() - serialWaitStart) < SERIAL_WAIT_TIMEOUT_MS) delay(10);
    analogReadResolution(ADC_RESOLUTION_BITS);
    analogSetPinAttenuation(JOY_X_PIN, ADC_11db);
    analogSetPinAttenuation(JOY_Y_PIN, ADC_11db);
    analogSetPinAttenuation(JOY_Z_PIN, ADC_11db);
    pinMode(JOY_X_PIN, INPUT); pinMode(JOY_Y_PIN, INPUT); pinMode(JOY_Z_PIN, INPUT);
    delay(20);
    joystickCenterX = readAdcAverage(JOY_X_PIN, JOYSTICK_CENTER_SAMPLES);
    joystickCenterY = readAdcAverage(JOY_Y_PIN, JOYSTICK_CENTER_SAMPLES);
    joystickCenterZ = readAdcAverage(JOY_Z_PIN, JOYSTICK_CENTER_SAMPLES);
    BLEDevice::init("");
    lastReconnectAttemptMs = millis() - BLE_RECONNECT_DELAY_MS;
}

void loop() {
    const uint32_t now = millis();
    if (!bleConnected || txCharacteristic == nullptr || bleClient == nullptr || !bleClient->isConnected()) {
        bleConnected = false; txCharacteristic = nullptr;
        if ((now - lastReconnectAttemptMs) >= BLE_RECONNECT_DELAY_MS) { lastReconnectAttemptMs = now; connectToGimbal(); }
        delay(10); return;
    }
    if ((now - lastSendMs) < BLE_SEND_INTERVAL_MS) { delay(1); return; }
    lastSendMs = now;
    const int x = processAxis(JOY_X_PIN, joystickCenterX, &filterX);
    const int y = processAxis(JOY_Y_PIN, joystickCenterY, &filterY);
    const int z = processAxis(JOY_Z_PIN, joystickCenterZ, &filterZ);
    char packet[BLE_PACKET_MAX_LEN];
    const int n = snprintf(packet, sizeof(packet), "%d,%d,%d", x, y, z);
    if (n <= 0 || n >= (int)sizeof(packet)) return;
    txCharacteristic->writeValue((uint8_t*)packet, (size_t)n, false);
    if (bleClient == nullptr || !bleClient->isConnected()) { bleConnected = false; txCharacteristic = nullptr; }
}
