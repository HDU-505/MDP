#include "BLEComm.h"
#include "BleHandle.h"
#include "../ErrorHandler.h"
#include <map>
#include <string>
#include <mutex>
#include <chrono>
#include <thread>
using namespace std;

map<uint64_t, int>BleDevices;
std::mutex bleDevicesMtx; // BUG-6 FIX: Thread safety for BleDevices
std::thread g_scanThread;
std::mutex g_scanThreadMtx;

BluetoothLEAdvertisementWatcher m_btWatcher;
winrt::event_token m_btWatcherToken{};
std::atomic<bool> g_isScanning{false};

// Convert char* to wide string LPWSTR
LPWSTR ConvertCharToLPWSTR(char* szString, WCHAR* addrchar)
{
	int dwLen = strlen(szString) + 1;
	int nwLen = MultiByteToWideChar(CP_ACP, 0, szString, dwLen, NULL, 0); // Get required wide characters

	MultiByteToWideChar(CP_ACP, 0, szString, dwLen, addrchar, nwLen);
	return addrchar;
}


// Convert wide string LPWSTR to char*
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
	auto supported = adapter.IsLowEnergySupported(); // Check if Windows supports BLE
	if (supported == false) return false;
	auto async = adapter.GetRadioAsync();
	auto radio = async.get();
	auto t = radio.State(); // Get adapter state 0=Unknown, 1=On, 2=Off, 3=Disabled
	if (t != winrt::Windows::Devices::Radios::RadioState::On) {
		return false;
	}
	return  true;
}



void Scanblebackfun(BluetoothLEAdvertisementWatcher w, BluetoothLEAdvertisementReceivedEventArgs e) {
	if (!g_isScanning) {
		return; // Instant flush: drop already-queued Windows thread pool callbacks if scan officially ended
	}
	
	if (e.AdvertisementType() == BluetoothLEAdvertisementType::ConnectableUndirected)
	{
		uint64_t address = e.BluetoothAddress();
		auto Rssi = e.RawSignalStrengthInDBm();
		
		{
			std::lock_guard<std::mutex> lock(bleDevicesMtx); // Lock map
			static std::map<uint64_t, std::chrono::steady_clock::time_point> lastReportTime;
			auto now = std::chrono::steady_clock::now();
			if (BleDevices.find(address) != BleDevices.end()) {
				// Throttle rediscovery events to suppress console spam but allow SDK to catch disconnected devices
				if (std::chrono::duration_cast<std::chrono::seconds>(now - lastReportTime[address]).count() < 3) {
					return;
				}
			}
			BleDevices[address] = Rssi;
			lastReportTime[address] = now;
		}
		
		// Map device properties
		try {
			BluetoothLEDevice dev = BluetoothLEDevice::FromBluetoothAddressAsync(address).get();
			if (!dev) {
				sdk::Logger::Log(sdk::LogLevel::DEBUG, sdk::ErrorCategory::BLUETOOTH, std::string("[BLEComm] Discovered device but OS returned null for address: ") + std::to_string(address));
				return;
			}
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
			if (mactemp != NULL) {
				errno_t err = strcpy_s(Address, 100, mactemp + 1);
			} else {
				strcpy_s(Address, 100, "UNKNOWN");
			}

			sdk::Logger::Log(sdk::LogLevel::DEBUG, sdk::ErrorCategory::BLUETOOTH, 
				std::string("[BLEComm] Discovered valid BLE device: ") + Name + " (MAC: " + Address + ") RSSI: " + std::to_string(e.RawSignalStrengthInDBm()));

			if (OnScanedBleDeviceCallBack != NULL) {
				OnScanedBleDeviceCallBack(ID, Name, Address, e.RawSignalStrengthInDBm(), DataSections, view.Size());
			}

		}
		catch (const std::exception& ex) {
			sdk::Logger::Log(sdk::LogLevel::WARNING, sdk::ErrorCategory::BLUETOOTH, std::string("Device scan parsing exception: ") + ex.what());
		}
	}
}


