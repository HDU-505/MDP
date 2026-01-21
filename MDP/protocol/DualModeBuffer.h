#pragma once

#include <vector>
#include <cstdint>
#include <mutex>
#include <deque>
#include "RingBuffer.h"

// Undefine Windows macros that conflict with std::min/max
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

namespace protocol {

    /**
     * @brief Dual-mode buffer optimized for different recording modes
     * 
     * - NORMAL mode: No data loss, expandable buffer with backpressure warning
     * - IMPEDANCE mode: Latest data only, ring buffer with auto-overwrite
     */
    class DualModeBuffer {
    public:
        enum class Mode {
            NORMAL,      // EEG mode: preserve all data, no loss
            IMPEDANCE    // Impedance mode: keep latest data only
        };

    private:
        Mode currentMode;
        
        // For NORMAL mode: expandable buffer (no data loss)
        std::deque<uint8_t> normalBuffer;
        const size_t normalBufferWarningSize = 32768;  // 32KB warning threshold
        const size_t normalBufferMaxSize = 65536;      // 64KB max (safety limit)
        
        // For IMPEDANCE mode: ring buffer (latest data only)
        std::unique_ptr<RingBuffer> ringBuffer;
        
        mutable std::mutex mutex;
        
        // Statistics
        uint64_t totalBytesReceived = 0;
        uint64_t bytesDropped = 0;
        bool bufferWarningIssued = false;

    public:
        /**
         * @brief Constructor
         * @param mode Buffer mode
         * @param ringBufferSize Size for ring buffer in impedance mode (default: 16KB)
         */
        explicit DualModeBuffer(Mode mode = Mode::NORMAL, size_t ringBufferSize = 16384)
            : currentMode(mode)
        {
            if (mode == Mode::IMPEDANCE) {
                ringBuffer = std::make_unique<RingBuffer>(ringBufferSize);
            }
        }

        /**
         * @brief Switch buffer mode
         * @param mode New mode
         * @param clearData Whether to clear existing data
         */
        void setMode(Mode mode, bool clearData = true) {
            std::lock_guard<std::mutex> lock(mutex);
            
            if (currentMode == mode && !clearData) {
                return;  // No change needed
            }

            if (clearData) {
                normalBuffer.clear();
                if (ringBuffer) {
                    ringBuffer->clear();
                }
                bufferWarningIssued = false;
            }

            currentMode = mode;
            
            // Initialize ring buffer if switching to impedance mode
            if (mode == Mode::IMPEDANCE && !ringBuffer) {
                ringBuffer = std::make_unique<RingBuffer>(16384);
            }
        }

        /**
         * @brief Write data to buffer
         * @param data Data pointer
         * @param len Data length
         * @return Number of bytes written (may be less than len if buffer full in NORMAL mode)
         */
        size_t write(const uint8_t* data, size_t len) {
            if (!data || len == 0) return 0;

            std::lock_guard<std::mutex> lock(mutex);
            totalBytesReceived += len;

            if (currentMode == Mode::NORMAL) {
                // NORMAL mode: expandable buffer with limits
                
                // Check if would exceed max size
                if (normalBuffer.size() + len > normalBufferMaxSize) {
                    // Drop oldest data to make room (last resort)
                    size_t overflow = (normalBuffer.size() + len) - normalBufferMaxSize;
                    normalBuffer.erase(normalBuffer.begin(), normalBuffer.begin() + overflow);
                    bytesDropped += overflow;
                }

                // Append data
                normalBuffer.insert(normalBuffer.end(), data, data + len);

                // Issue warning if buffer getting full
                if (normalBuffer.size() > normalBufferWarningSize && !bufferWarningIssued) {
                    bufferWarningIssued = true;
                    // Warning: Consumer not reading fast enough!
                }

                return len;
            }
            else {
                // IMPEDANCE mode: ring buffer with auto-overwrite
                return ringBuffer->write(data, len);
            }
        }

        /**
         * @brief Read data without removing it
         * @param data Output buffer
         * @param len Bytes to read
         * @return Number of bytes actually read
         */
        size_t peek(uint8_t* data, size_t len) const {
            if (!data || len == 0) return 0;

            std::lock_guard<std::mutex> lock(mutex);

            if (currentMode == Mode::NORMAL) {
                size_t toRead = std::min(len, normalBuffer.size());
                std::copy(normalBuffer.begin(), normalBuffer.begin() + toRead, data);
                return toRead;
            }
            else {
                return ringBuffer->peek(data, len);
            }
        }

