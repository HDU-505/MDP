/*------------------------------------------------------------------------------------------------*/
//  项目:    Amplifier Windows Library
/*------------------------------------------------------------------------------------------------*/
//  用途:    Brain Products 放大器硬件访问库
//  作者:    Norbert Hauser
//  日期:    2016-03-17
//
//  变更历史
//  17.03.2016  版本 1.xx    - 初始实现
//  05.12.2016  版本 2.xx    - 增加闪存记录功能和属性
//  22.03.2017  版本 3.0     - 新增模块属性 MPROP_B32_ImpedanceMeasurement
//                            - 根据模块是否支持GND/REF测量，ampGetImpedanceData将提供这些额外值
//  29.03.2017  版本 3.0     - 新增模块属性 MPROP_I32_UseableChannels
//  27.06.2017  版本 3.1     - 新增设备属性 DPROP_I32_RecordingState
//                            - 新增触发器输出模式相关模块属性：
//                              MPROP_I32_TriggerOutMode, MPROP_I32_TriggerSyncPin, MPROP_I32_TriggerSyncPeriod 和 MPROP_I32_TriggerSyncWidth
//                            - 通过通道属性 CPROP_I32_LedColor 和模块属性 MPROP_I32_LedColorREF、MPROP_I32_LedColorGND 手动控制活动电极LED
//                              设备属性 DPROP_I32_LedControl 用于开启/关闭手动LED控制
//                            - 通过模块属性 MPROP_I32_UserButtonState 和 MPROP_I32_UserButtonLed 获取用户按钮状态和控制
//                            - 新增设备属性 DPROP_I32_ActiveShieldGain
//                            - 现在可以通过通道属性 CPROP_UI32_OutputValue 设置数字通道(CT_TRG或CT_DIG)的输出值
//                              ampSetDigitalPort函数已过时并将被移除
//                              ampGetPropertyRange 可能返回可用位的掩码(RT_BITMASK)
//                            - 新增闪存格式化设备属性
//  24.11.2017  版本 3.1     - 新增设备错误 DEVICE_ERR_SYNC
//  26.06.2018  版本 3.2     - 新增通道类型 CT_TIM 用于时间戳
//                            - 新增触发器输出模式 TM_MIROR_SYNC
//  11.09.2018  版本 3.2     - 新增设备属性 DPROP_B32_FastDataAccess 用于actiCHamp快速数据访问模式
//  17.12.2020  版本 3.2     - 新增设备属性 DPROP_B32_ContinuousImpedance 用于CGX放大器
//  09.02.2021  版本 3.2     - 新增设备属性 DPROP_I32_SignalStrength (RSSI值，单位dBm)
/*------------------------------------------------------------------------------------------------*/

#pragma once

#ifdef AMPLIFIER_EXPORTS
#define AMPLIFIER_API __declspec(dllexport)
//#define AMPLIFIER_API 
#else
#define AMPLIFIER_API __declspec(dllimport)
#endif

#ifndef WINAPI
#include <windows.h>
#endif

#include <stdint.h>

/*------------------------------------------------------------------------------------------------*/
// 宏定义
/*------------------------------------------------------------------------------------------------*/

#define API_MAJOR               (3)         // 应用程序接口主版本号
#define API_MINOR               (2)         // 应用程序接口次版本号

/*------------------------------------------------------------------------------------------------*/
// 错误代码
/*------------------------------------------------------------------------------------------------*/

#define AMP_OK                  (0)         // 无错误
#define AMP_ERR_FAIL            (-1)        // 未指定的内部错误
#define AMP_ERR_PARAM           (-2)        // 函数参数或属性大小超出范围
#define AMP_ERR_VERSION         (-3)        // 版本不支持
#define AMP_ERR_MEMORY          (-4)        // 内存不足
#define AMP_ERR_BUSY            (-5)        // 请求的设备/文件正忙
#define AMP_ERR_NODEVICE        (-6)        // 设备不可用
#define AMP_ERR_NOSUPPORT       (-7)        // 放大器不支持此功能或属性
#define AMP_ERR_EXCEPTION       (-8)        // 抛出异常
#define AMP_ERR_FWVERSION       (-9)        // 放大器固件版本不支持
#define AMP_ERR_TIMEOUT         (-10)       // 发生超时
#define AMP_ERR_BUFFERSIZE      (-11)       // 传输缓冲区太小，有更多数据可用
#define AMP_ERR_CHANNEL_BUFFER_MISMATCH      (-12)       // 缓冲大小和可用通道大小不一致
#define AMP_ERR_UNSUPPORTED     (-13)       // 不支持

