# include "PropertyUtil.h"

t_VersionNumber apiVer = { 3, 2, 0, 0 };
t_VersionNumber libraryVer = { 1, 22, 2, 28 };

float baseSampleRate = 125.0f;
float subSampleDivisor = 1.0f;
t_RecordingMode recordingMode = RM_STOPPED;

// 设置数值类型属性
template <typename T>
static int SetVal(void* dest, uint32_t destSize, const T& val) {
	if (destSize < sizeof(T)) return AMP_ERR_PARAM;
	
	memcpy(dest, &val, sizeof(T));
	return AMP_OK;
}

// 设置字符串属性
static int SetStr(void* dest, uint32_t destSize, const char* val) {
	if (!dest || destSize == 0) return AMP_ERR_PARAM;
	strncpy_s((char*)dest, destSize, val, _TRUNCATE);
	return AMP_OK;
}

// 设置最小/最大值范围
template <typename T>
static int SetRangeMinMax(void* dest, uint32_t* destSize, t_PropertyRangeType* type, T min, T max) {
	if (*destSize < 2 * sizeof(T)) return AMP_ERR_BUFFERSIZE;
	T* arr = (T*)dest;
	arr[0] = min;
	arr[1] = max;
	*destSize = 2 * sizeof(T);
	*type = RT_MINMAX;
	return AMP_OK;
}

//设置离散值列表
template <typename T>
static int SetRangeDiscrete(void* dest, uint32_t* destSize, t_PropertyRangeType* type, const std::vector<T>& values) {
	size_t requiredSize = values.size() * sizeof(T);

	// 检查缓冲区是否足够
	if (*destSize < requiredSize) {
		return AMP_ERR_BUFFERSIZE;
	}

	// 复制数据
	T* arr = (T*)dest;
	for (size_t i = 0; i < values.size(); ++i) {
		arr[i] = values[i];
	}

	*destSize = (uint32_t)requiredSize;
	*type = RT_DISCRETE; // 设置类型为离散值
	return AMP_OK;
}

// 获取设备属性
int GetDeviceProperty(int32_t PropertyID, void* PropertyValue, uint32_t ValueByteSize) {
	switch (PropertyID) {
		// 字符串类型
		case DPROP_CHR_Family:
			return SetStr(PropertyValue, ValueByteSize, "MindTooth");
		case DPROP_CHR_Type:
			return SetStr(PropertyValue, ValueByteSize, "Device_MT");
		case DPROP_CHR_Interface:
			return SetStr(PropertyValue, ValueByteSize, "BT");
		case DPROP_CHR_Address:
			return SetStr(PropertyValue, ValueByteSize, "00: 00 : 00 : 00 : 00 : 00");
		case DPROP_CHR_SerialNumber:
			return SetStr(PropertyValue, ValueByteSize, "000");
		case DPROP_CHR_FlashWorkspaceDescription:
		case DPROP_CHR_FlashFileName:
			//return SetStr(PropertyValue, ValueByteSize, "Default");
			return AMP_ERR_VERSION;

		// 版本号类型
		case DPROP_TVN_HardwareRevision:
		case DPROP_TVN_FirmwareVersion:
			return SetVal(PropertyValue, ValueByteSize, apiVer);
		case DPROP_TVN_DriverVersion:
			return SetVal(PropertyValue, ValueByteSize, libraryVer);


		// Int32 / Bool32 类型
		case DPROP_I32_AvailableModules:
			return SetVal(PropertyValue, ValueByteSize, (int32_t)1);
		case DPROP_I32_AvailableChannels:
			return SetVal(PropertyValue, ValueByteSize, (int32_t)8);
		case DPROP_I32_BatteryLevel:
			//return SetVal(PropertyValue, ValueByteSize, (int32_t)BS_UNKNOWN);
		case DPROP_I32_ConnectionState:
			//return SetVal(PropertyValue, ValueByteSize, (int32_t)CS_DISCONNECTED);
		case DPROP_I32_SignalQuality:
			//return SetVal(PropertyValue, ValueByteSize, (int32_t)SQ_NOINFO);
		case DPROP_I32_RecordingState:
		case DPROP_I32_RecordingMode:
			return SetVal(PropertyValue, ValueByteSize, (int32_t)recordingMode);
		case DPROP_I32_SignalStrength:
		case DPROP_I32_GoodImpedanceLevel:
		case DPROP_I32_BadImpedanceLevel:
		case DPROP_I32_LedControl:
		case DPROP_I32_ActiveShieldGain:
		case DPROP_B32_FastDataAccess:
		case DPROP_B32_ContinuousImpedance:
		case DPROP_B32_FlashFormatting:
			//return SetVal(PropertyValue, ValueByteSize, (int32_t)0);
			return AMP_ERR_VERSION;

		// UInt32 类型
		case DPROP_UI32_FlashSegmentSize:
			//return SetVal(PropertyValue, ValueByteSize, (uint32_t)2047);
		case DPROP_UI32_ErrorFlags:
		case DPROP_UI32_FlashRecordingState:
		case DPROP_UI32_FlashFreeSpace:
		case DPROP_UI32_FlashFileSize:
			//return SetVal(PropertyValue, ValueByteSize, (uint32_t)0);
			return AMP_ERR_VERSION;

		// Float32 类型
		case DPROP_F32_BaseSampleRate:
			return SetVal(PropertyValue, ValueByteSize, baseSampleRate);
		case DPROP_F32_BatteryVoltage:
		case DPROP_F32_SubSampleDivisor:
			return SetVal(PropertyValue, ValueByteSize, subSampleDivisor);
			return AMP_ERR_VERSION;

		default:
			return AMP_ERR_PARAM;
	}
}

