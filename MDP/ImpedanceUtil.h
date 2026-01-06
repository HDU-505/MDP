#pragma once
#include <vector>
#include <cmath>
#include <complex>

// 工具类：用于处理EEG数据并计算阻抗
class ImpedanceUtil
{
public:
    /// <summary>   构造函数 </summary>
    /// <param name="samplingRate">  采样率 </param>
    /// <param name="targetFreq">    目标频率(f_loff) </param>
    /// <param name="windowSize">    滑动窗口大小(采样点数) </param>
    /// <param name="stepSize">      滑动步长(采样点数) </param>
    ImpedanceUtil(float samplingRate, float targetFreq, int windowSize, int stepSize);
    ~ImpedanceUtil();

    /// <summary>   处理单行EEG时域数据 (一个通道的数据？) </summary>
    /// <param name="data">  时域信号向量 </param>
    // 输出: 对应的阻抗值向量 (每个窗口一个值)
    std::vector<float> ImpedanceCalculation(const std::vector<float>& data);

private:
    float m_samplingRate;
    float m_targetFreq;
    int m_windowSize;
    int m_stepSize;
    std::vector<float> m_windowFunc;

    // 初始化窗口函数 (Hanning窗)
    void InitWindow();

    // 计算窗口数据的目标频率幅值 (使用DFT/Goertzel思想)
    float CalculateAmplitude(const std::vector<float>& windowData);

    // 根据幅值计算阻抗
    float CalculateImpedance(int32_t adc_code, double v_ref, double gain, double i_source);
};
