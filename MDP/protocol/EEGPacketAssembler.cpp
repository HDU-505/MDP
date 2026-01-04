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

        bool ready = cv.wait_for(
            lock,
            std::chrono::milliseconds(timeoutMs),
            [this] {
                size_t idx = findPacketStart(readPos);

                // 至少需要 8 字节才能解析到 payload length
                if (idx == SIZE_MAX || idx + 8 > buffer.size()) {
                    return false;
                }

                uint8_t lenH = buffer[idx + IDX_PAYLOAD_LEN_H];
                uint8_t lenL = buffer[idx + IDX_PAYLOAD_LEN_L];
                uint16_t payloadLen =
                    (static_cast<uint16_t>(lenH) << 8) | lenL;

                // 完整包长度 = 固定头 8 字节 + payload
                return (idx + 8 + payloadLen) <= buffer.size();
            });

        if (!ready) {
            return {};
        }

        return extractPacketsLocked(maxPackets);
    }

    std::vector<std::vector<uint8_t>>
    Processor::extractPackets(size_t maxPackets) {
        std::lock_guard<std::mutex> lock(mtx);
        return extractPacketsLocked(maxPackets);
    }

    std::vector<std::vector<uint8_t>>
        Processor::extractPacketsLocked(size_t maxPackets) {
        std::vector<std::vector<uint8_t>> packets;
        packets.reserve(maxPackets);

        size_t count = 0;

        while (count < maxPackets) {
            size_t idx = findPacketStart(readPos);

            if (idx == SIZE_MAX || idx + 8 > buffer.size()) {
                break;
            }

            uint8_t lenH = buffer[idx + IDX_PAYLOAD_LEN_H];
            uint8_t lenL = buffer[idx + IDX_PAYLOAD_LEN_L];
            uint16_t payloadLen =
                (static_cast<uint16_t>(lenH) << 8) | lenL;

            size_t totalPacketLen = 8 + payloadLen;

            if (idx + totalPacketLen > buffer.size()) {
                break;
            }

            // 拷贝原始 packet
            std::vector<uint8_t> pkt(buffer.begin() + idx,
                buffer.begin() + idx + totalPacketLen);

            // 在 payload 前插入计数编码（8 字节 little-endian）
            uint64_t counter = packetCounter++;
            uint8_t counterBytes[8];
            for (int i = 0; i < 8; ++i) {
                counterBytes[i] = static_cast<uint8_t>((counter >> (8 * i)) & 0xFF);
            }

            // 将计数编码插入到 packet 开头或者特定位置
            pkt.insert(pkt.begin() + 8, counterBytes, counterBytes + 8);
            // 注意：插入后 totalPacketLen 需要上层处理适配

            packets.emplace_back(std::move(pkt));

            readPos = idx + totalPacketLen;
            ++count;
        }

        compactIfNeeded();
        return packets;
    }


    size_t Processor::findPacketStart(size_t from) const {
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
