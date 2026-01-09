#pragma once

#include <cstdint>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <chrono>

namespace protocol {

    /**
     * @brief 字节流协议解析处理器
     *
     * 设计职责：
     *  - 接收并缓存来自底层链路的原始字节流
     *  - 按协议格式在字节流中定位并解析数据包
     *  - 支持从连续、不完整的数据流中提取完整帧
     *
     * 设计边界：
     *  - 仅负责协议层的数据拼包与拆包
     *  - 不涉及具体业务语义或数据处理逻辑
     */
    class Processor {
    public:
        /**
         * @brief 构造协议解析器
         *
         * @param packetLen  单个协议数据包的固定总长度（字节）
         * @param timeoutMs  数据等待超时时间（毫秒），用于阻塞等待场景
         */
        Processor(size_t packetLen, int timeoutMs);

        /**
         * @brief 追加原始字节数据到内部缓冲区（线程安全）
         *
         * @param data  原始字节数据指针
         * @param len   数据长度（字节）
         */
        void appendData(const uint8_t* data, size_t len);

        /**
         * @brief 阻塞等待并提取完整的数据包
         *
         * 若在超时时间内缓冲区中可解析出完整数据包，则返回；
         * 否则在超时后返回当前可提取的数据。
         *
         * @param maxPackets  本次最多提取的数据包数量
         * @return            提取到的完整数据包列表
         */
        std::vector<std::vector<uint8_t>>
            waitAndExtractPackets(size_t maxPackets);

        /**
         * @brief 非阻塞方式提取已缓存的完整数据包
         *
         * @param maxPackets  本次最多提取的数据包数量
         * @return            提取到的完整数据包列表
         */
        std::vector<std::vector<uint8_t>>
            extractPackets(size_t maxPackets);

        /**
         * @brief 清空内部缓冲区与解析状态
         */
        void clear();

    private:
        // ===== 内部解析接口（仅在已加锁状态下调用）=====

        /**
         * @brief 在互斥锁保护下执行数据包提取逻辑
         *
         * @param maxPackets  本次最多提取的数据包数量
         * @return            提取到的完整数据包列表
         */
        std::vector<std::vector<uint8_t>>
            extractPacketsLocked(size_t maxPackets);

        /**
         * @brief 从指定位置开始查找下一个合法的数据包头
         *
         * @param from  起始搜索位置
         * @return      包头起始索引；若未找到则返回 SIZE_MAX
         */
        size_t findPacketStart(size_t from) const;

        /**
         * @brief 根据已消费的数据位置对缓冲区进行压缩
         *
         * 当缓冲区前部数据已被解析完成时，
         * 将剩余未解析数据前移以避免缓冲区无限增长。
         */
        void compactIfNeeded();

    private:
        // ===== 配置参数 =====
        size_t fixedPacketLength;  // 协议数据包固定长度
        int    timeoutMs;          // 阻塞等待超时时间（ms）

        // ===== 数据缓冲区 =====
        std::vector<uint8_t> buffer;   // 原始字节缓存
        size_t               readPos{ 0 }; // 当前解析读取位置

        // ===== 同步原语 =====
        std::mutex              mtx; // 互斥锁，保护缓冲区与状态
        std::condition_variable cv;  // 条件变量，用于阻塞等待数据

        // ===== 时间状态 =====
        std::chrono::steady_clock::time_point lastAppendTime; // 最近一次数据追加时间
    };

} // namespace protocol