# MT08 SDK Protocol Documentation

## Overview

This document describes the communication protocol between the MT08 EEG hardware and the SDK.

---

## 1. Protocol Architecture

### Data Flow

```
Hardware (ADS1299)
    �� [36-byte packets @ 250Hz]
BLE Communication Layer (BLEComm)
    �� [Raw byte stream]
Protocol Manager (ProtocolManager)
    ��
Processor (Buffering & Packet Assembly)
    �� [Complete 36-byte packets]
Parser (Data Conversion)
    �� [Voltage values in ?V]
Upper Layer Application (ampGetData/ampGetImpedanceData)
```

### Module Responsibilities

| Module | Responsibility |
|--------|---------------|
| **BLEComm** | BLE communication, raw data reception |
| **ProtocolManager** | Coordinate data flow, manage components |
| **Processor** | Buffer raw bytes, extract complete packets |
| **Parser** | Validate and parse packets, convert to voltage |
| **ImpedanceUtil** | Calculate impedance using Goertzel algorithm |

---

## 2. Command Protocol (SDK �� Hardware)

### Format
```
AE 12 02 XX
��  ��  ��  ���� Command byte
��  ��  ���������� Protocol version (0x02)
��  ���������������� Header (0x12)
���������������������� Header (0xAE)
```

### Commands

| Command | Byte Sequence | Description |
|---------|---------------|-------------|
| Start EEG | `AE 12 02 10` | Start normal EEG data streaming |
| Start Impedance | `AE 12 02 11` | Start AC impedance measurement mode |
| Stop | `AE 12 02 12` | Stop data acquisition |

---

## 3. Data Protocol (Hardware �� SDK)

### Packet Structure (36 bytes)

```
Offset | Length | Field | Description
-------|--------|-------|-------------
0-1    | 2      | Header | 0x02 0x10 (EEG) or 0x02 0x11 (Impedance)
2-5    | 4      | Timestamp | 32-bit timestamp (big-endian)
6-9    | 4      | Sequence | 32-bit sample sequence number (big-endian)
10-33  | 24     | ADC Data | 8 channels �� 3 bytes (24-bit, big-endian)
34-35  | 2      | Tail | 0xAE 0x12
```

### Packet Types

- **0x02 0x10**: Normal EEG data mode
- **0x02 0x11**: AC impedance measurement mode

---

## 4. Data Conversion

### Step 1: 24-bit ADC to Signed Integer

Each channel's 3 bytes are converted to a 24-bit value in big-endian format:

```cpp
uint32_t raw = (Byte0 << 16) | (Byte1 << 8) | Byte2;
```

Then converted to signed integer using two's complement:

```cpp
if (raw > 0x7FFFFF) {  // MSB is 1 (negative)
    int32_t signedVal = raw - 0x1000000;  // Subtract 2^24
} else {
    int32_t signedVal = raw;
}
```

### Step 2: Convert to Voltage (?V)

Using ADS1299 specifications:
- Reference voltage (V_ref): 4.5V
- Gain: 1
- Resolution: 24-bit

```cpp
LSB = (2 �� V_ref) / (Gain �� (2^24 - 1))
    = (2 �� 4.5V) / (1 �� 16777215)
    �� 0.53644 ?V

Voltage(?V) = signedVal �� LSB
```

---

## 5. EEG Data Output Format

### Single Sample Output (40 bytes)

```
Offset | Length | Field | Format
-------|--------|-------|--------
0-7    | 8      | Sample Counter | uint64_t (little-endian)
8-11   | 4      | Channel 0 | float (voltage in ?V)
12-15  | 4      | Channel 1 | float (voltage in ?V)
16-19  | 4      | Channel 2 | float (voltage in ?V)
20-23  | 4      | Channel 3 | float (voltage in ?V)
24-27  | 4      | Channel 4 | float (voltage in ?V)
28-31  | 4      | Channel 5 | float (voltage in ?V)
32-35  | 4      | Channel 6 | float (voltage in ?V)
36-39  | 4      | Channel 7 | float (voltage in ?V)
```

### Usage Example

```cpp
// Request 10 samples
int bufferSize = 10 * 40;  // 400 bytes
uint8_t buffer[400];
int bytesReceived = ampGetData(handle, buffer, bufferSize, 10);

// Parse first sample
uint64_t counter = *(uint64_t*)&buffer[0];
float ch0 = *(float*)&buffer[8];
float ch1 = *(float*)&buffer[12];
// ...
```

---

## 6. Impedance Measurement

### Theory

The ADS1299 injects a 6nA RMS square wave at 31.25Hz. We measure the voltage response and calculate impedance:

```
Z (��) = V_RMS / I_RMS
```

### Goertzel Algorithm

To extract the 31.25Hz component from the 250Hz sampled signal:

1. **Window Size**: Must be a multiple of 8 (one cycle = 8 samples)
   - Recommended: 248 samples (31 complete cycles)

2. **Coefficient**: 
   ```
   �� = 2�� �� (31.25 / 250)
   coeff = 2 �� cos(��) = ��2 �� 1.414213562
   ```

3. **Algorithm**:
   ```cpp
   s0 = 0, s1 = 0, s2 = 0;
   for (int n = 0; n < 248; n++) {
       s0 = x[n] + coeff �� s1 - s2;
       s2 = s1;
       s1 = s0;
   }
   magnitude = sqrt(s1? + s2? - coeff �� s1 �� s2);
   RMS = magnitude �� ��2 / N;
   ```

4. **Calculate Impedance**:
   ```
   Z (k��) = (V_RMS (?V) �� 10^-6) / (6 �� 10^-9) �� 0.001
   ```

### Impedance Output Format (72 bytes)

