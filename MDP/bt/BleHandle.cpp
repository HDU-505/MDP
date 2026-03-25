#include "BleHandle.h"
#include "BLEComm.h"
#include "../ErrorHandler.h"
#include <thread>
// Global Definitions
map<string, BleHandle*>Pens;
BleDeviceRecvDataCallBack* OnRecvDataCallBack = NULL;             // Data callback
ScanedBleDeviceCallBack* OnScanedBleDeviceCallBack = NULL;        // Scan device callback
SacnBleDeviceFinishCallBack* OnSacnFinishCallBack = NULL;         // Scan complete
ConnectionBleDeviceStatusCallBack* OnConnectionStatusCallBack = NULL; // Connect stat callback

// Forward definition
void Characteristic_ValueChanged(GattCharacteristic const& characteristic, GattValueChangedEventArgs args);
void ConnectionStatus_ValueChanged(BluetoothLEDevice device, winrt::Windows::Foundation::IInspectable const& args);

BleHandle::BleHandle() {
	IsEnd = false;
}
BleHandle::~BleHandle() {
}

// BLE device connection pool
void ConnectBLEDeviceThreadBody(BleHandle* pHandle) {
	try
	{
		pHandle->IsEnd = false;
		WCHAR ID[255] = { 0 };
		ConvertCharToLPWSTR((char*)(pHandle->ID), ID);
		hstring hst(ID);
		sdk::Logger::Log(sdk::LogLevel::DEBUG, sdk::ErrorCategory::BLUETOOTH, std::string("[BleHandle] Thread started, attempting connection to ID: ") + (char*)(pHandle->ID));
		pHandle->device = BluetoothLEDevice::FromIdAsync(hst).get();
		if (!pHandle->device) {
			sdk::Logger::Log(sdk::LogLevel::DEBUG, sdk::ErrorCategory::BLUETOOTH, "[BleHandle] BluetoothLEDevice::FromIdAsync returned null. OS OS rejected connection or device un-cached.");
			pHandle->IsEnd = true;
			pHandle->services = nullptr;
			return;
		}
		pHandle->device.ConnectionStatusChanged(ConnectionStatus_ValueChanged);
		
		sdk::Logger::Log(sdk::LogLevel::DEBUG, sdk::ErrorCategory::BLUETOOTH, "[BleHandle] Connection handle acquired, querying GATT Services...");
		pHandle->result = pHandle->device.GetGattServicesAsync(BluetoothCacheMode::Cached).get();
		if (pHandle->result.Status() != GattCommunicationStatus::Success) {
			sdk::Logger::Log(sdk::LogLevel::DEBUG, sdk::ErrorCategory::BLUETOOTH, "[BleHandle] GetGattServicesAsync failed. Fake connection or immediate OS drop detected. Closing handle.");
			pHandle->device.Close();
			pHandle->device = nullptr;
			pHandle->IsEnd = true;
			pHandle->services = nullptr;
			return;
		}
		sdk::Logger::Log(sdk::LogLevel::DEBUG, sdk::ErrorCategory::BLUETOOTH, "[BleHandle] Successfully retrieved GATT services. Connection confirmed solid.");
		pHandle->services = pHandle->result.Services();
		pHandle->IsEnd = true;
	}
	catch (const winrt::hresult_error&) {
		pHandle->IsEnd = true;
		pHandle->services = nullptr;
	}
	catch (const std::exception&) {
		pHandle->IsEnd = true;
		pHandle->services = nullptr;
	}
	catch (...)
	{
		pHandle->IsEnd = true;
		pHandle->services = nullptr;
	}
}

bool BleHandle::ConnectBLEDevice() {
	// Force cleanup of any old connections or dangling handles before attempting to connect
	CloseBLEDevice();

	// FIX: Use std::thread for connection and proper lifecycle management
	std::thread t(ConnectBLEDeviceThreadBody, this);
	t.join(); // Block and wait for it to finish cleanly
	
	if (services == nullptr) return false;
	return true;

}

// Query services UUID wrapper

