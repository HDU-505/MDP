#include "ImpedanceUtil.h"
#include <numeric>
#include <iostream>
#include <cmath>
#include <iomanip>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

ImpedanceUtil::ImpedanceUtil(float samplingRate, float targetFreq, int windowSize, int stepSize)
    : m_samplingRate(samplingRate), m_targetFreq(targetFreq), m_windowSize(windowSize), m_stepSize(stepSize)
{
    InitWindow();
}

ImpedanceUtil::~ImpedanceUtil()
{
}

void ImpedanceUtil::InitWindow()
{
    m_windowFunc.resize(m_windowSize);
    for (int i = 0; i < m_windowSize; ++i)
    {
        // 使用Hanning窗 (主流应用常用)
        // w[n] = 0.5 * (1 - cos(2*pi*n / (N-1)))
        m_windowFunc[i] = 0.5f * (1.0f - std::cos(2.0f * (float)M_PI * i / (m_windowSize - 1)));
    }
}

std::vector<float> ImpedanceUtil::ImpedanceCalculation(const std::vector<float>& data)
{
    const float I_SOURCE_AMPS = 6.0e-9f; //注入电流6nA
    double v_ref = 4500;           // 参考电压 4500mV
    double gain = 24.0;           // 增益 24
    std::vector<float> impedances;

    // 如果数据长度小于窗口大小，无法处理
    if (data.size() < (size_t)m_windowSize)
    {
        return impedances;
    }

    // 计算窗口数量
    size_t numWindows = (data.size() - m_windowSize) / m_stepSize + 1;
    impedances.reserve(numWindows);

    // 滑动窗口处理
    for (size_t i = 0; i <= data.size() - m_windowSize; i += m_stepSize)
    {
        std::vector<float> windowData(m_windowSize);
        for (int j = 0; j < m_windowSize; ++j)
        {
            // 应用窗口函数
            windowData[j] = data[i + j] * m_windowFunc[j];
        }

        // 提取幅值
        float amplitude = CalculateAmplitude(windowData);

        // 计算阻抗, 提取出的幅值amplitude是电压幅值，还是ADC的计数值？
        //float impedance = amplitude / I_SOURCE_AMPS; //电压幅值
        float impedance = CalculateImpedance(amplitude, v_ref, gain, I_SOURCE_AMPS); //计数值
        impedances.push_back(impedance);
    }

    return impedances;
}

float ImpedanceUtil::CalculateAmplitude(const std::vector<float>& windowData)
{
    // 使用单频点DFT (Correlation) 提取特定频率 f_loff 的幅值
    // 公式: X(f) = sum(x[n] * exp(-j * 2 * pi * f * n / fs))

    float realPart = 0.0f;
    float imagPart = 0.0f;
    float angleStep = 2.0f * (float)M_PI * m_targetFreq / m_samplingRate;

    for (int n = 0; n < m_windowSize; ++n)
    {
        float angle = angleStep * n;
        realPart += windowData[n] * std::cos(angle);
        imagPart -= windowData[n] * std::sin(angle);
    }

    // 计算模值 (Magnitude)
    float magnitude = std::sqrt(realPart * realPart + imagPart * imagPart);

    // 幅值归一化
    // 对于Hanning窗，相干增益(Coherent Gain)为0.5，即 sum(w[n])/N = 0.5
    // 恢复信号真实幅值: Amplitude = Magnitude * 2 / sum(w[n])

    float windowSum = 0.0f;
    for (float w : m_windowFunc) windowSum += w;

    if (windowSum > 0)
        return magnitude * 2.0f / windowSum;
    else
        return 0.0f;
}

float ImpedanceUtil::CalculateImpedance(int32_t adc_code, double v_ref, double gain, double i_source)
{
    const double ADC_SCALE_FACTOR = 8388608.0;

    if (gain == 0 || i_source == 0) {
        std::cerr << "Error: Gain or Source Current cannot be zero." << std::endl;
        return 0.0;
    }

    // 分子: Code * V_REF
    double numerator = static_cast<double>(adc_code) * v_ref;

    // 分母: 2^23 * Gain * I_source
    double denominator = ADC_SCALE_FACTOR * gain * i_source;

    // 计算阻抗 Z
    return numerator / denominator;
}

// 测试代码
//int main()
//{
//    // 参数设置
//    float samplingRate = 256.0f; // 采样率：256Hz
//    float targetFreq = 62.5f;    // 目标频率：62.5Hz或32Hz
//    int windowSize = 256;        // 窗口：256点（1秒）
//    int stepSize = 50;           // 步长：50点（约0.2秒）
//
//    std::cout << "Sampling Rate: " << samplingRate << " Hz" << std::endl;
//    std::cout << "Target Freq: " << targetFreq << " Hz" << std::endl;
//
//    // 实例化计算器
//    ImpedanceUtil calculator(samplingRate, targetFreq, windowSize, stepSize);
//
//    // --- CSV 读取配置 ---
//    std::string csvFile = "p_test2_open.csv";   // CSV文件名
//    int targetChannel = 0;                      // 选择通道
//    int startRow = 1;                           // 跳过索引
//    int numRowsToRead = 1024;                   // 读取数据行数
//
//    // 加载数据
//    const int numChannels = 8;
//    const int numSamples = numRowsToRead;
//    std::vector<std::vector<float>> eegData(numChannels, std::vector<float>(numSamples));
//
//    for (int i = 0; i < 8; i++) {
//        eegData[i] = ReadChannelFromCSV(csvFile, i, startRow, numRowsToRead);
//    }
//    std::cout << "Data loaded successfully." << std::endl;
//
//    //处理所有通道
//    std::vector<std::vector<float>> allResults(numChannels);
//    for (int ch = 0; ch < numChannels; ++ch)
//    {
//        allResults[ch] = calculator.ImpedanceCalculation(eegData[ch]);
//    }
//
//    // 输出结果
//    int numPoints = allResults[0].size();
//    for (int i = 0; i < numPoints; ++i) {
//        std::cout << "Time Point " << std::setw(2) << i << ": ";
//        for (int ch = 0; ch < numChannels; ++ch) {
//            //输出阻抗值，保留1位小数；每一行的输出长度都相同
//
//            std::cout << std::fixed << std::setprecision(2) << std::setw(7) << allResults[ch][i];
//            if (ch < numChannels - 1) {
//                std::cout << ", ";
//            }
//        }
//        std::cout << std::endl;
//    }
//
//    return 0;
//}