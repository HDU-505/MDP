// EEGPacketAssembler.h
#pragma once
#include <vector>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <algorithm>
#include <cstdint>
#include <iostream>

namespace Protocol {

    class Processor {
    private:
        std::vector<uint8_t> buffer;
        size_t fixedPacketLength;
        int timeoutMs;
        std::chrono::steady_clock::time_point lastAppendTime;

        std::mutex mtx;
        std::condition_variable cv;


    private:
        // 查找包头索引
        size_t findPacketStart();

    public:
        Processor(size_t packetLen, int timeoutMs = 500);

        // 添加数据（线程安全）
        void appendData(const unsigned char* data, size_t len);

        // 检查是否有完整包（线程安全）
        bool hasCompletePacket();

        // 提取完整包（线程安全）
        std::vector<uint8_t> extractPacket();

        // 超时清理（线程安全）
        void checkTimeout();

        // 清空缓冲区（线程安全）
        void clearBuffer();

        // 可选：阻塞等待完整包
        std::vector<uint8_t> waitAndExtractPacket();


    };

}