```
Offset | Length | Field | Description
-------|--------|-------|-------------
0-3    | 4      | REF | Reference electrode (float, k��)
4-7    | 4      | GND | Ground electrode (float, k��)
8-11   | 4      | CH0 | Channel 0 impedance (float, k��)
12-15  | 4      | Reserved | -1.0
16-19  | 4      | CH1 | Channel 1 impedance (float, k��)
20-23  | 4      | Reserved | -1.0
...    | ...    | ... | (8 channels total)
```

---

## 7. Error Handling

### Error Codes

SDK functions return standard error codes defined in `Amplifier_LIB.h`:

| Code | Constant | Description |
|------|----------|-------------|
| 0 | `AMP_OK` | Success |
| -1 | `AMP_ERR_PARAM` | Invalid parameter |
| -2 | `AMP_ERR_NODEVICE` | Device not found |
| -3 | `AMP_ERR_BUSY` | Device busy |
| -4 | `AMP_ERR_VERSION` | Not supported |

### Logging

The SDK includes a comprehensive logging system:

```cpp
#include "ErrorHandler.h"

// Initialize logger
sdk::Logger::Initialize(sdk::LogLevel::INFO, true, true, "sdk.log");

// Use logger
sdk::Logger::Info("Operation successful");
sdk::Logger::Error(sdk::ErrorCategory::PROTOCOL, "Parse error");
```

---

## 8. Thread Safety

### Thread-Safe Components

- **Processor**: Uses mutexes to protect internal buffer
- **ProtocolManager**: Thread-safe data reception via `processData()`

### Usage Pattern

```cpp
// BLE callback thread
void OnBleDataReceived(uint8_t* data, size_t len) {
    protocolManager.processData(data, len);  // Thread-safe
}

// Application thread
uint8_t buffer[4000];
int bytes = ampGetData(handle, buffer, sizeof(buffer), 100);
```

---

## 9. Performance Considerations

### Buffer Management

- **Processor buffer**: Automatically compacts when exceeding 2048 bytes
- **Timeout**: 500ms for data waiting

### Memory Allocation

- Pre-allocate buffers where possible
- Use `std::vector::reserve()` to avoid reallocation

### Sampling Rate

- Default: 250 Hz
- One packet every 4ms
- Buffer should handle burst reception

---

## 10. Best Practices

### Initialization

```cpp
// 1. Enumerate devices
int deviceCount = ampEnumerateDevices("BT", 16, nullptr, 0);

// 2. Open device
HANDLE device;
ampOpenDevice(0, &device);

// 3. Set properties
float sampleRate = 250.0f;
ampSetProperty(device, PG_DEVICE, 0, DPROP_F32_BaseSampleRate, 
               &sampleRate, sizeof(sampleRate));

// 4. Start acquisition
ampStartAcquisition(device);
```

### Data Acquisition

```cpp
const int SAMPLES_PER_READ = 50;  // ~200ms @ 250Hz
const int BUFFER_SIZE = SAMPLES_PER_READ * 40;
uint8_t buffer[BUFFER_SIZE];

while (acquiring) {
    int bytes = ampGetData(device, buffer, BUFFER_SIZE, SAMPLES_PER_READ);
    if (bytes > 0) {
        processData(buffer, bytes);
    }
}
```

### Impedance Measurement

```cpp
// 1. Set impedance mode
int32_t mode = RM_IMPEDANCE;
ampSetProperty(device, PG_DEVICE, 0, DPROP_I32_RecordingMode, 
               &mode, sizeof(mode));

// 2. Start acquisition
ampStartAcquisition(device);

// 3. Wait for data accumulation (~1 second)
Sleep(1200);

// 4. Get impedance
float impedances[18];  // 2 + 8��2
ampGetImpedanceData(device, impedances, sizeof(impedances));

// 5. Process results
for (int ch = 0; ch < 8; ch++) {
    float imp = impedances[2 + ch * 2];
    if (imp > 0) {
        printf("Channel %d: %.2f k��\n", ch, imp);
    }
}
```

### Cleanup

```cpp
ampStopAcquisition(device);
ampCloseDevice(device);
sdk::Logger::Shutdown();
```

---

## 11. Troubleshooting

### No Data Received

- Check BLE connection status
- Verify command sent successfully
- Enable debug logging to trace data flow

### Packet Parse Errors

- Check for correct packet markers (0x02 0x10/0x11, 0xAE 0x12)
- Verify packet length is exactly 36 bytes
- Enable logging to see raw packet data

### Impedance Errors

- Ensure at least 248 samples collected
- Verify signal frequency is 31.25Hz
- Check electrode contact quality

### Performance Issues

- Reduce logging level in production
- Pre-allocate buffers
- Process data in batches

---

## 12. Version History

| Version | Date | Changes |
|---------|------|---------|
| 1.0 | 2026-01-21 | Initial protocol documentation |
|  |  | - 36-byte packet format |
|  |  | - Goertzel impedance calculation |
|  |  | - Unified error handling |

---

## Appendix A: Constants Reference

```cpp
// Packet structure
constexpr size_t NEW_PACKET_TOTAL_SIZE = 36;
constexpr size_t NEW_PACKET_HEADER_SIZE = 10;
constexpr size_t NEW_PACKET_ADC_SIZE = 24;
constexpr size_t SINGLE_SAMPLE_OUTPUT_SIZE = 40;

// Hardware parameters
constexpr double ADS1299_VREF = 4500.0;  // mV
constexpr double ADS1299_GAIN = 1.0;
constexpr double ADS1299_LSB_UV = 0.53644;  // ?V

// Impedance measurement
constexpr double IMP_CURRENT_AMPS = 6.0e-9;  // 6nA
constexpr double IMP_SIGNAL_FREQ = 31.25;  // Hz
constexpr int IMP_WINDOW_SIZE = 248;  // samples
```
