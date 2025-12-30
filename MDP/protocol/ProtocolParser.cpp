#include "pch.h"

#include "ProtocolParser.h"

namespace EEGProtocol {

    // 构造控制指令数据包
    std::vector<uint8_t> EEGProtocolParser::buildControlPacket(PacketType packetType) {
        std::vector<uint8_t> data(HEADER_LENGTH);

        /*------------------------------------------------------------------------------------------------*/
        // V2.1版本协议结构
        /*------------------------------------------------------------------------------------------------*/
        // 包头
        //data[EEGProtocol::IDX_SYNC_HEADER_H] = (SYNC_HEADER >> 8) & 0xFF;  // 高字节
        //data[EEGProtocol::IDX_SYNC_HEADER_L] = SYNC_HEADER & 0xFF;         // 低字节
        //data[EEGProtocol::IDX_VERSION] = PROTOCOL_VERSION;
        //data[EEGProtocol::IDX_PACKET_TYPE] = packetType;
        //data[EEGProtocol::IDX_SEQ_ID_L] = 0x00;  // 初步版本，不考虑具体序列号
        //data[EEGProtocol::IDX_SEQ_ID_H] = 0x00;
        //data[EEGProtocol::IDX_PAYLOAD_LEN_L] = 0x00;  // 不需要有效负载
        //data[EEGProtocol::IDX_PAYLOAD_LEN_H] = 0x00;
        

        /*------------------------------------------------------------------------------------------------*/
        // V2.5版本协议结构（暂时简单版本：2025-5-28）
        /*------------------------------------------------------------------------------------------------*/
        // 包头
        data[EEGProtocol::IDX_SYNC_HEADER_H] = (SYNC_HEADER >> 8) & 0xFF;  // 高字节
        data[EEGProtocol::IDX_SYNC_HEADER_L] = SYNC_HEADER & 0xFF;         // 低字节
        data[EEGProtocol::IDX_VERSION] = PROTOCOL_VERSION;
        data[EEGProtocol::IDX_PACKET_TYPE] = packetType;

        return data;
    }

    // 构造配置设置数据包（例如设置采样率）
    std::vector<uint8_t> EEGProtocolParser::buildConfigPacket(PacketType packetType, uint16_t value) {
        std::vector<uint8_t> data(HEADER_LENGTH + 2);

        // 包头
        data[EEGProtocol::IDX_SYNC_HEADER_H] = (SYNC_HEADER >> 8) & 0xFF;  // 高字节
        data[EEGProtocol::IDX_SYNC_HEADER_L] = SYNC_HEADER & 0xFF;         // 低字节
        data[EEGProtocol::IDX_VERSION] = PROTOCOL_VERSION;
        data[EEGProtocol::IDX_PACKET_TYPE] = packetType;
        data[EEGProtocol::IDX_SEQ_ID_L] = 0x00;  // 初步版本，不考虑具体序列号
        data[EEGProtocol::IDX_SEQ_ID_H] = 0x00;
        data[EEGProtocol::IDX_PAYLOAD_LEN_L] = 2;
        data[EEGProtocol::IDX_PAYLOAD_LEN_H] = 0x00;

        // 设置配置项 (例如设置采样率值)
        data.push_back((value >> 8) & 0xFF);  // 高字节
        data.push_back(value & 0xFF);  // 低字节
        return data;
    }

    // 构造开始/停止数据流指令包
    std::vector<uint8_t> EEGProtocolParser::buildStreamControlPacket(PacketType packetType, StreamMask streamMask) {
        std::vector<uint8_t> data(HEADER_LENGTH + 1);

        // 包头
        data[EEGProtocol::IDX_SYNC_HEADER_H] = (SYNC_HEADER >> 8) & 0xFF;  // 高字节
        data[EEGProtocol::IDX_SYNC_HEADER_L] = SYNC_HEADER & 0xFF;         // 低字节
        data[EEGProtocol::IDX_VERSION] = PROTOCOL_VERSION;
        data[EEGProtocol::IDX_PACKET_TYPE] = packetType;
        data[EEGProtocol::IDX_SEQ_ID_L] = 0x00;  // 初步版本，不考虑具体序列号
        data[EEGProtocol::IDX_SEQ_ID_H] = 0x00;
        data[EEGProtocol::IDX_PAYLOAD_LEN_L] = 1;
        data[EEGProtocol::IDX_PAYLOAD_LEN_H] = 0x00;

        // 添加流掩码
        data.push_back(static_cast<uint8_t>(streamMask));

        return data;
    }

    // 构造响应数据包（例如成功/失败）
    std::vector<uint8_t> EEGProtocolParser::buildResponsePacket(ResponseCode responseCode) {
        std::vector<uint8_t> data(HEADER_LENGTH + 1);

        // 包头
        data[EEGProtocol::IDX_SYNC_HEADER_H] = (SYNC_HEADER >> 8) & 0xFF;  // 高字节
        data[EEGProtocol::IDX_SYNC_HEADER_L] = SYNC_HEADER & 0xFF;         // 低字节
        data[EEGProtocol::IDX_VERSION] = PROTOCOL_VERSION;
        data[EEGProtocol::IDX_PACKET_TYPE] = PKT_PING;  // 对于响应，可以设置为PING包类型
        data[EEGProtocol::IDX_SEQ_ID_L] = 0x00;  // 初步版本，不考虑具体序列号
        data[EEGProtocol::IDX_SEQ_ID_H] = 0x00;
        data[EEGProtocol::IDX_PAYLOAD_LEN_L] = 1;
        data[EEGProtocol::IDX_PAYLOAD_LEN_H] = 0x00;

        // 添加响应码
        data.push_back(static_cast<uint8_t>(responseCode));

        return data;
    }

