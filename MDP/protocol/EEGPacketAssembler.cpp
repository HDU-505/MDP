#include "Processor.h"
#include <algorithm>
#include <chrono>
#include "Constants.h"

namespace protocol {

    Processor::Processor(size_t packetLen, int timeoutMs)
        : fixedPacketLength(packetLen),
        timeoutMs(timeoutMs),
        readPos(0),
        lastAppendTime(std::chrono::steady_clock::now()) {
    }

    void Processor::appendData(const uint8_t* data, size_t len) {
        {
            std::lock_guard<std::mutex> lock(mtx);

            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - lastAppendTime).count();

            // 如果超过超时时间，认为前一帧数据失效，重置缓冲区
            if (elapsed > timeoutMs) {
                buffer.clear();
                readPos = 0;
            }

            buffer.insert(buffer.end(), data, data + len);
            lastAppendTime = now;
        }
        cv.notify_one();
    }

    std::vector<std::vector<uint8_t>>
        Processor::waitAndExtractPackets(size_t maxPackets) {
        std::unique_lock<std::mutex> lock(mtx);

        cv.wait_for(lock, std::chrono::milliseconds(timeoutMs), [this] {
            return 0;
            });

        return extractPacketsLocked(maxPackets);
    }

    std::vector<std::vector<uint8_t>>
        Processor::extractPackets(size_t maxPackets) {
        std::lock_guard<std::mutex> lock(mtx);
        return extractPacketsLocked(maxPackets);
    }

    std::vector<std::vector<uint8_t>>
        Processor::extractPacketsLocked(size_t maxPackets)
    {
        std::vector<std::vector<uint8_t>> packets;
        packets.reserve(maxPackets);

        // 查找第一个有效的数据包起始位置
        size_t idx = findPacketStart(readPos);
        if (idx == SIZE_MAX) {
            return packets;  // 未找到包头
        }

        // 新协议：使用fixedPacketLength（32字节）
        size_t samplePacketLen = fixedPacketLength;

        // 检查是否有足够的数据
        if (idx + samplePacketLen > buffer.size()) {
            return packets;  // 数据不足一个完整包
        }

        // 计算未处理缓冲区的长度
        size_t noProcessedBufferLen = buffer.size() - idx;
        size_t noProcessedSampleLen = noProcessedBufferLen / samplePacketLen;
        size_t maxSampleCount = std::min(noProcessedSampleLen, maxPackets);

        // 验证并提取数据包
        for (size_t i = 0; i < maxSampleCount; i++) {
            size_t packetStart = idx + i * samplePacketLen;

            // 验证包的完整性（头标记和尾标记）
            if (packetStart + samplePacketLen <= buffer.size()) {
                // 验证头标记：0x02 0x10
                if (buffer[packetStart] == HEAD_MARKER_H &&
                    buffer[packetStart + 1] == HEAD_MARKER_L) {

                    // 验证尾标记：0xAE 0x12（在第30-31字节位置）
                    size_t tailPos = packetStart + NEW_PACKET_HEADER_SIZE + NEW_PACKET_ADC_SIZE;
                    if (buffer[tailPos] == TAIL_MARKER_H &&
                        buffer[tailPos + 1] == TAIL_MARKER_L) {

                        // 包验证通过，复制数据
                        std::vector<uint8_t> pkt(
                            buffer.begin() + packetStart,
                            buffer.begin() + packetStart + samplePacketLen
                        );
                        packets.push_back(std::move(pkt));
                    }
                    else {
                        // 尾标记不匹配，停止提取
                        break;
                    }
                }
                else {
                    // 头标记不匹配，停止提取
                    break;
                }
            }
        }

        // 清理已处理的数据
        if (!packets.empty()) {
            size_t processedBytes = idx + packets.size() * samplePacketLen;
            buffer.erase(buffer.begin(), buffer.begin() + processedBytes);
            readPos = 0;  // 重置读取位置
        }

        compactIfNeeded();
        return packets;
    }

    size_t Processor::findPacketStart(size_t from) const
    {
        if (buffer.size() < NEW_PACKET_TOTAL_SIZE || from >= buffer.size()) {
            return SIZE_MAX;
        }

        // 新协议：查找头标记 0x02 0x10
        for (size_t i = from; i <= buffer.size() - NEW_PACKET_TOTAL_SIZE; ++i) {
            // 检查头标记
            if (buffer[i] == HEAD_MARKER_H && buffer[i + 1] == HEAD_MARKER_L) {
                // 验证尾标记位置
                size_t tailPos = i + NEW_PACKET_HEADER_SIZE + NEW_PACKET_ADC_SIZE;
                if (tailPos + 1 < buffer.size()) {
                    // 检查尾标记
                    if (buffer[tailPos] == TAIL_MARKER_H &&
                        buffer[tailPos + 1] == TAIL_MARKER_L) {
                        return i;  // 找到有效的数据包起始位置
                    }
                }
            }
        }

        return SIZE_MAX;  // 未找到有效的包头
    }

    void Processor::compactIfNeeded() {
        // 当已读取位置超过阈值且占用了缓冲区一半以上空间时，进行压缩
        if (readPos > COMPACT_THRESHOLD && readPos >= buffer.size() / 2) {
            buffer.erase(buffer.begin(), buffer.begin() + readPos);
            readPos = 0;
        }
    }

    void Processor::clear() {
        std::lock_guard<std::mutex> lock(mtx);
        buffer.clear();
        readPos = 0;
    }

} // namespace protocol