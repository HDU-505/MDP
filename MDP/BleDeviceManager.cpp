# include"pch.h"
# include "BleDeviceManager.h"

using namespace std;

BleDeviceManager::BleDeviceManager(protocol::ProtocolManager* protocol)
    :protocol(protocol)
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

    // ��ȡBLE���еķ���UUID
    unsigned int UUIDArry[10];
    unsigned int ArryCount = 0;

    GetAllServersUUID(handle, UUIDArry, &ArryCount);
    // ��ӡ���л�ȡ���� UUID
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

