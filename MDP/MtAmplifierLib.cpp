# include "pch.h"
# include "Amplifier_LIB.h"
# include<string>
# include "bt/BleHandle.h"
# include "bt/BLEComm.h"
# include "bt/BleDeviceManager.h"
# include "protocol/ProtocolManager.h"
# include "PropertyUtil.h"

// ?????????
const int32_t AP_MAJOR = 3;
const int32_t AP_MINOR = 2;
const int32_t AP_BUILD = 0;
const int32_t AP_REVISION = 0;

// ?????
const int32_t LIB_MAJOR = 1;
const int32_t LIB_MINOR = 22;
const int32_t LIB_BUILD = 2;
const int32_t LIB_REVISION = 28;

using namespace std;


// SDK????????????
protocol::ProtocolManager protocolManager(RecordingMode::RM_NORMAL);
BleDeviceManager bleDeviceManager(&protocolManager);

// ????????????????????????????????
void MtBleDeviceRecvDataCallBack(HANDLE handle, unsigned int ServiceUUID, unsigned int CharacteristicUUID, unsigned char* recvData, unsigned int length) {
	// ?????????????????? UUID
	//std::cout << "Received data from service UUID: " << std::hex << ServiceUUID << " characteristic UUID: " << CharacteristicUUID << std::dec << std::endl;
	// ??????????????
	//std::cout << "Received data (" << length << " bytes): ";
	//for (unsigned int i = 0; i < length; ++i) {
	//    std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)recvData[i] << " ";
	//}
	//std::cout << std::dec << std::endl; // Reset hex format to decimal for further prints

	protocolManager.processData(recvData, length);
	//assembler.checkTimeout();
}

