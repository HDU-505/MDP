// EEGPacketAssembler.cpp
#include "pch.h"
#include "EEGPacketAssembler.h"


EEGPacketAssembler::EEGPacketAssembler(size_t packetLen, int timeoutMs)
    : fixedPacketLength(packetLen), timeoutMs(timeoutMs) {
    lastAppendTime = std::chrono::steady_clock::now();
}

void EEGPacketAssembler::appendData(const unsigned char* data, size_t len) {
    
    std::lock_guard<std::mutex> lock(mtx);
    buffer.insert(buffer.end(), data, data + len);
    lastAppendTime = std::chrono::steady_clock::now();
    cv.notify_one();  // 通知可能有完整包
    // 控制 buffer 大小
    if (buffer.size() > 4096) {
        buffer.erase(buffer.begin(), buffer.begin() + buffer.size() - 1024);
    }
}

bool EEGPacketAssembler::hasCompletePacket() {
    std::lock_guard<std::mutex> lock(mtx);
    size_t idx = findPacketStart();
    return (idx != SIZE_MAX && idx + fixedPacketLength <= buffer.size());
}

std::vector<uint8_t> EEGPacketAssembler::extractPacket() {
    std::lock_guard<std::mutex> lock(mtx);
    size_t idx = findPacketStart();
    if (idx != SIZE_MAX && idx + fixedPacketLength <= buffer.size()) {
        std::vector<uint8_t> packet(buffer.begin() + idx, buffer.begin() + idx + fixedPacketLength);
        buffer.erase(buffer.begin(), buffer.begin() + idx + fixedPacketLength);
        return packet;
    }
    return {};
}

std::vector<uint8_t> EEGPacketAssembler::waitAndExtractPacket() {
    std::unique_lock<std::mutex> lock(mtx);
    cv.wait(lock, [this] {
        size_t idx = findPacketStart();
        return (idx != SIZE_MAX && idx + fixedPacketLength <= buffer.size());
        });
    size_t idx = findPacketStart();
    std::vector<uint8_t> packet(buffer.begin() + idx, buffer.begin() + idx + fixedPacketLength);
    buffer.erase(buffer.begin(), buffer.begin() + idx + fixedPacketLength);
    return packet;
}

void EEGPacketAssembler::checkTimeout() {
    std::lock_guard<std::mutex> lock(mtx);
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastAppendTime).count();
    if (elapsed > timeoutMs) {
        buffer.clear();
    }
}

void EEGPacketAssembler::clearBuffer() {
    std::lock_guard<std::mutex> lock(mtx);
    buffer.clear();
}

// 查找包头索引
size_t EEGPacketAssembler::findPacketStart() {
    for (size_t i = 0; i + 4 <= buffer.size(); ++i) {
        if (buffer[i] == 0xAE && buffer[i + 1] == 0x12) {
            return i;
        }
    }
    // 没找到包头，丢掉无效数据
    auto it = std::search(buffer.begin(), buffer.end(), std::begin("\xAE\x12"), std::end("\xAE\x12") - 1);
    if (it != buffer.begin()) {
        buffer.erase(buffer.begin(), it);
    }
    return SIZE_MAX;
}
