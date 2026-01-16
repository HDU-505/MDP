#include "Parser.h"
#include <mutex>
#include <stdexcept>
#include <cstring>

using namespace std;

namespace protocol {

    // ================= 控制指令构建（旧协议格式保持不变）=================

    std::vector<uint8_t> Parser::buildControlPacket(PacketType packetType) {
        std::vector<uint8_t> data(HEADER_LENGTH);

        // V2.5版本协议结构
        data[IDX_SYNC_HEADER_H] = (SYNC_HEADER >> 8) & 0xFF;
        data[IDX_SYNC_HEADER_L] = SYNC_HEADER & 0xFF;
        data[IDX_VERSION] = PROTOCOL_VERSION;
        data[IDX_PACKET_TYPE] = packetType;

        return data;
    }

    std::vector<uint8_t> Parser::buildConfigPacket(PacketType packetType, uint16_t value) {
        std::vector<uint8_t> data(HEADER_LENGTH + 4 + 2);

        data[IDX_SYNC_HEADER_H] = (SYNC_HEADER >> 8) & 0xFF;
        data[IDX_SYNC_HEADER_L] = SYNC_HEADER & 0xFF;
        data[IDX_VERSION] = PROTOCOL_VERSION;
        data[IDX_PACKET_TYPE] = packetType;

        data[IDX_SEQ_ID_H] = 0x00;
        data[IDX_SEQ_ID_L] = 0x00;

        data[IDX_PAYLOAD_LEN_H] = 0x00;
        data[IDX_PAYLOAD_LEN_L] = 0x02;

        data[8] = (value >> 8) & 0xFF;
        data[9] = value & 0xFF;

        return data;
    }

    std::vector<uint8_t> Parser::buildStreamControlPacket(PacketType packetType, StreamMask streamMask) {
        std::vector<uint8_t> data(HEADER_LENGTH + 4 + 1);

        data[IDX_SYNC_HEADER_H] = (SYNC_HEADER >> 8) & 0xFF;
        data[IDX_SYNC_HEADER_L] = SYNC_HEADER & 0xFF;
        data[IDX_VERSION] = PROTOCOL_VERSION;
        data[IDX_PACKET_TYPE] = packetType;

        data[IDX_SEQ_ID_H] = 0x00;
        data[IDX_SEQ_ID_L] = 0x00;

        data[IDX_PAYLOAD_LEN_H] = 0x00;
        data[IDX_PAYLOAD_LEN_L] = 0x01;

        data[8] = static_cast<uint8_t>(streamMask);

        return data;
    }

    std::vector<uint8_t> Parser::buildResponsePacket(ResponseCode responseCode) {
        std::vector<uint8_t> data(HEADER_LENGTH + 4 + 1);

        data[IDX_SYNC_HEADER_H] = (SYNC_HEADER >> 8) & 0xFF;
        data[IDX_SYNC_HEADER_L] = SYNC_HEADER & 0xFF;
        data[IDX_VERSION] = PROTOCOL_VERSION;
        data[IDX_PACKET_TYPE] = PKT_PING;

        data[IDX_SEQ_ID_H] = 0x00;
        data[IDX_SEQ_ID_L] = 0x00;

        data[IDX_PAYLOAD_LEN_H] = 0x00;
        data[IDX_PAYLOAD_LEN_L] = 0x01;

        data[8] = static_cast<uint8_t>(responseCode);

        return data;
    }

    ResponseCode Parser::parseResponse(const std::vector<uint8_t>& data) {
        if (data.size() < 9) {
            return RESP_ERROR;
        }

        uint8_t responseCode = data[8];

        switch (responseCode) {
        case RESP_SUCCESS:
            return RESP_SUCCESS;
        case RESP_ERROR:
            return RESP_ERROR;
        case RESP_UNSUPPORTED:
            return RESP_UNSUPPORTED;
        default:
            return RESP_ERROR;
        }
    }

    // ================= 新协议EEG数据解析实现 =================

    bool Parser::validateNewPacket(const uint8_t* data, size_t len) {
        if (!data || len < NEW_PACKET_TOTAL_SIZE) {
            return false;
        }

        // 验证头标记：0x02 0x10
        if (data[IDX_HEAD_MARKER_H] != HEAD_MARKER_H ||
            data[IDX_HEAD_MARKER_L] != HEAD_MARKER_L) {
            return false;
        }

        // 验证尾标记：0xAE 0x12
        size_t tailPos = NEW_PACKET_HEADER_SIZE + NEW_PACKET_ADC_SIZE;
        if (data[tailPos] != TAIL_MARKER_H ||
            data[tailPos + 1] != TAIL_MARKER_L) {
            return false;
        }

        return true;
    }

