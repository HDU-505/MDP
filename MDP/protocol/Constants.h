#pragma once
#include <cstdint>

namespace protocol {

    // ================= 协议基础 =================

    constexpr uint8_t PACKET_HEADER[] = { 0xAE, 0x12 };
    // ==== 协议基础 ====
    constexpr uint16_t SYNC_HEADER = 0xAE12;
    constexpr uint8_t  PROTOCOL_VERSION = 0x02;
    constexpr uint8_t  HEADER_LENGTH = 4;

    constexpr uint8_t  EEG_CHANNEL_COUNT = 8;
    constexpr uint8_t  EEG_CHANNEL_BYTES = 3;
    constexpr uint8_t  EEG_CONTACT_BYTES = 1;
    constexpr uint8_t  EEG_PAYLOAD_SIZE = EEG_CHANNEL_COUNT * EEG_CHANNEL_BYTES + EEG_CONTACT_BYTES;  // 25 bytes

    constexpr uint8_t IMPEDANCE_CHANNEL_COUNT = 10;
    constexpr uint8_t IMPEDANCE_CHANNEL_BYTES = 3;
    constexpr uint8_t  IMPEDANCE_PAYLOAD_SIZE = IMPEDANCE_CHANNEL_COUNT * IMPEDANCE_CHANNEL_BYTES;

    constexpr uint16_t DEFAULT_SAMPLERATE = 250;  // Hz
    constexpr uint16_t DEFAULT_INTERVAL_MS = 1000 / DEFAULT_SAMPLERATE;

    // ==== 包类型分类 ====
    enum PacketType : uint8_t {
        // 系统控制 (0x00 ~ 0x0F)
        PKT_PING = 0x00,
        PKT_START_STREAM = 0x01,
        PKT_RESET_DEVICE = 0x03,
        PKT_ENTER_DFU = 0x04,

        // 数据采集 (0x10 ~ 0x1F)
        PKT_EEG_DATA_PUSH = 0x10,
        PKT_IMPEDANCE_DATA_PUSH = 0x11,
        PKT_GET_SINGLE_FRAME = 0x12,
        //PKT_ENABLE_REALTIME = 0x13,
        PKT_STOP_STREAM = 0x13,
        PKT_GET_IMPEDANCE = 0x14,

        // 配置管理 (0x20 ~ 0x2F)
        PKT_SET_SAMPLERATE = 0x20,
        PKT_SET_GAIN = 0x21,
        PKT_SET_FILTER = 0x22,
        PKT_GET_CONFIG = 0x23,
        PKT_GET_BATTERY_LEVEL = 0x24,

        // 升级维护 (预留 0x30 ~ 0x3F)
    };

    // ==== 流类型掩码 ====
    enum StreamMask : uint8_t {
        STREAM_EEG = 0x01,
        STREAM_IMPEDANCE = 0x02,
        STREAM_DIAGNOSTIC = 0x04
    };

    // ==== 响应码（可自定义扩展）====
    enum ResponseCode : uint8_t {
        RESP_SUCCESS = 0x00,
        RESP_ERROR = 0xFF,
        RESP_UNSUPPORTED = 0xFE
    };

    // ==== 滤波器结构配置 ====
    struct FilterConfig {
        float highpass;  // 高通
        float lowpass;   // 低通
        float notch;     // 陷波
        uint8_t type;    // 滤波类型（IIR, FIR等）
    };

    constexpr size_t COMPACT_THRESHOLD = 2048;

    // ==== 包头结构偏移量索引（如用于手动解析 buffer） ====
    enum HeaderIndex : uint8_t {
        IDX_SYNC_HEADER_L = 1,
        IDX_SYNC_HEADER_H = 0,
        IDX_VERSION = 2,
        IDX_PACKET_TYPE = 3,
        IDX_SEQ_ID_H = 4,
        IDX_SEQ_ID_L = 5,
        IDX_PAYLOAD_LEN_H = 6,
        IDX_PAYLOAD_LEN_L = 7,
    };

    // ==== 合法模块 ID 范围 ====
    constexpr uint8_t MODULE_ID_MIN = 0;
    constexpr uint8_t MODULE_ID_MAX = 7;

} // namespace protocol
