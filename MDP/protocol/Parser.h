#pragma once

#include "Constants.h"
#include <vector>
#include <cstdint>

namespace protocol {

    /**
     * @brief 协议解析器类
     *
     * 负责构建控制指令包和解析数据包
     * 新版本支持两种协议格式：
     * 1. 旧协议：用于控制指令（0xAE12开头）
     * 2. 新协议：用于EEG数据流（0x0210开头，0xAE12结尾）
     */
    class Parser {
    private:
        uint16_t sequenceID = 0; // 序列计数器

    public:
        // ================= 控制指令构建（使用旧协议格式）=================

        /**
         * @brief 构建控制指令包（旧协议格式）
         * @param packetType 包类型
         * @return 指令数据包
         */
        std::vector<uint8_t> buildControlPacket(PacketType packetType);

        /**
         * @brief 构建配置指令包
         * @param packetType 包类型
         * @param value 配置值
         * @return 指令数据包
         */
        std::vector<uint8_t> buildConfigPacket(PacketType packetType, uint16_t value);

        /**
         * @brief 构建流控制指令包
         * @param packetType 包类型
         * @param streamMask 流掩码
         * @return 指令数据包
         */
        std::vector<uint8_t> buildStreamControlPacket(PacketType packetType, StreamMask streamMask);

        /**
         * @brief 构建响应包
         * @param responseCode 响应码
         * @return 响应数据包
         */
        std::vector<uint8_t> buildResponsePacket(ResponseCode responseCode);

        /**
         * @brief 解析响应包
         * @param data 响应数据
         * @return 响应码
         */
        ResponseCode parseResponse(const std::vector<uint8_t>& data);

        // ================= 新协议EEG数据解析接口 =================

        /**
         * @brief 解析新协议EEG数据包并转换为字节流格式
         *
         * 新协议格式：
         * - 头标记：0x02 0x10 (2字节)
         * - 时间戳：16位 (2字节)
         * - 采样序号：16位 (2字节)
         * - ADC数据：24位×8通道 (24字节)
         * - 尾标记：0xAE 0x12 (2字节)
         *
         * 输出格式：
         * - 序号：8字节（uint64_t）
         * - 通道数据：每通道4字节float × 8通道
         *
         * @param data 原始数据包指针
         * @param len 数据包长度（应为32字节）
         * @param outBytes 输出字节流（序号+转换后的浮点数据）
         * @return 解析是否成功
         */
        bool parseNewEEGPacket2Byte(
            const uint8_t* data,
            size_t len,
            std::vector<uint8_t>& outBytes
        );

        /**
         * @brief 解析新协议EEG数据包并转换为浮点数组
         *
         * @param data 原始数据包指针
         * @param len 数据包长度（应为32字节）
         * @param outData 输出浮点数组（每通道一个float值）
         * @return 解析是否成功
         */
        bool parseNewEEGPacket2Float(
            const uint8_t* data,
            size_t len,
            std::vector<float>& outData
        );

        /**
         * @brief 从新协议包中提取时间戳
         * @param data 数据包指针
         * @param len 数据长度
         * @return 时间戳值（16位）
         */
        uint16_t getTimestampFromNewPacket(const uint8_t* data, size_t len);

        /**
         * @brief 从新协议包中提取采样序号
         * @param data 数据包指针
         * @param len 数据长度
         * @return 采样序号（16位）
         */
        uint16_t getSampleSeqFromNewPacket(const uint8_t* data, size_t len);

        /**
         * @brief 验证新协议包的完整性（头尾标记验证）
         * @param data 数据包指针
         * @param len 数据长度
         * @return 是否有效
         */
        bool validateNewPacket(const uint8_t* data, size_t len);

        // ================= 旧协议兼容接口（保留）=================

        /**
         * @brief 解析旧协议EEG数据包并转换为字节流格式（兼容旧版本）
         */
        bool parseEEGPacket2Byte(
            const uint8_t* data,
            size_t len,
            std::vector<uint8_t>& outBytes
        );

        /**
         * @brief 解析旧协议EEG数据包并转换为浮点数组（兼容旧版本）
         */
        bool parseEEGPacket2Float(
            const uint8_t* data,
            size_t len,
            std::vector<float>& outData
        );

        /**
         * @brief 解析EEG数据包到缓冲区
         */
        bool parseEEGPacketToBuffer(
            const unsigned char* recvData,
            size_t dataLen,
            std::vector<std::vector<float>>* buffer
        );

        /**
         * @brief 解析阻抗数据包到缓冲区
         */
        bool parseImpedancePacketToBuffer(
            const unsigned char* recvData,
            size_t dataLen,
            std::vector<std::vector<float>>* buffer
        );

        // ================= 通用工具方法 =================

        /**
         * @brief 从原始数据中获取包类型
         */
        uint8_t getPacketTypeFromRaw(const unsigned char* data, size_t len);

        /**
         * @brief 从原始数据中获取载荷长度（旧协议）
         */
        uint16_t getPayloadLengthFromRaw(const unsigned char* data, size_t len);

        /**
         * @brief 从原始数据中获取序列ID（旧协议）
         */
        uint16_t getSequenceIDFromRaw(const unsigned char* data, size_t len);

        /**
         * @brief 获取当前序列ID
         */
        uint16_t getSequenceID();
    };

} // namespace protocol