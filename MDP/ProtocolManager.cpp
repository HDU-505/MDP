#include "ProtocolManager.h"

protocol::ProtocolManager::ProtocolManager(RecordingMode recordingMode)
{
	//³õÊ¼»¯ProcessorºÍParser
	// recordingMode Ô¤Áô
	processor = new Processor(30,500);
	parser = new Parser();
}

void protocol::ProtocolManager::processData(const uint8_t* data, size_t len)
{
	processor->appendData(data,len);
}


