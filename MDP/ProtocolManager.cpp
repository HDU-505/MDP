#include "ProtocolManager.h"


using namespace std;

protocol::ProtocolManager::ProtocolManager(RecordingMode recordingMode)
{
	//初始化Processor和Parser
	// recordingMode 预留
	processor = new Processor(30,500);
	parser = new Parser();
    // 7.8Hz、31.2Hz
    impedanceUtil = new ImpedanceUtil(125.0f,31.2f,125,50);
}

void protocol::ProtocolManager::processData(const uint8_t* data, size_t len)
{
	processor->appendData(data,len);
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
        parser->parseEEGPacket2Byte(
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
    // 先默认提取1s的数据
    sampleLen = 125;
    std::vector<std::vector<uint8_t>> rawPackets = processor->waitAndExtractPackets(sampleLen);
    std::vector<std::vector<float>> eegData(8); // TODO 需要动态确定
    for (auto& channelRow : eegData) {
        channelRow.reserve(rawPackets.size());
    }

    for (const std::vector<uint8_t>& pkt : rawPackets) {
        std::vector<float> oneSample; // 临时存放单次采样（当前时间点的所有通道）

        if (parser->parseEEGPacket2Float(pkt.data(), pkt.size(), oneSample)) {
            // oneSample[0] 是 Counter
            // oneSample[1...N] 是各通道数据

            for (size_t i = 0; i < oneSample.size(); ++i) {
                // 将当前采样点的第 i 个分量，放入 eegData 的第 i 行中
                eegData[i].push_back(oneSample[i]);
            }
        }
    }

    // 解析阻抗
    for (const std::vector<float>& channelRow : eegData) {
        std::vector<float> impedance = impedanceUtil->ImpedanceCalculation(channelRow);
        result.push_back(impedance[impedance.size() - 1]);
        result.push_back(-1.0f);
    }

    return result;
}



int protocol::ProtocolManager::getSampleLength()
{
	// TODO 需要根据协议动态生成
	return 40;
}




