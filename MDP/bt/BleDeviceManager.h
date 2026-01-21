#pragma once
#include "BleHandle.h"
#include "BLEComm.h"
#include "BleDeviceCache.h"
#include "../protocol/ProtocolManager.h"
#include <map>
#include <string>

class BleDeviceManager {

private:
	// Device information storage
	struct DeviceInfo {
		std::string id;
		std::string name;
		std::string mac;
	};
	
	std::vector<std::string> bleDeviceList;
	std::map<std::string, DeviceInfo> deviceInfoMap;  // ID -> DeviceInfo mapping
	DeviceInfo currentDeviceInfo;  // Currently connected device info
	
	protocol::ProtocolManager* protocolManager;
	BleDeviceCache deviceCache;  // Device cache for faster reconnection

	HANDLE bleHandle = nullptr;
	// Write characteristic UUID
	unsigned int write_ServiceUUID;
	unsigned int write_CharacteristicUUID;

	// Read characteristic UUID
	unsigned int read_ServiceUUID;
	unsigned int read_CharacteristicUUID;

public:
	RecordingMode recordingMode = RM_NORMAL;

public:
	// Status variables
	std::mutex scanMtx;
	std::condition_variable scanCv;
	bool scanFinished = false;
	bool scanning = false;

	std::mutex connMtx;
	std::condition_variable connCv;
	bool isConnected = false;

public:
	BleDeviceManager(protocol::ProtocolManager* protocolManager);

	// Add device with full information
	void addDevice(const char* id, const char* name, const char* mac);

	// Start scan with retry and timeout configuration
	int searchDevice(int scanTimeMs = 10000, int maxRetries = 3);

	// Open device
	HANDLE openDevice(int32_t DeviceNr);

	bool startAcquisition(HANDLE DeviceHandle);

	bool stopAcquisition(HANDLE DeviceHandle);

	bool closeDevice(HANDLE DeviceHandle);

	void setRecordingMode(RecordingMode mode) {
		recordingMode = mode;
	}
};