    uint16_t Parser::getTimestampFromNewPacket(const uint8_t* data, size_t len) {
        if (!data || len < NEW_PACKET_HEADER_SIZE) return 0;
        return (static_cast<uint16_t>(data[IDX_TIMESTAMP_H]) << 8) |
            data[IDX_TIMESTAMP_L];
    }

    uint16_t Parser::getSampleSeqFromNewPacket(const uint8_t* data, size_t len) {
        if (!data || len < NEW_PACKET_HEADER_SIZE) return 0;
        return (static_cast<uint16_t>(data[IDX_SAMPLE_SEQ_H]) << 8) |
            data[IDX_SAMPLE_SEQ_L];
    }

    bool Parser::parseNewEEGPacket2Byte(
        const uint8_t* data,
        size_t len,
        std::vector<uint8_t>& outBytes
    )
    {
        // 验证数据包有效性
        if (!validateNewPacket(data, len)) {
            return false;
        }

        // 提取采样序号并递增内部计数器
        uint16_t sampleSeq = getSampleSeqFromNewPacket(data, len);
        sequenceID = sampleSeq;

        // 写入8字节序号（小端格式）
        for (int i = 0; i < 8; i++) {
            outBytes.push_back(
                static_cast<uint8_t>((sequenceID >> (i * 8)) & 0xFF)
            );
        }

        // 解析ADC数据起始位置
        size_t adcStart = IDX_ADC_DATA_START;

        // 解析8个通道的24位ADC数据
        for (int ch = 0; ch < EEG_CHANNEL_COUNT; ++ch) {
            size_t offset = adcStart + ch * EEG_CHANNEL_BYTES;

            // 24-bit 大端格式读取
            uint32_t raw =
                (static_cast<uint32_t>(data[offset]) << 16) |
                (static_cast<uint32_t>(data[offset + 1]) << 8) |
                static_cast<uint32_t>(data[offset + 2]);

            // 24-bit 符号扩展为32-bit
            if (raw & 0x800000) {
                raw |= 0xFF000000;
            }
            int32_t signedVal = static_cast<int32_t>(raw);

            // 转换为电压值（微伏）
            float value = static_cast<float>(signedVal);
            value *= (2.0f * 4.5f / 16777215.0f);  // ADC转电压
            value *= (1000000.0f / 24.0f);         // 转换为微伏并除以增益

            // 将float转换为字节流
            const uint8_t* p = reinterpret_cast<const uint8_t*>(&value);
            outBytes.insert(outBytes.end(), p, p + sizeof(float));
        }

        return true;
    }

    bool Parser::parseNewEEGPacket2Float(
        const uint8_t* data,
        size_t len,
        std::vector<float>& outData
    )
    {
        // 验证数据包有效性
        if (!validateNewPacket(data, len)) {
            return false;
        }

        // 解析ADC数据起始位置
        size_t adcStart = IDX_ADC_DATA_START;

        // 解析8个通道的24位ADC数据
        for (int ch = 0; ch < EEG_CHANNEL_COUNT; ++ch) {
            size_t offset = adcStart + ch * EEG_CHANNEL_BYTES;

            // 24-bit 大端格式读取
            uint32_t raw =
                (static_cast<uint32_t>(data[offset]) << 16) |
                (static_cast<uint32_t>(data[offset + 1]) << 8) |
                static_cast<uint32_t>(data[offset + 2]);

            // 24-bit 符号扩展为32-bit
            if (raw & 0x800000) {
                raw |= 0xFF000000;
            }
            int32_t signedVal = static_cast<int32_t>(raw);

            // 转换为电压值（微伏）
            float value = static_cast<float>(signedVal);
            value *= (2.0f * 4.5f / 16777215.0f);  // ADC转电压
            value *= (1000000.0f / 24.0f);         // 转换为微伏并除以增益

            outData.push_back(value);
        }

        return true;
    }

    // ================= 旧协议兼容实现（保持不变）=================

    uint8_t Parser::getPacketTypeFromRaw(const unsigned char* data, size_t len) {
        if (data == nullptr || len < HEADER_LENGTH) return 0xFF;
        return data[IDX_PACKET_TYPE];
    }

    uint16_t Parser::getPayloadLengthFromRaw(const unsigned char* data, size_t len) {
        if (data == nullptr || len < 8) return 0;
        return (static_cast<uint16_t>(data[IDX_PAYLOAD_LEN_H]) << 8) | data[IDX_PAYLOAD_LEN_L];
    }

    uint16_t Parser::getSequenceIDFromRaw(const unsigned char* data, size_t len) {
        if (data == nullptr || len < 6) return 0;
        return (static_cast<uint16_t>(data[IDX_SEQ_ID_H]) << 8) | data[IDX_SEQ_ID_L];
    }

    uint16_t Parser::getSequenceID() {
        return sequenceID;
    }

