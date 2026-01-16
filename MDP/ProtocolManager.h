#pragma once
#include "protocol/Processor.h"
#include "protocol/Constants.h"
#include "protocol/Parser.h"
#include "Amplifier_LIB.h"
#include "bt/BleHandle.h"
#include "bt/BLEComm.h"
#include "ImpedanceUtil.h"

namespace protocol {
    /**
     * @brief 协议管理器
     *
     * 负责管理协议的解析和数据处理
     * 支持新协议格式：
     * - 头标记：0x02 0x10 (2字节)
     * - 时间戳：16位 (2字节)
     * - 采样序号：16位 (2字节)
     * - ADC数据：24位×8通道 (24字节)
     * - 尾标记：0xAE 0x12 (2字节)
     * - 总长度：32字节
     */
    class ProtocolManager {
    private:
        Processor* processor;
        Parser* parser;
        ImpedanceUtil* impedanceUtil;

    public:
        /**
         * @brief 构造协议管理器
         * @param recordingMode 录制模式
         */
        ProtocolManager(RecordingMode recordingMode);

        /**
         * @brief 处理接收到的原始数据
         * @param data 数据指针
         * @param len 数据长度
         */
        void processData(const uint8_t* data, size_t len);

        /**
         * @brief 构建控制指令包
         * @param comandType 命令类型
         * @param packType 包类型
         * @param streamMask 流掩码
         * @return 指令包数据
         */
        vector<uint8_t> buildPacket(ComandType comandType, PacketType packType, StreamMask streamMask);

        /**
         * @brief 获取EEG数据
         * @param sampleLen 采样长度（包数量）
         * @return EEG数据字节流（序号+浮点数据）
         */
        std::vector<uint8_t> getEEGData(int sampleLen);

        /**
         * @brief 获取阻抗数据
         * @param sampleLen 采样长度（包数量）
         * @return 阻抗数据
         */
        std::vector<float> getImpedanceData(int sampleLen);

        /**
         * @brief 获取单个数据包长度
         * @return 新协议数据包长度（32字节）
         */
        int getSampleLength();
    };
}