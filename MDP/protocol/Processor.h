#pragma once

#include <cstdint>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include "DualModeBuffer.h"

namespace protocol {

    /**
     * @brief Optimized byte stream protocol processor with dual-mode buffering
     *
     * Key Features:
     * - NORMAL mode: No data loss, uses expandable deque buffer
     * - IMPEDANCE mode: Latest data only, uses ring buffer
     * - Automatic mode switching based on recording mode
     * - Thread-safe operations
     *
     * Performance:
     * - O(1) append in both modes
     * - O(n) extraction where n = number of packets
     * - Pre-allocated search buffer to avoid allocation
     */
    class Processor {
    public:
        /**
         * @brief Constructor
         *
         * @param packetLen  Fixed packet length in bytes (36 for new protocol)
         * @param timeoutMs  Data wait timeout in milliseconds (default: 500)
         */
        Processor(size_t packetLen, int timeoutMs);

        /**
         * @brief Append raw byte data to internal buffer (thread-safe)
         *
         * Behavior depends on current mode:
         * - NORMAL: Data is preserved, buffer expands (up to 64KB limit)
         * - IMPEDANCE: Latest data kept, old data auto-discarded
         *
         * @param data  Raw byte data pointer
         * @param len   Data length in bytes
         */
        void appendData(const uint8_t* data, size_t len);

        /**
         * @brief Set buffer mode
         * @param mode Buffer mode (NORMAL or IMPEDANCE)
         * @param clearData Whether to clear existing data when switching
         */
        void setBufferMode(DualModeBuffer::Mode mode, bool clearData = true);

        /**
         * @brief Blocking wait and extract complete packets
         *
         * @param maxPackets  Maximum number of packets to extract
         * @return List of complete packets extracted
         */
        std::vector<std::vector<uint8_t>>
            waitAndExtractPackets(size_t maxPackets);

        /**
         * @brief Non-blocking extraction of complete packets
         *
         * @param maxPackets  Maximum number of packets to extract
         * @return List of complete packets extracted
         */
        std::vector<std::vector<uint8_t>>
            extractPackets(size_t maxPackets);

        /**
         * @brief Clear internal buffer and reset state
         */
        void clear();

        /**
         * @brief Get buffer statistics
         */
        struct BufferStats {
            size_t bufferSize;
            size_t bufferCapacity;
            double fillPercentage;
            uint64_t totalBytesReceived;
            uint64_t packetsExtracted;
            uint64_t bytesDropped;
            bool bufferWarning;  // true if buffer is getting full in NORMAL mode
            DualModeBuffer::Mode mode;
        };

        BufferStats getStatistics() const;

    private:
        /**
         * @brief Extract packets with lock already held
         */
        std::vector<std::vector<uint8_t>>
            extractPacketsLocked(size_t maxPackets);

        /**
         * @brief Find next valid packet start in buffer
         */
        size_t findPacketStart(const uint8_t* searchBuffer, size_t bufferSize) const;

        /**
         * @brief Validate packet markers
         */
        bool validatePacket(const uint8_t* packet) const;

    private:
        // Configuration
        size_t fixedPacketLength;
        int    timeoutMs;

        // Dual-mode buffer (smart buffering based on mode)
        std::unique_ptr<DualModeBuffer> buffer;
        
        // Temporary search buffer (pre-allocated to avoid malloc)
        std::vector<uint8_t> searchBuffer;

        // Synchronization
        mutable std::mutex      mtx;
        std::condition_variable cv;

        // Timing
        std::chrono::steady_clock::time_point lastAppendTime;
        
        // Statistics
        uint64_t totalBytesReceived = 0;
        uint64_t totalPacketsExtracted = 0;
    };

} // namespace protocol
