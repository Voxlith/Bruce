#pragma once
#include <Arduino.h>
#include "ble_custom_spam.h"

// Structure pour stocker les payloads Xiaomi embarqués
struct XiaomiDevicePayload {
    const char* name;              // Nom du device (e.g., "Redmi Buds 4")
    const char* address;           // MAC complète "XX:XX:XX:XX:XX:XX"
    const uint8_t* rawAdvData;     // Pointeur vers raw advertisement data
    size_t advDataLen;             // Taille de rawAdvData
    const uint8_t* rawScanRsp;     // Pointeur vers scan response (optionnel)
    size_t scanRspLen;             // Taille de rawScanRsp
};

// Fonction principale du spam Xiaomi
void xiaomi_spam();
