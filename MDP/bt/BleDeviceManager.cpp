# include "BleDeviceManager.h"
#include "../HardwareConfig.h"
#include <thread>
#include <chrono>

using namespace std;

BleDeviceManager::BleDeviceManager(protocol::ProtocolManager* protocolManager)
    :protocolManager(protocolManager)
{
   
}

void BleDeviceManager::addDevice(const char* id, const char* name, const char* mac)
{
    if (!id) return;

    std::string sid(id);
    std::string sname = name ? name : "";
    std::string smac = mac ? mac : "";

    // Check if already in list
    auto it = std::find(bleDeviceList.begin(), bleDeviceList.end(), sid);
    bool alreadyInList = (it != bleDeviceList.end());
    
    if (!alreadyInList) {
        // Add new device to list
        bleDeviceList.emplace_back(sid);
    }
    
    // Always update device information (name, mac) even if device is already in list
    // This ensures we have the latest information from scan
    DeviceInfo info;
    info.id = sid;
    info.name = sname;
    info.mac = smac;
    deviceInfoMap[sid] = info;
}


int BleDeviceManager::searchDevice(int scanTimeMs, int maxRetries)
{
	
	if (scanning) {
		return AMP_ERR_BUSY;
	}
	
	// Check BLE support first
	if (!BLEIsLowEnergySupported()) {
		std::cerr << "[BleDeviceManager] ERROR: Bluetooth Low Energy not supported or disabled!" << std::endl;
		return 0;
	}
	
	// Clear previous scan results — start with empty list
	bleDeviceList.clear();
	
	// NOTE: Do NOT pre-add cached devices here.
	// Only devices actually discovered by scan callback should be in the list.
	// Cache is used AFTER scan for priority ordering.
	
	// Try multiple scan attempts
	int retryCount = 0;
	
	while (retryCount < maxRetries) {
		
		scanning = true;
		scanFinished = false;
		ScanBLEDevice(scanTimeMs);

		// ==========================================
		// 方案一：扫描找到一个目标设备后立马就结束扫描 (当前已启用)
		// ==========================================
		{
			std::unique_lock<std::mutex> lock(scanMtx);
			sdk::Logger::Info("[BleDeviceManager] Starting active early-stop BLE scan...");
			// 这里的 !scanFinished 就是超时判定：达到 scanTimeMs 时底层会自动设其为 true 并唤醒，保证不会死等
			while (!scanFinished) {
				// 每 50ms 唤醒一次，检查是否已经有目标设备被应用层识别并添加
				if (scanCv.wait_for(lock, std::chrono::milliseconds(50)) == std::cv_status::timeout) {
					if (!bleDeviceList.empty()) {
						sdk::Logger::Info("[BleDeviceManager] Target device found early. Stopping scan to save time.");
						lock.unlock(); // FIX DEADLOCK: Must release scanMtx before waiting on g_scanThread.join()!
						StopScanBLEDevice();
						break;
					}
				}
			}
			if (scanFinished && bleDeviceList.empty()) {
				sdk::Logger::Warning("[BleDeviceManager] Scan timeout reached without finding target device.");
			}
		}

		// ==========================================
		// 方案二：有固定扫描时间的方案 (已通过注释禁用)
		// ==========================================
		/*
		{
			std::unique_lock<std::mutex> lock(scanMtx);
			sdk::Logger::Info("[BleDeviceManager] Starting fixed-time BLE scan...");
			while (!scanFinished) {
				scanCv.wait(lock);
			}
			if (bleDeviceList.empty()) {
				sdk::Logger::Warning("[BleDeviceManager] Fixed-time scan finished, but no target devices found.");
			} else {
				sdk::Logger::Info("[BleDeviceManager] Fixed-time scan finished. Devices found.");
			}
		}
		*/
		
		scanning = false;
		
		// If devices found during scan, stop retrying
		if (!bleDeviceList.empty()) {
			break;
		}
		
		// No devices found, retry after short delay
		retryCount++;
		if (retryCount < maxRetries) {
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
	}
	
	int devicesFound = static_cast<int>(bleDeviceList.size());
	
	if (devicesFound > 0) {
		// Re-order: move cached (previously-used) devices to the front
		// This gives priority to known devices without adding offline ones
		auto cachedDevices = deviceCache.getCachedDevices();
		if (!cachedDevices.empty()) {
			std::vector<std::string> reordered;
			
			// First: cached devices that were actually found in this scan
			for (const auto& cachedId : cachedDevices) {
				auto it = std::find(bleDeviceList.begin(), bleDeviceList.end(), cachedId);
				if (it != bleDeviceList.end()) {
					reordered.push_back(cachedId);
				}
			}
			
			// Then: newly discovered devices (not in cache)
			for (const auto& deviceId : bleDeviceList) {
				auto it = std::find(reordered.begin(), reordered.end(), deviceId);
				if (it == reordered.end()) {
					reordered.push_back(deviceId);
				}
			}
			
			bleDeviceList = std::move(reordered);
		}
		
		// Update cache with currently visible devices
		for (const auto& deviceId : bleDeviceList) {
			deviceCache.addDevice(deviceId);
		}
	}
	
	scanFinished = false;
	return devicesFound;
}


HANDLE BleDeviceManager::openDevice(int32_t DeviceNr)
{
    if (DeviceNr < 0 || DeviceNr >= bleDeviceList.size()) {
        return nullptr;
    }
    const char* deviceId = bleDeviceList[DeviceNr].c_str();

    char* cstr = new char[strlen(deviceId) + 1];

    strcpy_s(cstr, strlen(deviceId) + 1, deviceId);

    HANDLE handle = ConnectBLEDevice(cstr);

    // If connection successful, update cache and hardware config
    if (handle != nullptr) {
        sdk::Logger::Info(std::string("[BleDeviceManager] Successfully connected to ") + deviceId);
        deviceCache.addDevice(deviceId);
        
        // Load device info and update global hardware config
        auto it = deviceInfoMap.find(deviceId);
        if (it != deviceInfoMap.end()) {
            currentDeviceInfo = it->second;
            g_HardwareConfig.SetDeviceInfo(it->second.name, it->second.mac);
        } else {
            // No info available, use defaults
            sdk::Logger::Warning(std::string("[BleDeviceManager] No device info found for ") + deviceId + ", using defaults.");
            currentDeviceInfo.id = deviceId;
            currentDeviceInfo.name = "Mindtooth";
            currentDeviceInfo.mac = "00:00:00:00:00:00";
            g_HardwareConfig.SetDeviceInfo("Mindtooth", "00:00:00:00:00:00");
        }
        
        // Wait for connection to stabilize before registering notify
        // This ensures GATT services are fully ready
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    bleHandle = handle;

    if (handle != nullptr) {
        unsigned int UUIDArry[10];
        unsigned int ArryCount = 0;

        GetAllServersUUID(handle, UUIDArry, &ArryCount);

        for (unsigned int i = 0; i < ArryCount; ++i) {
            if (UUIDArry[i] == 65520) {
                unsigned int UUIDArry_Char[10];
                unsigned int ArryCount_Char = 0;
                GetCharcteristicByUUID(handle, UUIDArry[i], UUIDArry_Char, &ArryCount_Char);

                for (unsigned int j = 0; j < ArryCount_Char; ++j) {
                    bool isread = false;
                    bool iswrite = false;
                    bool isnotify = false;
                    GetCharcteristicAction(handle, UUIDArry[i], UUIDArry_Char[j], &isread, &iswrite, &isnotify);
                    if (UUIDArry_Char[j] == 65521) {
                        read_ServiceUUID = UUIDArry[i];
                        read_CharacteristicUUID = UUIDArry_Char[j];
                        // Register notify immediately after connection is stable
                        RegisterReadNotify(handle, UUIDArry[i], UUIDArry_Char[j]);
                    }
                    else if (UUIDArry_Char[j] == 65522) {
                        write_ServiceUUID = UUIDArry[i];
                        write_CharacteristicUUID = UUIDArry_Char[j];
                    }
                }
            }
        }
    }
    
    return handle;
}


bool BleDeviceManager::startAcquisition(HANDLE DeviceHandle)
{
    // CRITICAL: If mode is changing and we're currently acquiring, 
    // we must stop first to ensure clean mode switch
    bool modeChanged = (lastAcquisitionMode != recordingMode);
    
    if (isAcquiring && modeChanged) {
        // Stop current acquisition
        // Even if stop fails, we should try to proceed with mode switch
        stopAcquisition(DeviceHandle);
        
        // Wait for data stream to stop and buffers to clear
        // This ensures hardware stops sending data before mode switch
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        
        // Clear buffers to remove any residual data from previous mode
        if (protocolManager) {
            protocolManager->clearBuffers();
        }
        
        // Additional wait to ensure hardware is ready
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        isAcquiring = false;
    }
    else if (!isAcquiring && modeChanged) {
        // Mode changed but not currently acquiring
        // Still clear buffers to ensure clean state
        if (protocolManager) {
            protocolManager->clearBuffers();
        }
    }
    
    vector<uint8_t> startCommand;
    
    // Reset drop packet detector to avoid seq carryovers
    protocolManager->resetConnectionState();
    protocolManager->resetStatistics();
    
    // Update protocol manager's recording mode (also switches buffer mode)
    // This should be done AFTER stopping previous acquisition
    protocolManager->setRecordingMode(recordingMode);
    
    if (recordingMode == RM_NORMAL) {
        // Send command: AE 12 02 10 (Normal EEG data mode)
        startCommand = protocolManager->buildPacket(
            protocol::COMAND_STREAM, 
            protocol::PKT_EEG_DATA_PUSH, 
            protocol::STREAM_EEG
        );
    }
    else if (recordingMode == RM_IMPEDANCE) {
        // Send command: AE 12 02 11 (AC impedance mode)
        startCommand = protocolManager->buildPacket(
            protocol::COMAND_STREAM, 
            protocol::PKT_IMPEDANCE_DATA_PUSH, 
            protocol::STREAM_EEG
        );
    }
    else {
        return false;
    }
    
    // Get baseline statistics before sending command
    uint64_t baselinePackets = 0;
    if (protocolManager) {
        auto stats = protocolManager->getStatistics();
        baselinePackets = stats.packetsReceived;
    }
    
    // Send command with robust BLE execution blocking; retry up to 3 times on transmit failures
    const int maxRetries = 3;
    const int retryDelayMs = 100;
    
    for (int retry = 0; retry < maxRetries; ++retry) {
        // Write command to BLE characteristic
        if (WriteDateByCharcteristic(DeviceHandle,
            write_ServiceUUID,
            write_CharacteristicUUID,
            startCommand.data(),
            startCommand.size())) {
            
            // Success: command acknowledged by device. 
            // We no longer block and re-send based on a hardware data timeout.
            // Hardware will begin streaming data in ~1s natively.
            isAcquiring = true;
            lastAcquisitionMode = recordingMode;
            sdk::Logger::Info("[BleDeviceManager] Acquisition command transmitted successfully.");
            return true;
        }
        else {
            // Write strictly failed at BLE stack layer, perform retry delay
            if (retry < maxRetries - 1) {
                sdk::Logger::Warning("[BleDeviceManager] Acquisition command transmit failed. Retrying...");
                std::this_thread::sleep_for(std::chrono::milliseconds(retryDelayMs));
            }
        }
    }
    
    // All retries failed
    return false;
}

bool BleDeviceManager::stopAcquisition(HANDLE DeviceHandle)
{
    // Get baseline packet count before stopping
    uint64_t baselinePackets = 0;
    if (protocolManager) {
        auto stats = protocolManager->getStatistics();
        baselinePackets = stats.packetsReceived;
    }
    
    // Send command: AE 12 02 12 (Stop acquisition)
    vector<uint8_t> stopCommand = protocolManager->buildPacket(
        protocol::COMAND_STREAM, 
        protocol::PKT_STOP_STREAM, 
        protocol::STREAM_EEG
    );
    
    // Send command with retry
    const int maxRetries = 2;
    bool commandSent = false;
    
    for (int retry = 0; retry < maxRetries; ++retry) {
        // Write command to BLE characteristic
        if (WriteDateByCharcteristic(DeviceHandle,
            write_ServiceUUID,
            write_CharacteristicUUID,
            stopCommand.data(),
            stopCommand.size())) {
            commandSent = true;
            break;
        }
        
        if (retry < maxRetries - 1) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }
    
    if (commandSent) {
        // Wait for command to take effect
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Print packet drop report after acquisition is stopped
        if (protocolManager) {
            std::string report = protocolManager->getPacketLossReport();
            sdk::Logger::Info(report);
        }
        
        // Update acquisition state
        isAcquiring = false;
        return true;
    }
    
    // Even if command send failed, update state to avoid stuck state
    isAcquiring = false;
    return false;
}

bool BleDeviceManager::verifyDataReception(uint64_t baselinePackets, int timeoutMs)
{
    if (!protocolManager) {
        return false;
    }
    
    const int checkIntervalMs = 50;
    int waitedMs = 0;
    
    while (waitedMs < timeoutMs) {
        auto stats = protocolManager->getStatistics();
        // Check if new packets have been received
        if (stats.packetsReceived > baselinePackets) {
            return true;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(checkIntervalMs));
        waitedMs += checkIntervalMs;
    }
    
    return false;
}

bool BleDeviceManager::closeDevice(HANDLE DeviceHandle)
{
    // Await device detachment, retry count MaxRetry times at IntervalMs frequency
    const int maxRetry = 5;     // Try 5 times maximum
    const int intervalMs = 20;   // Interval between tries 20ms
    int retry = 0;

    sdk::Logger::Info("[BleDeviceManager] Attempting to close BLE device.");
    {
        CloseBLEDevice(DeviceHandle); // FIX: Ensure this is only called ONCE to avoid Use-After-Free!
        while (retry < maxRetry) {
            std::unique_lock<std::mutex> lock(connMtx);
            connCv.wait_for(lock, std::chrono::milliseconds(intervalMs));
            retry++;
            if (!isConnected) {
                sdk::Logger::Info("[BleDeviceManager] Device disconnected successfully.");
                return true;
            }
        }
    }
    sdk::Logger::Warning("[BleDeviceManager] Device close timeout: OS might still hold the physical connection.");
    return false;
}
