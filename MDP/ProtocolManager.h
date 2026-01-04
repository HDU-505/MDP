#pragma once
#include "protocol/Processor.h"
#include "protocol/Constants.h"
#include "protocol//Parser.h"
#include "Amplifier_LIB.h"

namespace protocol {
	class ProtocolManager {
	private:
		Processor* processor;
		Parser* parser;
	public:
		ProtocolManager(RecordingMode recordingMode);

		void processData(const uint8_t* data, size_t len);

	};
}