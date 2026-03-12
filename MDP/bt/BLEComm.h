#pragma once

#include <windows.h>
#include "BleHandle.h"

// 定义 BLE 设备句柄类型
#define  HANDLE void *



// 数据段结构体，用于封装原始数据和长度
typedef struct DataSection {
	unsigned char* Data;  // 数据指针
	int Lenght;            // 数据长度
} DataSection;

// BLE 设备接收数据的回调函数类型
// handle：设备句柄
// ServiceUUID：服务 UUID
// CharacteristicUUID：特征值 UUID
// recvData：接收到的数据指针
// lenght：数据长度
typedef void BleDeviceRecvDataCallBack(HANDLE handle, unsigned int ServiceUUID, unsigned int CharacteristicUUID, unsigned char* recvData, unsigned int lenght);

// 扫描到 BLE 设备时的回调函数类型
// ID：设备标识符
// PenName：设备名称
// PenMac：设备 MAC 地址
// rssi：信号强度
// DataSections：广播数据内容
// DataSectionCount：广播数据段数量
typedef void ScanedBleDeviceCallBack(const char* ID, const char* PenName, const char* PenMac, int rssi, DataSection* DataSections, int DataSectionCount);

// BLE 扫描完成的回调函数类型
typedef void SacnBleDeviceFinishCallBack();

// BLE 连接状态变化回调函数类型
// handle：设备句柄
// PenMac：设备 MAC 地址
// IsConnect：是否连接
typedef void ConnectionBleDeviceStatusCallBack(HANDLE handle, const char* PenMac, bool IsConnect);

// 注册监听某个特征值的 Notify 通知
// handle：设备句柄
// ServiceUUID：服务 UUID
// CharacteristicUUID：特征值 UUID
void RegisterReadNotify(HANDLE handle, unsigned int ServiceUUID, unsigned int CharacteristicUUID);


// 所有已连接的 BLE 设备对象（按地址存储）
extern map<string, BleHandle*> Pens;

// 数据接收回调
extern BleDeviceRecvDataCallBack* OnRecvDataCallBack;

// 扫描到设备回调
extern ScanedBleDeviceCallBack* OnScanedBleDeviceCallBack;

// 扫描完成回调
extern SacnBleDeviceFinishCallBack* OnSacnFinishCallBack;

// 连接状态变化回调
extern ConnectionBleDeviceStatusCallBack* OnConnectionStatusCallBack;

// 注册设备扫描回调
void RegisterRecvBleDevice(ScanedBleDeviceCallBack CallBack);

// 注册设备扫描完成回调
void RegisterSacnBleDeviceFinish(SacnBleDeviceFinishCallBack CallBack);

// 注册 BLE 连接状态变化回调
void RegisterConnectionBleDeviceStatus(ConnectionBleDeviceStatusCallBack CallBack);

// 注册 BLE 数据接收回调
void RegisterBleDeviceRecvData(BleDeviceRecvDataCallBack CallBack);

// 判断当前设备是否支持 BLE（低功耗蓝牙）
bool BLEIsLowEnergySupported();

// 开始扫描 BLE 设备，timeout 为扫描时长（单位：秒）
void ScanBLEDevice(int timeout);

// 停止扫描 BLE 设备
void StopScanBLEDevice();

// 根据设备 ID 建立连接，返回设备句柄
HANDLE ConnectBLEDevice(char* ID);

// 获取指定设备的所有服务 UUID
// UUIDArry：返回的 UUID 数组
// ArryCount：返回的数量
void GetAllServersUUID(HANDLE handle, unsigned int* UUIDArry, unsigned int* ArryCount);

// 获取指定服务下的所有特征值 UUID
void GetCharcteristicByUUID(HANDLE handle, unsigned int ServiceUUID, unsigned int* UUIDArry, unsigned int* ArryCount);

// 查询某个特征值支持的操作（读、写、通知）
// IsRead、IsWrite、IsNotify：返回该特征值是否支持对应操作
void GetCharcteristicAction(HANDLE handle, unsigned int ServiceUUID, unsigned int CharacteristicUUID, bool* IsRead, bool* IsWrite, bool* IsNotify);

// 向某个特征值写入数据
bool WriteDateByCharcteristic(HANDLE handle, unsigned int ServiceUUID, unsigned int CharacteristicUUID, unsigned char* buff, unsigned int lenght);

// 读取某个特征值的数据
void ReadDataByCharcteristic(HANDLE handle, unsigned int ServiceUUID, unsigned int CharacteristicUUID);

// 关闭与设备的连接，释放资源
void CloseBLEDevice(HANDLE handle);

// 关闭所有已连接设备（DLL 卸载或程序退出时调用）
void CloseAllBLEDevices();

