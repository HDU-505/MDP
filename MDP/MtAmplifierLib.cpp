# include "pch.h"
# include "Amplifier_LIB.h"
# include<string>
# include "bt/BleHandle.h"
# include "bt/BLEComm.h"
# include "bt/BleDeviceManager.h"
# include "protocol/ProtocolManager.h"
# include "PropertyUtil.h"

// Revision Info: API
const int32_t AP_MAJOR = 3;
const int32_t AP_MINOR = 2;
const int32_t AP_BUILD = 0;
const int32_t AP_REVISION = 0;

// Revision Info: Library
const int32_t LIB_MAJOR = 1;
const int32_t LIB_MINOR = 22;
const int32_t LIB_BUILD = 2;
const int32_t LIB_REVISION = 28;

using namespace std;


// SDK鍏ㄥ眬绠＄悊鍣ㄥ璞?
protocol::ProtocolManager protocolManager(RecordingMode::RM_NORMAL);
BleDeviceManager bleDeviceManager(&protocolManager);

// BLE 鏁版嵁鎺ユ敹鍥炶皟灏嗘敹鍒扮殑鍘熷鏁版嵁浜ょ粰鍗忚绠＄悊鍣ㄥ鐞?
void MtBleDeviceRecvDataCallBack(HANDLE handle, unsigned int ServiceUUID, unsigned int CharacteristicUUID, unsigned char* recvData, unsigned int length) {
	protocolManager.processData(recvData, length);
}

// BLE 鎵弿鍙戠幇璁惧鍥炶皟
void MtScanedBleDeviceCallBack(const char* ID, const char* PenName, const char* PenMac, int rssi, DataSection* DataSections, int DataSectionCount)
{
	// 閫氳繃鍚嶅瓧杩囨护 Mindtooth 璁惧骞朵繚瀛樺畬鏁寸殑璁惧淇℃伅 (鍚嶇О + MAC)
	if (string(PenName).find("Mindtooth") != string::npos) {
		std::cout << "find mindtooth" << std::endl;
		bleDeviceManager.addDevice(ID, PenName, PenMac);
	}
}

void MtScanFinishBack()
{
	{
		std::lock_guard<std::mutex> lock(bleDeviceManager.scanMtx);
		bleDeviceManager.scanFinished = true;
		bleDeviceManager.scanning = false;
	}
	bleDeviceManager.scanCv.notify_one();
}

void MtConnectionBleDeviceStatusCallBack(HANDLE handle, const char* PenMac, bool IsConnect) {
	{
		std::lock_guard<std::mutex> lock(bleDeviceManager.connMtx);
		bleDeviceManager.isConnected = IsConnect;

	}
	bleDeviceManager.connCv.notify_one();
	
	// Bluetooth event status print tracking
	if (IsConnect) {
		sdk::Logger::Info("Device connected stream active: " + string(PenMac ? PenMac : "unknown"));
	} else {
		sdk::Logger::Log(sdk::LogLevel::WARNING, sdk::ErrorCategory::BLUETOOTH, "Device detached or lost: " + string(PenMac ? PenMac : "unknown"));
	}
}

/// <summary>    Get API version info </summary>
/// <param name="pAPIVersion">   [out] API version structure
///          Major Version = 0
///          Minor Version = 0
///          Build Number = Unused
///          Revision Number = Unused</param>
/// <returns>Status code</returns>
AMPAPI GetAPIVersion(t_VersionNumber* pAPIVersion) {
	pAPIVersion->Major = AP_MAJOR;
	pAPIVersion->Minor = AP_MINOR;
	pAPIVersion->Build = AP_BUILD;
	pAPIVersion->Revision = AP_REVISION;
	return AMP_OK;
}

/// <summary>    Get Library version info </summary>
/// <param name="pLibraryVersion">   [out] Library version structure
///          Major Version = Lib major version
///          Minor Version = Lib minor version
///          Build Number = Lib build version
///          Revision Number = Unused</param>
/// <returns>Status code</returns>
AMPAPI GetLibraryVersion(t_VersionNumber* pLibraryVersion) {

	pLibraryVersion->Major = LIB_MAJOR;
	pLibraryVersion->Minor = LIB_MINOR;
	pLibraryVersion->Build = LIB_BUILD;
	pLibraryVersion->Revision = LIB_REVISION;
	return AMP_OK;
}

