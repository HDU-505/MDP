#include "Parser.h"
#include "../ErrorHandler.h"
#include <mutex>
#include <stdexcept>
#include <cstring>
#include <string>

using namespace std;
using namespace sdk;

namespace protocol {

    // ================= Command Building =================

    std::vector<uint8_t> Parser::buildControlPacket(PacketType packetType) {
        std::vector<uint8_t> data(CMD_TOTAL_LENGTH);

        // Command format: AE 12 02 XX
        data[0] = CMD_HEADER_0;
        data[1] = CMD_HEADER_1;
        data[2] = CMD_HEADER_2;
        data[3] = packetType;

        return data;
    }

    // ================= Data Packet Validation =================

    bool Parser::validateNewPacket(const uint8_t* data, size_t len) {
        if (!data || len < NEW_PACKET_TOTAL_SIZE) {
            return false;
        }

        if (data[IDX_HEAD_MARKER_H] != HEAD_MARKER_H) {
            return false;
        }
        
        if (data[IDX_HEAD_MARKER_L] != 0x10 && data[IDX_HEAD_MARKER_L] != 0x11) {
            return false;
        }

        size_t tailPos = NEW_PACKET_HEADER_SIZE + NEW_PACKET_ADC_SIZE;
        if (data[tailPos] != TAIL_MARKER_H ||
            data[tailPos + 1] != TAIL_MARKER_L) {
            return false;
        }

        return true;
    }

    uint32_t Parser::getTimestampFromNewPacket(const uint8_t* data, size_t len) {
        if (!data || len < NEW_PACKET_HEADER_SIZE) return 0;
        return (static_cast<uint32_t>(data[IDX_TIMESTAMP_0]) << 24) |
               (static_cast<uint32_t>(data[IDX_TIMESTAMP_1]) << 16) |
               (static_cast<uint32_t>(data[IDX_TIMESTAMP_2]) <<  8) |
                static_cast<uint32_t>(data[IDX_TIMESTAMP_3]);
    }

    uint32_t Parser::getSampleSeqFromNewPacket(const uint8_t* data, size_t len) {
        if (!data || len < NEW_PACKET_HEADER_SIZE) return 0;
        return (static_cast<uint32_t>(data[IDX_SAMPLE_SEQ_0]) << 24) |
               (static_cast<uint32_t>(data[IDX_SAMPLE_SEQ_1]) << 16) |
               (static_cast<uint32_t>(data[IDX_SAMPLE_SEQ_2]) <<  8) |
                static_cast<uint32_t>(data[IDX_SAMPLE_SEQ_3]);
    }

    // ================= Data Conversion =================

    static int32_t convertRawToSignedInt(uint32_t raw) {
        if (raw > 0x7FFFFF) {
            return static_cast<int32_t>(raw - 0x1000000);
        }
        return static_cast<int32_t>(raw);
    }

    static float convertToVoltageUV(int32_t signedVal) {
        return static_cast<float>(signedVal) * ADS1299_LSB_UV;
    }

    // ================= Data Packet Parsing (含丢包检测与插值) =================

