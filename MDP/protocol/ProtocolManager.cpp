#include "ProtocolManager.h"
#include <algorithm>

using namespace std;
using namespace sdk;

namespace protocol {

    ProtocolManager::ProtocolManager(RecordingMode recordingMode)
        : currentMode(recordingMode)
    {
        try {
            // Initialize dual-mode buffer processor
            processor = std::make_unique<Processor>(NEW_PACKET_TOTAL_SIZE, 500);
            
            // Set initial buffer mode based on recording mode
            if (recordingMode == RM_IMPEDANCE) {
                processor->setBufferMode(DualModeBuffer::Mode::IMPEDANCE, false);
            } else {
                processor->setBufferMode(DualModeBuffer::Mode::NORMAL, false);
            }
            
            // Initialize parser
            parser = std::make_unique<Parser>();
            
            // Initialize real-time impedance calculator
            // Optimized for fast updates with sliding window
            realTimeImpedance = std::make_unique<RealTimeImpedanceCalculator>(
                8,      // 8 channels
                32,     // 32 samples window (4 complete cycles @ 31.25Hz, faster response)
                250.0f, // 250Hz sampling rate
                31.25f  // 31.25Hz target frequency
            );
            
            Logger::Info("ProtocolManager initialized with dual-mode buffer");
        }
        catch (const std::exception& e) {
            Logger::Fatal(ErrorCategory::GENERAL, 
                std::string("Failed to initialize ProtocolManager: ") + e.what());
            throw;
        }
    }

    void ProtocolManager::processData(const uint8_t* data, size_t len)
    {
        if (!data || len == 0) {
            Logger::Warning("Received null or empty data");
            return;
        }

        {
            std::lock_guard<std::mutex> lock(statsMutex);
            totalBytesReceived += len;
        }
        
        processor->appendData(data, len);
        
        // Log only at DEBUG level to avoid performance impact
        Logger::Log(LogLevel::DEBUG, ErrorCategory::PROTOCOL, 
            "Received " + std::to_string(len) + " bytes");
    }

    std::vector<uint8_t> ProtocolManager::buildPacket(
        ComandType commandType,
        PacketType packetType,
        StreamMask streamMask)
    {
        try {
            auto packet = parser->buildControlPacket(packetType);
            
            Logger::Log(LogLevel::DEBUG, ErrorCategory::PROTOCOL,
                "Built command packet: type=0x" + std::to_string(packetType));
            
            return packet;
        }
        catch (const std::exception& e) {
            Logger::Error(ErrorCategory::PROTOCOL, 
                std::string("Failed to build packet: ") + e.what());
            return std::vector<uint8_t>();
        }
    }

    std::vector<uint8_t> ProtocolManager::getEEGData(int sampleCount)
    {
        std::vector<uint8_t> result;
        
        if (sampleCount <= 0) {
            Logger::Warning("Invalid sample count: " + std::to_string(sampleCount));
            return result;
        }

        try {
            // Extract raw packets from ring buffer (efficient O(1) operations)
            auto rawPackets = processor->waitAndExtractPackets(sampleCount);
            
            if (rawPackets.empty()) {
                Logger::Log(LogLevel::DEBUG, ErrorCategory::PROTOCOL, "No packets available");
                return result;
            }

            {
                std::lock_guard<std::mutex> lock(statsMutex);
                totalPacketsReceived += rawPackets.size();
            }
            
            // Pre-allocate output buffer
            result.reserve(rawPackets.size() * SINGLE_SAMPLE_OUTPUT_SIZE);

            // Parse packets and update real-time impedance calculator
            size_t successCount = 0;
            
            for (const auto& pkt : rawPackets) {
                std::vector<float> voltageData;
                
                // Parse to get voltage values
                if (parser->parseNewEEGPacket2Float(pkt.data(), pkt.size(), voltageData)) {
                    // For EEG data output, we need byte format
                    if (!parser->parseNewEEGPacket2Byte(pkt.data(), pkt.size(), result)) {
                        std::lock_guard<std::mutex> lock(statsMutex);
                        totalPacketsDropped++;
                        Logger::Warning("Failed to convert packet to byte format");
                        continue;
                    }
                    
                    // Update real-time impedance calculator with latest data
                    if (currentMode == RM_IMPEDANCE && voltageData.size() == 8) {
                        realTimeImpedance->addSample(voltageData.data(), voltageData.size());
                    }
                    
                    successCount++;
                } else {
                    std::lock_guard<std::mutex> lock(statsMutex);
                    totalPacketsDropped++;
                    Logger::Warning("Failed to parse packet");
                }
            }

            Logger::Log(LogLevel::DEBUG, ErrorCategory::PROTOCOL,
                "Extracted " + std::to_string(successCount) + "/" + 
                std::to_string(rawPackets.size()) + " packets");
        }
        catch (const std::exception& e) {
            Logger::Error(ErrorCategory::PROTOCOL, 
                std::string("Error in getEEGData: ") + e.what());
            result.clear();
        }

        return result;
    }