void MtScanedBleDeviceCallBack(const char* ID, const char* PenName, const char* PenMac, int rssi, DataSection* DataSections, int DataSectionCount)
{
	// Filter devices by name and save full device info (name + MAC)
	if (string(PenName).find("Mindtooth") != string::npos) {
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
}

/// <summary>    ????????????? </summary>
 /// <param name="pAPIVersion">   [out] API??
 ///          ????? = 0
 ///          ?????? = 0
 ///          ????? = ???
 ///          ?????? = ????</param>
 /// <returns>???????</returns>
AMPAPI GetAPIVersion(t_VersionNumber* pAPIVersion) {
	pAPIVersion->Major = AP_MAJOR;
	pAPIVersion->Minor = AP_MINOR;
	pAPIVersion->Build = AP_BUILD;
	pAPIVersion->Revision = AP_REVISION;
	return AMP_OK;
}

/// <summary>    ???????? </summary>
/// <param name="pLibraryVersion">   [out] ???
///          ????? = ????????
///          ?????? = ??????
///          ????? = ???????
///          ?????? = ????</param>
/// <returns>???????</returns>
AMPAPI GetLibraryVersion(t_VersionNumber* pLibraryVersion) {

	pLibraryVersion->Major = LIB_MAJOR;
	pLibraryVersion->Minor = LIB_MINOR;
	pLibraryVersion->Build = LIB_BUILD;
	pLibraryVersion->Revision = LIB_REVISION;
	return AMP_OK;
}

/// <summary>    ??????? </summary>
/// <param name="HWI">              ?????????????�????"ANY"??????
///                                 ??????"ANY"?????????????????????????
///                                 ?????????"ANY", "USB", "BT"??"SIM"</param>
/// <param name="HWISize">          ??????????????</param>
/// <param name="DeviceAddress">    ?????????</param>
/// <param name="flags">            ???????</param>
/// <returns>    ??????????</returns>
AMPAPI ampEnumerateDevices(char* HWI, int32_t HWISize, const char* DeviceAddress, uint32_t flags) {

	if (!HWI || HWISize <= 0) {
		return AMP_ERR_PARAM;
	}
	std::string hwi = (HWI != nullptr) ? HWI : "";

	if (hwi == "BT") {
		// ???
	}
	else if (hwi == "USB") {
		return AMP_ERR_VERSION;
	}
	else if (hwi == "SIM") {
		return AMP_ERR_VERSION;
	}
	else if (hwi == "ANY") {
		// ????BT??
	}

	// ?????
	RegisterRecvBleDevice(MtScanedBleDeviceCallBack);
	RegisterSacnBleDeviceFinish(MtScanFinishBack);
	RegisterBleDeviceRecvData(MtBleDeviceRecvDataCallBack);

	// Enhanced scan with retry: 10s scan time, up to 3 retries
	// This helps find devices that may not be advertising continuously
	return bleDeviceManager.searchDevice(10000, 3);
}

/// <summary>    ???????? </summary>
/// <param name="DeviceNr">         ??0??????????</param>
/// <param name="DeviceAddress">    ???????????</param>
/// <param name="BufferSize">       ?????????</param>
/// <returns>    . </returns>
AMPAPI ampGetDeviceAddress(int32_t DeviceNr, char* DeviceAddress, int32_t BufferSize) {

	if (!DeviceAddress || BufferSize <= 0) return AMP_ERR_PARAM;
	if (DeviceNr != 0) return AMP_ERR_NODEVICE;
	strncpy_s(DeviceAddress, BufferSize, "FAKE_BT_DEVICE_0", _TRUNCATE);
	return AMP_OK;
}

/// <summary>    ???? </summary>
/// <param name="DeviceNr">         ??0??????????</param>
/// <param name="DeviceHandle">     ?????????</param>
/// <returns>    . </returns>
AMPAPI ampOpenDevice(int32_t DeviceNr, HANDLE* DeviceHandle) {

	HANDLE handle = bleDeviceManager.openDevice(DeviceNr);
	if (!handle) {
		return AMP_ERR_NODEVICE;
	}
	*DeviceHandle = handle;
	SetBleDeviceManager(&bleDeviceManager);
	return AMP_OK;
}

/// <summary>    ???????? </summary>
/// <param name="DeviceHandle">     ?????</param>
/// <param name="PropertyGroup">    ?????????</param>
/// <param name="Index">            ??????????????????????
///                                 ????????????0??????????
///                                 ????????????0?????????</param>
/// <param name="PropertyID">       ????????</param>
/// <param name="PropertyValue">    ???????????</param>
/// <param name="ValueByteSize">    ??????????(???)</param>
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

/// <summary>    ????????? </summary>
/// <param name="DeviceHandle">     ?????</param>
/// <param name="PropertyGroup">    ?????????</param>
/// <param name="Index">            ??????????????????????
///                                 ????????????0??????????
///                                 ????????????0?????????</param>
/// <param name="PropertyID">       ????????</param>
/// <param name="PropertyValue">    ???????????</param>
/// <param name="ValueByteSize">    ??????????(???)</param>
AMPAPI ampSetProperty(
	HANDLE DeviceHandle,
	t_PropertyGroup PropertyGroup,
	uint32_t Index,
	int32_t PropertyID,
	void* PropertyValue,
	uint32_t ValueByteSize
) {

	if (PropertyID == DPROP_I32_RecordingMode) {
		int value = *static_cast<int*>(PropertyValue);

		std::cout << "属性：DPROP_I32_RecordingMode: " << value << std::endl;
	}

	if (!DeviceHandle || !PropertyValue || ValueByteSize == 0)
		return AMP_ERR_PARAM;

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
/// <summary>    ????????????????? </summary>
/// <param name="DeviceHandle">     ?????</param>
/// <param name="PropertyGroup">    ?????????</param>
/// <param name="Index">            ??????????????????????
///                                 ????????????0??????????
///                                 ????????????0?????????</param>
/// <param name="PropertyID">       ????????</param>
/// <param name="RangeArray">       ??????????????
///                                 ??????????????????????????????????
///                                 ??????????RT_MINMAX????????????????????????
///                                 ????????????????????????????????????????LF??????</param>
/// <param name="ArrayByteSize">    ???????????(???)</param>
/// <param name="RangeType">        ??????????</param>
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


/// <summary>    ?????????? </summary>
/// <param name="DeviceHandle">     ?????</param>
/// <returns>    . </returns>
AMPAPI ampStartAcquisition(HANDLE DeviceHandle) {

	return bleDeviceManager.startAcquisition(DeviceHandle) ? AMP_OK : AMP_ERR_BUSY;
}
/// <summary>    ???????? </summary>
/// <param name="DeviceHandle">     ?????</param>
/// <returns>    . </returns>
AMPAPI ampStopAcquisition(HANDLE DeviceHandle) {

	return bleDeviceManager.stopAcquisition(DeviceHandle) ? AMP_OK : AMP_ERR_BUSY;
}

/// <summary>    ????? </summary>
/// <param name="DeviceHandle">     ?????</param>
/// <returns>    . </returns>
AMPAPI ampCloseDevice(HANDLE DeviceHandle) {

	return bleDeviceManager.closeDevice(DeviceHandle) ? AMP_OK : AMP_ERR_BUSY;
}
/// <summary>    ?????????? </summary>
/// <param name="DeviceHandle">     ?????</param>
/// <param name="PortNumber">       ????</param>
/// <param name="value">            ?</param>
/// <returns>    . </returns>
AMPAPI ampSetDigitalPort(HANDLE DeviceHandle, int32_t PortNumber, uint32_t value) {

	return AMP_ERR_NOSUPPORT;
}

/// <summary>    ????????????????
///              ????????????????
///              S1_SAMPLECOUNTER, S1_CH1 .. S1_CHn,
///              S2_SAMPLECOUNTER, S2_CH1 .. S2_CHn,
///              ...
///              Sn_SAMPLECOUNTER, Sn_CH1 .. Sn_CHn
///              ??????????????????????????????????
///              ????????????64??????????
/// </summary>
/// <param name="DeviceHandle">         ?????</param>
/// <param name="Buffer">               ?????????</param>
/// <param name="BufferSize">           ????????????(???)</param>
/// <param name="RequestedSamples">     ???????????(??????)</param>
/// <returns>??????????????????</returns>
AMPAPI ampGetData(HANDLE DeviceHandle, void* Buffer, int32_t BufferSize, int32_t RequestedSamples) {

	if (!Buffer || BufferSize <= 0 || RequestedSamples <= 0) {
		return IF_ERR_PARAMETER;
	}

	int sampleLen = protocolManager.getSampleLength();
	if (sampleLen <= 0) {
		return IF_ERR_PARAMETER;
	}

	// Buffer ????????????? sample
	int maxSampleCount = BufferSize / sampleLen;
	if (maxSampleCount <= 0) {
		return IF_ERR_PARAMETER;
	}

	// ???????? sample ??
	int requestCount = min(RequestedSamples, maxSampleCount);

	// 获取 EEG 数据（可能因插值产生比 requestCount 更多的帧）
	vector<uint8_t> data = protocolManager.getEEGData(requestCount);

	if (data.empty()) {
		return 0;
	}

	// 插值帧导致 data 可能大于 BufferSize，必须截断到 Buffer 容量
	size_t bytesToCopy = min(data.size(), static_cast<size_t>(BufferSize));
	// 对齐到整帧（不拷贝半帧）
	bytesToCopy = (bytesToCopy / sampleLen) * sampleLen;

	if (bytesToCopy == 0) {
		return 0;
	}

	memcpy(Buffer, data.data(), bytesToCopy);

	return static_cast<int>(bytesToCopy);
}


/// <summary>    ?????????????????
///              ????????????????
///              M0 GND??, M0 REF??, ... Mn GND, Mn REF, CH1+, CH1-, CH2+, CH2-, .. CHn+, CHn-
///              M0 - Mn??????MPROP_B32_ImpedanceMeasurement??????????????????
///              ??????????float??????[??]
///              ???????CH-???????????????????????-1
/// <param name="DeviceHandle">     ?????</param>
/// <param name="Buffer">           ?????????</param>
/// <param name="BufferSize">       ????????????(???)</param>
/// <returns>??????????????????</returns>
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

/// <summary>    ????????????? </summary>
/// <param name="DeviceHandle">     ?????</param>
/// <returns>    . </returns>
AMPAPI ampStartFlashRecording(HANDLE DeviceHandle) {
	return AMP_OK;
}

/// <summary>    ???????????? </summary>
/// <param name="DeviceHandle">     ?????</param>
/// <returns>    . </returns>
AMPAPI ampStopFlashRecording(HANDLE DeviceHandle) {
	return AMP_OK;
}