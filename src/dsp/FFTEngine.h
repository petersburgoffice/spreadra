#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>
#include <complex>
#include <fftw3.h>

/**
 * @brief Движок быстрого преобразования Фурье (FFT)
 * 
 * Обертка над FFTW3 библиотекой для выполнения FFT и IFFT операций.
 * Поддерживает различные размеры FFT и оптимизирована для реального времени.
 */
class FFTEngine
{
public:
    //==============================================================================
    FFTEngine();
    ~FFTEngine();

    //==============================================================================
    // Инициализация
    void prepare(int fftSize, double sampleRate);
    void reset();

    //==============================================================================
    // FFT операции
    void performForwardFFT(const float* input, std::complex<float>* output);
    void performInverseFFT(const std::complex<float>* input, float* output);
    
    // STFT операции
    void performSTFT(const float* input, std::complex<float>* output, int frameIndex);
    void performISTFT(const std::complex<float>* input, float* output, int frameIndex);

    //==============================================================================
    // Параметры
    struct Parameters
    {
        int fftSize = 2048;              // размер FFT (512, 1024, 2048, 4096)
        int hopSize = 512;               // размер hop (обычно fftSize/4)
        int windowType = 0;              // 0=Hanning, 1=Hamming, 2=Blackman
        float overlap = 0.75f;           // overlap factor (0.5-0.9)
        bool normalize = true;           // нормализация FFT
    };

    void setParameters(const Parameters& newParams);
    const Parameters& getParameters() const { return params; }

    //==============================================================================
    // Утилиты
    int getFFTSize() const { return fftSize; }
    int getHopSize() const { return hopSize; }
    double getSampleRate() const { return sampleRate; }
    
    // Окна
    void createWindow(std::vector<float>& window, int size, int type);
    void applyWindow(float* buffer, int size);
    
    // Магнитуда и фаза
    void getMagnitude(const std::complex<float>* spectrum, float* magnitude, int numBins);
    void getPhase(const std::complex<float>* spectrum, float* phase, int numBins);
    void setMagnitudeAndPhase(std::complex<float>* spectrum, const float* magnitude, 
                             const float* phase, int numBins);

private:
    //==============================================================================
    // Состояние
    Parameters params;
    int fftSize = 2048;
    int hopSize = 512;
    double sampleRate = 44100.0;
    bool isPrepared = false;

    // FFTW планы
    fftwf_plan fftPlan = nullptr;
    fftwf_plan ifftPlan = nullptr;
    
    // Буферы
    std::vector<float> fftBuffer;
    std::vector<std::complex<float>> fftSpectrum;
    std::vector<float> window;
    std::vector<float> synthesisWindow;
    
    // Overlap-add буферы
    std::vector<float> outputBuffer;
    std::vector<float> overlapBuffer;
    int outputIndex = 0;

    //==============================================================================
    // Внутренние методы
    void initializeFFT();
    void initializeWindows();
    void initializeBuffers();
    
    // Утилиты
    void normalizeSpectrum(std::complex<float>* spectrum, int numBins);
    void applyWindowToBuffer(float* buffer, const std::vector<float>& window);
    
    // Очистка ресурсов
    void cleanupFFT();

    // JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FFTEngine)
}; 