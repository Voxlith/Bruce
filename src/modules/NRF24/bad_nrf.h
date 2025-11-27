#ifndef __BAD_NRF_H__
#define __BAD_NRF_H__

#include <Arduino.h>
#include <RF24.h>
#include <vector>
#include <globals.h>

// Device type enumeration
enum DeviceType {
    DEVICE_UNKNOWN = 0,
    DEVICE_LOGITECH_UNIFYING,
    DEVICE_LOGITECH_GENERIC,
    DEVICE_MICROSOFT,
    DEVICE_GENERIC_2GHZ
};

// Structure to store detected NRF device information
struct NRFDevice {
    uint8_t address[5];           // 5-byte device address
    uint8_t channel;              // Radio channel (2-83)
    int8_t rssi;                  // Signal strength
    DeviceType type;              // Device type
    bool potentially_vulnerable;  // Vulnerability indicator
    uint32_t last_seen;          // Timestamp of last packet
    uint8_t packet_count;        // Number of packets seen
    String name;                 // User-friendly name
};

// Main menu and submenu functions
void badnrf_menu();
void badnrf_disclaimer();

// Scanner functions
void badnrf_scan();
void badnrf_stop_scan();

// Device management
void badnrf_show_devices();
void badnrf_save_target(NRFDevice* device);
NRFDevice badnrf_load_target(String filepath);
void badnrf_targets();

// Testing and injection
bool badnrf_test_target(NRFDevice* target);
void badnrf_test_menu();
void badnrf_run_payload(String script_path, NRFDevice* target);
void badnrf_run_payload();  // Menu version

// Helper functions
String deviceTypeToString(DeviceType type);
DeviceType detectDeviceType(uint8_t* packet, uint8_t len, uint8_t* address);
bool isValidPacket(uint8_t* packet, uint8_t len);
String addressToString(uint8_t* address, uint8_t len = 5);

#endif // __BAD_NRF_H__