/// <summary>    Enumerate devices on the system </summary>
/// <param name="HWI">              Hardware ID, pass "ANY" to list all
///                                 "ANY" must be null terminated
///                                 Supports: "ANY", "USB", "BT" or "SIM"</param>
/// <param name="HWISize">          Length of HWI</param>
/// <param name="DeviceAddress">    Device physical address (MAC or Port)</param>
/// <param name="flags">            Additional enum flags</param>
/// <returns>    Total number of devices found</returns>
AMPAPI ampEnumerateDevices(char* HWI, int32_t HWISize, const char* DeviceAddress, uint32_t flags) {

	if (!HWI || HWISize <= 0) {
		return AMP_ERR_PARAM;
	}
	std::string hwi = (HWI != nullptr) ? HWI : "";

	if (hwi == "BT") {
		// 钃濈墮鎵弿
	}
	else if (hwi == "USB") {
		return AMP_ERR_VERSION;
	}
	else if (hwi == "SIM") {
		return AMP_ERR_VERSION;
	}
	else if (hwi == "ANY") {
		// 鐩墠涓嶆敮鎸?USB锛岃繑鍥?
	}

	// 
	RegisterRecvBleDevice(MtScanedBleDeviceCallBack);
	RegisterSacnBleDeviceFinish(MtScanFinishBack);
	RegisterBleDeviceRecvData(MtBleDeviceRecvDataCallBack);

	// Enhanced scan with retry: 10s scan time, up to 3 retries
	// This helps find devices that may not be advertising continuously
	return bleDeviceManager.searchDevice(10000, 3);
}

/// <summary>    Get device address </summary>
/// <param name="DeviceNr">         Device index, starting from 0</param>
/// <param name="DeviceAddress">    Buffer to receive the address</param>
/// <param name="BufferSize">       Buffer size</param>
/// <returns>    Status code </returns>
AMPAPI ampGetDeviceAddress(int32_t DeviceNr, char* DeviceAddress, int32_t BufferSize) {

	if (!DeviceAddress || BufferSize <= 0) return AMP_ERR_PARAM;
	if (DeviceNr != 0) return AMP_ERR_NODEVICE;
	strncpy_s(DeviceAddress, BufferSize, "FAKE_BT_DEVICE_0", _TRUNCATE);
	return AMP_OK;
}

/// <summary>    Open and connect device </summary>
/// <param name="DeviceNr">         Device index, starting from 0</param>
/// <param name="DeviceHandle">     Pointer to receive the opened device handle</param>
/// <returns>    Status code </returns>
AMPAPI ampOpenDevice(int32_t DeviceNr, HANDLE* DeviceHandle) {

	HANDLE handle = bleDeviceManager.openDevice(DeviceNr);
	if (!handle) {
		return AMP_ERR_NODEVICE;
	}
	*DeviceHandle = handle;
	SetBleDeviceManager(&bleDeviceManager);
	return AMP_OK;
}

/// <summary>    Get device property value </summary>
/// <param name="DeviceHandle">     Device handle</param>
/// <param name="PropertyGroup">    Property group enum</param>
/// <param name="Index">            Index inside the property group
///                                 Fill 0 if unnecessary (e.g. fill 0 for global)</param>
/// <param name="PropertyID">       The Property ID to query</param>
/// <param name="PropertyValue">    Pointer to receive the property value</param>
/// <param name="ValueByteSize">    Available byte size for buffer</param>
AMPAPI ampGetProperty(
	HANDLE DeviceHandle,
	t_PropertyGroup PropertyGroup,
	uint32_t Index,
	int32_t PropertyID,
	void* PropertyValue,
	uint32_t ValueByteSize
) {

	if (!DeviceHandle || !PropertyValue || ValueByteSize == 0)
		return AMP_ERR_PARAM;

	memset(PropertyValue, 0, ValueByteSize);
	switch (PropertyGroup) {
	case PG_DEVICE:
		return GetDeviceProperty(PropertyID, PropertyValue, ValueByteSize); 
	case PG_MODULE:
		return GetModuleProperty(PropertyID, PropertyValue, ValueByteSize);
	case PG_CHANNEL:
		return GetChannelProperty(PropertyID, PropertyValue, ValueByteSize, Index);
	default:
		return AMP_ERR_PARAM;
	}
	return AMP_OK;
}

