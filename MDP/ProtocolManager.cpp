#include "ProtocolManager.h"


using namespace std;

protocol::ProtocolManager::ProtocolManager(RecordingMode recordingMode)
{
	//初始化Processor和Parser
	// recordingMode 预留
	processor = new Processor(30,500);
	parser = new Parser();
}

void protocol::ProtocolManager::processData(const uint8_t* data, size_t len)
{
	processor->appendData(data,len);
}

vector<uint8_t> protocol::ProtocolManager::buildPacket(ComandType comandType, PacketType packType, StreamMask streamMask)
{
	return parser->buildControlPacket(packType);
}

std::vector<std::vector<uint8_t>> protocol::ProtocolManager::getEEGData(int sampleLen)
{
	return processor->waitAndExtractPackets(sampleLen);
}

int protocol::ProtocolManager::getSampleLength()
{
	// TODO 需要根据协议动态生成
	return 40;
}




