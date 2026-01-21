#include "Processor.h"
#include <algorithm>
#include <chrono>
#include "Constants.h"

namespace protocol {

    Processor::Processor(size_t packetLen, int timeoutMs)
        : fixedPacketLength(packetLen),
        timeoutMs(timeoutMs),
        lastAppendTime(std::chrono::steady_clock::now()) {
        
        // Validate packet length
        if (packetLen != NEW_PACKET_TOTAL_SIZE) {
            throw std::runtime_error("Invalid packet length. Expected 36 bytes.");
        }

        // Initialize dual-mode buffer (default: NORMAL mode for EEG data)
        buffer = std::make_unique<DualModeBuffer>(DualModeBuffer::Mode::NORMAL, 16384);
        
        // Pre-allocate search buffer (4KB - enough for ~111 packets)
        searchBuffer.resize(4096);
    }

    void Processor::setBufferMode(DualModeBuffer::Mode mode, bool clearData) {
        if (buffer) {
            buffer->setMode(mode, clearData);
        }
    }

    void Processor::appendData(const uint8_t* data, size_t len) {
        if (!data || len == 0) return;

        {
            std::lock_guard<std::mutex> lock(mtx);

            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - lastAppendTime).count();

            // If timeout exceeded in NORMAL mode, might indicate disconnection
            // Clear buffer to avoid stale data
            if (elapsed > timeoutMs && buffer->getMode() == DualModeBuffer::Mode::NORMAL) {
                buffer->clear();
            }

            // Write to dual-mode buffer
            size_t written = buffer->write(data, len);
            totalBytesReceived += written;
            
            lastAppendTime = now;
        }
        cv.notify_one();
    }

    std::vector<std::vector<uint8_t>>
        Processor::waitAndExtractPackets(size_t maxPackets) {
        std::unique_lock<std::mutex> lock(mtx);

        // Wait for data or timeout
        cv.wait_for(lock, std::chrono::milliseconds(timeoutMs), [this] {
            return buffer->size() >= fixedPacketLength;
        });

        return extractPacketsLocked(maxPackets);
    }

    std::vector<std::vector<uint8_t>>
        Processor::extractPackets(size_t maxPackets) {
        std::lock_guard<std::mutex> lock(mtx);
        return extractPacketsLocked(maxPackets);
    }

    bool Processor::validatePacket(const uint8_t* packet) const {
        if (!packet) return false;

        // Check header: 0x02 0x10/0x11
        if (packet[0] != HEAD_MARKER_H) return false;
        if (packet[1] != 0x10 && packet[1] != 0x11) return false;

        // Check tail: 0xAE 0x12
        size_t tailPos = NEW_PACKET_HEADER_SIZE + NEW_PACKET_ADC_SIZE;
        if (packet[tailPos] != TAIL_MARKER_H) return false;
        if (packet[tailPos + 1] != TAIL_MARKER_L) return false;

        return true;
    }

    size_t Processor::findPacketStart(const uint8_t* searchBuf, size_t bufferSize) const {
        if (!searchBuf || bufferSize < fixedPacketLength) {
            return SIZE_MAX;
        }

        // Search for valid packet (header + tail markers)
        for (size_t i = 0; i <= bufferSize - fixedPacketLength; ++i) {
            // Quick check: header marker
            if (searchBuf[i] == HEAD_MARKER_H &&
                (searchBuf[i + 1] == 0x10 || searchBuf[i + 1] == 0x11)) {
                
                // Verify tail marker
                size_t tailPos = i + NEW_PACKET_HEADER_SIZE + NEW_PACKET_ADC_SIZE;
                if (searchBuf[tailPos] == TAIL_MARKER_H &&
                    searchBuf[tailPos + 1] == TAIL_MARKER_L) {
                    return i;
                }
            }
        }

        return SIZE_MAX;
    }

    std::vector<std::vector<uint8_t>>
        Processor::extractPacketsLocked(size_t maxPackets)
    {
        std::vector<std::vector<uint8_t>> packets;
        
        size_t availableBytes = buffer->size();
        if (availableBytes < fixedPacketLength) {
            return packets;  // Not enough data
        }

        // Limit search buffer size to avoid excessive memory use
        size_t searchSize = std::min(availableBytes, searchBuffer.size());
        
        // Peek data from buffer without removing
        size_t peeked = buffer->peek(searchBuffer.data(), searchSize);
        if (peeked < fixedPacketLength) {
            return packets;
        }

        // Find first valid packet
        size_t packetStart = findPacketStart(searchBuffer.data(), peeked);
        
        if (packetStart == SIZE_MAX) {
            // No valid packet found
            // Skip invalid data (keep last packetLength-1 bytes for potential incomplete packet)
            if (peeked > fixedPacketLength) {
                size_t toSkip = peeked - (fixedPacketLength - 1);
                buffer->skip(toSkip);
            }
            return packets;
        }

        // Skip invalid data before first packet
        if (packetStart > 0) {
            buffer->skip(packetStart);
            peeked -= packetStart;
        }

        // Calculate how many complete packets are available
        size_t availablePackets = peeked / fixedPacketLength;
        size_t packetsToExtract = std::min(availablePackets, maxPackets);

        // Pre-allocate storage
        packets.reserve(packetsToExtract);

        // Extract packets
        std::vector<uint8_t> tempPacket(fixedPacketLength);
        
        for (size_t i = 0; i < packetsToExtract; ++i) {
            // Read packet from buffer
            size_t read = buffer->read(tempPacket.data(), fixedPacketLength);
            
            if (read == fixedPacketLength) {
                // Validate packet before adding
                if (validatePacket(tempPacket.data())) {
                    packets.push_back(tempPacket);
                    totalPacketsExtracted++;
                }
                else {
                    // Invalid packet in middle of stream, stop extraction
                    break;
                }
            }
            else {
                break;
            }
        }

        return packets;
    }

    Processor::BufferStats Processor::getStatistics() const {
        std::lock_guard<std::mutex> lock(mtx);
        
        BufferStats stats;
        
        if (buffer) {
            auto bufferStats = buffer->getStatistics();
            stats.bufferSize = bufferStats.currentSize;
            stats.bufferCapacity = bufferStats.capacity;
            stats.fillPercentage = bufferStats.fillPercentage;
            stats.bytesDropped = bufferStats.droppedBytes;
            stats.bufferWarning = bufferStats.warningState;
            stats.mode = bufferStats.mode;
        }
        
        stats.totalBytesReceived = totalBytesReceived;
        stats.packetsExtracted = totalPacketsExtracted;
        
        return stats;
    }

    void Processor::clear() {
        std::lock_guard<std::mutex> lock(mtx);
        if (buffer) {
            buffer->clear();
        }
    }

} // namespace protocol
