#pragma once
#include <cstdint>

namespace protocol {

    enum ComandType : uint8_t {
        COMAND_CONTROL = 0x01,
        COMAND_CONFIG = 0x02,
        COMAND_STREAM = 0x03
    };

    // ================= 新协议包结构索引定义 =================
    // 数据包格式：
    // [0-1]   包头标记 0x02 0x10/0x11
    // [2-5]   时间戳（4字节，大端）
    // [6-9]   采样序号（4字节，大端）
    // [10-33] ADC数据（8通道 × 3字节 = 24字节，大端）
    // [34-35] 包尾标记 0xAE 0x12
    enum NewHeaderIndex : uint8_t {
        IDX_HEAD_MARKER_H = 0,      // 头标记高字节 0x02
        IDX_HEAD_MARKER_L = 1,      // 头标记低字节 0x10/0x11
        IDX_TIMESTAMP_0 = 2,        // 时间戳字节0（最高位）
        IDX_TIMESTAMP_1 = 3,        // 时间戳字节1
        IDX_TIMESTAMP_2 = 4,        // 时间戳字节2
        IDX_TIMESTAMP_3 = 5,        // 时间戳字节3（最低位）
        IDX_SAMPLE_SEQ_0 = 6,       // 采样序号字节0（最高位）
        IDX_SAMPLE_SEQ_1 = 7,       // 采样序号字节1
        IDX_SAMPLE_SEQ_2 = 8,       // 采样序号字节2
        IDX_SAMPLE_SEQ_3 = 9,       // 采样序号字节3（最低位）
        IDX_ADC_DATA_START = 10,    // ADC数据起始位置
    };

    // ================= 新协议同步标记定义 =================
    // 头标记（2字节）
    constexpr uint16_t HEAD_MARKER = 0x0210;
    constexpr uint8_t HEAD_MARKER_H = 0x02;
    constexpr uint8_t HEAD_MARKER_L = 0x10;

    // 尾标记（2字节）
    constexpr uint16_t TAIL_MARKER = 0xAE12;
    constexpr uint8_t TAIL_MARKER_H = 0xAE;
    constexpr uint8_t TAIL_MARKER_L = 0x12;

    // 新协议包头字节序列
    constexpr uint8_t NEW_PACKET_HEADER[2] = {
        HEAD_MARKER_H,
        HEAD_MARKER_L
    };

    // 新协议包尾字节序列
    constexpr uint8_t NEW_PACKET_TAIL[2] = {
        TAIL_MARKER_H,
        TAIL_MARKER_L
    };

    // ================= Command Protocol Definition =================
    // Command format: AE 12 02 XX (4 bytes)
    constexpr uint8_t CMD_HEADER_0 = 0xAE;
    constexpr uint8_t CMD_HEADER_1 = 0x12;
    constexpr uint8_t CMD_HEADER_2 = 0x02;
    constexpr size_t CMD_TOTAL_LENGTH = 4;

    // ================= EEG 数据相关定义 =================
    // EEG 通道数量
    constexpr uint8_t  EEG_CHANNEL_COUNT = 8;

    // 单个 EEG 通道数据字节数（24-bit）
    constexpr uint8_t  EEG_CHANNEL_BYTES = 3;

    // 接触质量 / 电极状态字节数（新协议中可能不需要）
    constexpr uint8_t  EEG_CONTACT_BYTES = 0;

    // ================= 新协议包长度定义 =================
    // 头标记(2) + 时间戳(4) + 序号(4) + ADC数据(24) + 尾标记(2) = 36字节
    constexpr size_t NEW_PACKET_HEADER_SIZE = 10;  // 头标记(2) + 时间戳(4) + 序号(4)
    constexpr size_t NEW_PACKET_ADC_SIZE = EEG_CHANNEL_COUNT * EEG_CHANNEL_BYTES;  // 24字节
    constexpr size_t NEW_PACKET_TAIL_SIZE = 2;    // 尾标记
    constexpr size_t NEW_PACKET_TOTAL_SIZE = NEW_PACKET_HEADER_SIZE +
        NEW_PACKET_ADC_SIZE +
        NEW_PACKET_TAIL_SIZE;  // 36字节

    // EEG 数据 payload 总长度（旧协议兼容）
    constexpr uint8_t  EEG_PAYLOAD_SIZE =
        EEG_CHANNEL_COUNT * EEG_CHANNEL_BYTES + EEG_CONTACT_BYTES;

    // ================= Impedance Data Definition =================
    // Impedance output format (for ampGetImpedanceData):
    // REF(float) + GND(float) + 8 channels * 2 values (impedance + reserved)
    // Total: (2 + 8 * 2) * 4 bytes = 72 bytes
    constexpr size_t IMP_OUTPUT_SIZE = (2 + EEG_CHANNEL_COUNT * 2) * sizeof(float);

    // ================= 采样参数 =================
    // 默认采样率（Hz）
    constexpr uint16_t DEFAULT_SAMPLERATE = 250;

