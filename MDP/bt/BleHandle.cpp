#include "BleHandle.h"
#include "BLEComm.h"
map<string, BleHandle*>Pens;
BleDeviceRecvDataCallBack* OnRecvDataCallBack = NULL;
ScanedBleDeviceCallBack* OnScanedBleDeviceCallBack = NULL;
SacnBleDeviceFinishCallBack* OnSacnFinishCallBack = NULL;
ConnectionBleDeviceStatusCallBack* OnConnectionStatusCallBack = NULL;

void Characteristic_ValueChanged(GattCharacteristic const& characteristic, GattValueChangedEventArgs args);
void ConnectionStatus_ValueChanged(BluetoothLEDevice device, winrt::Windows::Foundation::IInspectable const& args);


BleHandle::BleHandle() {
	IsEnd = false;
}
BleHandle::~BleHandle() {

}


DWORD WINAPI ConnectBLEDeviceThread(LPVOID lpParameter) {
	BleHandle* pHandle = (BleHandle*)lpParameter;
	try
	{
		pHandle->IsEnd = false;
		WCHAR ID[255] = { 0 };
		ConvertCharToLPWSTR((char*)(pHandle->ID), ID);
		hstring hst(ID);
		pHandle->device = BluetoothLEDevice::FromIdAsync(hst).get();
		pHandle->device.ConnectionStatusChanged(ConnectionStatus_ValueChanged);
		pHandle->result = pHandle->device.GetGattServicesAsync(BluetoothCacheMode::Uncached).get();
		pHandle->services = pHandle->result.Services();
		pHandle->IsEnd = true;
		return 0;
	}
	catch (...)
	{
		pHandle->IsEnd = true;
		pHandle->services = nullptr;
		return false;
		//co_return false;
	}
}

bool BleHandle::ConnectBLEDevice() {
	CreateThread(NULL, 0, ConnectBLEDeviceThread, (LPVOID)this, 0, NULL);
	while (!IsEnd)
	{
		Sleep(50);
	}
	if (services == nullptr) return false;
	return true;

}

void BleHandle::GetAllServersUUID(unsigned int* UUIDArry, unsigned int* ArryCount) {

	;//获取特征句柄
	try
	{
		*ArryCount = 0;
		for (int i = 0; i < 20 && services == NULL; i++)
		{
			Sleep(100);
		}
		if (services == nullptr) return;
		//auto services = result.Services();
		for (size_t i = 0; i < services.Size(); i++)
		{
			auto service = services.GetAt(i);
			auto uuid = service.Uuid(); //获取服务uuid
			UUIDArry[*ArryCount] = uuid.Data1;
			(*ArryCount)++;
			map<unsigned int, ServiceInfo*>::iterator it = ServicesInfo.find(uuid.Data1);
			if (it == ServicesInfo.end()) {
				ServiceInfo* info = new ServiceInfo();
				info->uuid = uuid.Data1;
				info->Service = service;
				ServicesInfo.insert(pair<unsigned int, ServiceInfo*>(uuid.Data1, info));
			}
		}
	}
	catch (...)
	{

	}

}


void BleHandle::GetCharcteristicByUUID(unsigned int ServiceUUID, unsigned int* UUIDArry, unsigned int* ArryCount) {

	try
	{
		(*ArryCount) = 0;
		map<unsigned int, ServiceInfo*>::iterator it = ServicesInfo.find(ServiceUUID);

		if (it == ServicesInfo.end()) return;
		auto charact = it->second->Service.GetCharacteristicsAsync().get();
		auto characts = charact.Characteristics();

		for (size_t j = 0; j < characts.Size(); j++)
		{
			auto charact = characts.GetAt(j);
			auto uuid = charact.Uuid(); //获取子服务的uuid
			UUIDArry[*ArryCount] = uuid.Data1;
			(*ArryCount)++;
			map<unsigned int, CharacteristicInfo*>::iterator cit = it->second->CharacteristicsInfo.find(uuid.Data1);
			if (cit == it->second->CharacteristicsInfo.end()) {
				CharacteristicInfo* info = new CharacteristicInfo();
				info->uuid = uuid.Data1;
				info->characteristic = characts.GetAt(j);
				info->AuthorityInfo.IsNotify = false;
				info->AuthorityInfo.IsRead = false;
				info->AuthorityInfo.IsWrite = false;
				it->second->CharacteristicsInfo.insert(pair<unsigned int, CharacteristicInfo*>(uuid.Data1, info));
			}
		}
	}
	catch (...)
	{

	}

}

