#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>

/**
 * @brief Банк фильтров для обработки сигнала
 * 
 * Предоставляет различные типы фильтров для обработки аудио:
 * - Low-pass фильтры
 * - High-pass фильтры
 * - Band-pass фильтры
 * - All-pass фильтры
 * - Многополосная обработка
 */
class FilterBank
{
public:
    //==============================================================================
    FilterBank();
    ~FilterBank();

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
        float lowPassFreq = 20000.0f;    // Hz, 20-20000
        float highPassFreq = 20.0f;      // Hz, 20-20000
        float bandPassFreq = 1000.0f;    // Hz, 20-20000
        float bandPassQ = 1.0f;          // Q factor, 0.1-10
        float allPassFreq = 1000.0f;     // Hz, 20-20000
        float allPassQ = 1.0f;           // Q factor, 0.1-10
        bool enableLowPass = false;
        bool enableHighPass = false;
        bool enableBandPass = false;
        bool enableAllPass = false;
    };

    void setParameters(const Parameters& newParams);
    const Parameters& getParameters() const { return params; }

    //==============================================================================
    // Индивидуальные параметры
    void setLowPassFrequency(float frequency);
    void setHighPassFrequency(float frequency);
    void setBandPassFrequency(float frequency);
    void setBandPassQ(float q);
    void setAllPassFrequency(float frequency);
    void setAllPassQ(float q);

    //==============================================================================
    // Специальные функции
    void enableLowPass(bool enabled);
    void enableHighPass(bool enabled);
    void enableBandPass(bool enabled);
    void enableAllPass(bool enabled);

private:
    //==============================================================================
    // Базовый класс фильтра
    class Filter
    {
    public:
        virtual ~Filter() = default;
        virtual void prepare(double sampleRate) = 0;
        virtual void reset() = 0;
        virtual float process(float input) = 0;
        virtual void setFrequency(float frequency) = 0;
        virtual void setQ(float q) = 0;
    };

    //==============================================================================
    // Biquad фильтр
    class BiquadFilter : public Filter
    {
    public:
        enum class Type
        {
            LowPass,
            HighPass,
            BandPass,
            AllPass
        };

        BiquadFilter(Type type);
        ~BiquadFilter() override;

        void prepare(double sampleRate) override;
        void reset() override;
        float process(float input) override;
        void setFrequency(float frequency) override;
        void setQ(float q) override;

        void setType(Type type);
        void calculateCoefficients();

    private:
        Type type = Type::LowPass;
        double sampleRate = 44100.0;
        float frequency = 1000.0f;
        float q = 1.0f;

        // Коэффициенты фильтра
        float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f;
        float a0 = 1.0f, a1 = 0.0f, a2 = 0.0f;

        // Состояние фильтра
        float x1 = 0.0f, x2 = 0.0f;
        float y1 = 0.0f, y2 = 0.0f;
    };

    //==============================================================================
    // Состояние
    Parameters params;
    double sampleRate = 44100.0;
    int blockSize = 512;
    bool isPrepared = false;

    // Фильтры
    std::unique_ptr<BiquadFilter> lowPassFilter;
    std::unique_ptr<BiquadFilter> highPassFilter;
    std::unique_ptr<BiquadFilter> bandPassFilter;
    std::unique_ptr<BiquadFilter> allPassFilter;

    // Временные буферы
    std::vector<float> tempBuffer;

    //==============================================================================
    // Внутренние методы
    void initializeFilters();
    void updateFilterParameters();

    // JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FilterBank)
}; 