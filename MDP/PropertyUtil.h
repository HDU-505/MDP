#pragma once
# include "Amplifier_LIB.h"
# include <vector>
# include "BleDeviceManager.h"


// 获取设备属性
int GetDeviceProperty(int32_t PropertyID, void* PropertyValue, uint32_t ValueByteSize);
// 获取模块属性
int GetModuleProperty(int32_t PropertyID, void* PropertyValue, uint32_t ValueByteSize);
// 获取通道属性
int GetChannelProperty(int32_t PropertyID, void* PropertyValue, uint32_t ValueByteSize, uint32_t Index);

// 设置设备属性
int SetDeviceProperty(int32_t PropertyID, void* PropertyValue, uint32_t ValueByteSize);
// 设置模块属性
int SetModuleProperty(int32_t PropertyID, void* PropertyValue, uint32_t ValueByteSize);
// 设置通道属性
int SetChannelProperty(int32_t PropertyID, void* PropertyValue, uint32_t ValueByteSize, uint32_t Index);

// 获取设备属性范围
int GetDevicePropertyRange(int32_t PropertyID, void* RangeArray, uint32_t* ArrayByteSize, t_PropertyRangeType* RangeType);
// 获取模块属性范围
int GetModulePropertyRange(int32_t PropertyID, void* RangeArray, uint32_t* ArrayByteSize, t_PropertyRangeType* RangeType);
// 获取通道属性范围
int GetChannelPropertyRange(int32_t PropertyID, void* RangeArray, uint32_t* ArrayByteSize, t_PropertyRangeType* RangeType, uint32_t Index);

bool SetBleDeviceManager(BleDeviceManager* bleDevice);