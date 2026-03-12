#include "ProtocolManager.h"
#include <algorithm>
#include <sstream>
#include <iomanip>

using namespace std;
using namespace sdk;

namespace protocol {

    ProtocolManager::ProtocolManager(RecordingMode recordingMode)
        : currentMode(recordingMode)
    {
        try {
            processor = std::make_unique<Processor>(NEW_PACKET_TOTAL_SIZE, 500);
            
            if (recordingMode == RM_IMPEDANCE) {
                processor->setBufferMode(DualModeBuffer::Mode::IMPEDANCE, false);
            } else {
                processor->setBufferMode(DualModeBuffer::Mode::NORMAL, false);
            }
            
            parser = std::make_unique<Parser>();
            
            realTimeImpedance = std::make_unique<RealTimeImpedanceCalculator>(
                8, 32, 250.0f, 31.25f
            );
            
        }
        catch (const std::exception& e) {
            sdk::Logger::Info(std::string("Failed to initialize ProtocolManager: ") + e.what());
            throw;
        }
    }

    void ProtocolManager::processData(const uint8_t* data, size_t len)
    {
        if (!data || len == 0) return;   // Silently drop empty buffers to avoid warnings

        {
            std::lock_guard<std::mutex> lock(statsMutex);
            totalBytesReceived += len;
        }
        
        processor->appendData(data, len);
    }