DWORD WINAPI ScanBleThread(LPVOID lpParameter) {
	int timeout = (int)(intptr_t)lpParameter;
	
	try {
		sdk::Logger::Log(sdk::LogLevel::DEBUG, sdk::ErrorCategory::BLUETOOTH, "[BLEComm] ScanBleThread starting watcher with timeout " + std::to_string(timeout) + "ms");
		
		// Reset watcher to completely clear any OS-level internal queues from previous runs
		m_btWatcher = BluetoothLEAdvertisementWatcher();
		m_btWatcher.ScanningMode(BluetoothLEScanningMode::Active);
		m_btWatcherToken = m_btWatcher.Received(Scanblebackfun);
		
		g_isScanning = true;
		m_btWatcher.Start();
		
		for(int i = 0; i< timeout/50;i++)
		{
			Sleep(50);
			if (m_btWatcher.Status() == BluetoothLEAdvertisementWatcherStatus::Stopped) {
				g_isScanning = false;
				m_btWatcher.Received(m_btWatcherToken); // REVOKE HANDLE
				// BUG-1 FIX: trigger callback even if exited early
				if (OnSacnFinishCallBack != NULL) {
					OnSacnFinishCallBack();
				}
				return 0;
			}
		}

		g_isScanning = false;
		m_btWatcher.Stop();
		m_btWatcher.Received(m_btWatcherToken); // REVOKE HANDLE
		sdk::Logger::Log(sdk::LogLevel::DEBUG, sdk::ErrorCategory::BLUETOOTH, "[BLEComm] ScanBleThread stopped and handler revoked successfully.");
		
		if (OnSacnFinishCallBack != NULL) {
			OnSacnFinishCallBack();
		}
	}
	catch (const std::exception& ex) {
		if (OnSacnFinishCallBack != NULL) {
			OnSacnFinishCallBack();
		}
	}
	
	return 0;
}


void ScanBLEDevice(int timeout) {
	{
		std::lock_guard<std::mutex> lock(bleDevicesMtx);
		BleDevices.clear();
	}
	// FIX: Use std::thread for correct thread lifecycle management
	std::lock_guard<std::mutex> threadLock(g_scanThreadMtx);
	if (g_scanThread.joinable()) {
		g_scanThread.join();
	}
	g_scanThread = std::thread(ScanBleThread, (LPVOID)(intptr_t)timeout);
}

void StopScanBLEDevice()
{
	g_isScanning = false; // Immediately cease processing backlogged callbacks
	m_btWatcher.Stop();
	std::lock_guard<std::mutex> threadLock(g_scanThreadMtx);
	if (g_scanThread.joinable()) {
		g_scanThread.join();
	}
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
	if (!ble) return;

	// Drop from map
	map<string, BleHandle*>::iterator it = Pens.find(ble->ID);
	if (it != Pens.end()) {
		Pens.erase(it);
	}

	// Close BLE connection
	ble->CloseBLEDevice();
	
	// BUG-7 FIX: let wait cycle complete
	Sleep(50);

	delete ble;
}

// Free all devices cleanly
void CloseAllBLEDevices() {
	map<string, BleHandle*> pensCopy = Pens;
	Pens.clear();

	for (auto& pair : pensCopy) {
		if (pair.second) {
			try {
				pair.second->CloseBLEDevice();
				Sleep(20);
				delete pair.second;
			}
			catch (...) {}
		}
	}
	
	StopScanBLEDevice();
}

// 拦截控制台异常退出的处理函数 (Ctrl+C, Ctrl+Break, 关闭窗口等)
BOOL WINAPI BleConsoleCtrlHandler(DWORD dwCtrlType) {
	if (dwCtrlType == CTRL_C_EVENT || dwCtrlType == CTRL_CLOSE_EVENT || dwCtrlType == CTRL_BREAK_EVENT) {
		sdk::Logger::Log(sdk::LogLevel::DEBUG, sdk::ErrorCategory::BLUETOOTH, "[BLEComm] Intercepted Console Exit Event. Forcing synchronous BLE disconnect...");
		CloseAllBLEDevices();
	}
	return FALSE; // 允许默认进程终止继续进行
}

// 利用全局静态结构的构造函数在 DLL/SDK 加载时自动注册拦截器
struct BleLifecycleManager {
	BleLifecycleManager() {
		SetConsoleCtrlHandler(BleConsoleCtrlHandler, TRUE);
	}
} g_bleLifecycleManager;

