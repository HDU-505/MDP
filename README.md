# MT08 EEG SDK

Professional EEG data acquisition SDK for MT08 hardware with ADS1299 ADC.

## Features

- **8-channel EEG data acquisition** at 250Hz sampling rate
- **AC impedance measurement** using Goertzel algorithm
- **Real-time data streaming** via Bluetooth Low Energy
- **Comprehensive error handling and logging**
- **Thread-safe data processing**
- **Professional-grade signal processing**

---

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                     Application Layer                        │
│  (ampGetData, ampGetImpedanceData, ampSetProperty, etc.)   │
└────────────────────────┬────────────────────────────────────┘
                         │
┌────────────────────────┴────────────────────────────────────┐
│                    MDP SDK Core                              │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  ProtocolManager (Coordination)                       │  │
│  │    ├─ Processor (Buffering & Packet Assembly)        │  │
│  │    ├─ Parser (Data Conversion)                       │  │
│  │    └─ ImpedanceUtil (Goertzel Algorithm)            │  │
│  └──────────────────────────────────────────────────────┘  │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  BleDeviceManager (Device Management)                │  │
│  └──────────────────────────────────────────────────────┘  │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  PropertyUtil & HardwareConfig (Configuration)        │  │
│  └──────────────────────────────────────────────────────┘  │
└────────────────────────┬────────────────────────────────────┘
                         │
┌────────────────────────┴────────────────────────────────────┐
│                  BLE Communication Layer                      │
│  (BLEComm - Windows BLE API Integration)                    │
└────────────────────────┬────────────────────────────────────┘
                         │
┌────────────────────────┴────────────────────────────────────┐
│                      MT08 Hardware                            │
│  (ADS1299 ADC + BLE Module)                                 │
└──────────────────────────────────────────────────────────────┘
```

---

## Quick Start

### 1. Include Headers

```cpp
#include "Amplifier_LIB.h"
```

### 2. Initialize and Connect

```cpp
// Enumerate devices
int deviceCount = ampEnumerateDevices("BT", 16, nullptr, 0);
if (deviceCount <= 0) {
    // No devices found
    return;
}

// Open device
HANDLE device;
if (ampOpenDevice(0, &device) != AMP_OK) {
    // Failed to open
    return;
}
```

### 3. Configure Device

```cpp
// Set sampling rate
float sampleRate = 250.0f;
ampSetProperty(device, PG_DEVICE, 0, DPROP_F32_BaseSampleRate, 
               &sampleRate, sizeof(sampleRate));

// Set recording mode (normal EEG)
int32_t mode = RM_NORMAL;
ampSetProperty(device, PG_DEVICE, 0, DPROP_I32_RecordingMode, 
               &mode, sizeof(mode));
```

### 4. Start Acquisition

```cpp
if (ampStartAcquisition(device) != AMP_OK) {
    // Failed to start
    return;
}
```

### 5. Read EEG Data

```cpp
const int SAMPLES = 50;  // 200ms of data
const int BUFFER_SIZE = SAMPLES * 40;  // 40 bytes per sample
uint8_t buffer[BUFFER_SIZE];

int bytesRead = ampGetData(device, buffer, BUFFER_SIZE, SAMPLES);
if (bytesRead > 0) {
    // Process data
    int numSamples = bytesRead / 40;
    for (int i = 0; i < numSamples; i++) {
        uint64_t counter = *(uint64_t*)&buffer[i * 40];
        float* channels = (float*)&buffer[i * 40 + 8];
        
        // Process 8 channels
        for (int ch = 0; ch < 8; ch++) {
            float voltage_uV = channels[ch];
            // Your processing here
        }
    }
}
```

### 6. Measure Impedance

```cpp
// Switch to impedance mode
int32_t mode = RM_IMPEDANCE;
ampSetProperty(device, PG_DEVICE, 0, DPROP_I32_RecordingMode, 
               &mode, sizeof(mode));
ampStartAcquisition(device);

// Wait for data accumulation
Sleep(1200);  // ~1 second

// Get impedance values
float impedances[18];  // REF + GND + 8 channels × 2
ampGetImpedanceData(device, impedances, sizeof(impedances));

// Process results
printf("REF: %.2f kΩ\n", impedances[0]);
printf("GND: %.2f kΩ\n", impedances[1]);
for (int ch = 0; ch < 8; ch++) {
    float imp = impedances[2 + ch * 2];
    printf("CH%d: %.2f kΩ\n", ch, imp);
}
```

### 7. Cleanup

```cpp
ampStopAcquisition(device);
ampCloseDevice(device);
```

---

## Data Format

### EEG Data Output (per sample)

| Offset | Size | Field | Type | Unit |
|--------|------|-------|------|------|
| 0-7 | 8 | Sample Counter | uint64_t | - |
| 8-11 | 4 | Channel 0 | float | µV |
| 12-15 | 4 | Channel 1 | float | µV |
| ... | ... | ... | ... | ... |
| 36-39 | 4 | Channel 7 | float | µV |

**Total: 40 bytes per sample**

### Impedance Data Output

| Offset | Size | Field | Unit |
|--------|------|-------|------|
| 0-3 | 4 | REF electrode | kΩ |
| 4-7 | 4 | GND electrode | kΩ |
| 8-11 | 4 | Channel 0 impedance | kΩ |
| 12-15 | 4 | Reserved | - |
| ... | ... | (8 channels × 2 values) | ... |

**Total: 72 bytes**

---

## Protocol

### Command Protocol (SDK → Hardware)

```
AE 12 02 XX