    std::vector<float> ProtocolManager::getImpedanceData()
    {
        std::vector<float> result;

        try {
            // IMPORTANT: Extract and parse new data packets to feed RealTimeImpedanceCalculator
            // We need to continuously update the sliding window with latest data
            auto rawPackets = processor->waitAndExtractPackets(100);  // Extract up to 100 packets
            
            if (!rawPackets.empty()) {
                for (const auto& pkt : rawPackets) {
                    std::vector<float> voltageData;
                    
                    // Parse to get voltage values
                    if (parser->parseNewEEGPacket2Float(pkt.data(), pkt.size(), voltageData)) {
                        // Feed data to real-time impedance calculator
                        if (voltageData.size() == 8) {
                            realTimeImpedance->addSample(voltageData.data(), voltageData.size());
                        }
                        
                        {
                            std::lock_guard<std::mutex> lock(statsMutex);
                            totalPacketsReceived++;
                        }
                    } else {
                        std::lock_guard<std::mutex> lock(statsMutex);
                        totalPacketsDropped++;
                    }
                }
                
                Logger::Log(LogLevel::DEBUG, ErrorCategory::PROTOCOL,
                    "Impedance: processed " + std::to_string(rawPackets.size()) + " packets");
            }
            
            // Reserve space: REF + GND + 8 channels × 2
            result.reserve(2 + EEG_CHANNEL_COUNT * 2);
            
            // First two values: REF and GND electrode impedance
            result.push_back(0.0f);  // REF (not measured)
            result.push_back(0.0f);  // GND (not measured)

            // Check if we have enough data for calculation
            if (!realTimeImpedance->areAllChannelsReady()) {
                Logger::Info("Insufficient data for impedance calculation");
                
                auto stats = realTimeImpedance->getStatistics();
                Logger::Info("Window fill: " + std::to_string(stats.currentFill) + "/" + 
                           std::to_string(stats.windowSize));
                
                // Return default values
                for (int ch = 0; ch < EEG_CHANNEL_COUNT; ++ch) {
                    result.push_back(-1.0f);  // Not ready
                    result.push_back(-1.0f);  // Reserved
                }
                return result;
            }

            // Calculate impedance for all channels using LATEST data
            float impedances[8];
            if (realTimeImpedance->calculateImpedance(impedances, 8)) {
                Logger::Info("Real-time impedance calculated successfully");
                
                for (int ch = 0; ch < EEG_CHANNEL_COUNT; ++ch) {
                    result.push_back(impedances[ch]);
                    result.push_back(-1.0f);  // Reserved for future use
                    
                    Logger::Log(LogLevel::DEBUG, ErrorCategory::GENERAL,
                        "CH" + std::to_string(ch) + ": " + 
                        std::to_string(impedances[ch]) + " kΩ");
                }
            }
            else {
                Logger::Warning("Impedance calculation failed");
                for (int ch = 0; ch < EEG_CHANNEL_COUNT; ++ch) {
                    result.push_back(-1.0f);
                    result.push_back(-1.0f);
                }
            }
        }
        catch (const std::exception& e) {
            Logger::Error(ErrorCategory::GENERAL,
                std::string("Error in getImpedanceData: ") + e.what());
            result.clear();
        }

        return result;
    }

    double ProtocolManager::getImpedanceReadiness() const
    {
        if (!realTimeImpedance) return 0.0;
        
        auto stats = realTimeImpedance->getStatistics();
        if (stats.windowSize == 0) return 0.0;
        
        return static_cast<double>(stats.currentFill) / stats.windowSize;
    }

    ProtocolManager::Statistics ProtocolManager::getStatistics() const
    {
        Statistics stats;
        
        {
            std::lock_guard<std::mutex> lock(statsMutex);
            stats.packetsReceived = totalPacketsReceived;
            stats.packetsDropped = totalPacketsDropped;
            stats.bytesReceived = totalBytesReceived;
            stats.dropRate = (totalPacketsReceived > 0) 
                ? static_cast<double>(totalPacketsDropped) / totalPacketsReceived 
                : 0.0;
        }
        
        if (processor) {
            auto bufferStats = processor->getStatistics();
            stats.bufferSize = bufferStats.bufferSize;
            stats.bufferCapacity = bufferStats.bufferCapacity;
            stats.bufferFill = bufferStats.fillPercentage;
        }
        
        stats.impedanceReady = isImpedanceReady();
        
        return stats;
    }

    void ProtocolManager::clearBuffers()
    {
        if (processor) {
            processor->clear();
        }
        
        if (realTimeImpedance) {
            realTimeImpedance->clear();
        }
        
        Logger::Info("All buffers cleared");
    }

} // namespace protocol