void BleHandle::GetCharcteristicAction(unsigned int ServiceUUID, unsigned int CharacteristicUUID, bool* IsRead, bool* IsWrite, bool* IsNotify) {
	try
	{

		map<unsigned int, ServiceInfo*>::iterator it = ServicesInfo.find(ServiceUUID);
		if (it == ServicesInfo.end()) return;

		map<unsigned int, CharacteristicInfo*>::iterator cit = it->second->CharacteristicsInfo.find(CharacteristicUUID);
		if (cit == it->second->CharacteristicsInfo.end()) return;
		*IsRead = false;
		*IsWrite = false;
		*IsNotify = false;

		auto GAttpro = cit->second->characteristic.CharacteristicProperties();
		/*if (GAttpro == GattCharacteristicProperties::Notify) {
			*IsNotify = true;
			cit->second->AuthorityInfo.IsNotify = true;
		}

		if (GAttpro == GattCharacteristicProperties::Write) {
			*IsWrite = true;
			cit->second->AuthorityInfo.IsWrite = true;
		}

		if (GAttpro == GattCharacteristicProperties::Read) {
			*IsRead = true;
			cit->second->AuthorityInfo.IsRead = true;
		}


		if (GAttpro == (GattCharacteristicProperties::Write | GattCharacteristicProperties::WriteWithoutResponse)) {
			*IsWrite = true;
			cit->second->AuthorityInfo.IsWrite = true;
		}

		if (GAttpro == (GattCharacteristicProperties::Notify | GattCharacteristicProperties::Write)) {
			*IsNotify = true;
			*IsWrite = true;
			cit->second->AuthorityInfo.IsNotify = true;
			cit->second->AuthorityInfo.IsWrite = true;
		}

		if (GAttpro == (GattCharacteristicProperties::Notify | GattCharacteristicProperties::Read)) {
			*IsNotify = true;
			*IsRead = true;
			cit->second->AuthorityInfo.IsNotify = true;
			cit->second->AuthorityInfo.IsRead = true;
		}

		if (GAttpro == (GattCharacteristicProperties::Write | GattCharacteristicProperties::Read)) {
			*IsRead = true;
			*IsWrite = true;
			cit->second->AuthorityInfo.IsRead = true;
			cit->second->AuthorityInfo.IsWrite = true;
		}*/
		// 按位与运算来判断 Notify 权限
		if ((static_cast<uint32_t>(GAttpro) & static_cast<uint32_t>(GattCharacteristicProperties::Notify)) != 0) {
			*IsNotify = true;
			cit->second->AuthorityInfo.IsNotify = true;
		}

		// 按位与运算来判断 Write 权限
		if ((static_cast<uint32_t>(GAttpro) & static_cast<uint32_t>(GattCharacteristicProperties::Write)) != 0) {
			*IsWrite = true;
			cit->second->AuthorityInfo.IsWrite = true;
		}

		// 按位与运算来判断 Read 权限
		if ((static_cast<uint32_t>(GAttpro) & static_cast<uint32_t>(GattCharacteristicProperties::Read)) != 0) {
			*IsRead = true;
			cit->second->AuthorityInfo.IsRead = true;
		}

		// 按位与运算来判断 WriteWithoutResponse 权限
		if ((static_cast<uint32_t>(GAttpro) & static_cast<uint32_t>(GattCharacteristicProperties::WriteWithoutResponse)) != 0) {
			*IsWrite = true;
			cit->second->AuthorityInfo.IsWrite = true;
		}
		return;
	}
	catch (...)
	{

	}


}


bool BleHandle::WriteDateByCharcteristic(unsigned int ServiceUUID, unsigned int CharacteristicUUID, unsigned char* buff, unsigned int lenght) {
	try {
		map<unsigned int, ServiceInfo*>::iterator it = ServicesInfo.find(ServiceUUID);
		if (it == ServicesInfo.end()) return false;

		map<unsigned int, CharacteristicInfo*>::iterator cit = it->second->CharacteristicsInfo.find(CharacteristicUUID);
		if (cit == it->second->CharacteristicsInfo.end()) return false;

		if (cit->second->AuthorityInfo.IsWrite == false) return false;

		winrt::Windows::Storage::Streams::DataWriter writer;
		writer.WriteBytes(array_view<uint8_t const>(buff, buff + lenght));
		winrt::Windows::Storage::Streams::IBuffer buffer = writer.DetachBuffer();
		auto status = cit->second->characteristic.WriteValueAsync(buffer);
		if (status.Status() == Windows::Foundation::AsyncStatus::Error) return false;
		return true;
	}
	catch (...)
	{
		return false;
	}

}