#define IF_ERR_FAIL             (-100-1)    // 接口连接错误
#define IF_ERR_BT_SERVICE       (-100-2)    // 接口无线电关闭
#define IF_ERR_MEMORY           (-100-3)    // 接口内存不足
#define IF_ERR_NODEVICE         (-100-4)    // 接口未找到
#define IF_ERR_CONNECT          (-100-5)    // 接口无法连接到放大器
#define IF_ERR_DISCONNECTED     (-100-6)    // 接口放大器断开连接
#define IF_ERR_TIMEOUT          (-100-7)    // 接口命令超时
#define IF_ERR_ALREADYOPEN      (-100-8)    // 接口放大器已打开
#define IF_ERR_PARAMETER        (-100-9)    // 接口无效命令参数
#define IF_ERR_ATCOMMAND        (-100-10)   // 接口AT命令失败

#define DEVICE_ERR_BASE         (-200)      // 设备错误基础值
#define DEVICE_ERR_FAIL         (-200-1)    // 未指定的内部错误
#define DEVICE_ERR_PARAM        (-200-2)    // 函数参数超出范围
#define DEVICE_ERR_VERSION      (-200-3)    // 此固件版本不支持该功能
#define DEVICE_ERR_MEMORY       (-200-4)    // 内存不足
#define DEVICE_ERR_BUSY         (-200-5)    // 设备忙，无法执行功能
#define DEVICE_ERR_SDWRITE      (-200-6)    // SD卡写入错误
#define DEVICE_ERR_SDREAD       (-200-7)    // SD卡读取错误
#define DEVICE_ERR_NOSD         (-200-8)    // SD卡未插入
#define DEVICE_ERR_SDFS         (-200-9)    // SD卡文件系统损坏
#define DEVICE_ERR_SDACQ        (-200-10)   // 在记录到SD卡前未启动采集
#define DEVICE_ERR_AUXBOX       (-200-11)   // AuxBox不可用
#define DEVICE_ERR_SDFULL       (-200-12)   // SD卡已满
#define DEVICE_ERR_WSPACE       (-200-13)   // 工作区设置不可用
#define DEVICE_ERR_CLOCKED      (-200-14)   // 此放大器型号不支持请求的EEG通道
#define DEVICE_ERR_SUPPORT      (-200-15)   // 当前操作模式不支持该功能
#define DEVICE_ERR_FILEINUSE    (-200-16)   // 请求的文件名已被使用
#define DEVICE_ERR_SYNC         (-200-17)   // 设备未同步，无法执行功能

/*------------------------------------------------------------------------------------------------*/
// 标志位
/*------------------------------------------------------------------------------------------------*/

// 属性 DPROP_UI32_FlashRecordingState 的状态位定义
#define FLAG_REC_AVAILABLE      0x0001      // 内部闪存记录可用
#define FLAG_REC_ACTIVE         0x0002      // 内部闪存记录激活中
#define FLAG_REC_PREPARE        0x0004      // 准备内部闪存记录进行中
// 属性 DPROP_UI32_FlashRecordingState 的错误标志
#define FLAG_REC_FILESYSTEM     0x00010000  // 文件系统损坏
#define FLAG_REC_WRITE          0x00020000  // 写入错误
#define FLAG_REC_FULL           0x00040000  // 内存已满
#define FLAG_REC_FILE_FULL      0x00080000  // 达到4GB文件大小限制
#define FLAG_REC_FRAGMENTED     0x00100000  // 簇映射失败，闪存碎片过多
#define FLAG_REC_ALLOCATION     0x00200000  // 簇预分配失败
#define FLAG_REC_BUFFEROVF      0x00400000  // 扇区缓冲区溢出，可能内存碎片过多
#define FLAG_REC_FORMAT         0x00800000  // 内存未正确格式化

/*------------------------------------------------------------------------------------------------*/
// 枚举类型
/*------------------------------------------------------------------------------------------------*/

// 属性所属的组
typedef enum PropertyGroup
{
    PG_DEVICE = 0,              // 设备属性
    PG_MODULE = 1,              // 放大器模块属性
    PG_CHANNEL = 2              // 放大器通道属性
} t_PropertyGroup;

