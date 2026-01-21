#pragma once

#include <cstdint>
#include <mutex>
#include <string>

/**
 * @brief Hardware Configuration Structure
 * 
 * This structure maintains the hardware configuration state.
 * All properties can be set via SDK but will only take effect
 * when corresponding hardware commands are implemented.
 */
struct HardwareConfig {
    // Device identification (set when device is opened)
    std::string deviceType = "Mindtooth";           // Device type
    std::string deviceSerialNumber = "00000";       // Device serial number
    std::string deviceAddress = "00:00:00:00:00:00"; // BLE MAC address
    std::string deviceName = "";                    // Full BLE device name
    
    // Sampling configuration
    float sampleRate = 250.0f;          // Sampling rate in Hz
    float subSampleDivisor = 1.0f;      // Sub-sampling divisor
    
    // Channel configuration (per channel)
    struct ChannelConfig {
        float gain = 1.0f;              // Amplifier gain
        float highPassHz = 0.0f;        // High-pass filter cutoff (Hz)
        float lowPassHz = 0.0f;         // Low-pass filter cutoff (Hz)
        float notchHz = 0.0f;           // Notch filter frequency (Hz)
        bool enabled = true;            // Channel recording enabled
        bool impedanceMeasurement = true; // Enable impedance measurement
    };
    ChannelConfig channels[8];          // 8 channel configurations
    
    // Device status (read-only)
    int32_t batteryLevel = 0;           // Battery level (0-100)
    float batteryVoltage = 0.0f;        // Battery voltage
    int32_t signalQuality = 0;          // Signal quality indicator
    int32_t signalStrength = 0;         // Signal strength
    
    // Impedance measurement settings
    int32_t goodImpedanceLevel = 10;   // Good impedance threshold (kOhm)
    int32_t badImpedanceLevel = 50;    // Bad impedance threshold (kOhm)
    bool continuousImpedance = false;  // Continuous impedance monitoring
    
    // LED control
    int32_t ledControl = 0;             // LED control mode
    
    // Mutex for thread-safe access
    mutable std::mutex configMutex;
    
    /**
     * @brief Get channel configuration
     */
    ChannelConfig GetChannelConfig(uint32_t channelIndex) {
        std::lock_guard<std::mutex> lock(configMutex);
        if (channelIndex < 8) {
            return channels[channelIndex];
        }
        return ChannelConfig();
    }
    
    /**
     * @brief Set channel configuration
     */
    bool SetChannelConfig(uint32_t channelIndex, const ChannelConfig& config) {
        std::lock_guard<std::mutex> lock(configMutex);
        if (channelIndex < 8) {
            channels[channelIndex] = config;
            return true;
        }
        return false;
    }
    
    /**
     * @brief Get sampling rate
     */
    float GetSampleRate() {
        std::lock_guard<std::mutex> lock(configMutex);
        return sampleRate;
    }
    
    /**
     * @brief Set sampling rate
     * @note This will be applied to hardware when command is implemented
     */
    bool SetSampleRate(float rate) {
        std::lock_guard<std::mutex> lock(configMutex);
        // Validate sample rate
        if (rate == 125.0f || rate == 250.0f || rate == 500.0f) {
            sampleRate = rate;
            return true;
        }
        return false;
    }
    
    /**
     * @brief Set device information
     * Called when device is opened
     */
    void SetDeviceInfo(const std::string& name, const std::string& mac) {
        std::lock_guard<std::mutex> lock(configMutex);
        deviceName = name;
        deviceAddress = mac;
        
        // Parse device type and serial number from name
        // Format: "Mindtooth 10001-1"
        size_t spacePos = name.find(' ');
        if (spacePos != std::string::npos) {
            deviceType = name.substr(0, spacePos);
            if (spacePos + 1 < name.length()) {
                deviceSerialNumber = name.substr(spacePos + 1);
            }
        } else {
            deviceType = name.empty() ? "Mindtooth" : name;
            deviceSerialNumber = "00000";
        }
    }
    
    /**
     * @brief Get device information (thread-safe)
     */
    std::string GetDeviceType() const {
        std::lock_guard<std::mutex> lock(configMutex);
        return deviceType;
    }
    
    std::string GetDeviceSerialNumber() const {
        std::lock_guard<std::mutex> lock(configMutex);
        return deviceSerialNumber;
    }
    
    std::string GetDeviceAddress() const {
        std::lock_guard<std::mutex> lock(configMutex);
        return deviceAddress;
    }
};

/**
 * @brief Global hardware configuration instance
 */
extern HardwareConfig g_HardwareConfig;