/// <summary>    Set device property value </summary>
/// <param name="DeviceHandle">     Device handle</param>
/// <param name="PropertyGroup">    Property group enum</param>
/// <param name="Index">            Index inside the property group</param>
/// <param name="PropertyID">       The Property ID to alter</param>
/// <param name="PropertyValue">    Pointer containing the new value</param>
/// <param name="ValueByteSize">    Byte length of the new value</param>
AMPAPI ampSetProperty(
	HANDLE DeviceHandle,
	t_PropertyGroup PropertyGroup,
	uint32_t Index,
	int32_t PropertyID,
	void* PropertyValue,
	uint32_t ValueByteSize
) {
	if (!DeviceHandle || !PropertyValue || ValueByteSize == 0)
		return AMP_ERR_PARAM;

	if (PropertyID == DPROP_I32_RecordingMode) {
		int value = *static_cast<int*>(PropertyValue);
	}

	switch (PropertyGroup) {
	case PG_DEVICE:
		return SetDeviceProperty(PropertyID, PropertyValue, ValueByteSize);
	case PG_MODULE:
		return SetModuleProperty(PropertyID, PropertyValue, ValueByteSize);
	case PG_CHANNEL:
		return SetChannelProperty(PropertyID, PropertyValue, ValueByteSize, Index);
	default:
		return AMP_ERR_PARAM;
	}
}
/// <summary>    Get the bounds or enumeration limitations for a property </summary>
/// <param name="DeviceHandle">     Device handle</param>
/// <param name="PropertyGroup">    Property group enum</param>
/// <param name="Index">            Index inside the property group</param>
/// <param name="PropertyID">       Target property ID</param>
/// <param name="RangeArray">       Buffer pointer to receive bounds</param>
/// <param name="ArrayByteSize">    Capacity array limit, mutates to actual requested size upon return</param>
/// <param name="RangeType">        Returns whether the limit is an interval or a discrete set</param>
AMPAPI ampGetPropertyRange(
	HANDLE DeviceHandle,
	t_PropertyGroup PropertyGroup,
	uint32_t Index,
	int32_t PropertyID,
	void* RangeArray,
	uint32_t* ArrayByteSize,
	t_PropertyRangeType* RangeType
) {

	if (!PropertyID || !RangeArray || !ArrayByteSize || !RangeType)
		return AMP_ERR_PARAM;

	memset(RangeArray, 0, *ArrayByteSize);
	switch (PropertyGroup) {
		case PG_DEVICE:
			return GetDevicePropertyRange(PropertyID, RangeArray, ArrayByteSize, RangeType);
		case PG_MODULE:
			return GetModulePropertyRange(PropertyID, RangeArray, ArrayByteSize, RangeType);
		case PG_CHANNEL:
			return GetChannelPropertyRange(PropertyID, RangeArray, ArrayByteSize, RangeType, Index);
		default:
			return AMP_ERR_PARAM;
	}
}


/// <summary>    Start acquisition </summary>
/// <param name="DeviceHandle">     Device handle</param>
/// <returns>    Status code </returns>
AMPAPI ampStartAcquisition(HANDLE DeviceHandle) {

	return bleDeviceManager.startAcquisition(DeviceHandle) ? AMP_OK : AMP_ERR_BUSY;
}
/// <summary>    Stop acquisition </summary>
/// <param name="DeviceHandle">     Device handle</param>
/// <returns>    Status code </returns>
AMPAPI ampStopAcquisition(HANDLE DeviceHandle) {

	return bleDeviceManager.stopAcquisition(DeviceHandle) ? AMP_OK : AMP_ERR_BUSY;
}