        /**
         * @brief Read and remove data
         * @param data Output buffer
         * @param len Bytes to read
         * @return Number of bytes actually read
         */
        size_t read(uint8_t* data, size_t len) {
            if (!data || len == 0) return 0;

            std::lock_guard<std::mutex> lock(mutex);

            if (currentMode == Mode::NORMAL) {
                size_t toRead = std::min(len, normalBuffer.size());
                std::copy(normalBuffer.begin(), normalBuffer.begin() + toRead, data);
                normalBuffer.erase(normalBuffer.begin(), normalBuffer.begin() + toRead);
                
                // Reset warning flag if buffer drained enough
                if (normalBuffer.size() < normalBufferWarningSize / 2) {
                    bufferWarningIssued = false;
                }
                
                return toRead;
            }
            else {
                return ringBuffer->read(data, len);
            }
        }

        /**
         * @brief Skip/discard data
         * @param len Bytes to skip
         * @return Number of bytes actually skipped
         */
        size_t skip(size_t len) {
            std::lock_guard<std::mutex> lock(mutex);

            if (currentMode == Mode::NORMAL) {
                size_t toSkip = std::min(len, normalBuffer.size());
                normalBuffer.erase(normalBuffer.begin(), normalBuffer.begin() + toSkip);
                return toSkip;
            }
            else {
                return ringBuffer->skip(len);
            }
        }

        /**
         * @brief Get current data size
         */
        size_t size() const {
            std::lock_guard<std::mutex> lock(mutex);
            
            if (currentMode == Mode::NORMAL) {
                return normalBuffer.size();
            }
            else {
                return ringBuffer->size();
            }
        }

        /**
         * @brief Get buffer capacity
         */
        size_t getCapacity() const {
            if (currentMode == Mode::NORMAL) {
                return normalBufferMaxSize;
            }
            else {
                return ringBuffer->getCapacity();
            }
        }

        /**
         * @brief Check if buffer is empty
         */
        bool isEmpty() const {
            std::lock_guard<std::mutex> lock(mutex);
            
            if (currentMode == Mode::NORMAL) {
                return normalBuffer.empty();
            }
            else {
                return ringBuffer->isEmpty();
            }
        }

        /**
         * @brief Check if buffer is approaching full (NORMAL mode warning)
         */
        bool isNearFull() const {
            std::lock_guard<std::mutex> lock(mutex);
            
            if (currentMode == Mode::NORMAL) {
                return normalBuffer.size() > normalBufferWarningSize;
            }
            else {
                return ringBuffer->getFillPercentage() > 0.8;
            }
        }

        /**
         * @brief Clear all data
         */
        void clear() {
            std::lock_guard<std::mutex> lock(mutex);
            
            normalBuffer.clear();
            if (ringBuffer) {
                ringBuffer->clear();
            }
            bufferWarningIssued = false;
        }

        /**
         * @brief Get fill percentage
         */
        double getFillPercentage() const {
            std::lock_guard<std::mutex> lock(mutex);
            
            if (currentMode == Mode::NORMAL) {
                return static_cast<double>(normalBuffer.size()) / normalBufferMaxSize;
            }
            else {
                return ringBuffer->getFillPercentage();
            }
        }

        /**
         * @brief Get current mode
         */
        Mode getMode() const {
            std::lock_guard<std::mutex> lock(mutex);
            return currentMode;
        }

        /**
         * @brief Get statistics
         */
        struct Stats {
            uint64_t totalBytes;
            uint64_t droppedBytes;
            size_t currentSize;
            size_t capacity;
            double fillPercentage;
            bool warningState;
            Mode mode;
        };

        Stats getStatistics() const {
            std::lock_guard<std::mutex> lock(mutex);
            
            Stats stats;
            stats.totalBytes = totalBytesReceived;
            stats.droppedBytes = bytesDropped;
            stats.warningState = bufferWarningIssued;
            stats.mode = currentMode;
            
            if (currentMode == Mode::NORMAL) {
                stats.currentSize = normalBuffer.size();
                stats.capacity = normalBufferMaxSize;
                stats.fillPercentage = static_cast<double>(normalBuffer.size()) / normalBufferMaxSize;
            }
            else {
                stats.currentSize = ringBuffer->size();
                stats.capacity = ringBuffer->getCapacity();
                stats.fillPercentage = ringBuffer->getFillPercentage();
            }
            
            return stats;
        }
    };

} // namespace protocol