    // 解析指令响应数据包
    ResponseCode EEGProtocolParser::parseResponse(const std::vector<uint8_t>& data) {
        if (data.size() < HEADER_LENGTH) {
            throw std::invalid_argument("Invalid response packet size");
        }

        uint8_t responseCode = data[HEADER_LENGTH];

        switch (responseCode) {
        case RESP_SUCCESS:
            return RESP_SUCCESS;
        case RESP_ERROR:
            return RESP_ERROR;
        case RESP_UNSUPPORTED:
            return RESP_UNSUPPORTED;
        default:
            throw std::invalid_argument("Unknown response code");
        }
    }

    // 提取 packet type（第 4 字节）
    uint8_t EEGProtocolParser::getPacketTypeFromRaw(const unsigned char* data, size_t len) {
        if (data == nullptr || len < HEADER_LENGTH) return 0xFF;
        return data[IDX_PACKET_TYPE];
    }

    // 提取 payload length（第 7-8 字节）
    uint16_t EEGProtocolParser::getPayloadLengthFromRaw(const unsigned char* data, size_t len) {
        if (data == nullptr || len < HEADER_LENGTH) return 0;
        return (static_cast<uint16_t>(data[IDX_PAYLOAD_LEN_H]) << 8) | data[IDX_PAYLOAD_LEN_L];
    }

    // 提取序列号（第 5-6 字节）
    uint16_t EEGProtocolParser::getSequenceIDFromRaw(const unsigned char* data, size_t len) {
        if (data == nullptr || len < HEADER_LENGTH) return 0;
        return (static_cast<uint16_t>(data[IDX_SEQ_ID_H]) << 8) | data[IDX_SEQ_ID_L];
    }

    // 解析 EEG 数据包（压缩的 3 字节 * 8）
    bool EEGProtocolParser::parseEEGPacketToBuffer(
        const unsigned char* recvData, size_t dataLen, EEGDevice& device)
    {
        //float* floatBuffer = static_cast<float*>(device.Buffer);
        vector<float> floatBuffer(10,0);
        size_t payloadStart = 6;
        const auto& channels = device.channels;
        size_t enabled_channel_size = device.getEnabledChannelSize();

        size_t bufIndex = 0;

        for (int ch = 0; ch < 8; ++ch) {
            if (!channels[ch].enable) continue;

            size_t offset = payloadStart + ch * 3;
            if (offset + 2 >= dataLen) return false;

            uint32_t raw = ((uint32_t)recvData[offset] << 16) |
                ((uint32_t)recvData[offset + 1] << 8) |
                ((uint32_t)recvData[offset + 2]);

            if (raw & 0x800000) raw |= 0xFF000000;  // 符号扩展

            int32_t signedVal = static_cast<int32_t>(raw);

            float value = static_cast<float>(signedVal);
            value *= (2 * 4.5 / 16777215);
            value *= (1000 * 1000 / 24);

            floatBuffer[bufIndex++] = value;
        }
        floatBuffer[8] = 0;
        floatBuffer[9] = 0;
        std::unique_lock<std::mutex> lock(device.sdkDataBufferMtx);
        device.sdkDataBuffer.push_back(floatBuffer);
        lock.unlock();
        return true;
    }

	// 解析阻抗数据包（压缩的 3 字节 * 8）
    bool EEGProtocolParser::parseImpedancePacketToBuffer(
        const unsigned char* recvData, size_t dataLen,
        EEGDevice& device)
    {

        size_t enabled_channel_size = device.getEnabledChannelSize();


        if (dataLen < 6) return false; // 防止越界访问

        //float* floatBuffer = static_cast<float*>(device.Buffer);
        vector<float> floatBuffer(10, 0);
        const auto& channels = device.channels;

        uint16_t raw = (recvData[4] << 8) | recvData[5]; // 大端

        size_t bufIndex = 0;

        // 通道 0~7（bit15~bit8）
        for (int ch = 0; ch < 8 && ch < static_cast<int>(channels.size()); ++ch) {
            if (!channels[ch].enable) continue;
            int bitIndex = 15 - ch;
            int bitValue = (raw >> bitIndex) & 0x01;
            floatBuffer[bufIndex++] = static_cast<float>(bitValue);
        }

        // 通道 8：bit7 ~ bit4
        if (channels.size() > 8 && channels[8].enable) {
            floatBuffer[bufIndex++] = static_cast<float>((raw >> 4) & 0x0F);
        }

        // 通道 9：bit3 ~ bit0
        if (channels.size() > 9 && channels[9].enable) {
            floatBuffer[bufIndex++] = static_cast<float>(raw & 0x0F);
        }

        std::unique_lock<std::mutex> lock(device.sdkDataBufferMtx);
        device.sdkDataBuffer.push_back(floatBuffer);
        lock.unlock();
        return true;
    }


    // 获取序列号（用于构造数据包）
    uint16_t EEGProtocolParser::getSequenceID() {
        return sequenceID++;
    }

} // namespace EEGProtocol
