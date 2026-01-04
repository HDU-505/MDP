#pragma once
#include "bt/BleHandle.h"
#include "bt/BLEComm.h"
#include "ProtocolManager.h"

class BleDeviceManager {

private:
	std::vector<std::string> bleDeviceList;
	protocol::ProtocolManager* protocol;

	HANDLE bleHandle = nullptr;
	// 写特性UUID
	unsigned int write_ServiceUUID;
	unsigned int write_CharacteristicUUID;

	// 读特征UUID
	unsigned int read_ServiceUUID;
	unsigned int read_CharacteristicUUID;

public:
	// 状态变量
	std::mutex scanMtx;
	std::condition_variable scanCv;
	bool scanFinished = false;
	bool scanning = false;
public:
	BleDeviceManager(protocol::ProtocolManager* protocol);

	// 添加设备
	void addDevice(const char* id);

	// 开始扫描
	int searchDevice();

	// 打开设备
	HANDLE openDevice(int32_t DeviceNr);

};