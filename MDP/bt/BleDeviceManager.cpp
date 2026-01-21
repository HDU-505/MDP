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
    if (it != bleDeviceList.end())
        return;

    bleDeviceList.emplace_back(sid);
    
    // Store device information
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
	
	// Try multiple scan attempts if first attempt finds nothing
	int retryCount = 0;
	int devicesFound = 0;
	
	while (retryCount < maxRetries) {
		// Clear previous scan results
		bleDeviceList.clear();
		
		// Start BLE scan
		scanning = true;
		scanFinished = false;
		ScanBLEDevice(scanTimeMs);

		// Wait for scan to complete
		{
			std::unique_lock<std::mutex> lock(scanMtx);
			while (!scanFinished) {
				scanCv.wait(lock);
			}
		}
		
		scanning = false;
		devicesFound = bleDeviceList.size();
		
		// If devices found, success!
		if (devicesFound > 0) {
			// Cache all found devices for faster reconnection next time
			for (const auto& deviceId : bleDeviceList) {
				deviceCache.addDevice(deviceId);
			}
			break;
		}
		
		// No devices found, retry after short delay
		retryCount++;
		if (retryCount < maxRetries) {
			// Short delay before retry (100ms)
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
	}
	
	// If still no devices found after retries, check cache
	if (devicesFound == 0) {
		auto cachedDevices = deviceCache.getCachedDevices();
		if (!cachedDevices.empty()) {
			// Add cached devices to scan list
			// User can try to connect to these even if not currently advertising
			bleDeviceList = cachedDevices;
			devicesFound = cachedDevices.size();
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
        deviceCache.addDevice(deviceId);
        
        // Load device info and update global hardware config
        auto it = deviceInfoMap.find(deviceId);
        if (it != deviceInfoMap.end()) {
            currentDeviceInfo = it->second;
            g_HardwareConfig.SetDeviceInfo(it->second.name, it->second.mac);
        } else {
            // No info available, use defaults
            currentDeviceInfo.id = deviceId;
            currentDeviceInfo.name = "Mindtooth";
            currentDeviceInfo.mac = "00:00:00:00:00:00";
            g_HardwareConfig.SetDeviceInfo("Mindtooth", "00:00:00:00:00:00");
        }
    }

    bleHandle = handle;

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
                    RegisterReadNotify(handle, UUIDArry[i], UUIDArry_Char[j]);
                }
                else if (UUIDArry_Char[j] == 65522) {
                    write_ServiceUUID = UUIDArry[i];
                    write_CharacteristicUUID = UUIDArry_Char[j];
                }
            }
        }
    }
    return handle;
}


bool BleDeviceManager::startAcquisition(HANDLE DeviceHandle)
{
    vector<uint8_t> startCommand;
    
    // Update protocol manager's recording mode (also switches buffer mode)
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
    
    // Write command to BLE characteristic
    if (WriteDateByCharcteristic(DeviceHandle,
        write_ServiceUUID,
        write_CharacteristicUUID,
        startCommand.data(),
        startCommand.size())) {
        return true;
    }
    
    return false;
}

bool BleDeviceManager::stopAcquisition(HANDLE DeviceHandle)
{
    // Send command: AE 12 02 12 (Stop acquisition)
    vector<uint8_t> stopCommand = protocolManager->buildPacket(
        protocol::COMAND_STREAM, 
        protocol::PKT_STOP_STREAM, 
        protocol::STREAM_EEG
    );
    
    // Write command to BLE characteristic
    if (WriteDateByCharcteristic(DeviceHandle,
        write_ServiceUUID,
        write_CharacteristicUUID,
        stopCommand.data(),
        stopCommand.size())) {
        return true;
    }
    
    return false;
}

bool BleDeviceManager::closeDevice(HANDLE DeviceHandle)
{
    // 尝试等待设备断开，最多等待 maxRetry 次，每次间隔 intervalMs 毫秒
    const int maxRetry = 5;     // 最大尝试次数
    const int intervalMs = 20;   // 每次间隔 20ms
    int retry = 0;

    {
        while (retry < maxRetry) {
            CloseBLEDevice(DeviceHandle);
            std::unique_lock<std::mutex> lock(connMtx);
            connCv.wait_for(lock, std::chrono::milliseconds(intervalMs));
            retry++;
            if (!isConnected) {
                return true;
            }
        }
    }
    return false;
}
