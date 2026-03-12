#include "ImpedanceUtil.h"
#include "ErrorHandler.h" // Includes Logger def
#include <numeric>
#include <iostream>
#include <cmath>
#include <iomanip>
#include <string> // Includes std::to_string

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ADS1299 impedance measurement parameters
constexpr double I_SOURCE_AMPS = 6.0e-9;   // 6nA injected current
constexpr double UV_TO_V = 1.0e-6;         // Microvolts to volts
// constexpr double OHM_TO_KOHM = 1.0e-3;     // Deprecated: Impedances are now reported directly in ohms

ImpedanceUtil::ImpedanceUtil(float samplingRate, float targetFreq, int windowSize)
    : m_samplingRate(samplingRate), 
      m_targetFreq(targetFreq), 
      m_windowSize(windowSize),
      m_coeff(0.0)
{
    // Validate window size (must be multiple of 8 for 31.25Hz at 250Hz sampling)
    if (m_windowSize % 8 != 0) {
        std::string warningMsg = "Window size should be multiple of 8. Adjusting to: " + std::to_string((m_windowSize / 8) * 8);
        sdk::Logger::Log(sdk::LogLevel::WARNING, sdk::ErrorCategory::GENERAL, warningMsg);
        m_windowSize = (m_windowSize / 8) * 8;
    }
    
    CalculateCoefficient();
}

ImpedanceUtil::~ImpedanceUtil()
{
}

void ImpedanceUtil::CalculateCoefficient()
{
    // Goertzel coefficient: 2 * cos(2 * pi * targetFreq / samplingRate)
    // For 31.25Hz @ 250Hz: 2 * cos(pi/4) = sqrt(2)  1.414213562
    double omega = 2.0 * M_PI * m_targetFreq / m_samplingRate;
    m_coeff = 2.0 * std::cos(omega);
    
    sdk::Logger::Log(sdk::LogLevel::DEBUG, sdk::ErrorCategory::GENERAL, "Goertzel coefficient: " + std::to_string(m_coeff));
}

float ImpedanceUtil::GoertzelAmplitude(const std::vector<float>& voltageData)
{
    if (voltageData.size() < static_cast<size_t>(m_windowSize)) {
        sdk::Logger::Error(sdk::ErrorCategory::GENERAL, "Insufficient data for Goertzel calculation");
        return 0.0f;
    }

    // Goertzel algorithm variables
    double s0 = 0.0;
    double s1 = 0.0;
    double s2 = 0.0;

    // Process exactly windowSize samples for accurate frequency extraction
    int N = m_windowSize;
    
    // Goertzel iteration
    for (int n = 0; n < N; ++n) {
        s0 = voltageData[n] + m_coeff * s1 - s2;
        s2 = s1;
        s1 = s0;
    }

    // Calculate magnitude
    // |X[k]| = sqrt(s1^2 + s2^2 - coeff * s1 * s2)
    double magnitude = std::sqrt(s1 * s1 + s2 * s2 - m_coeff * s1 * s2);

    // Convert magnitude to RMS amplitude
    // For Goertzel: Amplitude = 2 * magnitude / N
    // Then convert to RMS: RMS = Amplitude / sqrt(2)
    // Combined: RMS = magnitude * sqrt(2) / N
    double rms = magnitude * std::sqrt(2.0) / N;

    return static_cast<float>(rms);
}

float ImpedanceUtil::VoltageToImpedance(float amplitudeUV)
{
    // Convert microvolts to volts
    double voltageV = amplitudeUV * UV_TO_V;
    
    // Calculate impedance: Z = V_RMS / I_RMS
    // I_RMS = 6nA for square wave
    double impedanceOhm = voltageV / I_SOURCE_AMPS;
    
    // Return impedance in ohms
    return static_cast<float>(impedanceOhm);
}

float ImpedanceUtil::CalculateSingleImpedance(const std::vector<float>& voltageData)
{
    if (voltageData.size() < static_cast<size_t>(m_windowSize)) {
        sdk::Logger::Error(sdk::ErrorCategory::GENERAL, "Insufficient data for impedance calculation");
        return -1.0f;
    }

    // Extract amplitude at target frequency using Goertzel
    float amplitudeUV = GoertzelAmplitude(voltageData);
    
    // Convert amplitude to impedance
    float impedanceOhm = VoltageToImpedance(amplitudeUV);
    
    return impedanceOhm;
}

std::vector<float> ImpedanceUtil::CalculateImpedance(const std::vector<float>& voltageData)
{
    std::vector<float> impedances;

    // Check if we have enough data
    if (voltageData.size() < static_cast<size_t>(m_windowSize)) {
        sdk::Logger::Error(sdk::ErrorCategory::GENERAL, "Insufficient data for impedance calculation");
        return impedances;
    }

    // Calculate impedance for each non-overlapping window
    // Use windowSize as step to avoid overlap
    size_t numWindows = voltageData.size() / m_windowSize;
    impedances.reserve(numWindows);

    for (size_t i = 0; i < numWindows; ++i) {
        size_t startIdx = i * m_windowSize;
        
        // Extract window data
        std::vector<float> windowData(
            voltageData.begin() + startIdx,
            voltageData.begin() + startIdx + m_windowSize
        );

        // Calculate impedance for this window
        float impedance = CalculateSingleImpedance(windowData);
        impedances.push_back(impedance);
    }

    return impedances;
}
