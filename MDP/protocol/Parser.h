#pragma once

#include "Constants.h"
#include <vector>
#include <cstdint>
#include <algorithm>
#include <string>

namespace protocol {

    /**
     * @brief Protocol Parser Class
     *
     * Responsible for building control command packets and parsing data packets.
     * New protocol format:
     * - Commands: AE 12 02 XX (4 bytes)
     * - Data packets: 02 10/11 (header 2) + timestamp(4) + sequence(4) + ADC(24) + AE 12 (tail 2) = 36 bytes
     *
     * 丢包检测与插值：
     * - 通过硬件采样序号检测帧丢失
     * - 小 gap (<=50帧) 使用线性插值补全
     * - 大 gap (>50帧) 使用零值帧占位
     */
    class Parser {
    public:
        // 最大允许线性插值的帧数（@250Hz ≈ 0.2秒）
        static constexpr uint32_t MAX_INTERPOLATION_GAP = 50;

    private:
        uint32_t sequenceID = 0;

        // ── 丢包检测状态 ──
        bool     hasLastSample  = false;
        uint32_t lastSampleSeq  = 0;
        float    lastChannelData[8] = {};

        // ── 统计 ──
        uint64_t totalLostPackets        = 0;
        uint64_t totalInterpolatedSamples = 0;

    public:
        // ================= Control Command Building =================
        std::vector<uint8_t> buildControlPacket(PacketType packetType);

        // ================= Data Packet Parsing =================

        /**
         * @brief 解析 EEG 数据包 → 字节流（含丢包插值）
         *
         * 输出格式 (40 bytes/sample):
         * - 序号: 8 bytes (uint64_t, little-endian)
         * - 通道数据: 4 bytes float × 8 (uV)
         *
         * 如检测到帧丢失，会在输出中自动插入插值帧
         */
        bool parseNewEEGPacket2Byte(
            const uint8_t* data,
            size_t len,
            std::vector<uint8_t>& outBytes
        );

        /**
         * @brief 解析 EEG 数据包 → float 数组
         */
        bool parseNewEEGPacket2Float(
            const uint8_t* data,
            size_t len,
            std::vector<float>& outData
        );

        uint32_t getTimestampFromNewPacket(const uint8_t* data, size_t len);
        uint32_t getSampleSeqFromNewPacket(const uint8_t* data, size_t len);
        bool validateNewPacket(const uint8_t* data, size_t len);

        uint32_t getSequenceID() const { return sequenceID; }

        // ================= 丢包统计接口 =================
        uint64_t getLostPackets() const { return totalLostPackets; }
        uint64_t getInterpolatedSamples() const { return totalInterpolatedSamples; }

        void resetStats() {
            totalLostPackets = 0;
            totalInterpolatedSamples = 0;
        }

        /** 重置连接状态（断开重连时调用，避免序号跳跃误判为丢包） */
        void resetLossState() {
            hasLastSample = false;
            lastSampleSeq = 0;
            std::fill(std::begin(lastChannelData), std::end(lastChannelData), 0.0f);
        }
    };

} // namespace protocol