// 获取模块属性
int GetModuleProperty(int32_t PropertyID, void* PropertyValue, uint32_t ValueByteSize) {
	switch (PropertyID) {
		// 字符串类型
		case MPROP_CHR_Type:
			return SetStr(PropertyValue, ValueByteSize, "Module_MT");
		case MPROP_CHR_SerialNumber:
			return SetStr(PropertyValue, ValueByteSize, "000");

		// 版本号类型
		case MPROP_TVN_HardwareRevision:
		case MPROP_TVN_FirmwareVersion:
			return SetVal(PropertyValue, ValueByteSize, apiVer);

		// Int32 / Bool32 类型
		case MPROP_I32_UseableChannels:
			return SetVal(PropertyValue, ValueByteSize, (int32_t)8);
		case MPROP_B32_ImpedanceMeasurement:
		case MPROP_I32_TriggerOutMode:
		case MPROP_I32_TriggerSyncPin:
		case MPROP_I32_TriggerSyncPeriod:
		case MPROP_I32_TriggerSyncWidth:
		case MPROP_I32_LedColorREF:
		case MPROP_I32_LedColorGND:
		case MPROP_I32_UserButtonState:
		case MPROP_I32_UserButtonLed:
			//return SetVal(PropertyValue, ValueByteSize, (int32_t)0);
			return AMP_ERR_VERSION;

		default:
			return AMP_ERR_PARAM;
	}
}

// 获取通道属性
int GetChannelProperty(int32_t PropertyID, void* PropertyValue, uint32_t ValueByteSize, uint32_t Index) {
	switch (PropertyID) {
		// 字符串类型
		case CPROP_CHR_Function:
		case CPROP_CHR_Unit:
			//return AMP_ERR_VERSION; //这里必须返回一个字符串类型，不能返回AMP_ERR_VERSION，好像DisplayAmpInfo在open的时候会调用这里
			return SetStr(PropertyValue, ValueByteSize, "");

		case CPROP_I32_ChannelName: 
			return SetVal(PropertyValue, ValueByteSize, (int32_t)10);

		// Int32 / Bool32 类型
		case CPROP_I32_Type:
			return SetVal(PropertyValue, ValueByteSize, (int32_t)CT_EEG);
		case CPROP_I32_ChannelNumber:
			return SetVal(PropertyValue, ValueByteSize, (int32_t)Index);
		case CPROP_I32_ModuleNumber:
			return SetVal(PropertyValue, ValueByteSize, (int32_t)0);
		case CPROP_I32_Electrode:
		case CPROP_I32_DataType:
			return SetVal(PropertyValue, ValueByteSize,(int32_t)6);
		case CPROP_I32_LedColor:
		case CPROP_B32_ReferenceChannel:
		case CPROP_B32_ImpedanceMeasurement:
		case CPROP_B32_RecordingEnabled:
			return SetVal(PropertyValue, ValueByteSize, (int32_t)1);

		// UInt32 类型
		case CPROP_UI32_OutputValue:
			return SetVal(PropertyValue, ValueByteSize, (uint32_t)0);

		// Float32 类型
		case CPROP_F32_Resolution:
			return SetVal(PropertyValue, ValueByteSize, (float)1);
		case CPROP_F32_Gain:
		case CPROP_F32_HighPass:
		case CPROP_F32_LowPass:
		case CPROP_F32_NotchFilter:
			//return SetVal(PropertyValue, ValueByteSize, 0.0f);
			return AMP_ERR_VERSION;

		default:
			return AMP_ERR_PARAM;
	}
}

