#pragma once

#include <vector>
#include <cstdint>
#include <stdexcept>
#include <mutex>

// Undefine Windows macros that conflict with std::min/max
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

namespace protocol {

    /**
     * @brief High-performance ring buffer for raw packet data
     * 
     * Optimized for continuous data streaming with:
     * - O(1) write and read operations
     * - No memory reallocation after initialization
     * - Thread-safe operations
     * - Automatic overwrite of old data when full
     */
    class RingBuffer {
    private:
        std::vector<uint8_t> buffer;
        size_t capacity;
        size_t head;        // Write position
        size_t tail;        // Read position
        size_t dataSize;    // Current data size
        mutable std::mutex mutex;

    public:
        /**
         * @brief Constructor
         * @param size Buffer capacity in bytes (default: 8KB for ~222 packets @ 36 bytes)
         */
        explicit RingBuffer(size_t size = 8192) 
            : capacity(size), head(0), tail(0), dataSize(0) {
            buffer.resize(capacity);
        }

        /**
         * @brief Write data to ring buffer
         * @param data Data pointer
         * @param len Data length
         * @return Number of bytes actually written
         */
        size_t write(const uint8_t* data, size_t len) {
            if (!data || len == 0) return 0;

            std::lock_guard<std::mutex> lock(mutex);

            // If buffer would overflow, drop oldest data
            if (dataSize + len > capacity) {
                // Calculate how much to drop
                size_t overflow = (dataSize + len) - capacity;
                // Move tail forward to drop old data
                tail = (tail + overflow) % capacity;
                dataSize -= overflow;
            }

            size_t written = 0;
            while (written < len && dataSize < capacity) {
                buffer[head] = data[written];
                head = (head + 1) % capacity;
                dataSize++;
                written++;
            }

            return written;
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

            size_t toRead = std::min(len, dataSize);
            size_t pos = tail;
            
            for (size_t i = 0; i < toRead; i++) {
                data[i] = buffer[pos];
                pos = (pos + 1) % capacity;
            }

            return toRead;
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

            size_t toRead = std::min(len, dataSize);
            
            for (size_t i = 0; i < toRead; i++) {
                data[i] = buffer[tail];
                tail = (tail + 1) % capacity;
            }

            dataSize -= toRead;
            return toRead;
        }

        /**
         * @brief Skip/discard data
         * @param len Bytes to skip
         * @return Number of bytes actually skipped
         */
        size_t skip(size_t len) {
            std::lock_guard<std::mutex> lock(mutex);

            size_t toSkip = std::min(len, dataSize);
            tail = (tail + toSkip) % capacity;
            dataSize -= toSkip;
            
            return toSkip;
        }

        /**
         * @brief Get current data size
         */
        size_t size() const {
            std::lock_guard<std::mutex> lock(mutex);
            return dataSize;
        }

        /**
         * @brief Get buffer capacity
         */
        size_t getCapacity() const {
            return capacity;
        }

        /**
         * @brief Check if buffer is empty
         */
        bool isEmpty() const {
            std::lock_guard<std::mutex> lock(mutex);
            return dataSize == 0;
        }

        /**
         * @brief Check if buffer is full
         */
        bool isFull() const {
            std::lock_guard<std::mutex> lock(mutex);
            return dataSize >= capacity;
        }

        /**
         * @brief Clear all data
         */
        void clear() {
            std::lock_guard<std::mutex> lock(mutex);
            head = 0;
            tail = 0;
            dataSize = 0;
        }

        /**
         * @brief Get fill percentage
         * @return Percentage (0.0 to 1.0)
         */
        double getFillPercentage() const {
            std::lock_guard<std::mutex> lock(mutex);
            return static_cast<double>(dataSize) / capacity;
        }
    };

} // namespace protocol