// 记录模式
typedef enum RecordingMode
{
    RM_STOPPED = 0,             // 未激活
    RM_NORMAL = 1,              // 正常记录
    RM_IMPEDANCE = 2,           // 阻抗测量
    RM_TEST = 3                 // 放大器测试信号
} t_RecordingMode;

// 通道类型
typedef enum ChannelType
{
    CT_EEG = 0,                 // EEG通道
    CT_BIP = 1,                 // 双极通道
    CT_AUX = 2,                 // 辅助通道
    CT_TRG = 3,                 // 触发器通道
    CT_DIG = 4,                 // 数字通道
    CT_TIM = 5                  // 时间戳通道
} t_ChannelType;

// 通道数据类型
typedef enum ChannelDataType
{
    DT_INT16 = 0,
    DT_UINT16 = 1,
    DT_INT32 = 2,
    DT_UINT32 = 3,
    DT_INT64 = 4,
    DT_UINT64 = 5,
    DT_FLOAT32 = 6,
    DT_FLOAT64 = 7
} t_ChannelDataType;

// 电极类型
typedef enum ElectrodeType
{
    EL_NONE = 0,                // 无电极连接(如AUX通道)
    EL_PASSIVE = 1,             // 被动电极
    EL_ACTIVE = 2               // 主动电极
} t_ElectrodeType;

// 电池电量状态
typedef enum BatteryState
{
    BS_UNKNOWN = 0,
    BS_EMPTY = 1,
    BS_MEDIUM = 2,
    BS_FULL = 3,
    BS_CHARGING = 4
} t_BatteryState;

// 连接状态
typedef enum ConnectionState
{
    CS_DISCONNECTED = 0,        // 设备已断开
    CS_RECONNECT = 1,           // 设备当前断开，库正在尝试重新连接
    CS_CONNECTED = 2            // 设备已连接
} t_ConnectionState;

// 信号质量
typedef enum SignalQuality
{
    SQ_NOINFO = 0,
    SQ_GOOD = 1,
    SQ_MEDIUM = 2,
    SQ_BAD = 3
} t_SignalQuality;

// 属性范围类型
typedef enum PropertyRangeType
{
    RT_READONLY = 0,            // 该属性只读
    RT_MINMAX = 1,              // 返回的属性范围数组包含最小值和最大值
    RT_DISCRETE = 2,            // 返回的属性范围数组包含离散值
    RT_BITMASK = 3              // 返回的属性范围数组包含读写位掩码
}t_PropertyRangeType;

// 触发器输出模式
typedef enum TriggerOutputMode
{
    TM_DEFAULT = 0,             // 触发器输出端口独立于输入
    TM_MIRROR = 1,              // 将触发器输入复制到输出端口
    TM_SYNC = 2,                // 生成可配置周期和脉冲宽度的同步输出信号
    TM_MIRROR_SYNC = 3          // 同时支持镜像和同步模式
}t_TriggerOutputMode;

// 电极LED颜色
typedef enum ElectrodeLedColor
{
    LED_OFF = 0,                // 关闭电极LED
    LED_GREEN = 1,              // 绿色
    LED_RED = 2,                // 红色
    LED_YELLOW = 3              // 黄色
}t_ElectrodeLedColor;

// 用户按钮状态
typedef enum UserButtonState
{
    BTN_NONE = 0,               // 按钮未按下(默认)
    BTN_HOLD = 1,               // 按钮按下并保持
    BTN_PUSH = 2,               // 自上次读取以来按钮从无到有按下
    BTN_RELEASE = 3             // 自上次读取以来按钮从保持到释放
}t_UserButtonState;

/*------------------------------------------------------------------------------------------------*/
// 属性定义
// 
// 每个属性标签包含属性数据类型：
//   I32 = int32_t
//   F32 = float 32位
//   B32 = BOOL 32位
//   CHR = 以零结尾的ASCII字符串
//   TVN = t_VersionNumber
/*------------------------------------------------------------------------------------------------*/

// 覆盖结构体的默认对齐方式
#pragma pack(push, 1)

// 版本号结构
typedef struct VersionNumber
{
    int32_t Major;
    int32_t Minor;
    int32_t Build;
    int32_t Revision;
} t_VersionNumber;

