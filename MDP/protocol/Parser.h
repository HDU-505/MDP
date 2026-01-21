#pragma once

#include "Constants.h"
#include <vector>
#include <cstdint>

namespace protocol {

    /**
     * @brief Protocol Parser Class
     *
     * Responsible for building control command packets and parsing data packets.
     * New protocol format:
     * - Commands: AE 12 02 XX (4 bytes)
     * - Data packets: 02 10/11 (header 2) + timestamp(4) + sequence(4) + ADC(24) + AE 12 (tail 2) = 36 bytes
     */
    class Parser {
    private:
        uint32_t sequenceID = 0; // Sequence counter (32-bit)

    public:
        // ================= Control Command Building =================

        /**
         * @brief Build control command packet
         * Command format: AE 12 02 XX
         * - 0x10: Normal EEG data mode
         * - 0x11: AC impedance mode
         * - 0x12: Stop acquisition
         * @param packetType Packet type
         * @return Command packet data (4 bytes)
         */
        std::vector<uint8_t> buildControlPacket(PacketType packetType);

        // ================= Data Packet Parsing =================

        /**
         * @brief Parse new protocol EEG data packet and convert to byte stream
         *
         * Packet format (36 bytes total):
         * - Header marker: 0x02 0x10/0x11 (2 bytes)
         * - Timestamp: 32-bit (4 bytes, big-endian)
         * - Sample sequence: 32-bit (4 bytes, big-endian)
         * - ADC data: 24-bit × 8 channels (24 bytes, big-endian)
         * - Tail marker: 0xAE 0x12 (2 bytes)
         *
         * Output format (40 bytes per sample):
         * - Sequence counter: 8 bytes (uint64_t, little-endian)
         * - Channel data: 4 bytes float × 8 channels (voltage in uV)
         *
         * @param data Raw packet pointer
         * @param len Packet length (must be 36 bytes)
         * @param outBytes Output byte stream (sequence + converted float data)
         * @return true if parsing succeeds
         */
        bool parseNewEEGPacket2Byte(
            const uint8_t* data,
            size_t len,
            std::vector<uint8_t>& outBytes
        );

        /**
         * @brief Parse new protocol EEG data packet and convert to float array
         *
         * @param data Raw packet pointer
         * @param len Packet length (must be 36 bytes)
         * @param outData Output float array (8 channels, voltage in uV)
         * @return true if parsing succeeds
         */
        bool parseNewEEGPacket2Float(
            const uint8_t* data,
            size_t len,
            std::vector<float>& outData
        );

        /**
         * @brief Extract timestamp from packet
         * @param data Packet pointer
         * @param len Data length
         * @return Timestamp value (32-bit)
         */
        uint32_t getTimestampFromNewPacket(const uint8_t* data, size_t len);

        /**
         * @brief Extract sample sequence from packet
         * @param data Packet pointer
         * @param len Data length
         * @return Sample sequence (32-bit)
         */
        uint32_t getSampleSeqFromNewPacket(const uint8_t* data, size_t len);

        /**
         * @brief Validate packet integrity (header/tail markers)
         * @param data Packet pointer
         * @param len Data length
         * @return true if valid
         */
        bool validateNewPacket(const uint8_t* data, size_t len);

        /**
         * @brief Get current sequence ID
         * @return Sequence ID
         */
        uint32_t getSequenceID() { return sequenceID; }
    };

} // namespace protocol