    bool Parser::parseNewEEGPacket2Byte(
        const uint8_t* data,
        size_t len,
        std::vector<uint8_t>& outBytes
    )
    {
        if (!validateNewPacket(data, len)) {
            return false;
        }

        // ── Step 1: 解析序号 ──
        uint32_t sampleSeq = getSampleSeqFromNewPacket(data, len);

        // ── Step 2: 解析 8 通道 ADC → float uV ──
        float currentData[8];
        size_t adcStart = IDX_ADC_DATA_START;

        for (int ch = 0; ch < EEG_CHANNEL_COUNT; ++ch) {
            size_t offset = adcStart + ch * EEG_CHANNEL_BYTES;
            uint32_t raw =
                (static_cast<uint32_t>(data[offset])     << 16) |
                (static_cast<uint32_t>(data[offset + 1]) <<  8) |
                 static_cast<uint32_t>(data[offset + 2]);
            currentData[ch] = convertToVoltageUV(convertRawToSignedInt(raw));
        }

        // ── 辅助: 将序号写入输出 (uint32→uint64 小端) ──
        auto writeSeq = [&](uint32_t seq) {
            uint64_t seq64 = static_cast<uint64_t>(seq);
            for (int i = 0; i < 8; i++) {
                outBytes.push_back(static_cast<uint8_t>((seq64 >> (i * 8)) & 0xFF));
            }
        };

        // ── 辅助: 将 float 写入输出 ──
        auto writeFloat = [&](float v) {
            const uint8_t* p = reinterpret_cast<const uint8_t*>(&v);
            outBytes.insert(outBytes.end(), p, p + sizeof(float));
        };

        // ── Step 3: 丢包检测与插值 ──
        if (hasLastSample) {
            bool rollover = (lastSampleSeq > 0xFFFF0000u && sampleSeq < 0x00010000u);

            if (!rollover && sampleSeq > lastSampleSeq + 1) {
                uint32_t gap = sampleSeq - lastSampleSeq - 1;
                totalLostPackets += gap;

                if (gap <= MAX_INTERPOLATION_GAP) {
                    // 线性插值填充
                    for (uint32_t g = 1; g <= gap; ++g) {
                        float alpha = static_cast<float>(g) /
                                      static_cast<float>(gap + 1);
                        writeSeq(lastSampleSeq + g);
                        for (int ch = 0; ch < EEG_CHANNEL_COUNT; ++ch) {
                            float v = lastChannelData[ch] * (1.0f - alpha)
                                    + currentData[ch]     * alpha;
                            writeFloat(v);
                        }
                    }
                    totalInterpolatedSamples += gap;

#ifdef _DEBUG
                    Logger::Log(LogLevel::DEBUG, ErrorCategory::PROTOCOL,
                        "[PktLoss] Interpolated " + to_string(gap) +
                        " frames seq=[" + to_string(lastSampleSeq + 1) +
                        "-" + to_string(sampleSeq - 1) + "]");
#endif
                }
                else {
                    // gap 过大：零值帧占位
                    for (uint32_t g = 1; g <= gap; ++g) {
                        writeSeq(lastSampleSeq + g);
                        for (int ch = 0; ch < EEG_CHANNEL_COUNT; ++ch) {
                            writeFloat(0.0f);
                        }
                    }

                    // 大 gap 在 Release 也报告（异常事件，不频繁）
                    Logger::Log(LogLevel::WARNING, ErrorCategory::PROTOCOL,
                        "[PktLoss] Large gap=" + to_string(gap) +
                        " zero-filled at seq=" + to_string(sampleSeq));
                }
            }

#ifdef _DEBUG
            // Debug: 重复包 / 乱序包检测
            if (!rollover && sampleSeq == lastSampleSeq) {
                Logger::Log(LogLevel::DEBUG, ErrorCategory::PROTOCOL,
                    "[PktLoss] Duplicate seq=" + to_string(sampleSeq));
            }
            else if (!rollover && sampleSeq < lastSampleSeq) {
                Logger::Log(LogLevel::DEBUG, ErrorCategory::PROTOCOL,
                    "[PktLoss] Out-of-order last=" + to_string(lastSampleSeq) +
                    " curr=" + to_string(sampleSeq));
            }
#endif
        }

        // ── Step 4: 写入当前帧 ──
        writeSeq(sampleSeq);
        for (int ch = 0; ch < EEG_CHANNEL_COUNT; ++ch) {
            writeFloat(currentData[ch]);
        }

        // ── Step 5: 保存状态 ──
        memcpy(lastChannelData, currentData, sizeof(currentData));
        lastSampleSeq = sampleSeq;
        hasLastSample = true;
        sequenceID    = sampleSeq;

        return true;
    }

    bool Parser::parseNewEEGPacket2Float(
        const uint8_t* data,
        size_t len,
        std::vector<float>& outData
    )
    {
        if (!validateNewPacket(data, len)) {
            return false;
        }

        size_t adcStart = IDX_ADC_DATA_START;

        for (int ch = 0; ch < EEG_CHANNEL_COUNT; ++ch) {
            size_t offset = adcStart + ch * EEG_CHANNEL_BYTES;
            uint32_t raw =
                (static_cast<uint32_t>(data[offset])     << 16) |
                (static_cast<uint32_t>(data[offset + 1]) <<  8) |
                 static_cast<uint32_t>(data[offset + 2]);
            int32_t signedVal = convertRawToSignedInt(raw);
            float value = convertToVoltageUV(signedVal);
            outData.push_back(value);
        }

        return true;
    }

} // namespace protocol
