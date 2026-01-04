#pragma once
#include <cstdint>

namespace protocol {

    enum ComandType : uint8_t {
        COMAND_CONTROL = 0x01,
        COMAND_CONFIG = 0x02,
        COMAND_STREAM = 0x03
    };

    enum HeaderIndex : uint8_t {
        IDX_SYNC_HEADER_H = 0,
        IDX_SYNC_HEADER_L,
        IDX_VERSION,
        IDX_PACKET_TYPE,

        IDX_HEADER_END,  // 永远放最后

        IDX_SEQ_ID_H,
        IDX_SEQ_ID_L,
        IDX_PAYLOAD_LEN_H,
        IDX_PAYLOAD_LEN_L,

    };

    // ================= 同步头定义 =================
    // 数据包同步字（2 字节），用于帧起始定位
    constexpr uint16_t SYNC_HEADER = 0xAE12;

    // 同步头高字节
    constexpr uint8_t SYNC_HEADER_H =
        static_cast<uint8_t>((SYNC_HEADER >> 8) & 0xFF);

    // 同步头低字节
    constexpr uint8_t SYNC_HEADER_L =
        static_cast<uint8_t>(SYNC_HEADER & 0xFF);

    // 同步头字节序列（用于流中查找帧头）
    constexpr uint8_t PACKET_HEADER[2] = {
        SYNC_HEADER_H,
        SYNC_HEADER_L
    };

    // 协议版本号
    constexpr uint8_t  PROTOCOL_VERSION = 0x02;

    // 固定协议头长度（不包含 payload）
    constexpr size_t HEADER_LENGTH =
        static_cast<size_t>(HeaderIndex::IDX_HEADER_END);

    // ================= EEG 数据相关定义 =================
    // EEG 通道数量
    constexpr uint8_t  EEG_CHANNEL_COUNT = 8;

    // 单个 EEG 通道数据字节数（24-bit）
    constexpr uint8_t  EEG_CHANNEL_BYTES = 3;

    // 接触质量 / 电极状态字节数
    constexpr uint8_t  EEG_CONTACT_BYTES = 1;

    // EEG 数据 payload 总长度（单位：字节）
    constexpr uint8_t  EEG_PAYLOAD_SIZE =
        EEG_CHANNEL_COUNT * EEG_CHANNEL_BYTES + EEG_CONTACT_BYTES;  // 25 bytes

    // ================= 阻抗数据相关定义 =================
    // 阻抗测量通道数量
    constexpr uint8_t IMPEDANCE_CHANNEL_COUNT = 10;

    // 单通道阻抗数据字节数
    constexpr uint8_t IMPEDANCE_CHANNEL_BYTES = 3;

    // 阻抗数据 payload 总长度
    constexpr uint8_t  IMPEDANCE_PAYLOAD_SIZE =
        IMPEDANCE_CHANNEL_COUNT * IMPEDANCE_CHANNEL_BYTES;

    // ================= 采样参数 =================
    // 默认采样率（Hz）
    constexpr uint16_t DEFAULT_SAMPLERATE = 250;

    // 默认采样周期（ms）
    constexpr uint16_t DEFAULT_INTERVAL_MS =
        1000 / DEFAULT_SAMPLERATE;

    // ================= 数据包类型定义 =================
    // PacketType：定义主机与设备之间的功能指令与数据类型
    enum PacketType : uint8_t {
        // ---- 系统控制类指令（0x00 ~ 0x0F）----
        PKT_PING = 0x00, // 心跳 / 连通性检测
        PKT_START_STREAM = 0x01, // 启动数据流
        PKT_RESET_DEVICE = 0x03, // 设备软复位
        PKT_ENTER_DFU = 0x04, // 进入固件升级模式

        // ---- 数据采集相关（0x10 ~ 0x1F）----
        PKT_EEG_DATA_PUSH = 0x10, // EEG 实时数据推送
        PKT_IMPEDANCE_DATA_PUSH = 0x11, // 阻抗数据推送
        PKT_GET_SINGLE_FRAME = 0x12, // 请求单帧数据
        PKT_STOP_STREAM = 0x13, // 停止数据流
        PKT_GET_IMPEDANCE = 0x14, // 请求阻抗测量

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

    // ================= 协议头字段索引 =================

    // ================= 模块 ID 范围 =================
    // 合法的模块编号区间（用于多模块设备扩展）
    constexpr uint8_t MODULE_ID_MIN = 0;
    constexpr uint8_t MODULE_ID_MAX = 7;

} // namespace protocol
