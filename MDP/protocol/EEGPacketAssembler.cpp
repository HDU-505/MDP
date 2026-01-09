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

            // 若超过超时时间，认为前一帧数据失效，重置缓冲区
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

        size_t idx = findPacketStart(readPos);
        if (idx == SIZE_MAX || idx + HEADER_LENGTH > buffer.size()) {
            return packets;
        }
        // TODO 需要动态根据协议调整解析包长度
        size_t samplePacketLen = 30;
        if (idx + samplePacketLen > buffer.size()) {
            return packets;
        }
        size_t noProcessedBufferLen = std::distance(buffer.begin() + idx, buffer.end());
        size_t noProcessedSampleLen = noProcessedBufferLen / samplePacketLen;
        size_t maxSampleCount = std::min(noProcessedSampleLen, maxPackets);
        size_t maxTotalPacketLen = maxSampleCount * samplePacketLen;

        // 拷贝原始 packet 
        for (size_t i = 0; i < maxSampleCount; i++) {
            std::vector<uint8_t> pkt(buffer.begin() + idx, buffer.begin() + idx + samplePacketLen);
            packets.push_back(std::move(pkt));
        }
        buffer.erase(buffer.begin(), buffer.begin() + idx + maxTotalPacketLen);

        compactIfNeeded();
        return packets;
    }


    size_t Processor::findPacketStart(size_t from) const
    {
        if (buffer.size() < 2 || from >= buffer.size()) {
            return SIZE_MAX;
        }

        auto it = std::search(
            buffer.begin() + from,
            buffer.end(),
            std::begin(PACKET_HEADER),
            std::end(PACKET_HEADER)
        );

        if (it == buffer.end()) {
            return SIZE_MAX;
        }

        return static_cast<size_t>(std::distance(buffer.begin(), it));
    }

    void Processor::compactIfNeeded() {
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
