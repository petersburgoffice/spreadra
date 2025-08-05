#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "ReverbEngine.h"
#include "FilterBank.h"

/**
 * @brief Основной DSP алгоритм для реверберации
 * 
 * Этот класс интегрирует все DSP компоненты в единый pipeline:
 * Input → Reverb → Output
 * 
 * Алгоритм основан на работе Schroeder (1961) и современных
 * методах цифровой обработки сигналов.
 */
class ReverbAlgorithm
{
public:
    //==============================================================================
    ReverbAlgorithm();
    ~ReverbAlgorithm();

    //==============================================================================
    // Основная обработка аудио
    void process(const float* input, float* output, int numSamples);
    void processStereo(const float* inputL, const float* inputR, 
                      float* outputL, float* outputR, int numSamples);

    //==============================================================================
    // Подготовка к воспроизведению
    void prepare(double sampleRate, int blockSize);
    void reset();

    //==============================================================================
    // Параметры алгоритма
    struct Parameters
    {
        // Spreadra parameters
        float stereoWidth = 100.0f;    // %, 0-150
        float dryWet = 50.0f;          // %, 0-100
    };

    void setParameters(const Parameters& newParams);
    const Parameters& getParameters() const { return params; }

    //==============================================================================
    // Индивидуальные параметры
    void setDryWet(float dryWetPercent);
    void setStereoWidth(float stereoWidthPercent);

    //==============================================================================
    // DSP компоненты
    ReverbEngine& getReverbEngine() { return reverbEngine; }
    FilterBank& getFilterBank() { return filterBank; }

    //==============================================================================
    // Метрики и диагностика
    float getCpuUsage() const;
    float getLatency() const;
    void getSpectrum(float* spectrum, int numBins);

private:
    //==============================================================================
    // DSP компоненты
    ReverbEngine reverbEngine;
    FilterBank filterBank;

    // Параметры
    Parameters params;
    
    // Состояние
    double sampleRate = 44100.0;
    int blockSize = 512;
    bool isPrepared = false;

    // Временные буферы
    std::vector<float> tempBuffer1;
    std::vector<float> tempBuffer2;
    std::vector<float> tempBuffer3;
    
    // Стерео буферы
    std::vector<float> tempBufferL;
    std::vector<float> tempBufferR;

    //==============================================================================
    // Внутренние методы
    void updateDSPParameters();
    void processStereoInternal(const float* inputL, const float* inputR, 
                              float* outputL, float* outputR, int numSamples);

    // Метрики производительности
    float cpuUsage = 0.0f;
    float latencyMs = 0.0f;
    
    // JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ReverbAlgorithm)
}; 