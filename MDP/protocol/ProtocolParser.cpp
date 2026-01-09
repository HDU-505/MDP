
#include "Parser.h"
#include <mutex>
#include <stdexcept>

using namespace std;

namespace protocol {

    // Build control packet
    std::vector<uint8_t> Parser::buildControlPacket(PacketType packetType) {
        std::vector<uint8_t> data(HEADER_LENGTH); // Header(4) + Seq(2) + Len(2)

        //// Sync Header
        //data[IDX_SYNC_HEADER_H] = (SYNC_HEADER >> 8) & 0xFF;
        //data[IDX_SYNC_HEADER_L] = SYNC_HEADER & 0xFF;
        //data[IDX_VERSION] = PROTOCOL_VERSION;
        //data[IDX_PACKET_TYPE] = packetType;
        //
        //// Seq ID (0)
        //data[IDX_SEQ_ID_H] = 0x00;
        //data[IDX_SEQ_ID_L] = 0x00;
        //
        //// Payload Length (0)
        //data[IDX_PAYLOAD_LEN_H] = 0x00;
        //data[IDX_PAYLOAD_LEN_L] = 0x00;

                /*------------------------------------------------------------------------------------------------*/
        // V2.5版本协议结构（暂时简单版本：2025-5-28）
        /*------------------------------------------------------------------------------------------------*/
        // 包头
        data[IDX_SYNC_HEADER_H] = (SYNC_HEADER >> 8) & 0xFF;  // 高字节
        data[IDX_SYNC_HEADER_L] = SYNC_HEADER & 0xFF;         // 低字节
        data[IDX_VERSION] = PROTOCOL_VERSION;
        data[IDX_PACKET_TYPE] = packetType;

        return data;
    }

    // Build config packet
    std::vector<uint8_t> Parser::buildConfigPacket(PacketType packetType, uint16_t value) {
        std::vector<uint8_t> data(HEADER_LENGTH + 4 + 2); // Header(4) + Seq(2) + Len(2) + Payload(2)

        data[IDX_SYNC_HEADER_H] = (SYNC_HEADER >> 8) & 0xFF;
        data[IDX_SYNC_HEADER_L] = SYNC_HEADER & 0xFF;
        data[IDX_VERSION] = PROTOCOL_VERSION;
        data[IDX_PACKET_TYPE] = packetType;
        
        data[IDX_SEQ_ID_H] = 0x00;
        data[IDX_SEQ_ID_L] = 0x00;
        
        // Payload Length (2)
        data[IDX_PAYLOAD_LEN_H] = 0x00;
        data[IDX_PAYLOAD_LEN_L] = 0x02;

        // Payload
        data[8] = (value >> 8) & 0xFF;
        data[9] = value & 0xFF;
        
        return data;
    }

    // Build stream control packet
    std::vector<uint8_t> Parser::buildStreamControlPacket(PacketType packetType, StreamMask streamMask) {
        std::vector<uint8_t> data(HEADER_LENGTH + 4 + 1); // Header(4) + Seq(2) + Len(2) + Payload(1)

        data[IDX_SYNC_HEADER_H] = (SYNC_HEADER >> 8) & 0xFF;
        data[IDX_SYNC_HEADER_L] = SYNC_HEADER & 0xFF;
        data[IDX_VERSION] = PROTOCOL_VERSION;
        data[IDX_PACKET_TYPE] = packetType;
        
        data[IDX_SEQ_ID_H] = 0x00;
        data[IDX_SEQ_ID_L] = 0x00;
        
        // Payload Length (1)
        data[IDX_PAYLOAD_LEN_H] = 0x00;
        data[IDX_PAYLOAD_LEN_L] = 0x01;

        // Payload
        data[8] = static_cast<uint8_t>(streamMask);

        return data;
    }

    // Build response packet
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

    // Parse response
    ResponseCode Parser::parseResponse(const std::vector<uint8_t>& data) {
        if (data.size() < 9) { // Header(4) + Seq(2) + Len(2) + Payload(1)
            return RESP_ERROR;
        }

        // Payload starts at 8
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

        /* ---------- 1. 解析 8B 计数器 ---------- */
        sequenceID++;

        // 写入 counter（大端）
        for (int i = 0; i < 8; i++) {
            outBytes.push_back(
                static_cast<uint8_t>((sequenceID >> (i * 8)) & 0xFF)
            );
        }

        size_t eegStart = payloadStart + 8;

        /* ---------- 2. 解析 EEG 通道 ---------- */
        for (int ch = 0; ch < EEG_CHANNEL_COUNT; ++ch) {
            size_t offset = eegStart + ch * EEG_CHANNEL_BYTES;

            uint32_t raw =
                (static_cast<uint32_t>(data[offset]) << 16) |
                (static_cast<uint32_t>(data[offset + 1]) << 8) |
                static_cast<uint32_t>(data[offset + 2]);

            // 24-bit 符号扩展
            if (raw & 0x800000) raw |= 0xFF000000;
            int32_t signedVal = static_cast<int32_t>(raw);

            float value = static_cast<float>(signedVal);
            value *= (2.0f * 4.5f / 16777215.0f);
            value *= (1000000.0f / 24.0f);

            // float → byte
            const uint8_t* p = reinterpret_cast<const uint8_t*>(&value);
            outBytes.insert(outBytes.end(), p, p + sizeof(float));
        }

        return true;
    }