// 设置设备属性
int SetDeviceProperty(int32_t PropertyID, void* PropertyValue, uint32_t ValueByteSize) {
	switch (PropertyID) {
		// 可写属性
		case DPROP_F32_BaseSampleRate: {
			// TODO: 修改硬件的采样率，这里用一个全局变量表示
			if (ValueByteSize < sizeof(float))
				return AMP_ERR_PARAM;
			memcpy(&baseSampleRate, PropertyValue, sizeof(float));
			return AMP_OK;
		}
		case DPROP_F32_SubSampleDivisor: {
			// TODO: 修改硬件的采样率，这里用一个全局变量表示
			if (ValueByteSize < sizeof(float))
				return AMP_ERR_PARAM;
			memcpy(&subSampleDivisor, PropertyValue, sizeof(float));
			return AMP_OK;
		}
		case DPROP_I32_RecordingMode: {
			memcpy(&recordingMode, PropertyValue, sizeof(int));
			return AMP_OK;
		}

		// 只读属性
		case DPROP_CHR_Family:
		case DPROP_CHR_Type:
		case DPROP_CHR_Interface:
		case DPROP_CHR_Address:
		case DPROP_CHR_SerialNumber:
		case DPROP_TVN_HardwareRevision:
		case DPROP_TVN_FirmwareVersion:
		case DPROP_TVN_DriverVersion:
		case DPROP_I32_AvailableModules:
		case DPROP_I32_AvailableChannels:
		case DPROP_F32_BatteryVoltage:
		case DPROP_I32_BatteryLevel:
		case DPROP_I32_ConnectionState:
		case DPROP_I32_SignalQuality:
		case DPROP_UI32_ErrorFlags:
		case DPROP_I32_RecordingState:
		case DPROP_I32_SignalStrength:
		

		case DPROP_I32_GoodImpedanceLevel:
		case DPROP_I32_BadImpedanceLevel:
		case DPROP_I32_LedControl:
		case DPROP_I32_ActiveShieldGain:
		case DPROP_B32_FastDataAccess:
		case DPROP_B32_ContinuousImpedance:
		case DPROP_UI32_FlashRecordingState:
		case DPROP_UI32_FlashSegmentSize:
		case DPROP_CHR_FlashWorkspaceDescription:
		case DPROP_UI32_FlashFreeSpace:
		case DPROP_UI32_FlashFileSize:
		case DPROP_CHR_FlashFileName:
		case DPROP_B32_FlashFormatting:
			return AMP_ERR_VERSION;

		default:
			return AMP_ERR_PARAM;
	}
}

// 设置模块属性
int SetModuleProperty(int32_t PropertyID, void* PropertyValue, uint32_t ValueByteSize) {
	switch (PropertyID) {
		// 可写属性
			// TODO: 保存属性值
			//return AMP_OK;

		// 只读属性
		case MPROP_CHR_Type:
		case MPROP_CHR_SerialNumber:
		case MPROP_TVN_HardwareRevision:
		case MPROP_TVN_FirmwareVersion:
		case MPROP_B32_ImpedanceMeasurement:
		case MPROP_I32_UseableChannels:
		case MPROP_I32_TriggerOutMode:
		case MPROP_I32_TriggerSyncPin:
		case MPROP_I32_TriggerSyncPeriod:
		case MPROP_I32_TriggerSyncWidth:
		case MPROP_I32_LedColorREF:
		case MPROP_I32_LedColorGND:
		case MPROP_I32_UserButtonState:
		case MPROP_I32_UserButtonLed:
			return AMP_ERR_VERSION;

		default:
			return AMP_ERR_PARAM;
	}
}

