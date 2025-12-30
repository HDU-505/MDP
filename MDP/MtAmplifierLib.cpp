# include "pch.h"
# include "Amplifier_LIB.h"
# include<string>

// 应用程序版本信息
const int32_t AP_MAJOR = 1;
const int32_t AP_MINOR = 1;
const int32_t AP_BUILD = 0;
const int32_t AP_REVISION = 0;

// 库版本号
const int32_t LIB_MAJOR = 1;
const int32_t LIB_MINOR = 2025;
const int32_t LIB_BUILD = 12;
const int32_t LIB_REVISION = 29;

using namespace std;


/// <summary>    获取应用程序接口版本号 </summary>
 /// <param name="pAPIVersion">   [out] API版本
 ///          修订号 = 0
 ///          构建号 = 0
 ///          次版本号 = 次版本
 ///          主版本号 = 主版本</param>
 /// <returns>错误代码</returns>
int GetAPIVersion(t_VersionNumber* pAPIVersion) {
	pAPIVersion->Major = AP_MAJOR;
	pAPIVersion->Minor = AP_MINOR;
	pAPIVersion->Build = AP_BUILD;
	pAPIVersion->Revision = AP_REVISION;
	return AMP_OK;
}

/// <summary>    获取库版本号 </summary>
/// <param name="pLibraryVersion">   [out] 库版本
///          修订号 = 发布日期
///          构建号 = 发布月份
///          次版本号 = 发布年份
///          主版本号 = 主版本</param>
/// <returns>错误代码</returns>
int GetLibraryVersion(t_VersionNumber* pLibraryVersion) {
	pLibraryVersion->Major = LIB_MAJOR;
	pLibraryVersion->Minor = LIB_MINOR;
	pLibraryVersion->Build = LIB_BUILD;
	pLibraryVersion->Revision = LIB_REVISION;
	return AMP_OK;
}

/// <summary>    枚举可用设备 </summary>
/// <param name="HWI">              选择硬件通信接口或让库通过"ANY"自动选择
///                                 如果设为"ANY"，则通过此变量返回自动选择的接口
///                                 可能值包括"ANY", "USB", "BT"和"SIM"</param>
/// <param name="HWISize">          字符串缓冲区大小</param>
/// <param name="DeviceAddress">    预选的设备地址</param>
/// <param name="flags">            设备相关标志</param>
/// <returns>    可用设备数量</returns>
int ampEnumerateDevices(char* HWI, int32_t HWISize, const char* DeviceAddress, uint32_t flags) {
	if (!HWI || HWISize <= 0) {
		return AMP_ERR_PARAM;
	}
	std::string hwi = (HWI != nullptr) ? HWI : "";

	if (hwi == "BT") {
		// 支持
	}
	else if (hwi == "USB") {
		return AMP_ERR_VERSION;
	}
	else if (hwi == "SIM") {
		return AMP_ERR_VERSION;
	}
	else if (hwi == "ANY") {
		// 支持（BT）
	}

	// 获取设备信息



	return AMP_OK;
}

/// <summary>    获取设备地址 </summary>
/// <param name="DeviceNr">         从0开始的设备编号</param>
/// <param name="DeviceAddress">    设备地址缓冲区</param>
/// <param name="BufferSize">       缓冲区大小</param>
/// <returns>    . </returns>
AMPAPI ampGetDeviceAddress(int32_t DeviceNr, char* DeviceAddress, int32_t BufferSize);

/// <summary>    打开设备 </summary>
/// <param name="DeviceNr">         从0开始的设备编号</param>
/// <param name="DeviceHandle">     返回设备句柄</param>
/// <returns>    . </returns>
AMPAPI ampOpenDevice(int32_t DeviceNr, HANDLE* DeviceHandle);

/// <summary>    获取属性值 </summary>
/// <param name="DeviceHandle">     设备句柄</param>
/// <param name="PropertyGroup">    属性组选择</param>
/// <param name="Index">            设备属性组不需要此参数，应为零
///                                 通道属性组为从0开始的通道号
///                                 模块属性组为从0开始的模块号</param>
/// <param name="PropertyID">       属性标识符</param>
/// <param name="PropertyValue">    属性值缓冲区</param>
/// <param name="ValueByteSize">    值缓冲区大小(字节)</param>
AMPAPI ampGetProperty(HANDLE DeviceHandle, t_PropertyGroup PropertyGroup, uint32_t Index, int32_t PropertyID, void* PropertyValue, uint32_t ValueByteSize);

