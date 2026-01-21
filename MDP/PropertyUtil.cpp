# include "PropertyUtil.h"
# include "HardwareConfig.h"

t_VersionNumber apiVer = { 3, 2, 0, 0 };
t_VersionNumber libraryVer = { 1, 22, 2, 28 };

BleDeviceManager* device;

// Helper functions for setting property values
template <typename T>
static int SetVal(void* dest, uint32_t destSize, const T& val) {
	if (destSize < sizeof(T)) return AMP_ERR_PARAM;
	
	memcpy(dest, &val, sizeof(T));
	return AMP_OK;
}

static int SetStr(void* dest, uint32_t destSize, const char* val) {
	if (!dest || destSize == 0) return AMP_ERR_PARAM;
	strncpy_s((char*)dest, destSize, val, _TRUNCATE);
	return AMP_OK;
}

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

template <typename T>
static int SetRangeDiscrete(void* dest, uint32_t* destSize, t_PropertyRangeType* type, const std::vector<T>& values) {
	size_t requiredSize = values.size() * sizeof(T);

	if (*destSize < requiredSize) {
		return AMP_ERR_BUFFERSIZE;
	}

	T* arr = (T*)dest;
	for (size_t i = 0; i < values.size(); ++i) {
		arr[i] = values[i];
	}

	*destSize = (uint32_t)requiredSize;
	*type = RT_DISCRETE;
	return AMP_OK;
}

// Get device properties
int GetDeviceProperty(int32_t PropertyID, void* PropertyValue, uint32_t ValueByteSize) {
	switch (PropertyID) {
		// String properties
	case DPROP_CHR_Family:
		return SetVal(PropertyValue, ValueByteSize, "0");
	case DPROP_CHR_Type:
	{
		// Get device type from hardware config (parsed from BLE name)
		std::string deviceType = g_HardwareConfig.GetDeviceType();
		return SetStr(PropertyValue, ValueByteSize, deviceType.c_str());
	}
	case DPROP_CHR_Interface:
		return SetStr(PropertyValue, ValueByteSize, "BT");
	case DPROP_CHR_Address:
	{
		// Get MAC address from hardware config (from BLE device)
		std::string deviceAddr = g_HardwareConfig.GetDeviceAddress();
		return SetStr(PropertyValue, ValueByteSize, deviceAddr.c_str());
	}
	case DPROP_CHR_SerialNumber:
	{
		// Get serial number from hardware config (parsed from BLE name)
		std::string serialNum = g_HardwareConfig.GetDeviceSerialNumber();
		return SetStr(PropertyValue, ValueByteSize, serialNum.c_str());
	}
	case DPROP_CHR_FlashWorkspaceDescription:
	case DPROP_CHR_FlashFileName:
		return AMP_ERR_VERSION;

		// Version properties
		case DPROP_TVN_HardwareRevision:
		case DPROP_TVN_FirmwareVersion:
			return SetVal(PropertyValue, ValueByteSize, apiVer);
		case DPROP_TVN_DriverVersion:
			return SetVal(PropertyValue, ValueByteSize, libraryVer);

		// Int32 properties
		case DPROP_I32_AvailableModules:
			return SetVal(PropertyValue, ValueByteSize, (int32_t)1);
		case DPROP_I32_AvailableChannels:
			return SetVal(PropertyValue, ValueByteSize, (int32_t)8);
		case DPROP_I32_BatteryLevel:
			return SetVal(PropertyValue, ValueByteSize, g_HardwareConfig.batteryLevel);
		case DPROP_I32_RecordingMode:
			return SetVal(PropertyValue, ValueByteSize, (int32_t)device->recordingMode);
		case DPROP_I32_SignalQuality:
			return SetVal(PropertyValue, ValueByteSize, g_HardwareConfig.signalQuality);
		case DPROP_I32_SignalStrength:
			return SetVal(PropertyValue, ValueByteSize, g_HardwareConfig.signalStrength);
		case DPROP_I32_GoodImpedanceLevel:
			return SetVal(PropertyValue, ValueByteSize, g_HardwareConfig.goodImpedanceLevel);
		case DPROP_I32_BadImpedanceLevel:
			return SetVal(PropertyValue, ValueByteSize, g_HardwareConfig.badImpedanceLevel);
		case DPROP_B32_ContinuousImpedance:
			return SetVal(PropertyValue, ValueByteSize, (int32_t)g_HardwareConfig.continuousImpedance);
		case DPROP_I32_LedControl:
			return SetVal(PropertyValue, ValueByteSize, g_HardwareConfig.ledControl);
		case DPROP_I32_ConnectionState:
		case DPROP_I32_RecordingState:
		case DPROP_I32_ActiveShieldGain:
		case DPROP_B32_FastDataAccess:
		case DPROP_B32_FlashFormatting:
			return AMP_ERR_VERSION;

		// UInt32 properties
		case DPROP_UI32_FlashSegmentSize:
		case DPROP_UI32_ErrorFlags:
		case DPROP_UI32_FlashRecordingState:
		case DPROP_UI32_FlashFreeSpace:
		case DPROP_UI32_FlashFileSize:
			return AMP_ERR_VERSION;

		// Float32 properties
		case DPROP_F32_BaseSampleRate:
			return SetVal(PropertyValue, ValueByteSize, g_HardwareConfig.GetSampleRate());
		case DPROP_F32_BatteryVoltage:
			return SetVal(PropertyValue, ValueByteSize, g_HardwareConfig.batteryVoltage);
		case DPROP_F32_SubSampleDivisor:
			return SetVal(PropertyValue, ValueByteSize, g_HardwareConfig.subSampleDivisor);

		default:
			return AMP_ERR_PARAM;
	}
}