// 设置通道属性
int SetChannelProperty(int32_t PropertyID, void* PropertyValue, uint32_t ValueByteSize, uint32_t Index) {
	switch (PropertyID) {
		// 可写属性
			// TODO: 保存属性值
			//return AMP_OK;

		// 只读属性
		case CPROP_I32_Type:
		case CPROP_I32_ChannelNumber:
		case CPROP_I32_ModuleNumber:
		case CPROP_CHR_Function:
		case CPROP_I32_Electrode:
		case CPROP_I32_DataType:
		case CPROP_F32_Resolution:
		case CPROP_CHR_Unit:
		case CPROP_F32_Gain:
		case CPROP_F32_HighPass:
		case CPROP_F32_LowPass:
		case CPROP_F32_NotchFilter:
		case CPROP_B32_ReferenceChannel:
		case CPROP_B32_ImpedanceMeasurement:
		case CPROP_B32_RecordingEnabled:
		case CPROP_I32_LedColor:
		case CPROP_UI32_OutputValue:
		case CPROP_I32_ChannelName:
			return AMP_ERR_VERSION;

		default:
			return AMP_ERR_PARAM;
	}
}

// 获取设备属性范围
int GetDevicePropertyRange(int32_t PropertyID, void* RangeArray, uint32_t * ArrayByteSize, t_PropertyRangeType * RangeType) {
	*RangeType = RT_READONLY; // 可写属性在SetRangeMinMax会改变RangeType，其余属性全为只读

	switch (PropertyID) {
	case DPROP_I32_RecordingMode:
		return SetRangeMinMax(RangeArray, ArrayByteSize, RangeType, (int32_t)RM_STOPPED, (int32_t)RM_TEST);
	case DPROP_F32_BaseSampleRate:
		return SetRangeDiscrete(RangeArray, ArrayByteSize, RangeType, std::vector<float>{125.0f, 256.0f, 512.0f});
	case DPROP_F32_SubSampleDivisor:
		return SetRangeDiscrete(RangeArray, ArrayByteSize, RangeType, std::vector<float>{1.0f, 2.0f});

	default:
		// 其他属性暂时默认为只读
		return AMP_OK;
	}
}

// 获取模块属性范围
int GetModulePropertyRange(int32_t PropertyID, void* RangeArray, uint32_t * ArrayByteSize, t_PropertyRangeType * RangeType) {
	*RangeType = RT_READONLY;

	switch (PropertyID) {
	case MPROP_I32_TriggerOutMode:
		return SetRangeMinMax(RangeArray, ArrayByteSize, RangeType, (int32_t)TM_DEFAULT, (int32_t)TM_MIRROR_SYNC);
	case MPROP_I32_LedColorREF:
	case MPROP_I32_LedColorGND:
		return SetRangeMinMax(RangeArray, ArrayByteSize, RangeType, (int32_t)LED_OFF, (int32_t)LED_YELLOW);
	case MPROP_I32_UserButtonLed:
		return SetRangeMinMax(RangeArray, ArrayByteSize, RangeType, (int32_t)100, (int32_t)2000); // 0.1 - 2s
	default:
		return AMP_OK;
	}
}

// 获取通道属性范围
int GetChannelPropertyRange(int32_t PropertyID, void* RangeArray, uint32_t * ArrayByteSize, t_PropertyRangeType * RangeType, uint32_t Index) {
	*RangeType = RT_READONLY;

	switch (PropertyID) {
	case CPROP_B32_RecordingEnabled:
		return SetRangeMinMax(RangeArray, ArrayByteSize, RangeType, (int32_t)0, (int32_t)1);
	case CPROP_I32_LedColor:
		return SetRangeMinMax(RangeArray, ArrayByteSize, RangeType, (int32_t)LED_OFF, (int32_t)LED_YELLOW);
	case CPROP_F32_Gain:
		return SetRangeMinMax(RangeArray, ArrayByteSize, RangeType, 0.0f, 100.0f);
	default:
		return AMP_OK;
	}
}
