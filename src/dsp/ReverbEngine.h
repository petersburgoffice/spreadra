#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>
#include <memory>
#include "utils/Logger.h"

/**
 * @brief ReverbEngine на основе Schroeder/FDN с настоящим стерео
 * 
 * Основано на классических работах Schroeder (1961) и Moorer (1979)
 * Реализует настоящий стерео реверб как в Freeverb:
 * - 6 параллельных comb фильтров для каждого канала с разными задержками
 * - 2 последовательных all-pass фильтра для каждого канала
 * - stereoSpread для декорреляции между каналами
 * - Cross-mixing для стерео ширины
 */
class ReverbEngine
{
public:
    //==============================================================================
    ReverbEngine();
    ~ReverbEngine();

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
        float roomSize = 1000.0f;      // m², 10-10000
        float decayTime = 3.0f;        // seconds, 0.1-20
        float damping = 50.0f;         // %, 0-100
        float preDelay = 0.0f;         // ms, 0-500
        float stereoWidth = 100.0f;    // %, 0-150
        float dryWetMix = 50.0f;       // %, 0-100 (0=dry, 100=wet)
        int numCombFilters = 6;        // Фиксированное количество по Schroeder
        int numAllPassFilters = 2;     // Фиксированное количество по Schroeder
        int stereoSpread = 23;         // Разница в задержках между каналами (сэмплы)
    };

    void setParameters(const Parameters& newParams);
    const Parameters& getParameters() const { return params; }

    //==============================================================================
    // Индивидуальные параметры
    void setRoomSize(float roomSizeM2);
    void setDecayTime(float decayTimeSeconds);
    void setDamping(float dampingPercent);
    void setPreDelay(float preDelayMs);
    void setStereoWidth(float widthPercent);
    void setDryWetMix(float mixPercent);

private:
    //==============================================================================
    // Comb Filter
    struct CombFilter
    {
        std::vector<float> buffer;
        size_t readIndex = 0;
        size_t writeIndex = 0;
        size_t delayTime = 0;
        float feedback = 0.0f;
        float damping = 0.0f;
        
        // ПРОСТОЙ CROSSFADE для выходного сигнала при изменении параметров
        float outputGain = 1.0f;        // Текущий gain выхода
        float targetOutputGain = 1.0f;  // Целевой gain
        int fadeRemaining = 0;          // Оставшиеся сэмплы fade
    };

    //==============================================================================
    // All-Pass Filter
    struct AllPassFilter
    {
        std::vector<float> buffer;
        size_t readIndex = 0;
        size_t writeIndex = 0;
        size_t delayTime = 0;
        float feedback = 0.0f;
        
        // ПРОСТОЙ CROSSFADE для выходного сигнала при изменении параметров
        float outputGain = 1.0f;        // Текущий gain выхода
        float targetOutputGain = 1.0f;  // Целевой gain
        int fadeRemaining = 0;          // Оставшиеся сэмплы fade
    };

    //==============================================================================
    // Early Reflections
    struct EarlyReflection
    {
        std::vector<float> buffer;
        size_t readIndex = 0;
        size_t writeIndex = 0;
        size_t delayTime = 0;
        float gain = 1.0f;
    };

    //==============================================================================
    // Состояние
    Parameters params;
    double sampleRate = 44100.0;
    int blockSize = 512;
    bool isPrepared = false;

    // Компоненты реверберации - СТЕРЕО
    std::vector<CombFilter> combFiltersL;      // Левый канал
    std::vector<CombFilter> combFiltersR;      // Правый канал
    std::vector<AllPassFilter> allPassFiltersL; // Левый канал
    std::vector<AllPassFilter> allPassFiltersR; // Правый канал
    std::vector<EarlyReflection> earlyReflectionsL; // Левый канал
    std::vector<EarlyReflection> earlyReflectionsR; // Правый канал
    
    // Pre-delay - стерео
    std::vector<float> preDelayBufferL;
    std::vector<float> preDelayBufferR;
    size_t preDelayIndexL = 0;
    size_t preDelayIndexR = 0;
    int preDelaySamples = 0;
    
    // Временные буферы - стерео
    std::vector<float> tempBufferL;
    std::vector<float> tempBufferR;
    std::vector<float> combOutputL;
    std::vector<float> combOutputR;
    std::vector<float> allPassOutputL;
    std::vector<float> allPassOutputR;
    std::vector<float> earlyReflectionsBufferL;
    std::vector<float> earlyReflectionsBufferR;
    
    // Параметры микширования для стерео
    float wet1 = 1.0f;  // Основной wet gain
    float wet2 = 0.0f;  // Cross-channel wet gain
    float dry = 0.0f;   // Dry gain

    //==============================================================================
    // Внутренние методы
    void initializeCombFilters();
    void initializeAllPassFilters();
    void initializeEarlyReflections();
    void updateFilterParameters();
    void updatePreDelay();
    void updateEarlyReflections();
    void updateStereoMixing();
    
    // НОВЫЕ МЕТОДЫ: Обновление времен задержек без переинициализации буферов
    void updateDelayTimes();
    void updateEarlyReflectionDelayTimes();
    
    // Масштабируемые времена задержек - для стерео
    std::vector<int> getScaledCombDelays(bool isRightChannel = false);
    std::vector<int> getScaledAllPassDelays(bool isRightChannel = false);
    std::vector<float> getEarlyReflectionDelays(bool isRightChannel = false);
    
    // Математические утилиты
    static bool areMutuallyPrime(const std::vector<int>& delays);
    static int gcd(int a, int b);
    static float calculateFeedback(float decayTime, double sampleRate);
    static float calculateRoomScale(float roomSize);

    void processCombFilter(const float* input, float* output, int numSamples, CombFilter& filter);
    void processAllPassFilter(const float* input, float* output, int numSamples, AllPassFilter& filter);
    void processEarlyReflections(const float* input, float* output, int numSamples, 
                                std::vector<EarlyReflection>& reflections);

    float calculateReverbTime();

    #ifdef DEBUG
    void logFeedbackValues() const;
    #endif
    
    // Логирование состояния реверберации
    void logReverbState() const;

    // JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ReverbEngine)
}; 