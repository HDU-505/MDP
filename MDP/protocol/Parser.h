#ifndef EEG_PROTOCOL_PARSER_H
#define EEG_PROTOCOL_PARSER_H
#include "pch.h"

#include "ProtocolConstants.h"
#include <vector>
#include <cstring>
#include <iostream>
#include "EEGDevice.h"


namespace EEGProtocol {

    class EEGProtocolParser {
    public:
        EEGProtocolParser() : sequenceID(0) {}

        // 构造控制指令数据包
        static std::vector<uint8_t> buildControlPacket(PacketType packetType);

        // 构造配置设置数据包（例如设置采样率）
        static std::vector<uint8_t> buildConfigPacket(PacketType packetType, uint16_t value);

        // 构造开始/停止数据流指令包
        static std::vector<uint8_t> buildStreamControlPacket(PacketType packetType, StreamMask streamMask);

        // 构造响应数据包（例如成功/失败）
        static std::vector<uint8_t> buildResponsePacket(ResponseCode responseCode);

        // 解析指令响应数据包
        static ResponseCode parseResponse(const std::vector<uint8_t>& data);
        
        // 工具方法：从原始数据中提取字段
        static uint8_t getPacketTypeFromRaw(const unsigned char* data, size_t len);
        static uint16_t getPayloadLengthFromRaw(const unsigned char* data, size_t len);
        static uint16_t getSequenceIDFromRaw(const unsigned char* data, size_t len);

        // 解析 EEG 包到 float* buffer
        static bool parseEEGPacketToBuffer(const unsigned char* recvData, size_t dataLen, EEGDevice& device);
		// 解析阻抗包到 float* buffer
		static bool parseImpedancePacketToBuffer(const unsigned char* recvData, size_t dataLen, EEGDevice& device);



        // 获取序列号（用于构造数据包）
        uint16_t getSequenceID();

    private:
        uint16_t sequenceID; // 包序列号
    };

}

#endif // EEG_PROTOCOL_PARSER_H