    std::vector<uint8_t> ProtocolManager::buildPacket(
        ComandType commandType,
        PacketType packetType,
        StreamMask streamMask)
    {
        try {
            return parser->buildControlPacket(packetType);
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
        
        if (sampleCount <= 0) return result;

        try {
            // Non-blocking drain: return immediately if buffer is starved (offline resilience)
            auto rawPackets = processor->extractPackets(sampleCount);
            
            if (rawPackets.empty()) {
                return result;  // Return empty block instead of logging
            }

            {
                std::lock_guard<std::mutex> lock(statsMutex);
                totalPacketsReceived += rawPackets.size();
            }
            
            result.reserve(rawPackets.size() * SINGLE_SAMPLE_OUTPUT_SIZE);

            size_t successCount = 0;
            
            for (const auto& pkt : rawPackets) {
                // BUG-3 FIX: only invoke parseNewEEGPacket2Byte to perform packet drop detection and interpolation
                // Avoids incrementing sequence number prematurely resulting in jumping calculation logic
                if (parser->parseNewEEGPacket2Byte(pkt.data(), pkt.size(), result)) {
                    
                    // Only parse Float voltage maps when active Impedance mode requires it
                    if (currentMode == RM_IMPEDANCE) {
                        std::vector<float> voltageData;
                        if (parser->parseNewEEGPacket2Float(pkt.data(), pkt.size(), voltageData)) {
                            if (voltageData.size() == 8) {
                                realTimeImpedance->addSample(voltageData.data(), voltageData.size());
                            }
                        }
                    }
                    
                    successCount++;
                } else {
                    std::lock_guard<std::mutex> lock(statsMutex);
                    totalPacketsDropped++;
                }
            }

#ifdef _DEBUG
            if (successCount > 0) {
                Logger::Log(LogLevel::DEBUG, ErrorCategory::PROTOCOL,
                    "EEG: " + to_string(successCount) + "/" + 
                    to_string(rawPackets.size()) + " packets OK");
            }
#endif
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
            // Non-blocking data extraction (offline resilience)
            auto rawPackets = processor->extractPackets(100);
            
            if (!rawPackets.empty()) {
                for (const auto& pkt : rawPackets) {
                    std::vector<float> voltageData;
                    
                    if (parser->parseNewEEGPacket2Float(pkt.data(), pkt.size(), voltageData)) {
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
            }
            
            result.reserve(2 + EEG_CHANNEL_COUNT * 2);

            if (!realTimeImpedance->areAllChannelsReady()) {
                // Insufficient data: return silent defaults without flooding the log
                result.push_back(0.0f);
                result.push_back(0.0f);
                for (int ch = 0; ch < EEG_CHANNEL_COUNT; ++ch) {
                    result.push_back(-1.0f);
                    result.push_back(-1.0f);
                }
                return result;
            }

            float impedances[8];
            float refImpedance = 0.0f;
            float gndImpedance = 0.0f;
            
            if (realTimeImpedance->calculateImpedance(impedances, 8)) {
                constexpr float MAX_NORMAL_IMPEDANCE = 50000.0f;
                constexpr float MIN_VALID_IMPEDANCE = 0.0f;
                
                std::vector<float> validImpedances;
                float maxImpedance = 0.0f;
                
                for (int ch = 0; ch < EEG_CHANNEL_COUNT; ++ch) {
                    if (impedances[ch] > MIN_VALID_IMPEDANCE && impedances[ch] < MAX_NORMAL_IMPEDANCE) {
                        validImpedances.push_back(impedances[ch]);
                    }
                    if (impedances[ch] > maxImpedance) {
                        maxImpedance = impedances[ch];
                    }
                }
                
                if (!validImpedances.empty()) {
                    float sum = 0.0f;
                    for (float imp : validImpedances) { sum += imp; }
                    float avg = sum / validImpedances.size();
                    refImpedance = avg * 1.05f;
                    gndImpedance = avg * 0.95f;
                } else {
                    refImpedance = maxImpedance * 1.05f;
                    gndImpedance = maxImpedance * 0.95f;
                }
                
                result.push_back(refImpedance);
                result.push_back(gndImpedance);
                
                for (int ch = 0; ch < EEG_CHANNEL_COUNT; ++ch) {
                    result.push_back(impedances[ch]);
                    result.push_back(-1.0f);
                }

#ifdef _DEBUG
                Logger::Log(LogLevel::DEBUG, ErrorCategory::GENERAL,
                    "Impedance: REF=" + to_string(refImpedance) +
                    " GND=" + to_string(gndImpedance));
#endif
            }
            else {
                result.push_back(0.0f);
                result.push_back(0.0f);
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
            stats.packetsDropped  = totalPacketsDropped;
            stats.bytesReceived   = totalBytesReceived;
            stats.dropRate = (totalPacketsReceived > 0) 
                ? static_cast<double>(totalPacketsDropped) / totalPacketsReceived 
                : 0.0;
        }
        
        if (processor) {
            auto bufferStats = processor->getStatistics();
            stats.bufferSize     = bufferStats.bufferSize;
            stats.bufferCapacity = bufferStats.bufferCapacity;
            stats.bufferFill     = bufferStats.fillPercentage;
        }
        
        if (parser) {
            stats.packetsLost         = parser->getLostPackets();
            stats.interpolatedSamples = parser->getInterpolatedSamples();
        } else {
            stats.packetsLost         = 0;
            stats.interpolatedSamples = 0;
        }
        
        stats.impedanceReady = isImpedanceReady();
        
        return stats;
    }

    std::string ProtocolManager::getPacketLossReport() const
    {
        auto stats = getStatistics();

        double lossRatePct = (stats.packetsReceived > 0)
            ? 100.0 * static_cast<double>(stats.packetsLost) / stats.packetsReceived
            : 0.0;

        uint64_t zeroFilled = (stats.packetsLost > stats.interpolatedSamples)
            ? stats.packetsLost - stats.interpolatedSamples
            : 0;

        std::ostringstream ss;
        ss << std::fixed << std::setprecision(3);
        ss << "[PacketLoss Report]\n";
        ss << "  Received     : " << stats.packetsReceived    << "\n";
        ss << "  Lost (seq)   : " << stats.packetsLost        << "\n";
        ss << "  Loss rate    : " << lossRatePct              << "%\n";
        ss << "  Interpolated : " << stats.interpolatedSamples << "\n";
        ss << "  Zero-filled  : " << zeroFilled               << "\n";
        ss << "  Decode errors: " << stats.packetsDropped     << "\n";
        return ss.str();
    }

    void ProtocolManager::clearBuffers()
    {
        if (processor) {
            processor->clear();
        }
        
        if (realTimeImpedance) {
            realTimeImpedance->clear();
        }
    }

} // namespace protocol
