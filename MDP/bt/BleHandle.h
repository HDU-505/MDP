#pragma once

#include <map>
#include <windows.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <mutex> // 互斥锁，用于线程安全
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Devices.Bluetooth.h>
#include <winrt/Windows.Devices.Enumeration.h>
#include <winrt/Windows.Devices.Bluetooth.Advertisement.h>
#include <winrt/Windows.Devices.Bluetooth.GenericAttributeProfile.h>
#include <winrt/Windows.Storage.Streams.h>
#include <winrt/Windows.Devices.Radios.h>
#include <winrt/Windows.Web.Syndication.h>
#include <windows.devices.bluetooth.h>
#include <windows.foundation.h>
#include <coroutine>
//#include "BLEComm.h"

using namespace winrt;
using namespace Windows::Devices::Bluetooth;
using namespace Windows::Devices::Bluetooth::Advertisement;
using namespace Windows::Devices::Bluetooth::GenericAttributeProfile;
using namespace winrt::Windows::Devices::Radios;
using namespace Windows::Foundation;
#pragma comment(lib, "windowsapp")
#pragma comment(lib, "WindowsApp.lib")
using namespace std;

// 特征值权限信息结构体
struct CharacteristicAuthorityInfo {
	bool IsRead;   // 是否支持读操作
	bool IsWrite;  // 是否支持写操作
	bool IsNotify; // 是否支持通知（Notify）
};

// 特征值信息结构体
struct CharacteristicInfo {
	unsigned int uuid; // 特征值 UUID
	GattCharacteristic characteristic{ nullptr }; // 对应的 Gatt 特征对象
	GattCharacteristic::ValueChanged_revoker revoker; // 用于注销通知
	CharacteristicAuthorityInfo AuthorityInfo; // 权限信息
};

// 服务信息结构体
struct ServiceInfo {
	unsigned int uuid; // 服务 UUID
	Windows::Devices::Bluetooth::GenericAttributeProfile::GattDeviceService Service{ nullptr }; // Gatt 服务对象
	map<unsigned int, CharacteristicInfo*> CharacteristicsInfo; // 当前服务下的特征值信息集合

	// 清理所有特征值信息
	void clear() {
		for (auto it = CharacteristicsInfo.begin(); it != CharacteristicsInfo.end(); it++) {
			delete(it->second);
			it->second = NULL;
		}
		CharacteristicsInfo.clear();
	}
};

// BLE 设备封装类
class BleHandle
{
public:
	BleHandle();
	~BleHandle();

public:
	char ID[255] = { 0 };       // 设备 ID
	char Address[255] = { 0 };  // MAC 地址
	map<unsigned int, ServiceInfo*> ServicesInfo; // 服务信息映射
	bool IsEnd;                 // 是否断开标志

public:
	BluetoothLEDevice device{ nullptr }; // BLE 设备对象
	GattDeviceServicesResult result{ nullptr }; // 服务查询结果
	Windows::Foundation::Collections::IVectorView<Windows::Devices::Bluetooth::GenericAttributeProfile::GattDeviceService> services{ nullptr }; // Gatt 服务集合

public:
	// 连接 BLE 设备
	bool ConnectBLEDevice();

	// 获取所有服务 UUID
	void GetAllServersUUID(unsigned int* UUIDArry, unsigned int* ArryCount);

	// 获取某服务下所有特征值 UUID
	void GetCharcteristicByUUID(unsigned int ServiceUUID, unsigned int* UUIDArry, unsigned int* ArryCount);

	// 获取特征值的权限信息（读、写、通知）
	void GetCharcteristicAction(unsigned int ServiceUUID, unsigned int CharacteristicUUID, bool* IsRead, bool* IsWrite, bool* IsNotify);

	// 写数据到指定特征值
	bool WriteDateByCharcteristic(unsigned int ServiceUUID, unsigned int CharacteristicUUID, unsigned char* buff, unsigned int lenght);

	// 读取指定特征值的数据
	void ReadDataByCharcteristic(unsigned int ServiceUUID, unsigned int CharacteristicUUID);

	// 注册通知（Notify）监听
	void RegisterReadNotify(unsigned int ServiceUUID, unsigned int CharacteristicUUID);

	// 断开设备连接并释放资源
	void CloseBLEDevice();
};



// 字符串转换函数：char* -> LPWSTR（宽字符串）
LPWSTR ConvertCharToLPWSTR(char* szString, WCHAR* addrchar);

// 字符串转换函数：LPWSTR -> char*
unsigned char* ConvertLPWSTRToChar(LPCTSTR widestr, unsigned char* addrchar);

// 全局回调函数指针定义（由外部注册实现）

