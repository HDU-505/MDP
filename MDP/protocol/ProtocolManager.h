#pragma once
#include "Processor.h"
#include "Constants.h"
#include <sstream>
#include <iomanip>
#include "Parser.h"
#include "../Amplifier_LIB.h"
#include "../ImpedanceUtil.h"
#include "../RealTimeImpedanceCalculator.h"
#include "../ErrorHandler.h"
#include <vector>
#include <memory>

namespace protocol {
    
    // Single sample output size constant
    // 8 bytes (counter) + 8 channels * 4 bytes (float) = 40 bytes
    constexpr size_t SINGLE_SAMPLE_OUTPUT_SIZE = 8 + (EEG_CHANNEL_COUNT * sizeof(float));
    
    /**
     * @brief Optimized Protocol Manager
     *
     * Key optimizations:
     * - Ring buffer for efficient data handling
     * - Real-time impedance calculation with sliding window
     * - Minimal memory allocation
     * - Thread-safe operations
     *
     * Data Flow:
     * 1. Raw bytes → processData() → RingBuffer (O(1) operations)
     * 2. RingBuffer → extractPackets() → Complete 36-byte packets
     * 3. Parser → parsePacket() → Voltage data (uV)
     * 4. RealTimeImpedanceCalculator → Sliding window → Latest impedance
     */
    class ProtocolManager {
    private:
        std::unique_ptr<Processor> processor;
        std::unique_ptr<Parser> parser;
        std::unique_ptr<RealTimeImpedanceCalculator> realTimeImpedance;
        
        RecordingMode currentMode;
        
        // Statistics
        uint64_t totalPacketsReceived = 0;
        uint64_t totalPacketsDropped = 0;
        uint64_t totalBytesReceived = 0;
        
        mutable std::mutex statsMutex;

    public:
        /**
         * @brief Constructor
         * @param recordingMode Initial recording mode (default: RM_NORMAL)
         */
        explicit ProtocolManager(RecordingMode recordingMode = RM_NORMAL);
        
        ~ProtocolManager() = default;

        /**
         * @brief Process incoming raw data from hardware
         * 
         * Optimized for high-frequency calls (250Hz):
         * - O(1) write to ring buffer
         * - No memory allocation
         * - Minimal lock contention
         * 
         * @param data Raw byte data pointer
         * @param len Data length in bytes
         * 
         * Thread-safe. Can be called from BLE callback thread.
         */
        void processData(const uint8_t* data, size_t len);

        /**
         * @brief Build control command packet
         * @param commandType Command type (not used in current protocol)
         * @param packetType Packet type (0x10=EEG, 0x11=Impedance, 0x12=Stop)
         * @param streamMask Stream mask (not used in current protocol)
         * @return Command packet (4 bytes: AE 12 02 XX)
         */
        std::vector<uint8_t> buildPacket(
            ComandType commandType,
            PacketType packetType,
            StreamMask streamMask);

        /**
         * @brief Get EEG data samples
         * 
         * Output format per sample (40 bytes):
         * - Bytes 0-7:   Sample counter (uint64_t, little-endian)
         * - Bytes 8-39:  Channel data (8 × float, voltage in uV)
         * 
         * Performance:
         * - Uses pre-allocated buffers
         * - Efficient ring buffer extraction
         * - Typical latency: < 1ms for 50 samples
         * 
         * @param sampleCount Maximum number of samples to retrieve
         * @return Byte vector containing formatted samples
         */
        std::vector<uint8_t> getEEGData(int sampleCount);

        /**
         * @brief Get real-time impedance data (OPTIMIZED)
         * 
         * New approach:
         * - Uses sliding window of latest 248 samples per channel
         * - Always returns impedance based on MOST RECENT data
         * - Fast calculation: < 10ms for all 8 channels
         * - No need to wait for full 1 second
         * 
         * Output format (72 bytes):
         * - Bytes 0-3:   REF electrode (float, kΩ)
         * - Bytes 4-7:   GND electrode (float, kΩ)
         * - Bytes 8-71:  8 channels × 2 (impedance + reserved)
         * 
         * @return Vector of floats containing impedance values
         * 
         * Returns -1.0 for channels with insufficient data (< 248 samples)
         */
        std::vector<float> getImpedanceData();

        /**
         * @brief Check if impedance data is ready
         * @return true if all channels have enough samples
         */
        bool isImpedanceReady() const {
            return realTimeImpedance && realTimeImpedance->areAllChannelsReady();
        }

        /**
         * @brief Get impedance readiness percentage
         * @return Percentage (0.0 to 1.0)
         */
        double getImpedanceReadiness() const;

        /**
         * @brief Get single sample output size
         * @return Size in bytes (40 bytes: 8-byte counter + 8×4 float data)
         */
        int getSampleLength() const {
            return static_cast<int>(SINGLE_SAMPLE_OUTPUT_SIZE);
        }

        /**
         * @brief Set recording mode
         * @param mode New recording mode
         * 
         * Automatically switches buffer mode:
         * - RM_NORMAL: Uses expandable deque (no data loss)
         * - RM_IMPEDANCE: Uses ring buffer (latest data only for real-time display)
         * 
         * Note: Only clears buffers if mode actually changes
         */
        void setRecordingMode(RecordingMode mode) {
            // Check if mode is actually changing
            if (currentMode == mode) {
                return;  // No change needed, don't clear buffers!
            }
            
            currentMode = mode;
            
            // Switch buffer mode based on recording mode
            if (processor) {
                if (mode == RM_IMPEDANCE) {
                    processor->setBufferMode(DualModeBuffer::Mode::IMPEDANCE, true);
                    sdk::Logger::Info("Switched to IMPEDANCE mode: ring buffer (latest data only)");
                } else {
                    processor->setBufferMode(DualModeBuffer::Mode::NORMAL, true);
                    sdk::Logger::Info("Switched to NORMAL mode: expandable buffer (no data loss)");
                }
            }
            
            // Clear impedance calculator when switching modes
            if (realTimeImpedance) {
                realTimeImpedance->clear();
            }
        }

        /**
         * @brief Get current recording mode
         */
        RecordingMode getRecordingMode() const {
            return currentMode;
        }

        /**
         * @brief Get comprehensive statistics
         */
        struct Statistics {
            uint64_t packetsReceived;
            uint64_t packetsDropped;
            uint64_t bytesReceived;
            double dropRate;
            size_t bufferSize;
            size_t bufferCapacity;
            double bufferFill;
            bool impedanceReady;
            uint64_t packetsLost;           // 序号检测到的丢帧
            uint64_t interpolatedSamples;   // 插值补全的帧
        };
        
        Statistics getStatistics() const;

        /**
         * @brief 丢包情况报告（Debug 和 Release 均可用）
         */
        std::string getPacketLossReport() const;

        void resetStatistics() {
            std::lock_guard<std::mutex> lock(statsMutex);
            totalPacketsReceived = 0;
            totalPacketsDropped  = 0;
            totalBytesReceived   = 0;
            if (parser) { parser->resetStats(); }
        }

        /** 断开重连时调用，避免序号跳跃误判为丢包 */
        void resetConnectionState() {
            if (parser) { parser->resetLossState(); }
        }

        void clearBuffers();
    };

} // namespace protocol
