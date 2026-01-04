#pragma once
#include "protocol/Processor.h"
#include "protocol/Constants.h"
#include "protocol//Parser.h"
#include "Amplifier_LIB.h"
#include "bt/BleHandle.h"
#include "bt/BLEComm.h"

namespace protocol {
	class ProtocolManager {
	private:
		Processor* processor;
		Parser* parser;
	public:
		ProtocolManager(RecordingMode recordingMode);

		void processData(const uint8_t* data, size_t len);

		vector<uint8_t> buildPacket(ComandType comandType,PacketType packType, StreamMask streamMask);

		std::vector<std::vector<uint8_t>> getEEGData(int sampleLen);

		int getSampleLength();

	};
}