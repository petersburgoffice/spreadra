#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>
#include <memory>

/**
 * @brief Feedback Delay Network (FDN) с модуляцией и диффузией
 * 
 * Поддерживает:
 * - Несколько линий задержки (prime delays)
 * - Модуляцию (chorus/vibrato)
 * - Диффузию (feedback matrix)
 * - Параметры: delayTime, feedback, diffusion, modulation, modulationDepth
 */
class DelayEngine
{
public:
    //==============================================================================
    DelayEngine();
    ~DelayEngine();

    //==============================================================================
    // Основная обработка
    void process(const float* input, float* output, int numSamples);
    void processStereo(const float* inputL, const float* inputR, 
                      float* outputL, float* outputR, int numSamples);

    //==============================================================================
    // Подготовка
    void prepare(double sampleRate, int blockSize);
    void reset();

    //==============================================================================
    // Параметры
    struct Parameters
    {
        float delayTime = 500.0f;      // ms, 10-2000
        float feedback = 50.0f;        // %, 0-95 (ограничено для стабильности)
        float diffusion = 50.0f;       // %, 0-100
        float modulation = 0.0f;       // Hz, 0-10
        float modulationDepth = 0.0f;  // ms, 0-50
        int numDelayLines = 4;         // 2-8
    };

    void setParameters(const Parameters& newParams);
    const Parameters& getParameters() const { return params; }

    //==============================================================================
    // Индивидуальные параметры
    void setDelayTime(float delayTimeMs);
    void setFeedback(float feedbackPercent);
    void setDiffusion(float diffusionPercent);
    void setModulation(float modulationHz);
    void setModulationDepth(float depthMs);

    //==============================================================================
    // Специальные функции
    void setDelayTimes(const std::vector<float>& delayTimes);
    void setFeedbackMatrix(const std::vector<float>& matrix);
    void enableModulation(bool enabled);

private:
    //==============================================================================
    // Структура линии задержки
    struct DelayLine
    {
        std::vector<float> buffer;
        size_t writeIndex = 0;
        size_t delayTime = 0; // samples
        float feedback = 0.0f;
        float modulationPhase = 0.0f;
        float modulationIncrement = 0.0f;
        float modulationDepth = 0.0f;
        bool modulationEnabled = false;
    };

    //==============================================================================
    // Состояние
    Parameters params;
    double sampleRate = 44100.0;
    int blockSize = 512;
    bool isPrepared = false;

    // Линии задержки
    std::vector<DelayLine> delayLines;
    
    // Матрица обратной связи (Hadamard matrix)
    std::vector<float> feedbackMatrix;
    
    // Временные буферы
    std::vector<float> tempBuffer;
    std::vector<float> modulationBuffer;

    //==============================================================================
    // Внутренние методы
    void initializeDelayLines();
    void initializeFeedbackMatrix();
    void updateDelayTimes();
    void updateModulation();
    
    // Обработка модуляции
    float getModulatedDelayTime(size_t lineIndex);
    void updateModulationPhase();
    
    // Математические утилиты
    std::vector<float> createHadamardMatrix(int size);
    bool isPrime(int n);
    std::vector<int> generatePrimeDelays(int count, int minDelay, int maxDelay);

    // JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DelayEngine)
}; 