// 通道属性ID
typedef enum ChannelPropertyID
{
    CPROP_I32_Type = 0,                             // 通道类型(t_ChannelType)
    CPROP_I32_ChannelNumber = 1,                    // 物理通道号(从0开始)
    CPROP_I32_ModuleNumber = 2,                     // 通道所属的放大器模块号(从0开始)
    CPROP_CHR_Function = 3,                         // 专用AUX通道的功能名称(如加速度计通道的ACC X,Y,Z)
    CPROP_I32_Electrode = 4,                        // 电极类型(t_ElectrodeType)
    CPROP_I32_DataType = 5,                         // 接收数据流中的通道数据类型(t_ChannelDataType)
    CPROP_F32_Resolution = 6,                       // 信号分辨率，单位/位
    CPROP_CHR_Unit = 7,                             // SI单位的ASCII字符串，可选前缀表示倍数或分数
    CPROP_F32_Gain = 8,                             // 放大器增益(0=设置不可用)
    CPROP_F32_HighPass = 9,                         // 应用的高通滤波器频率(Hz)(0=直流)
    CPROP_F32_LowPass = 10,                         // 应用的低通滤波器频率(Hz)(0=关闭)
    CPROP_F32_NotchFilter = 11,                     // 陷波滤波器频率(Hz)(0=关闭)
    CPROP_B32_ReferenceChannel = 12,                // 1=可用作参考通道或正用作参考通道
    CPROP_B32_ImpedanceMeasurement = 13,            // 1=此通道支持阻抗测量
    CPROP_B32_RecordingEnabled = 14,                // 1=通道已启用记录
    CPROP_I32_LedColor = 15,                        // 活动电极LED的手动颜色选择(t_ElectrodeLedColor)
    CPROP_UI32_OutputValue = 16,                    // 数字通道(CT_TRG或CT_DIG)的输出值
    CPROP_I32_ChannelName = 17,                     // 物理通道名
} t_ChannelPropertyID;

// 放大器模块属性ID
typedef enum ModulePropertyID
{
    MPROP_CHR_Type = 1,                             // 模块类型名称(以零结尾的ASCII字符串)
    MPROP_CHR_SerialNumber = 2,                     // 可读格式的序列号
    MPROP_TVN_HardwareRevision = 3,                 // 硬件版本
    MPROP_TVN_FirmwareVersion = 4,                  // 固件版本
    MPROP_B32_ImpedanceMeasurement = 20,            // 1=此模块支持REF和GND阻抗测量
    MPROP_I32_UseableChannels = 21,                 // 此模块中可用(可选)通道数
    MPROP_I32_TriggerOutMode = 30,                  // 触发器输出模式(t_TriggerOutputMode)
    MPROP_I32_TriggerSyncPin = 31,                  // TM_SYNC模式的触发器输出同步引脚号(从0开始)
    MPROP_I32_TriggerSyncPeriod = 32,               // 同步输出信号周期(样本数)
    MPROP_I32_TriggerSyncWidth = 33,                // 同步输出信号脉冲宽度(样本数)
    MPROP_I32_LedColorREF = 40,                     // 活动REF电极LED的手动颜色选择(t_ElectrodeLedColor)
    MPROP_I32_LedColorGND = 41,                     // 活动GND电极LED的手动颜色选择(t_ElectrodeLedColor)
    MPROP_I32_UserButtonState = 42,                 // 用户按钮状态(t_UserButtonState)
    MPROP_I32_UserButtonLed = 43                    // 用户按钮LED控制，闪烁间隔(ms)(最小值=关闭，最大值=开启)
} t_ModulePropertyID;

