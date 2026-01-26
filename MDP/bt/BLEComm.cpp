#include "BLEComm.h"
#include "BleHandle.h"
#include <map>
#include <string>
using namespace std;

map<uint64_t, int>BleDevices;

BluetoothLEAdvertisementWatcher m_btWatcher;

LPWSTR ConvertCharToLPWSTR(char* szString, WCHAR* addrchar)
{
	int dwLen = strlen(szString) + 1;
	int nwLen = MultiByteToWideChar(CP_ACP, 0, szString, dwLen, NULL, 0);//???????????
	MultiByteToWideChar(CP_ACP, 0, szString, dwLen, addrchar, nwLen);
	return addrchar;
}


unsigned char* ConvertLPWSTRToChar(LPCTSTR widestr, unsigned char* addrchar)
{
	int num = WideCharToMultiByte(CP_OEMCP, NULL, widestr, -1, NULL, 0, NULL, FALSE);
	WideCharToMultiByte(CP_OEMCP, NULL, widestr, -1, (char*)addrchar, num, NULL, FALSE);
	return addrchar;
}



void RegisterRecvBleDevice(ScanedBleDeviceCallBack CallBack)
{
	OnScanedBleDeviceCallBack = CallBack;
}


void RegisterSacnBleDeviceFinish(SacnBleDeviceFinishCallBack CallBack)
{
	OnSacnFinishCallBack = CallBack;
}

void RegisterConnectionBleDeviceStatus(ConnectionBleDeviceStatusCallBack CallBack)
{
	OnConnectionStatusCallBack = CallBack;
}

void RegisterBleDeviceRecvData(BleDeviceRecvDataCallBack CallBack)
{
	OnRecvDataCallBack = CallBack;
}




bool BLEIsLowEnergySupported() {

	auto getadapter_op = Windows::Devices::Bluetooth::BluetoothAdapter::GetDefaultAsync();
	auto adapter = getadapter_op.get();
	auto supported = adapter.IsLowEnergySupported(); // ???windows??????????ble
	if (supported == false) return false;
	auto async = adapter.GetRadioAsync();
	auto radio = async.get();
	auto t = radio.State(); // ????????????? 0????1????2????3??????????
	if (t != winrt::Windows::Devices::Radios::RadioState::On) {
		return false;
	}
	return  true;
}



void Scanblebackfun(BluetoothLEAdvertisementWatcher w, BluetoothLEAdvertisementReceivedEventArgs e) {
	
	if (e.AdvertisementType() == BluetoothLEAdvertisementType::ConnectableUndirected)
	{
		uint64_t address = e.BluetoothAddress();
		auto Rssi = e.RawSignalStrengthInDBm();
		
		
	 	if (BleDevices.find(address) != BleDevices.end()) {
			std::cout << "[BLE] Device already in list, skipping" << std::endl;
			return;
		}
		BleDevices.insert(pair<uint64_t, int>(address, Rssi));
		
		// Get device info
		try {
			BluetoothLEDevice dev = BluetoothLEDevice::FromBluetoothAddressAsync(address).get();
			int cid = 0;
			auto id = dev.BluetoothDeviceId();
			auto name = dev.Name();

			auto advertisement = e.Advertisement();
			auto Datas = advertisement.DataSections();
			auto view = Datas.GetView();
			DataSection DataSections[10];
			for (size_t i = 0; i < view.Size(); i++)
			{
				auto data = Datas.GetAt(i);
				DataSections[i].Data = data.Data().data();
				DataSections[i].Lenght = data.Data().Length();
			}

			dev.Close();

			char ID[MAXBYTE] = { 0 };
			char Name[MAXBYTE] = { 0 };
			char Address[MAXBYTE] = { 0 };

			ConvertLPWSTRToChar(id.Id().c_str(), (unsigned char*)ID);
			ConvertLPWSTRToChar(name.c_str(), (unsigned char*)Name);

			PCHAR mactemp = NULL;
			mactemp = strchr((char*)ID, '-');
			errno_t err = strcpy_s(Address, 100, mactemp + 1);


			OnScanedBleDeviceCallBack(ID,Name, Address, e.RawSignalStrengthInDBm(), DataSections, view.Size());

		}
		catch (const std::exception& ex) {
		}
		//printf("Device : Id :%s	Name:%s address: %s\n", ble->ID, ble->Name, ble->Address);
	}
}


