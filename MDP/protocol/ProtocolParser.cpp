
#include "Parser.h"
#include <mutex>
#include <stdexcept>

using namespace std;

namespace protocol {

    // Build control packet
    std::vector<uint8_t> Parser::buildControlPacket(PacketType packetType) {
        std::vector<uint8_t> data(HEADER_LENGTH + 4); // Header(4) + Seq(2) + Len(2)

        // Sync Header
        data[IDX_SYNC_HEADER_H] = (SYNC_HEADER >> 8) & 0xFF;
        data[IDX_SYNC_HEADER_L] = SYNC_HEADER & 0xFF;
        data[IDX_VERSION] = PROTOCOL_VERSION;
        data[IDX_PACKET_TYPE] = packetType;
        
        // Seq ID (0)
        data[IDX_SEQ_ID_H] = 0x00;
        data[IDX_SEQ_ID_L] = 0x00;
        
        // Payload Length (0)
        data[IDX_PAYLOAD_LEN_H] = 0x00;
        data[IDX_PAYLOAD_LEN_L] = 0x00;

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

    // Parse EEG Packet
    bool Parser::parseEEGPacketToBuffer(
        const unsigned char* recvData, size_t dataLen, vector<vector<float>>* buffer)
    {
        if (recvData == nullptr || buffer == nullptr) return false;

        // Header (4) + Seq(2) + Len(2) = 8 bytes overhead
        size_t payloadStart = 8;
        
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
