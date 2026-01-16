#pragma once
#include "bt/BleHandle.h"
#include "bt/BLEComm.h"
#include "ProtocolManager.h"

class BleDeviceManager {

private:
	std::vector<std::string> bleDeviceList;
	protocol::ProtocolManager* protocolManager;

	HANDLE bleHandle = nullptr;
	// 写特性UUID
	unsigned int write_ServiceUUID;
	unsigned int write_CharacteristicUUID;

	// 读特征UUID
	unsigned int read_ServiceUUID;
	unsigned int read_CharacteristicUUID;

	std::vector<std::vector<float>> sBuffer;
	std::mutex sMtx;
	std::condition_variable sCv;

public:
	// 状态变量
	std::mutex scanMtx;
	std::condition_variable scanCv;
	bool scanFinished = false;
	bool scanning = false;

	std::mutex connMtx;
	std::condition_variable connCv;
	bool isConnected;

	t_RecordingMode recordingMode = RM_STOPPED;


public:
	BleDeviceManager(protocol::ProtocolManager* protocolManager);

	// 添加设备
	void addDevice(const char* id);

	// 开始扫描
	int searchDevice();

	// 打开设备
	HANDLE openDevice(int32_t DeviceNr);

	bool startAcquisition(HANDLE DeviceHandle);

	bool stopAcquisition(HANDLE DeviceHandle);

	bool closeDevice(HANDLE DeviceHandle);

	
	
};