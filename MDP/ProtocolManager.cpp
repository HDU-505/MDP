#include "ProtocolManager.h"

using namespace std;

protocol::ProtocolManager::ProtocolManager(RecordingMode recordingMode)
{
    // 初始化Processor和Parser
    // 新协议数据包长度为36字节
    processor = new Processor(36, 500);  // 修改为36字节
    parser = new Parser();
    // 7.8Hz、31.2Hz
    impedanceUtil = new ImpedanceUtil(125.0f, 7.8f, 125, 50);
}

void protocol::ProtocolManager::processData(const uint8_t* data, size_t len)
{
    processor->appendData(data, len);
}

vector<uint8_t> protocol::ProtocolManager::buildPacket(ComandType comandType, PacketType packType, StreamMask streamMask)
{
    return parser->buildControlPacket(packType);
}

std::vector<uint8_t>
protocol::ProtocolManager::getEEGData(int sampleLen)
{
    std::vector<uint8_t> result;

    auto rawPackets = processor->waitAndExtractPackets(sampleLen);

    for (const auto& pkt : rawPackets) {
        // 使用新协议解析接口
        parser->parseNewEEGPacket2Byte(
            pkt.data(),
            pkt.size(),
            result
        );
    }

    return result;
}

std::vector<float> protocol::ProtocolManager::getImpedanceData(int sampleLen)
{
    std::vector<float> result;

    result.push_back(0);
    result.push_back(0);    // 参考电极

    // 默认提取1s的数据
    sampleLen = 125;
    std::vector<std::vector<uint8_t>> rawPackets = processor->waitAndExtractPackets(sampleLen);
    if (rawPackets.size() == 0) {
        return result;
    }
    std::vector<std::vector<float>> eegData(8);  // 8通道

    for (auto& channelRow : eegData) {
        channelRow.reserve(rawPackets.size());
    }

    for (const std::vector<uint8_t>& pkt : rawPackets) {
        std::vector<float> oneSample;  // 临时存放单次采样

        // 使用新协议解析接口
        if (parser->parseNewEEGPacket2Float(pkt.data(), pkt.size(), oneSample)) {
            // oneSample包含8个通道的数据
            for (size_t i = 0; i < oneSample.size(); ++i) {
                // 将当前采样点的第i个分量，放入eegData的第i行中
                eegData[i].push_back(oneSample[i]);
            }
        }
    }

    // 解析阻抗
    for (const std::vector<float>& channelRow : eegData) {
        std::vector<float> impedance = impedanceUtil->ImpedanceCalculation(channelRow);
        if (impedance.size() != 0) {
            result.push_back(impedance[impedance.size() - 1]);
        }
        else {
            result.push_back(1735);
        }
        result.push_back(-1.0f);
    }

    return result;
}

int protocol::ProtocolManager::getSampleLength()
{
    // 新协议数据包长度为32字节
    return 40;
}