// Get module properties
int GetModuleProperty(int32_t PropertyID, void* PropertyValue, uint32_t ValueByteSize) {
	switch (PropertyID) {
		// String properties
		case MPROP_CHR_Type:
			return SetStr(PropertyValue, ValueByteSize, "Module_MT");
		case MPROP_CHR_SerialNumber:
			return SetStr(PropertyValue, ValueByteSize, "000");

		// Version properties
		case MPROP_TVN_HardwareRevision:
		case MPROP_TVN_FirmwareVersion:
			return SetVal(PropertyValue, ValueByteSize, apiVer);

		// Int32 properties
		case MPROP_I32_UseableChannels:
			return SetVal(PropertyValue, ValueByteSize, (int32_t)8);
		case MPROP_B32_ImpedanceMeasurement:
			return SetVal(PropertyValue, ValueByteSize, (int32_t)1);
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

// Get channel properties
int GetChannelProperty(int32_t PropertyID, void* PropertyValue, uint32_t ValueByteSize, uint32_t Index) {
	switch (PropertyID) {
		// String properties
		case CPROP_CHR_Function:
		case CPROP_CHR_Unit:
			return SetStr(PropertyValue, ValueByteSize, "");

		case CPROP_I32_ChannelName: 
			return SetVal(PropertyValue, ValueByteSize, (int32_t)10);

		// Int32 properties
		case CPROP_I32_Type:
			return SetVal(PropertyValue, ValueByteSize, (int32_t)CT_EEG);
		case CPROP_I32_ChannelNumber:
			return SetVal(PropertyValue, ValueByteSize, (int32_t)Index);
		case CPROP_I32_ModuleNumber:
			return SetVal(PropertyValue, ValueByteSize, (int32_t)0);
		case CPROP_I32_Electrode:
		case CPROP_I32_DataType:
			return SetVal(PropertyValue, ValueByteSize, (int32_t)6);
		case CPROP_I32_LedColor:
			return SetVal(PropertyValue, ValueByteSize, (int32_t)0);
		case CPROP_B32_ReferenceChannel:
		case CPROP_B32_ImpedanceMeasurement:
		case CPROP_B32_RecordingEnabled: {
			auto config = g_HardwareConfig.GetChannelConfig(Index);
			return SetVal(PropertyValue, ValueByteSize, (int32_t)(config.enabled ? 1 : 0));
		}

		// UInt32 properties
		case CPROP_UI32_OutputValue:
			return SetVal(PropertyValue, ValueByteSize, (uint32_t)0);

		// Float32 properties
		case CPROP_F32_Resolution:
			return SetVal(PropertyValue, ValueByteSize, (float)1);
		case CPROP_F32_Gain: {
			auto config = g_HardwareConfig.GetChannelConfig(Index);
			return SetVal(PropertyValue, ValueByteSize, config.gain);
		}
		case CPROP_F32_HighPass: {
			auto config = g_HardwareConfig.GetChannelConfig(Index);
			return SetVal(PropertyValue, ValueByteSize, config.highPassHz);
		}
		case CPROP_F32_LowPass: {
			auto config = g_HardwareConfig.GetChannelConfig(Index);
			return SetVal(PropertyValue, ValueByteSize, config.lowPassHz);
		}
		case CPROP_F32_NotchFilter: {
			auto config = g_HardwareConfig.GetChannelConfig(Index);
			return SetVal(PropertyValue, ValueByteSize, config.notchHz);
		}

		default:
			return AMP_ERR_PARAM;
	}
}

// Set device properties
int SetDeviceProperty(int32_t PropertyID, void* PropertyValue, uint32_t ValueByteSize) {
	switch (PropertyID) {
		// Writable properties
		case DPROP_F32_BaseSampleRate: {
			// Set sampling rate (will take effect when hardware command is implemented)
			if (ValueByteSize < sizeof(float))
				return AMP_ERR_PARAM;
			float rate;
			memcpy(&rate, PropertyValue, sizeof(float));
			if (g_HardwareConfig.SetSampleRate(rate)) {
				// TODO: Send command to hardware to set sample rate
				// vector<uint8_t> cmd = buildSetSampleRateCommand(rate);
				// device->sendCommand(cmd);
				return AMP_OK;
			}
			return AMP_ERR_PARAM;
		}
		case DPROP_F32_SubSampleDivisor: {
			// Set sub-sample divisor (will take effect when hardware command is implemented)
			if (ValueByteSize < sizeof(float))
				return AMP_ERR_PARAM;
			memcpy(&g_HardwareConfig.subSampleDivisor, PropertyValue, sizeof(float));
			// TODO: Send command to hardware to set sub-sample divisor
			return AMP_OK;
		}
		case DPROP_I32_RecordingMode: {
			// Set recording mode
			memcpy(&device->recordingMode, PropertyValue, sizeof(int));
			// TODO: Send command to hardware to switch mode
			// Command is handled in BleDeviceManager::startAcquisition
			return AMP_OK;
		}

		// Read-only properties
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

// Set module properties
int SetModuleProperty(int32_t PropertyID, void* PropertyValue, uint32_t ValueByteSize) {
	switch (PropertyID) {
		// Writable properties
		// TODO: Add writable module properties when needed

		// Read-only properties
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

// Set channel properties
int SetChannelProperty(int32_t PropertyID, void* PropertyValue, uint32_t ValueByteSize, uint32_t Index) {
	switch (PropertyID) {
		// Writable properties
		case CPROP_F32_Gain: {
			// Set channel gain (will take effect when hardware command is implemented)
			if (ValueByteSize < sizeof(float) || Index >= 8)
				return AMP_ERR_PARAM;
			auto config = g_HardwareConfig.GetChannelConfig(Index);
			memcpy(&config.gain, PropertyValue, sizeof(float));
			g_HardwareConfig.SetChannelConfig(Index, config);
			// TODO: Send command to hardware to set channel gain
			return AMP_OK;
		}
		case CPROP_F32_HighPass: {
			// Set high-pass filter (will take effect when hardware command is implemented)
			if (ValueByteSize < sizeof(float) || Index >= 8)
				return AMP_ERR_PARAM;
			auto config = g_HardwareConfig.GetChannelConfig(Index);
			memcpy(&config.highPassHz, PropertyValue, sizeof(float));
			g_HardwareConfig.SetChannelConfig(Index, config);
			// TODO: Send command to hardware to set high-pass filter
			return AMP_OK;
		}
		case CPROP_F32_LowPass: {
			// Set low-pass filter (will take effect when hardware command is implemented)
			if (ValueByteSize < sizeof(float) || Index >= 8)
				return AMP_ERR_PARAM;
			auto config = g_HardwareConfig.GetChannelConfig(Index);
			memcpy(&config.lowPassHz, PropertyValue, sizeof(float));
			g_HardwareConfig.SetChannelConfig(Index, config);
			// TODO: Send command to hardware to set low-pass filter
			return AMP_OK;
		}
		case CPROP_F32_NotchFilter: {
			// Set notch filter (will take effect when hardware command is implemented)
			if (ValueByteSize < sizeof(float) || Index >= 8)
				return AMP_ERR_PARAM;
			auto config = g_HardwareConfig.GetChannelConfig(Index);
			memcpy(&config.notchHz, PropertyValue, sizeof(float));
			g_HardwareConfig.SetChannelConfig(Index, config);
			// TODO: Send command to hardware to set notch filter
			return AMP_OK;
		}
		case CPROP_B32_RecordingEnabled: {
			// Set channel enable (will take effect when hardware command is implemented)
			if (ValueByteSize < sizeof(int32_t) || Index >= 8)
				return AMP_ERR_PARAM;
			auto config = g_HardwareConfig.GetChannelConfig(Index);
			int32_t enabled;
			memcpy(&enabled, PropertyValue, sizeof(int32_t));
			config.enabled = (enabled != 0);
			g_HardwareConfig.SetChannelConfig(Index, config);
			// TODO: Send command to hardware to set channel enable
			return AMP_OK;
		}

		// Read-only properties
		case CPROP_I32_Type:
		case CPROP_I32_ChannelNumber:
		case CPROP_I32_ModuleNumber:
		case CPROP_CHR_Function:
		case CPROP_I32_Electrode:
		case CPROP_I32_DataType:
		case CPROP_F32_Resolution:
		case CPROP_CHR_Unit:
		case CPROP_B32_ReferenceChannel:
		case CPROP_B32_ImpedanceMeasurement:
		case CPROP_I32_LedColor:
		case CPROP_UI32_OutputValue:
		case CPROP_I32_ChannelName:
			return AMP_ERR_VERSION;

		default:
			return AMP_ERR_PARAM;
	}
}

// Get device property range
int GetDevicePropertyRange(int32_t PropertyID, void* RangeArray, uint32_t* ArrayByteSize, t_PropertyRangeType* RangeType) {
	*RangeType = RT_READONLY;

	switch (PropertyID) {
	case DPROP_I32_RecordingMode:
		return SetRangeMinMax(RangeArray, ArrayByteSize, RangeType, (int32_t)RM_STOPPED, (int32_t)RM_TEST);
	case DPROP_F32_BaseSampleRate:
		return SetRangeDiscrete(RangeArray, ArrayByteSize, RangeType, std::vector<float>{125.0f, 250.0f, 500.0f});
	case DPROP_F32_SubSampleDivisor:
		return SetRangeDiscrete(RangeArray, ArrayByteSize, RangeType, std::vector<float>{1.0f, 2.0f});

	default:
		return AMP_OK;
	}
}

// Get module property range
int GetModulePropertyRange(int32_t PropertyID, void* RangeArray, uint32_t* ArrayByteSize, t_PropertyRangeType* RangeType) {
	*RangeType = RT_READONLY;

	switch (PropertyID) {
	case MPROP_I32_TriggerOutMode:
		return SetRangeMinMax(RangeArray, ArrayByteSize, RangeType, (int32_t)TM_DEFAULT, (int32_t)TM_MIRROR_SYNC);
	case MPROP_I32_LedColorREF:
	case MPROP_I32_LedColorGND:
		return SetRangeMinMax(RangeArray, ArrayByteSize, RangeType, (int32_t)LED_OFF, (int32_t)LED_YELLOW);
	case MPROP_I32_UserButtonLed:
		return SetRangeMinMax(RangeArray, ArrayByteSize, RangeType, (int32_t)100, (int32_t)2000);
	default:
		return AMP_OK;
	}
}

// Get channel property range
int GetChannelPropertyRange(int32_t PropertyID, void* RangeArray, uint32_t* ArrayByteSize, t_PropertyRangeType* RangeType, uint32_t Index) {
	*RangeType = RT_READONLY;

	switch (PropertyID) {
	case CPROP_B32_RecordingEnabled:
		return SetRangeMinMax(RangeArray, ArrayByteSize, RangeType, (int32_t)0, (int32_t)1);
	case CPROP_I32_LedColor:
		return SetRangeMinMax(RangeArray, ArrayByteSize, RangeType, (int32_t)LED_OFF, (int32_t)LED_YELLOW);
	case CPROP_F32_Gain:
		// ADS1299 supports gains: 1, 2, 4, 6, 8, 12, 24
		return SetRangeDiscrete(RangeArray, ArrayByteSize, RangeType, std::vector<float>{1.0f, 2.0f, 4.0f, 6.0f, 8.0f, 12.0f, 24.0f});
	case CPROP_F32_HighPass:
	case CPROP_F32_LowPass:
	case CPROP_F32_NotchFilter:
		return SetRangeMinMax(RangeArray, ArrayByteSize, RangeType, 0.0f, 100.0f);
	default:
		return AMP_OK;
	}
}

bool SetBleDeviceManager(BleDeviceManager* bleDevice)
{
	device = bleDevice;
	return true;
}
