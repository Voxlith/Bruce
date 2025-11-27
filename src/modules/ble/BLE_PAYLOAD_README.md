# BLE Custom Payload Scanner & Spammer

## Overview

This implementation adds two new features to Bruce firmware for BLE (Bluetooth Low Energy) operations:

1. **Scan Advertiser** - Scan and capture BLE advertisement packets, extract manufacturer data and other information, then save as custom payloads
2. **Custom Payload** - Load and spam previously captured BLE payloads from JSON files

## Features

### Scan Advertiser (`ble_scanner.cpp/h`)

- Scans for nearby BLE devices
- Displays devices with name, address, and RSSI
- Shows detailed information for selected devices including:
  - Device name
  - MAC address
  - RSSI (signal strength)
  - Manufacturer ID and data
  - Service UUIDs
  - TX Power
- Saves captured device information to JSON payload files
- Storage location: `/BruceSD/ble/payloads/` (SD card) or `/ble/payloads/` (LittleFS)

### Custom Payload (`ble_custom_spam.cpp/h`)

- Browse and load previously captured payload JSON files
- Display payload information before spamming
- Spam loaded payloads with configurable interval
- Automatically randomizes MAC address for each advertisement
- Shows spam counter and allows stopping at any time

## Usage

### Scanning and Capturing Devices

1. Navigate to `Bluetooth Menu` → `Scan Advertiser`
2. Press OK to start scanning (10 second scan)
3. Browse the list of found devices using Up/Down
4. Press OK on a device to view details
5. Press OK again to save the payload to a JSON file
6. The file is automatically named based on device name or MAC address

### Spamming Custom Payloads

1. Navigate to `Bluetooth Menu` → `Custom Payload`
2. Press OK to browse for a payload file
3. Navigate to `/BruceSD/ble/payloads/` (or `/ble/payloads/`)
4. Select a `.json` payload file
5. Review the payload information
6. Press OK to start spamming
7. Press ESC to stop

## Payload JSON Format

The payload files use the following JSON structure:

```json
{
  "version": "1.0",
  "name": "Device Name",
  "type": "advertisement",
  "address": "AA:BB:CC:DD:EE:FF",
  "rssi": -45,
  "adv_data": {
    "flags": "0x06",
    "complete_name": "Full Device Name",
    "tx_power": -10,
    "manufacturer_data": {
      "company_id": "0x004C",
      "data": "0201061AFF4C000C0E00..."
    },
    "service_uuids": ["0xFE9F", "0x180F"]
  },
  "raw_adv_data": "020106..."
}
```

### Field Descriptions

- `version`: Format version (currently "1.0")
- `name`: Human-readable device name
- `type`: Always "advertisement" for BLE advertisement packets
- `address`: Original device MAC address (not used during spam)
- `rssi`: Signal strength when captured
- `adv_data`: Structured advertisement data
  - `flags`: BLE advertisement flags (hex string)
  - `complete_name`: Full device name as advertised
  - `tx_power`: Transmission power level (dBm)
  - `manufacturer_data`: Company-specific data
    - `company_id`: Bluetooth Company ID (hex, e.g., 0x004C = Apple)
    - `data`: Hex-encoded manufacturer data
  - `service_uuids`: Array of advertised service UUIDs
- `raw_adv_data`: Complete raw advertisement packet (hex-encoded)

## Technical Details

### Scanner Implementation

- Uses NimBLE library for BLE operations
- Active scanning with scan response collection
- 10-second scan duration (configurable)
- Stores up to memory limit of devices
- Parses all standard BLE AD Types

### Spammer Implementation

- Generates random MAC address for each advertisement burst
- 100ms interval between advertisements (configurable)
- Uses maximum TX power for range
- Properly reinitializes BLE stack between spams
- Clean shutdown on stop

## File Structure

```
src/modules/ble/
├── ble_scanner.h           # Scanner class and functions
├── ble_scanner.cpp         # Scanner implementation
├── ble_custom_spam.h       # Custom payload spammer class
└── ble_custom_spam.cpp     # Spammer implementation

src/core/menu_items/
└── BleMenu.cpp             # Menu integration (modified)
```

## Menu Integration

Added to `BleMenu.cpp`:
- "Scan Advertiser" menu entry (line 28)
- "Custom Payload" menu entry (line 29)

Both entries are only available when `LITE_VERSION` is not defined.

## Example Payloads

### Apple AirPods

```json
{
  "version": "1.0",
  "name": "AirPods Pro",
  "type": "advertisement",
  "adv_data": {
    "flags": "0x06",
    "complete_name": "AirPods Pro",
    "manufacturer_data": {
      "company_id": "0x004C",
      "data": "071907200075AA3001000045121212"
    }
  }
}
```

### Samsung Watch

```json
{
  "version": "1.0",
  "name": "Galaxy Watch",
  "type": "advertisement",
  "adv_data": {
    "flags": "0x06",
    "manufacturer_data": {
      "company_id": "0x0075",
      "data": "0001000200010100FF00004301"
    }
  }
}
```

## Limitations

- Scanner can hold limited devices in memory (depends on available RAM)
- Payload files must be valid JSON format
- Only advertisement packets are supported (no connection-based operations)
- Raw payload data takes precedence over structured fields if both present

## Future Enhancements

- Support for scan response data editing
- Configurable spam interval and duration
- Batch spam multiple payloads
- Import/export payload libraries
- MAC address spoofing of specific targets
- Advertisement timing analysis

## Credits

- Based on Bruce firmware architecture
- Uses NimBLE-Arduino library (v1.4.3)
- Inspired by existing BLE spam functionality in Bruce

## License

Same as Bruce firmware - follows the parent project's license.
