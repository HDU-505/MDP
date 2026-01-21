#pragma once

#include <vector>
#include <deque>
#include <cstdint>
#include <mutex>
#include "ImpedanceUtil.h"

// Undefine Windows macros that conflict with std::min/max
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

/**
 * @brief Real-time impedance calculator with sliding window
 * 
 * Optimized for real-time impedance display:
 * - Maintains sliding window of latest samples per channel
 * - Always uses most recent data for calculation
 * - Fast impedance updates (< 10ms)
 * - Thread-safe operations
 */
class RealTimeImpedanceCalculator {
private:
    // Sliding window for each channel (stores voltage in uV)
    std::vector<std::deque<float>> channelWindows;
    
    ImpedanceUtil* impedanceUtil;
    
    const size_t windowSize;
    const size_t minSamplesRequired;
    
    mutable std::mutex mutex;
    
    // Statistics
    uint64_t totalSamplesProcessed = 0;
    uint64_t calculationCount = 0;

public:
    /**
     * @brief Constructor
     * @param numChannels Number of EEG channels (default: 8)
     * @param windowSize Size of sliding window (default: 248 samples)
     * @param samplingRate Sampling rate in Hz (default: 250Hz)
     * @param targetFreq Target frequency for Goertzel (default: 31.25Hz)
     */
    RealTimeImpedanceCalculator(
        size_t numChannels = 8,
        size_t windowSize = 248,
        float samplingRate = 250.0f,
        float targetFreq = 31.25f)
        : windowSize(windowSize)
        , minSamplesRequired(windowSize)
    {
        // Initialize channel windows
        channelWindows.resize(numChannels);
        
        // Create impedance calculator
        impedanceUtil = new ImpedanceUtil(samplingRate, targetFreq, static_cast<int>(windowSize));
    }

    ~RealTimeImpedanceCalculator() {
        delete impedanceUtil;
    }

    /**
     * @brief Add new sample data
     * @param channelData Array of channel voltages (uV) for one sample
     * @param numChannels Number of channels in array
     * 
     * This maintains a sliding window - oldest sample is dropped when window is full
     */
    void addSample(const float* channelData, size_t numChannels) {
        if (!channelData) return;

        std::lock_guard<std::mutex> lock(mutex);

        size_t channels = std::min(numChannels, channelWindows.size());
        
        for (size_t ch = 0; ch < channels; ++ch) {
            // Add new sample
            channelWindows[ch].push_back(channelData[ch]);
            
            // Remove oldest if exceeds window size
            if (channelWindows[ch].size() > windowSize) {
                channelWindows[ch].pop_front();
            }
        }

        totalSamplesProcessed++;
    }

    /**
     * @brief Add multiple samples at once
     * @param samples Vector of sample data (each inner vector is one sample's channel data)
     */
    void addSamples(const std::vector<std::vector<float>>& samples) {
        if (samples.empty()) return;

        std::lock_guard<std::mutex> lock(mutex);

        for (const auto& sample : samples) {
            size_t channels = std::min(sample.size(), channelWindows.size());
            
            for (size_t ch = 0; ch < channels; ++ch) {
                channelWindows[ch].push_back(sample[ch]);
                
                if (channelWindows[ch].size() > windowSize) {
                    channelWindows[ch].pop_front();
                }
            }
            
            totalSamplesProcessed++;
        }
    }

    /**
     * @brief Calculate impedance for all channels using latest data
     * @param impedances Output array (size should be >= numChannels)
     * @param numChannels Number of channels to calculate
     * @return true if calculation successful (enough samples available)
     */
    bool calculateImpedance(float* impedances, size_t numChannels) {
        if (!impedances) return false;

        std::lock_guard<std::mutex> lock(mutex);

        size_t channels = std::min(numChannels, channelWindows.size());
        bool success = true;

        for (size_t ch = 0; ch < channels; ++ch) {
            if (channelWindows[ch].size() >= minSamplesRequired) {
                // Convert deque to vector for impedance calculation
                std::vector<float> windowData(
                    channelWindows[ch].begin(),
                    channelWindows[ch].end()
                );

                try {
                    impedances[ch] = impedanceUtil->CalculateSingleImpedance(windowData);
                    calculationCount++;
                }
                catch (...) {
                    impedances[ch] = -1.0f;
                    success = false;
                }
            }
            else {
                impedances[ch] = -1.0f;
                success = false;
            }
        }

        return success;
    }

    /**
     * @brief Check if enough samples available for calculation
     * @param channelIndex Channel index (0-7)
     * @return true if ready for calculation
     */
    bool isReady(size_t channelIndex = 0) const {
        std::lock_guard<std::mutex> lock(mutex);
        
        if (channelIndex >= channelWindows.size()) return false;
        return channelWindows[channelIndex].size() >= minSamplesRequired;
    }

    /**
     * @brief Check if all channels are ready
     */
    bool areAllChannelsReady() const {
        std::lock_guard<std::mutex> lock(mutex);
        
        for (const auto& window : channelWindows) {
            if (window.size() < minSamplesRequired) {
                return false;
            }
        }
        return true;
    }

    /**
     * @brief Get current window fill for a channel
     * @param channelIndex Channel index
     * @return Number of samples currently in window
     */
    size_t getWindowFill(size_t channelIndex) const {
        std::lock_guard<std::mutex> lock(mutex);
        
        if (channelIndex >= channelWindows.size()) return 0;
        return channelWindows[channelIndex].size();
    }

    /**
     * @brief Get minimum samples required for calculation
     */
    size_t getMinSamplesRequired() const {
        return minSamplesRequired;
    }

    /**
     * @brief Clear all window data
     */
    void clear() {
        std::lock_guard<std::mutex> lock(mutex);
        
        for (auto& window : channelWindows) {
            window.clear();
        }
        
        totalSamplesProcessed = 0;
        calculationCount = 0;
    }

    /**
     * @brief Get statistics
     */
    struct Statistics {
        uint64_t totalSamples;
        uint64_t calculations;
        size_t windowSize;
        size_t currentFill;  // Average fill across channels
    };

    Statistics getStatistics() const {
        std::lock_guard<std::mutex> lock(mutex);
        
        Statistics stats;
        stats.totalSamples = totalSamplesProcessed;
        stats.calculations = calculationCount;
        stats.windowSize = windowSize;
        
        // Calculate average fill
        size_t totalFill = 0;
        for (const auto& window : channelWindows) {
            totalFill += window.size();
        }
        stats.currentFill = channelWindows.empty() ? 0 : totalFill / channelWindows.size();
        
        return stats;
    }
};
