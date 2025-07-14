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
        // Reverbix parameters
        float roomSize = 1000.0f;      // m², 10-10000
        float decayTime = 3.0f;        // seconds, 0.1-20
        float damping = 50.0f;         // %, 0-100
        float preDelay = 0.0f;         // ms, 0-500
        float stereoWidth = 50.0f;     // %, 0-100
        
        // Filter parameters
        float lowPassFreq = 8000.0f;   // Hz, 20-20000
        float highPassFreq = 80.0f;    // Hz, 20-20000
        bool enableLowPass = false;    // Enable low-pass filter
        bool enableHighPass = false;   // Enable high-pass filter
        
        // Mix parameters
        float dryWet = 50.0f;          // %, 0-100
    };

    void setParameters(const Parameters& newParams);
    const Parameters& getParameters() const { return params; }

    //==============================================================================
    // Индивидуальные параметры
    void setRoomSize(float roomSizeM2);
    void setDecayTime(float decayTimeSeconds);
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