    bool Parser::parseEEGPacket2Float(
        const uint8_t* data,
        size_t len,
        std::vector<float>& outData // 修改：这里改为 float 的 vector
    )
    {
        // 检查数据长度是否足够
        if (!data || len < HEADER_LENGTH + EEG_PAYLOAD_SIZE) {
            return false;
        }

        size_t payloadStart = HEADER_LENGTH;

        size_t eegStart = payloadStart + 8; // 保持原偏移逻辑（假设协议中这里确实跳过了8字节）

        /* ---------- 2. 解析 EEG 通道并存为 float ---------- */
        for (int ch = 0; ch < EEG_CHANNEL_COUNT; ++ch) {
            size_t offset = eegStart + ch * EEG_CHANNEL_BYTES;

            // 保持原有的 24-bit 大端解析逻辑
            uint32_t raw =
                (static_cast<uint32_t>(data[offset]) << 16) |
                (static_cast<uint32_t>(data[offset + 1]) << 8) |
                static_cast<uint32_t>(data[offset + 2]);

            // 24-bit 符号扩展
            if (raw & 0x800000) raw |= 0xFF000000;
            int32_t signedVal = static_cast<int32_t>(raw);

            // 转换为电压值
            float value = static_cast<float>(signedVal);
            value *= (2.0f * 4.5f / 16777215.0f); // 缩放因子
            value *= (1000000.0f / 24.0f);        // 增益调整

            // 修改：直接存入 float 值，不需要转回 byte
            outData.push_back(value);
        }

        return true;
    }



    // Parse EEG Packet
    bool Parser::parseEEGPacketToBuffer(
        const unsigned char* recvData, size_t dataLen, vector<vector<float>>* buffer)
    {
        if (recvData == nullptr || buffer == nullptr) return false;

        // Header (4) + Seq(2) + Len(2) = 8 bytes overhead
        size_t payloadStart = 6;
        
        if (dataLen < payloadStart + EEG_PAYLOAD_SIZE) return false;

        vector<float> floatBuffer;
        floatBuffer.reserve(EEG_CHANNEL_COUNT);

        for (int ch = 0; ch < EEG_CHANNEL_COUNT; ++ch) {
            size_t offset = payloadStart + ch * EEG_CHANNEL_BYTES;
            
            // 24-bit big endian
            uint32_t raw = ((uint32_t)recvData[offset] << 16) |
                ((uint32_t)recvData[offset + 1] << 8) |
                ((uint32_t)recvData[offset + 2]);

            // Sign extension
            if (raw & 0x800000) raw |= 0xFF000000;

            int32_t signedVal = static_cast<int32_t>(raw);

            // Scale to uV (example conversion factor, adjust as needed)
            float value = static_cast<float>(signedVal);
            value *= (2.0f * 4.5f / 16777215.0f); // Vref = 4.5V, Gain = 24?
            value *= (1000.0f * 1000.0f / 24.0f); // Convert to uV

            floatBuffer.push_back(value);
        }
        
        buffer->push_back(floatBuffer);
        return true;
    }

    // Parse Impedance Packet
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

             // 24-bit big endian
            uint32_t raw = ((uint32_t)recvData[offset] << 16) |
                ((uint32_t)recvData[offset + 1] << 8) |
                ((uint32_t)recvData[offset + 2]);

             // Assuming impedance is also signed or unsigned? 
             // Usually impedance is positive, but using same parsing logic for now
            if (raw & 0x800000) raw |= 0xFF000000;
            int32_t signedVal = static_cast<int32_t>(raw);
            
            float value = static_cast<float>(signedVal);
            // Impedance conversion might be different, but keeping it raw-ish or same scale for now
            // user can adjust conversion factor
            
            floatBuffer.push_back(value);
        }

        buffer->push_back(floatBuffer);
        return true;
    }

}