Commands:
- 0x10: Start normal EEG mode
- 0x11: Start impedance mode  
- 0x12: Stop acquisition
```

### Data Protocol (Hardware → SDK)

```
36-byte packet format:
[0-1]   Header: 0x02 0x10/0x11
[2-5]   Timestamp (32-bit, big-endian)
[6-9]   Sequence (32-bit, big-endian)
[10-33] ADC data (8 channels × 3 bytes)
[34-35] Tail: 0xAE 0x12
```

See [PROTOCOL.md](PROTOCOL.md) for detailed protocol specification.

---

## Configuration

### Supported Sample Rates

- 125 Hz
- 250 Hz (default)
- 500 Hz

### Channel Configuration

All 8 channels support:
- Gain settings: 1, 2, 4, 6, 8, 12, 24
- High-pass, low-pass, and notch filters (configurable)
- Individual enable/disable

### Hardware Properties

Get/Set properties using:
```cpp
ampGetProperty(device, propertyGroup, index, propertyID, value, size);
ampSetProperty(device, propertyGroup, index, propertyID, value, size);
```

Property groups:
- `PG_DEVICE`: Device-level properties
- `PG_MODULE`: Module-level properties  
- `PG_CHANNEL`: Per-channel properties

---

## Error Handling

### Return Codes

| Code | Constant | Description |
|------|----------|-------------|
| 0 | `AMP_OK` | Success |
| -1 | `AMP_ERR_PARAM` | Invalid parameter |
| -2 | `AMP_ERR_NODEVICE` | Device not found |
| -3 | `AMP_ERR_BUSY` | Device busy |
| -4 | `AMP_ERR_VERSION` | Feature not supported |

### Logging

Enable SDK logging:

```cpp
#include "ErrorHandler.h"

// Initialize logger (call once at startup)
sdk::Logger::Initialize(
    sdk::LogLevel::INFO,  // Minimum level
    true,                 // Console output
    true,                 // File output
    "sdk.log"            // Log file path
);

// Shutdown (call at exit)
sdk::Logger::Shutdown();
```

Log levels: `DEBUG`, `INFO`, `WARNING`, `ERROR`, `FATAL`

---

## Performance

### Throughput

- **Sampling rate**: 250 Hz
- **Channels**: 8
- **Data rate**: ~10 KB/s (250 samples/s × 40 bytes)
- **Latency**: < 100ms (typical)

### Memory Usage

- **Processor buffer**: ~2-4 KB
- **Parser overhead**: Minimal
- **Total SDK footprint**: < 1 MB

### Recommendations

- Poll data every 100-200ms for smooth operation
- Pre-allocate buffers to avoid reallocation
- Use `std::vector::reserve()` when possible

---

## Thread Safety

### Thread-Safe Operations

- `processData()` - Can be called from BLE callback thread
- `ampGetData()` - Can be called from any thread
- `ampGetImpedanceData()` - Can be called from any thread

### Non-Thread-Safe Operations

- Device enumeration and connection
- Property get/set operations
- Start/stop acquisition

**Recommendation**: Perform device management on main thread only.

---

## Troubleshooting

### No Data Received

1. Check BLE connection status
2. Verify device is in range
3. Enable debug logging: `sdk::Logger::Initialize(sdk::LogLevel::DEBUG, ...)`
4. Check if `ampStartAcquisition()` succeeded

### Incorrect Data Values

1. Verify gain settings are correct
2. Check electrode connections
3. Ensure proper grounding
4. Review sample rate configuration

### Impedance Measurement Issues

1. Ensure electrodes are properly applied
2. Wait at least 1 second before reading
3. Verify measurement mode is set: `RM_IMPEDANCE`
4. Check for 31.25Hz signal injection

### Performance Issues

1. Reduce logging level in production builds
2. Increase polling interval if CPU usage is high
3. Check for buffer overflows in debug log
4. Monitor packet drop rate via statistics

---

## Advanced Features

### Statistics Monitoring

```cpp
// Get protocol statistics
auto stats = protocolManager.getStatistics();
printf("Packets received: %llu\n", stats.packetsReceived);
printf("Packets dropped: %llu\n", stats.packetsDropped);
printf("Drop rate: %.2f%%\n", stats.dropRate * 100);
```

### Buffer Management

```cpp
// Clear internal buffers
protocolManager.clearBuffers();
```

### Custom Signal Processing

Access raw voltage data directly:

```cpp
std::vector<float> channelData;
parser.parseNewEEGPacket2Float(packet, length, channelData);
// channelData now contains 8 voltage values in µV
```

---

## System Requirements

### Minimum Requirements

- Windows 10 or later with BLE support
- Visual Studio 2019 or later (C++17)
- 2 GB RAM
- Bluetooth 4.0 or later

### Recommended Requirements

- Windows 11
- Visual Studio 2022
- 4 GB RAM
- Bluetooth 5.0 or later

---

## License

Copyright © 2026 Mindtooth. All rights reserved.

---

## Support

For technical support and bug reports:
- Email: support@mindtooth.com
- Documentation: [PROTOCOL.md](PROTOCOL.md)

---

## Version

**SDK Version**: 1.0.0  
**Protocol Version**: 1.0  
**Last Updated**: 2026-01-21
