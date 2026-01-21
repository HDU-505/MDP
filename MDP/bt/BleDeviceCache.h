#pragma once

#include <vector>
#include <string>
#include <mutex>
#include <fstream>
#include <algorithm>

/**
 * @brief BLE Device Cache Manager
 * 
 * Manages a cache of known BLE device addresses for faster reconnection.
 * Supports multiple devices of the same product type.
 */
class BleDeviceCache {
private:
    std::vector<std::string> cachedDevices;
    mutable std::mutex cacheMutex;
    std::string cacheFilePath;
    const size_t maxCacheSize = 10;  // Maximum number of cached devices

public:
    /**
     * @brief Constructor
     * @param cachePath Path to cache file (default: "ble_device_cache.txt")
     */
    explicit BleDeviceCache(const std::string& cachePath = "ble_device_cache.txt")
        : cacheFilePath(cachePath)
    {
        loadFromFile();
    }

    /**
     * @brief Add device to cache
     * @param deviceId Device ID/Address to cache
     */
    void addDevice(const std::string& deviceId) {
        if (deviceId.empty()) return;

        std::lock_guard<std::mutex> lock(cacheMutex);

        // Check if already in cache
        auto it = std::find(cachedDevices.begin(), cachedDevices.end(), deviceId);
        if (it != cachedDevices.end()) {
            // Move to front (most recently used)
            cachedDevices.erase(it);
            cachedDevices.insert(cachedDevices.begin(), deviceId);
        }
        else {
            // Add new device at front
            cachedDevices.insert(cachedDevices.begin(), deviceId);
            
            // Keep cache size limited
            if (cachedDevices.size() > maxCacheSize) {
                cachedDevices.pop_back();
            }
        }

        saveToFile();
    }

    /**
     * @brief Remove device from cache
     * @param deviceId Device ID/Address to remove
     */
    void removeDevice(const std::string& deviceId) {
        std::lock_guard<std::mutex> lock(cacheMutex);

        auto it = std::find(cachedDevices.begin(), cachedDevices.end(), deviceId);
        if (it != cachedDevices.end()) {
            cachedDevices.erase(it);
            saveToFile();
        }
    }

    /**
     * @brief Get all cached devices
     * @return Vector of cached device IDs
     */
    std::vector<std::string> getCachedDevices() const {
        std::lock_guard<std::mutex> lock(cacheMutex);
        return cachedDevices;
    }

    /**
     * @brief Check if device is in cache
     * @param deviceId Device ID to check
     * @return true if device is cached
     */
    bool isDeviceCached(const std::string& deviceId) const {
        std::lock_guard<std::mutex> lock(cacheMutex);
        return std::find(cachedDevices.begin(), cachedDevices.end(), deviceId) 
               != cachedDevices.end();
    }

    /**
     * @brief Get number of cached devices
     */
    size_t getCacheSize() const {
        std::lock_guard<std::mutex> lock(cacheMutex);
        return cachedDevices.size();
    }

    /**
     * @brief Clear all cached devices
     */
    void clear() {
        std::lock_guard<std::mutex> lock(cacheMutex);
        cachedDevices.clear();
        saveToFile();
    }

private:
    /**
     * @brief Load cache from file
     */
    void loadFromFile() {
        std::ifstream file(cacheFilePath);
        if (!file.is_open()) {
            return;  // File doesn't exist yet, that's OK
        }

        std::string line;
        while (std::getline(file, line)) {
            if (!line.empty()) {
                cachedDevices.push_back(line);
            }
        }
        file.close();

        // Limit size
        if (cachedDevices.size() > maxCacheSize) {
            cachedDevices.resize(maxCacheSize);
        }
    }

    /**
     * @brief Save cache to file
     */
    void saveToFile() const {
        std::ofstream file(cacheFilePath);
        if (!file.is_open()) {
            return;  // Can't write, just skip
        }

        for (const auto& deviceId : cachedDevices) {
            file << deviceId << std::endl;
        }
        file.close();
    }
};
