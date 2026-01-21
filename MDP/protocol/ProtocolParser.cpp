#include "Parser.h"
#include <mutex>
#include <stdexcept>
#include <cstring>

using namespace std;

namespace protocol {

    // ================= Command Building =================

    std::vector<uint8_t> Parser::buildControlPacket(PacketType packetType) {
        std::vector<uint8_t> data(CMD_TOTAL_LENGTH);

        // Command format: AE 12 02 XX
        // AE 12 02 10 - Normal EEG data mode (0x10)
        // AE 12 02 11 - AC impedance mode (0x11)
        // AE 12 02 12 - Stop acquisition (0x12)
        data[0] = CMD_HEADER_0;
        data[1] = CMD_HEADER_1;
        data[2] = CMD_HEADER_2;
        data[3] = packetType;

        return data;
    }

    // ================= Data Packet Validation =================

    bool Parser::validateNewPacket(const uint8_t* data, size_t len) {
        if (!data || len < NEW_PACKET_TOTAL_SIZE) {
            return false;
        }

        // Validate header marker: 0x02 0x10 or 0x02 0x11
        if (data[IDX_HEAD_MARKER_H] != HEAD_MARKER_H) {
            return false;
        }
        
        // 0x10 = Normal data mode, 0x11 = Impedance mode
        if (data[IDX_HEAD_MARKER_L] != 0x10 && data[IDX_HEAD_MARKER_L] != 0x11) {
            return false;
        }

        // Validate tail marker: 0xAE 0x12
        size_t tailPos = NEW_PACKET_HEADER_SIZE + NEW_PACKET_ADC_SIZE;
        if (data[tailPos] != TAIL_MARKER_H ||
            data[tailPos + 1] != TAIL_MARKER_L) {
            return false;
        }

        return true;
    }

    uint32_t Parser::getTimestampFromNewPacket(const uint8_t* data, size_t len) {
        if (!data || len < NEW_PACKET_HEADER_SIZE) return 0;
        // 4-byte timestamp, big-endian
        return (static_cast<uint32_t>(data[IDX_TIMESTAMP_0]) << 24) |
               (static_cast<uint32_t>(data[IDX_TIMESTAMP_1]) << 16) |
               (static_cast<uint32_t>(data[IDX_TIMESTAMP_2]) << 8) |
               static_cast<uint32_t>(data[IDX_TIMESTAMP_3]);
    }

    uint32_t Parser::getSampleSeqFromNewPacket(const uint8_t* data, size_t len) {
        if (!data || len < NEW_PACKET_HEADER_SIZE) return 0;
        // 4-byte sample sequence, big-endian
        return (static_cast<uint32_t>(data[IDX_SAMPLE_SEQ_0]) << 24) |
               (static_cast<uint32_t>(data[IDX_SAMPLE_SEQ_1]) << 16) |
               (static_cast<uint32_t>(data[IDX_SAMPLE_SEQ_2]) << 8) |
               static_cast<uint32_t>(data[IDX_SAMPLE_SEQ_3]);
    }

    // ================= Data Conversion =================

    /**
     * @brief Convert 24-bit ADC raw data to signed integer
     * @param raw 24-bit unsigned data
     * @return Signed integer
     */
    static int32_t convertRawToSignedInt(uint32_t raw) {
        // 24-bit two's complement conversion to 32-bit signed integer
        // If MSB is 1 (negative), extend sign bit
        if (raw > 0x7FFFFF) {
            return static_cast<int32_t>(raw - 0x1000000);
        }
        return static_cast<int32_t>(raw);
    }

    /**
     * @brief Convert signed integer to voltage value (microvolts)
     * @param signedVal Signed integer
     * @return Voltage value (uV)
     */
    static float convertToVoltageUV(int32_t signedVal) {
        // V_lsb = (2 * V_ref) / (Gain * (2^24 - 1))
        // V_ref = 4.5V, Gain = 1
        // LSB = 9V / 16777215 ≈ 0.53644uV
        return static_cast<float>(signedVal) * ADS1299_LSB_UV;
    }

    // ================= Data Packet Parsing =================

    bool Parser::parseNewEEGPacket2Byte(
        const uint8_t* data,
        size_t len,
        std::vector<uint8_t>& outBytes
    )
    {
        // Validate packet
        if (!validateNewPacket(data, len)) {
            return false;
        }

        // Get sample sequence
        uint32_t sampleSeq = getSampleSeqFromNewPacket(data, len);
        sequenceID = sampleSeq;

        // Write 8-byte sequence counter (little-endian)
        for (int i = 0; i < 8; i++) {
            outBytes.push_back(
                static_cast<uint8_t>((sequenceID >> (i * 8)) & 0xFF)
            );
        }

        // Parse 8 channels of 24-bit ADC data
        size_t adcStart = IDX_ADC_DATA_START;

        for (int ch = 0; ch < EEG_CHANNEL_COUNT; ++ch) {
            size_t offset = adcStart + ch * EEG_CHANNEL_BYTES;

            // Read 24-bit big-endian
            uint32_t raw =
                (static_cast<uint32_t>(data[offset]) << 16) |
                (static_cast<uint32_t>(data[offset + 1]) << 8) |
                static_cast<uint32_t>(data[offset + 2]);

            // Convert 24-bit two's complement to 32-bit signed integer
            int32_t signedVal = convertRawToSignedInt(raw);

            // Convert to voltage (microvolts)
            float value = convertToVoltageUV(signedVal);

            // Convert float to byte stream
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
        // Validate packet
        if (!validateNewPacket(data, len)) {
            return false;
        }

        // Parse 8 channels of 24-bit ADC data
        size_t adcStart = IDX_ADC_DATA_START;

        for (int ch = 0; ch < EEG_CHANNEL_COUNT; ++ch) {
            size_t offset = adcStart + ch * EEG_CHANNEL_BYTES;

            // Read 24-bit big-endian
            uint32_t raw =
                (static_cast<uint32_t>(data[offset]) << 16) |
                (static_cast<uint32_t>(data[offset + 1]) << 8) |
                static_cast<uint32_t>(data[offset + 2]);

            // Convert 24-bit two's complement to 32-bit signed integer
            int32_t signedVal = convertRawToSignedInt(raw);

            // Convert to voltage (microvolts)
            float value = convertToVoltageUV(signedVal);

            outData.push_back(value);
        }

        return true;
    }

} // namespace protocol