    // 默认采样周期（ms）
    constexpr uint16_t DEFAULT_INTERVAL_MS =
        1000 / DEFAULT_SAMPLERATE;

    // ================= ADS1299 硬件参数 =================
    // ADS1299 参考电压（mV）
    constexpr double ADS1299_VREF = 4500.0;  // 4.5V = 4500mV
    
    // ADS1299 增益（当前配置）
    constexpr double ADS1299_GAIN = 1.0;
    
    // ADS1299 24位ADC最大值
    constexpr uint32_t ADS1299_ADC_MAX = 0x7FFFFF;  // 2^23 - 1
    constexpr uint32_t ADS1299_ADC_FULL_SCALE = 0x1000000;  // 2^24
    
    // ADS1299 LSB电压值（微伏 uV）
    // LSB = (2 * Vref) / (Gain * (2^24 - 1))
    constexpr double ADS1299_LSB_UV = (2.0 * ADS1299_VREF * 1000.0) / (ADS1299_GAIN * (ADS1299_ADC_FULL_SCALE - 1));
    
    // ================= 阻抗测量参数 =================
    // 注入电流幅值（安培）
    constexpr double IMP_CURRENT_AMPS = 6.0e-9;  // 6nA
    
    // 注入信号频率（Hz）
    constexpr double IMP_SIGNAL_FREQ = 31.25;
    
    // Goertzel算法系数（针对31.25Hz @ 250Hz采样率）
    // Coeff = 2 * cos(2 * pi * f / fs) = 2 * cos(pi / 4)
    constexpr double GOERTZEL_COEFF = 1.41421356237309504880;  // 2 * cos(pi/4) = sqrt(2)
    
    // 阻抗测量窗口大小（必须是8的倍数）
    // 31.25Hz信号在250Hz采样率下每周期8个点
    constexpr int IMP_WINDOW_SIZE = 32;  // 32点（4个完整周期，更快响应 ~128ms）
    
    // 阻抗单位：欧姆（不再转换为千欧）
    // constexpr double OHM_TO_KOHM = 0.001;  // 已弃用：阻抗现在直接以欧姆为单位

    // ================= 数据包类型定义 =================
    // PacketType：定义主机与设备之间的功能指令与数据类型
    enum PacketType : uint8_t {
        // ---- 系统控制类指令（0x00 ~ 0x0F）----
        PKT_PING = 0x00, // 心跳 / 连通性检测
        PKT_START_STREAM = 0x01, // 启动数据流
        PKT_RESET_DEVICE = 0x03, // 设备软复位
        PKT_ENTER_DFU = 0x04, // 进入固件升级模式

        // ---- 数据采集相关（0x10 ~ 0x1F）----
        PKT_EEG_DATA_PUSH = 0x10, // EEG 实时数据推送（新协议格式）
        PKT_IMPEDANCE_DATA_PUSH = 0x11, // 阻抗数据推送
        PKT_STOP_STREAM = 0x12, // 停止数据流
        PKT_GET_IMPEDANCE = 0x13, // 请求阻抗测量

        // ---- 配置与查询指令（0x20 ~ 0x2F）----
        PKT_SET_SAMPLERATE = 0x20, // 设置采样率
        PKT_SET_GAIN = 0x21, // 设置放大增益
        PKT_SET_FILTER = 0x22, // 设置滤波参数
        PKT_GET_CONFIG = 0x23, // 查询当前配置
        PKT_GET_BATTERY_LEVEL = 0x24, // 查询电池电量

        // ---- 预留维护与扩展（0x30 ~ 0x3F）----
    };

    // ================= 数据流掩码定义 =================
    // 用于组合指定需要开启的数据流类型
    enum StreamMask : uint8_t {
        STREAM_EEG = 0x01, // EEG 数据流
        STREAM_IMPEDANCE = 0x02, // 阻抗数据流
        STREAM_DIAGNOSTIC = 0x04  // 诊断 / 调试数据流
    };

    // ================= 响应码定义 =================
    // 设备对指令的统一响应状态
    enum ResponseCode : uint8_t {
        RESP_SUCCESS = 0x00, // 执行成功
        RESP_ERROR = 0xFF, // 执行失败（通用错误）
        RESP_UNSUPPORTED = 0xFE  // 不支持的指令
    };

    // ================= 滤波参数结构 =================
    // 用于描述设备端数字滤波配置
    struct FilterConfig {
        float highpass;  // 高通截止频率（Hz）
        float lowpass;   // 低通截止频率（Hz）
        float notch;     // 陷波频率（Hz）
        uint8_t type;    // 滤波器类型（如 IIR / FIR）
    };

    // ================= 缓冲区管理参数 =================
    // 超过该阈值时建议执行 buffer 压缩或重整
    constexpr size_t COMPACT_THRESHOLD = 2048;

    // ================= 模块 ID 范围 =================
    // 合法的模块编号区间（用于多模块设备扩展）
    constexpr uint8_t MODULE_ID_MIN = 0;
    constexpr uint8_t MODULE_ID_MAX = 7;

} // namespace protocol