// 放大器设备属性ID
typedef enum DevicePropertyID
{
    DPROP_CHR_Family = 0,                       // 放大器系列名称(以零结尾的ASCII字符串)
    DPROP_CHR_Type = 1,                         // 放大器类型名称(以零结尾的ASCII字符串)
    DPROP_CHR_Interface = 2,                    // 放大器使用的硬件接口("ANY", "USB", "BT", "SIMULATIOM", ...)
    DPROP_CHR_Address = 3,                      // 硬件接口的地址标识符(如MAC地址、USB或COM端口)
    DPROP_CHR_SerialNumber = 4,                 // 可读格式的序列号
    DPROP_TVN_HardwareRevision = 5,             // 硬件版本
    DPROP_TVN_FirmwareVersion = 6,              // 固件版本
    DPROP_TVN_DriverVersion = 7,                // 设备驱动版本
    DPROP_I32_AvailableModules = 8,             // 可用模块数
    DPROP_I32_AvailableChannels = 9,            // 总可用通道数(所有模块)
    // 设备状态
    DPROP_F32_BatteryVoltage = 100,             // 当前电池电压[V]
    DPROP_I32_BatteryLevel = 101,               // 电池电量状态(t_BatteryState)
    DPROP_I32_ConnectionState = 102,            // 连接信息(t_ConnectionState)
    DPROP_I32_SignalQuality = 103,              // 信号质量(如可用，t_SignalQuality)
    DPROP_UI32_ErrorFlags = 104,                // 设备特定错误标志
    DPROP_I32_RecordingState = 105,             // 设备当前记录模式(t_RecordingMode)
    DPROP_I32_SignalStrength = 106,             // 无线信号强度(如可用，RSSI)
    // 记录参数
    DPROP_I32_RecordingMode = 200,              // 下次ampStartAcquisition请求的记录模式(t_RecordingMode)
    DPROP_F32_BaseSampleRate = 201,             // 放大器基础采样频率
    DPROP_F32_SubSampleDivisor = 202,           // 子采样除数
    DPROP_I32_GoodImpedanceLevel = 203,         // 良好阻抗水平(Ω)，低于此值显示绿色，否则黄色
    DPROP_I32_BadImpedanceLevel = 204,          // 不良阻抗水平(Ω)，高于此值显示红色
    DPROP_I32_LedControl = 205,                 // 活动电极LED的手动控制(0=关闭，1=更新所有电极到选定值)
    DPROP_I32_ActiveShieldGain = 206,           // 主动屏蔽增益(0=无主动屏蔽)
    DPROP_B32_FastDataAccess = 207,             // 快速数据访问模式
    DPROP_B32_ContinuousImpedance = 220,        // 连续阻抗测量(FALSE=关闭，TRUE=开启)

    // 闪存记录状态和参数
    DPROP_UI32_FlashRecordingState = 300,       // 记录到内部内存时的设备特定标志(如可用)
    DPROP_UI32_FlashSegmentSize = 301,          // 预分配段大小(MB)，有效范围1-4095MB，超出将被裁剪
    DPROP_CHR_FlashWorkspaceDescription = 302,  // 工作区设置的XML描述(通道标签、触发器选择等)
    DPROP_UI32_FlashFreeSpace = 303,            // 空闲磁盘空间(MB)
    DPROP_UI32_FlashFileSize = 304,             // 当前记录文件大小(MB)
    DPROP_CHR_FlashFileName = 305,              // 当前记录文件名
    DPROP_B32_FlashFormatting = 306             // 闪存格式化
} t_DevicePropertyID;

// 恢复结构体的默认对齐方式
#pragma pack(pop)

/*------------------------------------------------------------------------------------------------*/
// 接口函数
// 所有接口函数将返回>=0的值作为有效响应，或返回负数表示错误
/*------------------------------------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {  // 仅在C++源代码中使用时需要导出C接口
#endif

#define AMPAPI AMPLIFIER_API int WINAPI

    /// <summary>    获取应用程序接口版本号 </summary>
    /// <param name="pAPIVersion">   [out] API版本
    ///          修订号 = 0
    ///          构建号 = 0
    ///          次版本号 = 次版本
    ///          主版本号 = 主版本</param>
    /// <returns>错误代码</returns>
    AMPAPI GetAPIVersion(t_VersionNumber* pAPIVersion);

    /// <summary>    获取库版本号 </summary>
    /// <param name="pLibraryVersion">   [out] 库版本
    ///          修订号 = 发布日期
    ///          构建号 = 发布月份
    ///          次版本号 = 发布年份
    ///          主版本号 = 主版本</param>
    /// <returns>错误代码</returns>
    AMPAPI GetLibraryVersion(t_VersionNumber* pLibraryVersion);

    /// <summary>    枚举可用设备 </summary>
    /// <param name="HWI">              选择硬件通信接口或让库通过"ANY"自动选择
    ///                                 如果设为"ANY"，则通过此变量返回自动选择的接口
    ///                                 可能值包括"ANY", "USB", "BT"和"SIM"</param>
    /// <param name="HWISize">          字符串缓冲区大小</param>
    /// <param name="DeviceAddress">    预选的设备地址</param>
    /// <param name="flags">            设备相关标志</param>
    /// <returns>    可用设备数量</returns>
    AMPAPI ampEnumerateDevices(char* HWI, int32_t HWISize, const char* DeviceAddress, uint32_t flags);

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

#ifdef __cplusplus
}
#endif
