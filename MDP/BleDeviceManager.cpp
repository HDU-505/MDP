# include"pch.h"
# include "BleDeviceManager.h"

using namespace std;

BleDeviceManager::BleDeviceManager(protocol::ProtocolManager* protocolManager)
    :protocolManager(protocolManager)
{
   
}

void BleDeviceManager::addDevice(const char* id)
{
    if (!id) return;

    std::string sid(id);

    auto it = std::find(bleDeviceList.begin(), bleDeviceList.end(), sid);
    if (it != bleDeviceList.end())
        return;

    bleDeviceList.emplace_back(std::move(sid));
}


int BleDeviceManager::searchDevice()
{
	if (scanning) {
		return AMP_ERR_BUSY;
	}
	
	//// ��������ɨ��
	scanning = true;
	ScanBLEDevice(10000);

	// �ȴ�ɨ�����
	{
        std::unique_lock<std::mutex> lock(scanMtx);
        while (!scanFinished) {
            scanCv.wait(lock);
        }
	}
	scanFinished = false;
	scanning = false;

	return bleDeviceList.size();
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
    if (recordingMode == RM_NORMAL) {
        vector<uint8_t> startComand = protocolManager->buildPacket(protocol::COMAND_STREAM, protocol::PKT_EEG_DATA_PUSH, protocol::STREAM_EEG);
        if (WriteDateByCharcteristic(DeviceHandle,
            write_ServiceUUID,
            write_CharacteristicUUID,
            startComand.data(),
            startComand.size())) {
            return true;
        }
        return false;
    }
    else if (recordingMode == RM_IMPEDANCE) {
        vector<uint8_t> startComand = protocolManager->buildPacket(protocol::COMAND_STREAM, protocol::PKT_IMPEDANCE_DATA_PUSH, protocol::STREAM_EEG);
        if (WriteDateByCharcteristic(DeviceHandle,
            write_ServiceUUID,
            write_CharacteristicUUID,
            startComand.data(),
            startComand.size())) {
            return true;
        }
        return false;
    }
}

bool BleDeviceManager::stopAcquisition(HANDLE DeviceHandle)
{
    vector<uint8_t> stopComand = protocolManager->buildPacket(protocol::COMAND_STREAM, protocol::PKT_STOP_STREAM, protocol::STREAM_EEG);
    if (WriteDateByCharcteristic(DeviceHandle,
        write_ServiceUUID,
        write_CharacteristicUUID,
        stopComand.data(),
        stopComand.size())) {
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