/// <summary>    设置属性值 </summary>
/// <param name="DeviceHandle">     设备句柄</param>
/// <param name="PropertyGroup">    属性组选择</param>
/// <param name="Index">            设备属性组不需要此参数，应为零
///                                 通道属性组为从0开始的通道号
///                                 模块属性组为从0开始的模块号</param>
/// <param name="PropertyID">       属性标识符</param>
/// <param name="PropertyValue">    属性值缓冲区</param>
/// <param name="ValueByteSize">    值缓冲区大小(字节)</param>
AMPAPI ampSetProperty(HANDLE DeviceHandle, t_PropertyGroup PropertyGroup, uint32_t Index, int32_t PropertyID, void* PropertyValue, uint32_t ValueByteSize);

/// <summary>    获取属性范围和范围类型 </summary>
/// <param name="DeviceHandle">     设备句柄</param>
/// <param name="PropertyGroup">    属性组选择</param>
/// <param name="Index">            设备属性组不需要此参数，应为零
///                                 通道属性组为从0开始的通道号
///                                 模块属性组为从0开始的模块号</param>
/// <param name="PropertyID">       属性标识符</param>
/// <param name="RangeArray">       属性范围数组缓冲区
///                                 数组元素与属性本身具有相同的数据类型，
///                                 字符串属性的RT_MINMAX有整数元素表示最小和最大字符数
///                                 离散字符串属性范围作为以零结尾的字符串返回，元素以LF字符分隔</param>
/// <param name="ArrayByteSize">    数组缓冲区大小(字节)</param>
/// <param name="RangeType">        属性范围类型</param>
AMPAPI ampGetPropertyRange(HANDLE DeviceHandle, t_PropertyGroup PropertyGroup, uint32_t Index, int32_t PropertyID, void* RangeArray, uint32_t* ArrayByteSize, t_PropertyRangeType* RangeType);

/// <summary>    启动数据采集 </summary>
/// <param name="DeviceHandle">     设备句柄</param>
/// <returns>    . </returns>
AMPAPI ampStartAcquisition(HANDLE DeviceHandle);

/// <summary>    停止数据采集 </summary>
/// <param name="DeviceHandle">     设备句柄</param>
/// <returns>    . </returns>
AMPAPI ampStopAcquisition(HANDLE DeviceHandle);

/// <summary>    关闭设备 </summary>
/// <param name="DeviceHandle">     设备句柄</param>
/// <returns>    . </returns>
AMPAPI ampCloseDevice(HANDLE DeviceHandle);

/// <summary>    设置数字端口 </summary>
/// <param name="DeviceHandle">     设备句柄</param>
/// <param name="PortNumber">       端口号</param>
/// <param name="value">            值</param>
/// <returns>    . </returns>
AMPAPI ampSetDigitalPort(HANDLE DeviceHandle, int32_t PortNumber, uint32_t value);

/// <summary>    从设备读取采集的数据
///              缓冲区中的通道顺序为
///              S1_SAMPLECOUNTER, S1_CH1 .. S1_CHn,
///              S2_SAMPLECOUNTER, S2_CH1 .. S2_CHn,
///              ...
///              Sn_SAMPLECOUNTER, Sn_CH1 .. Sn_CHn
///              样本大小取决于启用的通道数和通道数据类型
///              样本计数器是64位无符号整数
/// </summary>
/// <param name="DeviceHandle">         设备句柄</param>
/// <param name="Buffer">               接收缓冲区</param>
/// <param name="BufferSize">           接收缓冲区大小(字节)</param>
/// <param name="RequestedSamples">     请求的样本数(尚未支持)</param>
/// <returns>写入接收缓冲区的字节数</returns>
AMPAPI ampGetData(HANDLE DeviceHandle, void* Buffer, int32_t BufferSize, int32_t RequestedSamples);

/// <summary>    获取选定通道的阻抗数据
///              缓冲区中的通道顺序为
///              M0 GND阻抗, M0 REF阻抗, ... Mn GND, Mn REF, CH1+, CH1-, CH2+, CH2-, .. CHn+, CHn-
///              M0 - Mn是所有MPROP_B32_ImpedanceMeasurement属性设置的模块的地和参考阻抗
///              所有值类型为float，单位为[Ω]
///              双极电极的CH-值才有效，如果阻抗值不可用则为-1
/// <param name="DeviceHandle">     设备句柄</param>
/// <param name="Buffer">           接收缓冲区</param>
/// <param name="BufferSize">       接收缓冲区大小(字节)</param>
/// <returns>写入接收缓冲区的字节数</returns>
AMPAPI ampGetImpedanceData(HANDLE DeviceHandle, void* Buffer, int32_t BufferSize);

/// <summary>    开始记录到内部内存 </summary>
/// <param name="DeviceHandle">     设备句柄</param>
/// <returns>    . </returns>
AMPAPI ampStartFlashRecording(HANDLE DeviceHandle);

/// <summary>    停止记录到内部内存 </summary>
/// <param name="DeviceHandle">     设备句柄</param>
/// <returns>    . </returns>
AMPAPI ampStopFlashRecording(HANDLE DeviceHandle);