DWORD WINAPI ScanBleThread(LPVOID lpParameter) {
	int timeout = (int)lpParameter;
	
	
	try {
		m_btWatcher.ScanningMode(BluetoothLEScanningMode::Passive);
		m_btWatcher.Received(Scanblebackfun);
		
		m_btWatcher.Start();
		
		for(int i = 0; i< timeout/50;i++)
		{
			Sleep(50);
			if (m_btWatcher.Status() == BluetoothLEAdvertisementWatcherStatus::Stopped) {
				return 0;
			}
		}

		m_btWatcher.Stop();
		
		if (OnSacnFinishCallBack != NULL) {
			OnSacnFinishCallBack();
		}
		else {
		}
	}
	catch (const std::exception& ex) {
	}
	
	return 0;
}


void ScanBLEDevice(int timeout) {
	BleDevices.clear();
	CreateThread(NULL, 0, ScanBleThread,(LPVOID)timeout, 0, NULL);
}

void StopScanBLEDevice()
{
	m_btWatcher.Stop();
}

HANDLE ConnectBLEDevice(char* ID) {

	map<string, BleHandle*>::iterator it = Pens.find(ID);

	if (it == Pens.end()) {
		BleHandle* ble = new BleHandle();
		errno_t err = strncpy_s(ble->ID, sizeof(ble->ID), ID, _TRUNCATE);
		Pens.insert(pair<string, BleHandle*>(ID, ble));
		if (ble->ConnectBLEDevice() == false) return NULL;
		return ble;
	}
	else {
		if (it->second->ConnectBLEDevice() == false) return NULL;
		return it->second;
	}
	
}

void GetAllServersUUID(HANDLE handle, unsigned int* UUIDArry, unsigned int* ArryCount)
{
	BleHandle* ble= (BleHandle*)handle;
	ble->GetAllServersUUID(UUIDArry, ArryCount);
}


void GetCharcteristicByUUID(HANDLE handle, unsigned int ServiceUUID, unsigned int* UUIDArry, unsigned int* ArryCount)
{
	BleHandle* ble = (BleHandle*)handle;
	ble->GetCharcteristicByUUID(ServiceUUID, UUIDArry, ArryCount);
}

void GetCharcteristicAction(HANDLE handle, unsigned int ServiceUUID, unsigned int CharacteristicUUID, bool* IsRead, bool* IsWrite, bool* IsNotify)
{
	BleHandle* ble = (BleHandle*)handle;
	ble->GetCharcteristicAction(ServiceUUID, CharacteristicUUID, IsRead, IsWrite, IsNotify);
}

bool WriteDateByCharcteristic(HANDLE handle, unsigned int ServiceUUID, unsigned int CharacteristicUUID, unsigned char* buff, unsigned int lenght) {
	BleHandle* ble = (BleHandle*)handle;
	return ble->WriteDateByCharcteristic(ServiceUUID, CharacteristicUUID, buff, lenght);
}

void ReadDataByCharcteristic(HANDLE handle, unsigned int ServiceUUID, unsigned int CharacteristicUUID) {
	BleHandle* ble = (BleHandle*)handle;
	ble->ReadDataByCharcteristic(ServiceUUID, CharacteristicUUID);
}

void RegisterReadNotify(HANDLE handle, unsigned int ServiceUUID, unsigned int CharacteristicUUID) {
	BleHandle* ble = (BleHandle*)handle;
	ble->RegisterReadNotify(ServiceUUID, CharacteristicUUID);
}

void CloseBLEDevice(HANDLE handle) {
	BleHandle* ble = (BleHandle*)handle;
	ble->CloseBLEDevice();
	map<string, BleHandle*>::iterator it = Pens.find(ble->ID);
	if (it != Pens.end()) {
		Pens.erase(ble->ID);
	}
	delete(ble);
	ble = NULL;
}