/// <summary>    Close device </summary>
/// <param name="DeviceHandle">     Device handle</param>
/// <returns>    Status code </returns>
AMPAPI ampCloseDevice(HANDLE DeviceHandle) {

	return bleDeviceManager.closeDevice(DeviceHandle) ? AMP_OK : AMP_ERR_BUSY;
}
/// <summary>    Set digital port </summary>
/// <param name="DeviceHandle">     Device handle</param>
/// <param name="PortNumber">       Port number</param>
/// <param name="value">            Target value</param>
/// <returns>    Status code </returns>
AMPAPI ampSetDigitalPort(HANDLE DeviceHandle, int32_t PortNumber, uint32_t value) {

	return AMP_ERR_NOSUPPORT;
}

/// <summary>    
///              Get acquired data
///              
///              S1_SAMPLECOUNTER, S1_CH1 .. S1_CHn,
///              S2_SAMPLECOUNTER, S2_CH1 .. S2_CHn,
///              ...
///              Sn_SAMPLECOUNTER, Sn_CH1 .. Sn_CHn
///              
///              64
/// </summary>
/// <param name="DeviceHandle">         Device handle</param>
/// <param name="Buffer">               Buffer pointer to retrieve samples</param>
/// <param name="BufferSize">           Available raw byte capacity of Buffer pointer</param>
/// <param name="RequestedSamples">     Maximum sampling block to extract per round</param>
/// <returns>Amount of data returned in bytes</returns>
AMPAPI ampGetData(HANDLE DeviceHandle, void* Buffer, int32_t BufferSize, int32_t RequestedSamples) {

	if (!Buffer || BufferSize <= 0 || RequestedSamples <= 0) {
		return IF_ERR_PARAMETER;
	}

	int sampleLen = protocolManager.getSampleLength();
	if (sampleLen <= 0) {
		return IF_ERR_PARAMETER;
	}

	// Buffer max possible samples
	int maxSampleCount = BufferSize / sampleLen;
	if (maxSampleCount <= 0) {
		return IF_ERR_PARAMETER;
	}

	// Clamp limits
	int requestCount = min(RequestedSamples, maxSampleCount);

	// Get EEG data stream 
	vector<uint8_t> data = protocolManager.getEEGData(requestCount);

	if (data.empty()) {
		return 0;
	}

	// Calculate copy size and clamp to buffer limit mapping
	size_t bytesToCopy = min(data.size(), static_cast<size_t>(BufferSize));
	// Snap frame boundary limit
	bytesToCopy = (bytesToCopy / sampleLen) * sampleLen;

	if (bytesToCopy == 0) {
		return 0;
	}

	memcpy(Buffer, data.data(), bytesToCopy);

	return static_cast<int>(bytesToCopy);
}


/// <summary>    
///              Get realtime impedance mapping
///              
///              M0 GND??, M0 REF??, ... Mn GND, Mn REF, CH1+, CH1-, CH2+, CH2-, .. CHn+, CHn-
///              Require MPROP_B32_ImpedanceMeasurement property
///              float array return
///              CH--1 limit logic
/// <param name="DeviceHandle">     Device handle</param>
/// <param name="Buffer">           Float buffer pointer targeting property output</param>
/// <param name="BufferSize">       Buffer byte limits</param>
/// <returns>Bytes read</returns>
AMPAPI ampGetImpedanceData(HANDLE DeviceHandle, void* Buffer, int32_t BufferSize) {
	if (!Buffer || BufferSize <= 0) {
		return IF_ERR_PARAMETER;
	}

	// Get real-time impedance (uses latest 248 samples from sliding window)
	vector<float> impedances = protocolManager.getImpedanceData();

	if (impedances.empty()) {
		return 0;  // No data available
	}

	// Calculate bytes to copy (up to buffer size)
	size_t bytesToCopy = min(
		static_cast<size_t>(BufferSize),
		impedances.size() * sizeof(float)
	);

	memcpy(Buffer, impedances.data(), bytesToCopy);

	return static_cast<int>(bytesToCopy);
}

/// <summary>     Start device firmware flash records (Flash) </summary>
/// <param name="DeviceHandle">     Device handle </param>
/// <returns>    Status code </returns>
AMPAPI ampStartFlashRecording(HANDLE DeviceHandle) {
	return AMP_OK;
}

/// <summary>     Stop flash recording </summary>
/// <param name="DeviceHandle">     Device handle</param>
/// <returns>    Status code </returns>
AMPAPI ampStopFlashRecording(HANDLE DeviceHandle) {
	return AMP_OK;
}