void BleHandle::ReadDataByCharcteristic(unsigned int ServiceUUID, unsigned int CharacteristicUUID) {
	map<unsigned int, ServiceInfo*>::iterator it = ServicesInfo.find(ServiceUUID);
	if (it == ServicesInfo.end()) return;

	map<unsigned int, CharacteristicInfo*>::iterator cit = it->second->CharacteristicsInfo.find(CharacteristicUUID);
	if (cit == it->second->CharacteristicsInfo.end()) return;
	auto result = cit->second->characteristic.ReadValueAsync().get();
	auto status = result.Status();
	if (status != GattCommunicationStatus::Success) return;
	auto value = result.Value();
	if (OnRecvDataCallBack != NULL) {
		OnRecvDataCallBack(it->second, ServiceUUID, CharacteristicUUID, value.data(), value.Length());
	}
}


void BleHandle::RegisterReadNotify(unsigned int ServiceUUID, unsigned int CharacteristicUUID) {

	try {
		map<unsigned int, ServiceInfo*>::iterator it = ServicesInfo.find(ServiceUUID);
		if (it == ServicesInfo.end()) return;

		map<unsigned int, CharacteristicInfo*>::iterator cit = it->second->CharacteristicsInfo.find(CharacteristicUUID);
		if (cit == it->second->CharacteristicsInfo.end()) return;

		if (cit->second->AuthorityInfo.IsNotify == false) return;

		GattClientCharacteristicConfigurationDescriptorValue cccdValue = GattClientCharacteristicConfigurationDescriptorValue::None;
		if ((cit->second->characteristic.CharacteristicProperties() & GattCharacteristicProperties::Indicate) != GattCharacteristicProperties::None)
		{
			cccdValue = GattClientCharacteristicConfigurationDescriptorValue::Indicate;
		}

		else if ((cit->second->characteristic.CharacteristicProperties() & GattCharacteristicProperties::Notify) != GattCharacteristicProperties::None)
		{
			cccdValue = GattClientCharacteristicConfigurationDescriptorValue::Notify;
		}

		auto statuss = cit->second->characteristic.WriteClientCharacteristicConfigurationDescriptorAsync(cccdValue);

		for (int i = 0; i < 10; i++)
		{
			Sleep(20);
		}
		if (statuss.Status() == Windows::Foundation::AsyncStatus::Error)
		{
			return;
		}
		auto status = statuss.get();
		if (status != GattCommunicationStatus::Success) return;

		cit->second->revoker = cit->second->characteristic.ValueChanged(auto_revoke, &Characteristic_ValueChanged);
	}
	catch (...) {

	}

}

void BleHandle::CloseBLEDevice() {
	try
	{
		if (device == nullptr) return;

		device.Close();
	}
	catch (...)
	{

	}

	map<unsigned int, ServiceInfo*>::iterator it = ServicesInfo.begin();
	for (it = ServicesInfo.begin(); it != ServicesInfo.end(); it++)
	{

		it->second->clear();
		delete(it->second);
		it->second = NULL;
	}
	ServicesInfo.clear();
}

void Characteristic_ValueChanged(GattCharacteristic const& characteristic, GattValueChangedEventArgs args)
{
	auto Device = characteristic.Service().Device();
	auto id = Device.BluetoothDeviceId(); //获取蓝牙唯一id
	char ID[100] = { 0 };
	char Address[100] = { 0 };
	ConvertLPWSTRToChar(id.Id().c_str(), (unsigned char*)ID);

	map<string, BleHandle*>::iterator it = Pens.find(ID);
	if (it == Pens.end()) return;

	if (OnRecvDataCallBack != NULL) {
		OnRecvDataCallBack(it->second, characteristic.Service().Uuid().Data1, characteristic.Uuid().Data1, args.CharacteristicValue().data(), args.CharacteristicValue().Length());
	}

}

void ConnectionStatus_ValueChanged(BluetoothLEDevice device, winrt::Windows::Foundation::IInspectable const& args) {
	auto id = device.BluetoothDeviceId(); //获取蓝牙唯一id
	char ID[100] = { 0 };
	char Address[100] = { 0 };
	ConvertLPWSTRToChar(id.Id().c_str(), (unsigned char*)ID);

	map<string, BleHandle*>::iterator it = Pens.find(ID);
	if (it == Pens.end()) return;

	if (OnConnectionStatusCallBack != NULL) {
		if (device.ConnectionStatus() == BluetoothConnectionStatus::Connected) OnConnectionStatusCallBack(it->second, it->second->Address, true);
		if (device.ConnectionStatus() == BluetoothConnectionStatus::Disconnected) OnConnectionStatusCallBack(it->second, it->second->Address, false);
	}

}