void BleHandle::GetAllServersUUID(unsigned int* UUIDArry, unsigned int* ArryCount) {

	;// Char handle query
	try
	{
		*ArryCount = 0;
		if (services == nullptr) return;
		//auto services = result.Services();
		for (size_t i = 0; i < services.Size(); i++)
		{
			auto service = services.GetAt(i);
			auto uuid = service.Uuid(); // Get ID list
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
		auto charact = it->second->Service.GetCharacteristicsAsync(BluetoothCacheMode::Cached).get();
		auto characts = charact.Characteristics();

		for (size_t j = 0; j < characts.Size(); j++)
		{
			auto charact = characts.GetAt(j);
			auto uuid = charact.Uuid(); // Extract sub-char UUID
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
		// Map characteristics
		if ((static_cast<uint32_t>(GAttpro) & static_cast<uint32_t>(GattCharacteristicProperties::Notify)) != 0) {
			*IsNotify = true;
			cit->second->AuthorityInfo.IsNotify = true;
		}

		// Write permission
		if ((static_cast<uint32_t>(GAttpro) & static_cast<uint32_t>(GattCharacteristicProperties::Write)) != 0) {
			*IsWrite = true;
			cit->second->AuthorityInfo.IsWrite = true;
		}

		// Read permission
		if ((static_cast<uint32_t>(GAttpro) & static_cast<uint32_t>(GattCharacteristicProperties::Read)) != 0) {
			*IsRead = true;
			cit->second->AuthorityInfo.IsRead = true;
		}

		// WriteWithoutResponse
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
		auto status = cit->second->characteristic.WriteValueAsync(buffer).get();
		if (status != GattCommunicationStatus::Success) return false;
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
		// BUG-8 FIX: Directly pass BleHandle over ServiceInfo address to caller
		map<string, BleHandle*>::iterator penIt = Pens.find(this->ID);
		if (penIt != Pens.end()) {
			OnRecvDataCallBack(penIt->second, ServiceUUID, CharacteristicUUID, value.data(), value.Length());
		}
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
		auto status = statuss.get();
		if (status != GattCommunicationStatus::Success) return;

		cit->second->revoker = cit->second->characteristic.ValueChanged(auto_revoke, &Characteristic_ValueChanged);
	}
	catch (...) {

	}

}

// Terminate active BLE channel safely
void BleHandle::CloseBLEDevice() {
	// BUG-7 FIX: Break notify handlers prior to hardware device drop
	for (auto& svcPair : ServicesInfo) {
		if (svcPair.second) {
			for (auto& charPair : svcPair.second->CharacteristicsInfo) {
				if (charPair.second) {
					charPair.second->revoker = {}; // Release token
					charPair.second->characteristic = nullptr; // Ensure reference drop
				}
			}
			try {
				// Explicitly close the GATT device service to notify OS to tear down quickly
				svcPair.second->Service.Close();
				svcPair.second->Service = nullptr;
			} catch (...) {}
		}
	}

	try
	{
		services = nullptr;
		result = nullptr;
		if (device != nullptr) {
			device.Close();
			device = nullptr;
		}
	}
	catch (...) {}

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
	auto id = Device.BluetoothDeviceId(); // Target device identifier
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
	auto id = device.BluetoothDeviceId(); // Identifier match token
	char ID[100] = { 0 };
	char Address[100] = { 0 };
	ConvertLPWSTRToChar(id.Id().c_str(), (unsigned char*)ID);

	map<string, BleHandle*>::iterator it = Pens.find(ID);
	if (it == Pens.end()) return;

	sdk::Logger::Log(sdk::LogLevel::DEBUG, sdk::ErrorCategory::BLUETOOTH, 
		std::string("[BleHandle] ConnectionStatus_ValueChanged callback invoked for ") + ID + 
		"! New status: " + (device.ConnectionStatus() == BluetoothConnectionStatus::Connected ? "Connected" : "Disconnected"));

	if (OnConnectionStatusCallBack != NULL) {
		if (device.ConnectionStatus() == BluetoothConnectionStatus::Connected) OnConnectionStatusCallBack(it->second, it->second->Address, true);
		if (device.ConnectionStatus() == BluetoothConnectionStatus::Disconnected) OnConnectionStatusCallBack(it->second, it->second->Address, false);
	}

}
