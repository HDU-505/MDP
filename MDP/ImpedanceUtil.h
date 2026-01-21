#pragma once
#include <vector>
#include <cmath>

/**
 * @brief Impedance Calculation Utility using Goertzel Algorithm
 *
 * This class implements impedance calculation for EEG data using the Goertzel algorithm
 * to extract the amplitude at a specific frequency (31.25Hz for MT08 ADS1299).
 *
 * Key parameters:
 * - Sampling rate: 250 Hz
 * - Target frequency: 31.25 Hz (injected AC signal)
 * - Injected current: 6 nA RMS
 * - Window size: Must be multiple of 8 (recommended: 248 samples)
 */
class ImpedanceUtil
{
public:
    /**
     * @brief Constructor
     * @param samplingRate Sampling rate in Hz (default: 250Hz)
     * @param targetFreq Target frequency in Hz (default: 31.25Hz)
     * @param windowSize Window size in samples (must be multiple of 8, default: 248)
     */
    ImpedanceUtil(float samplingRate = 250.0f, 
                  float targetFreq = 31.25f, 
                  int windowSize = 248);
    ~ImpedanceUtil();

    /**
     * @brief Calculate impedance values from EEG voltage data
     * @param voltageData Time-domain voltage signal in microvolts (uV)
     * @return Impedance values in kilohms (k��)
     */
    std::vector<float> CalculateImpedance(const std::vector<float>& voltageData);

    /**
     * @brief Calculate single impedance value from one window of data
     * @param voltageData Voltage data window in microvolts (uV)
     * @return Impedance value in kilohms (k��)
     */
    float CalculateSingleImpedance(const std::vector<float>& voltageData);

private:
    float m_samplingRate;      // Sampling rate (Hz)
    float m_targetFreq;        // Target frequency (Hz)
    int m_windowSize;          // Window size (samples)
    double m_coeff;            // Goertzel coefficient

    /**
     * @brief Calculate Goertzel coefficient
     * Coeff = 2 * cos(2 * pi * targetFreq / samplingRate)
     */
    void CalculateCoefficient();

    /**
     * @brief Extract amplitude at target frequency using Goertzel algorithm
     * @param voltageData Voltage data in microvolts (uV)
     * @return RMS amplitude in microvolts (uV)
     */
    float GoertzelAmplitude(const std::vector<float>& voltageData);

    /**
     * @brief Convert voltage amplitude to impedance
     * @param amplitudeUV Voltage amplitude in microvolts (uV)
     * @return Impedance in kilohms (k��)
     */
    float VoltageToImpedance(float amplitudeUV);
};