    bool Parser::parseEEGPacket2Byte(
        const uint8_t* data,
        size_t len,
        std::vector<uint8_t>& outBytes
    )
    {
        if (!data || len < HEADER_LENGTH + EEG_PAYLOAD_SIZE) {
            return false;
        }

        size_t payloadStart = HEADER_LENGTH;

        sequenceID++;

        // 写入counter（大端）
        for (int i = 0; i < 8; i++) {
            outBytes.push_back(
                static_cast<uint8_t>((sequenceID >> (i * 8)) & 0xFF)
            );
        }

        size_t eegStart = payloadStart + 8;

        for (int ch = 0; ch < EEG_CHANNEL_COUNT; ++ch) {
            size_t offset = eegStart + ch * EEG_CHANNEL_BYTES;

            uint32_t raw =
                (static_cast<uint32_t>(data[offset]) << 16) |
                (static_cast<uint32_t>(data[offset + 1]) << 8) |
                static_cast<uint32_t>(data[offset + 2]);

            if (raw & 0x800000) raw |= 0xFF000000;
            int32_t signedVal = static_cast<int32_t>(raw);

            float value = static_cast<float>(signedVal);
            value *= (2.0f * 4.5f / 16777215.0f);
            value *= (1000000.0f / 24.0f);

            const uint8_t* p = reinterpret_cast<const uint8_t*>(&value);
            outBytes.insert(outBytes.end(), p, p + sizeof(float));
        }

        return true;
    }

    bool Parser::parseEEGPacket2Float(
        const uint8_t* data,
        size_t len,
        std::vector<float>& outData
    )
    {
        if (!data || len < HEADER_LENGTH + EEG_PAYLOAD_SIZE) {
            return false;
        }

        size_t payloadStart = HEADER_LENGTH;
        size_t eegStart = payloadStart + 8;

        for (int ch = 0; ch < EEG_CHANNEL_COUNT; ++ch) {
            size_t offset = eegStart + ch * EEG_CHANNEL_BYTES;

            uint32_t raw =
                (static_cast<uint32_t>(data[offset]) << 16) |
                (static_cast<uint32_t>(data[offset + 1]) << 8) |
                static_cast<uint32_t>(data[offset + 2]);

            if (raw & 0x800000) raw |= 0xFF000000;
            int32_t signedVal = static_cast<int32_t>(raw);

            float value = static_cast<float>(signedVal);
            value *= (2.0f * 4.5f / 16777215.0f);
            value *= (1000000.0f / 24.0f);

            outData.push_back(value);
        }

        return true;
    }

    bool Parser::parseEEGPacketToBuffer(
        const unsigned char* recvData, size_t dataLen, vector<vector<float>>* buffer)
    {
        if (recvData == nullptr || buffer == nullptr) return false;

        size_t payloadStart = 6;

        if (dataLen < payloadStart + EEG_PAYLOAD_SIZE) return false;

        vector<float> floatBuffer;
        floatBuffer.reserve(EEG_CHANNEL_COUNT);

        for (int ch = 0; ch < EEG_CHANNEL_COUNT; ++ch) {
            size_t offset = payloadStart + ch * EEG_CHANNEL_BYTES;

            uint32_t raw = ((uint32_t)recvData[offset] << 16) |
                ((uint32_t)recvData[offset + 1] << 8) |
                ((uint32_t)recvData[offset + 2]);

            if (raw & 0x800000) raw |= 0xFF000000;

            int32_t signedVal = static_cast<int32_t>(raw);

            float value = static_cast<float>(signedVal);
            value *= (2.0f * 4.5f / 16777215.0f);
            value *= (1000.0f * 1000.0f / 24.0f);

            floatBuffer.push_back(value);
        }

        buffer->push_back(floatBuffer);
        return true;
    }

    bool Parser::parseImpedancePacketToBuffer(
        const unsigned char* recvData, size_t dataLen,
        std::vector<std::vector<float>>* buffer)
    {
        if (recvData == nullptr || buffer == nullptr) return false;

        size_t payloadStart = 8;

        if (dataLen < payloadStart + IMPEDANCE_PAYLOAD_SIZE) return false;

        vector<float> floatBuffer;
        floatBuffer.reserve(IMPEDANCE_CHANNEL_COUNT);

        for (int ch = 0; ch < IMPEDANCE_CHANNEL_COUNT; ++ch) {
            size_t offset = payloadStart + ch * IMPEDANCE_CHANNEL_BYTES;

            uint32_t raw = ((uint32_t)recvData[offset] << 16) |
                ((uint32_t)recvData[offset + 1] << 8) |
                ((uint32_t)recvData[offset + 2]);

            if (raw & 0x800000) raw |= 0xFF000000;
            int32_t signedVal = static_cast<int32_t>(raw);

            float value = static_cast<float>(signedVal);

            floatBuffer.push_back(value);
        }

        buffer->push_back(floatBuffer);
        return true;
    }

}