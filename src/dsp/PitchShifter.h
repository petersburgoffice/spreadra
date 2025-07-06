#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>
#include <complex>
#include <fftw3.h>

/**
 * @brief PitchShifter на основе phase vocoder с поддержкой formant preservation
 * 
 * Поддерживает:
 * - Моно и стерео обработку
 * - Параметры: pitchShift (semitones), formantPreservation (%), quality, fftSize, hopSize, windowType
 * - Алгоритм: STFT → phase unwrapping → pitch shifting → formant envelope → ISTFT
 */
class PitchShifter
{
public:
    //==============================================================================
    PitchShifter();
    ~PitchShifter();

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
        float pitchShift = 12.0f;          // semitones, -24 to +24
        float formantPreservation = 80.0f; // %, 0-100
        float quality = 1.0f;              // 0.5-1.0
        int fftSize = 2048;                // 512, 1024, 2048, 4096
        float hopSize = 0.25f;             // 0.1-0.5 (доля fftSize)
        int windowType = 0;                // 0=Hann, 1=Hamming, 2=Blackman
    };

    void setParameters(const Parameters& newParams);
    const Parameters& getParameters() const { return params; }

    //==============================================================================
    // Индивидуальные параметры
    void setPitchShift(float semitones);
    void setFormantPreservation(float percent);
    void setQuality(float quality);
    void setFFTSize(int size);
    void setWindowType(int windowType);
    void enableFormantPreservation(bool enabled);

    static void getMagnitude(const std::complex<float>* spectrum, float* magnitude, int numBins);
    static void getPhase(const std::complex<float>* spectrum, float* phase, int numBins);
    static void setMagnitudeAndPhase(std::complex<float>* spectrum, const float* magnitude, const float* phase, int numBins);

private:
    //==============================================================================
    // Структура для STFT
    struct STFTFrame
    {
        std::vector<std::complex<float>> spectrum;
        std::vector<float> phase;
        std::vector<float> magnitude;
        std::vector<float> previousPhase;
    };

    //==============================================================================
    // Состояние
    Parameters params;
    double sampleRate = 44100.0;
    int blockSize = 512;
    bool isPrepared = false;

    // FFT планы
    fftwf_plan fftPlan = nullptr;
    fftwf_plan ifftPlan = nullptr;
    
    // Буферы FFT
    std::vector<float> fftBuffer;
    std::vector<std::complex<float>> fftSpectrum;
    
    // Окно
    std::vector<float> window;
    std::vector<float> synthesisWindow;
    
    // STFT параметры
    int fftSize = 2048;
    int hopSize = 512;
    int overlap = 4;
    int windowType = 0;
    
    // Буферы overlap-add
    std::vector<float> outputBuffer;
    std::vector<float> overlapBuffer;
    int outputIndex = 0;
    
    // Phase vocoder состояние
    std::vector<float> previousPhase;
    std::vector<float> phaseAccumulator;
    
    // Формантное сохранение
    std::vector<float> formantEnvelope;
    std::vector<float> shiftedFormantEnvelope;
    bool formantPreservationEnabled = true;

    //==============================================================================
    // Внутренние методы
    void initializeFFT();
    void initializeWindows();
    void initializeBuffers();
    
    // STFT обработка
    void performSTFT(const float* input, std::complex<float>* spectrum, int frameIndex);
    void performISTFT(std::complex<float>* spectrum, float* output, int frameIndex);
    
    // Phase vocoder
    void unwrapPhase(std::complex<float>* spectrum);
    void shiftPitch(std::complex<float>* spectrum);
    void preserveFormants(std::complex<float>* spectrum);
    
    // Формантное сохранение
    void calculateFormantEnvelope(const std::complex<float>* spectrum);
    void shiftFormantEnvelope(float pitchShiftRatio);
    void applyFormantCorrection(std::complex<float>* spectrum);
    
    // Утилиты
    void applyWindow(float* buffer, int size);
    void createWindow(std::vector<float>& window, int size, int type);
    float semitonesToRatio(float semitones);
    
    // Очистка ресурсов
    void cleanupFFT();

    // JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PitchShifter)
}; 