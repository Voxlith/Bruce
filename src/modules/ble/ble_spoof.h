/**
 * BLE Audio Spoof + HID Injection Module for Bruce Firmware
 * 
 * This module allows:
 * - Scanning and capturing BLE device profiles (especially audio devices like earbuds)
 * - Spoofing these devices with identical appearance
 * - Announcing both Audio and HID (keyboard) profiles simultaneously
 * - Injecting keystrokes once connected
 * 
 * Author: Bruce Project
 * Date: 2025-12-02
 */

#ifndef __BLE_SPOOF_H__
#define __BLE_SPOOF_H__

#include <Arduino.h>
#include <NimBLEDevice.h>
#include <NimBLEScan.h>
#include <NimBLEAdvertisedDevice.h>
#include <NimBLEHIDDevice.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <SD.h>
#include <vector>
#include <globals.h>

// NimBLE native functions for address management
extern "C" {
    #include "host/ble_hs_id.h"
}

// Forward declaration
class BleDeviceProfile;

/**
 * Structure to store a complete BLE device profile
 */
struct BleDeviceProfile {
    String name;                    // Device name
    String address;                 // MAC address (format: AA:BB:CC:DD:EE:FF)
    uint8_t addressType;            // Address type (0=public, 1=random)
    int rssi;                       // Signal strength
    uint16_t appearance;            // BLE appearance value
    int txPower;                    // TX power level
    String manufacturerData;        // Manufacturer data (hex string)
    std::vector<String> serviceUUIDs;  // List of service UUIDs
    String payload;                 // Complete raw advertisement data (hex string)
    size_t payloadLength;           // Length of payload
    
    // Constructor
    BleDeviceProfile() : addressType(0), rssi(0), appearance(0), txPower(0), payloadLength(0) {}
    
    // Clear all data
    void clear() {
        name = "";
        address = "";
        addressType = 0;
        rssi = 0;
        appearance = 0;
        txPower = 0;
        manufacturerData = "";
        serviceUUIDs.clear();
        payload = "";
        payloadLength = 0;
    }
};

/**
 * Custom callback class for BLE scanning
 * Captures complete device information including manufacturer data
 */
class BleSpoofScanCallbacks : public NimBLEAdvertisedDeviceCallbacks {
public:
    BleSpoofScanCallbacks();
    void onResult(NimBLEAdvertisedDevice *advertisedDevice) override;
    std::vector<BleDeviceProfile> getDevices() const { return devices; }
    void clearDevices() { devices.clear(); }
    
private:
    std::vector<BleDeviceProfile> devices;
    String payloadToHex(const uint8_t *payload, size_t length);
};

/**
 * Main BLE Spoof module class
 */
class BleSpoofModule {
public:
    BleSpoofModule();
    ~BleSpoofModule();
    
    // Scanning functions
    bool startScan(int duration = 5);
    void stopScan();
    std::vector<BleDeviceProfile> getScannedDevices() const;
    void clearScannedDevices();
    
    // Profile management
    bool saveProfile(const BleDeviceProfile &profile, FS &fs, const String &filename);
    bool loadProfile(BleDeviceProfile &profile, FS &fs, const String &filepath);
    bool deleteProfile(FS &fs, const String &filepath);
    std::vector<String> listProfiles(FS &fs, const String &directory = "/ble_spoof/profiles");
    
    // JSON serialization/deserialization
    bool profileToJson(const BleDeviceProfile &profile, JsonDocument &doc);
    bool jsonToProfile(const JsonDocument &doc, BleDeviceProfile &profile);
    
    // Spoofing functions (Phase 3)
    bool setSpoofName(const String &name);
    bool setSpoofAddress(const String &address);
    bool setSpoofAppearance(uint16_t appearance);
    bool setAdvertisementData(const String &hexData);
    bool startSpoofing(const BleDeviceProfile &profile);
    bool stopSpoofing();
    bool isSpoofing() const { return spoofingActive; }
    
    // HID functions (Phase 4)
    bool enableHID(bool enable = true);
    bool isHIDEnabled() const { return hidEnabled; }
    bool sendKeyPress(uint8_t key, uint8_t modifiers = 0);
    bool sendKeyRelease();
    bool sendString(const String &text);
    
    // Battery service (Phase 4)
    bool setBatteryLevel(uint8_t level);
    uint8_t getBatteryLevel() const { return batteryLevel; }
    
    // Get current spoof profile
    BleDeviceProfile getCurrentProfile() const { return currentProfile; }
    
private:
    BleSpoofScanCallbacks *scanCallbacks;
    NimBLEScan *pBLEScan;
    bool isScanning;
    
    // Spoofing state
    bool spoofingActive;
    BleDeviceProfile currentProfile;
    NimBLEServer *pServer;
    NimBLEAdvertising *pAdvertising;
    
    // HID state (Phase 4)
    bool hidEnabled;
    NimBLEHIDDevice *pHID;
    NimBLECharacteristic *pInputKeyboard;
    NimBLECharacteristic *pOutputKeyboard;
    uint8_t batteryLevel;
    
    // Helper functions
    void ensureDirectoryExists(FS &fs, const String &path);
    String hexToString(const uint8_t *data, size_t length);
    std::vector<uint8_t> hexStringToBytes(const String &hex);
    bool setupHIDService();
    bool setupBatteryService();
};

// Global instance (will be initialized in .cpp)
extern BleSpoofModule bleSpoofModule;

// UI Functions - to be called from BleMenu
void ble_spoof_scan_and_capture();
void ble_spoof_load_profile();
void ble_spoof_start_spoofing();
void ble_spoof_stop_spoofing();
void ble_spoof_main_menu();

#endif // __BLE_SPOOF_H__
