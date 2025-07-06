#include "FFTEngine.h"
#include "utils/MathUtils.h"
#include <algorithm>

//==============================================================================
FFTEngine::FFTEngine()
{
    // Инициализация параметров по умолчанию
    params.fftSize = 2048;
    params.hopSize = 512;
    params.windowType = 0;
    params.overlap = 0.75f;
    params.normalize = true;
    
    // Инициализация FFT планов
    fftPlan = nullptr;
    ifftPlan = nullptr;
}

FFTEngine::~FFTEngine()
{
    cleanupFFT();
}

//==============================================================================
void FFTEngine::prepare(int fftSize, double sampleRate)
{
    this->fftSize = fftSize;
    this->sampleRate = sampleRate;
    this->hopSize = static_cast<int>(fftSize * params.overlap);
    
    // Инициализация FFT
    initializeFFT();
    
    // Инициализация окон
    initializeWindows();
    
    // Инициализация буферов
    initializeBuffers();
    
    isPrepared = true;
}

void FFTEngine::reset()
{
    // Очистка буферов
    std::fill(fftBuffer.begin(), fftBuffer.end(), 0.0f);
    std::fill(fftSpectrum.begin(), fftSpectrum.end(), std::complex<float>(0.0f, 0.0f));
    std::fill(outputBuffer.begin(), outputBuffer.end(), 0.0f);
    std::fill(overlapBuffer.begin(), overlapBuffer.end(), 0.0f);
    
    outputIndex = 0;
}

//==============================================================================
void FFTEngine::performForwardFFT(const float* input, std::complex<float>* output)
{
    if (!isPrepared)
        return;
    
    // Копирование входных данных
    std::copy(input, input + fftSize, fftBuffer.data());
    
    // Применение окна
    applyWindowToBuffer(fftBuffer.data(), window);
    
    // Выполнение FFT (заглушка)
    // В реальной реализации:
    // fftwf_execute(fftPlan);
    
    // Копирование результата
    std::copy(fftSpectrum.begin(), fftSpectrum.end(), output);
    
    // Нормализация если включена
    if (params.normalize)
    {
        normalizeSpectrum(output, fftSize);
    }
}

void FFTEngine::performInverseFFT(const std::complex<float>* input, float* output)
{
    if (!isPrepared)
        return;
    
    // Копирование входных данных
    std::copy(input, input + fftSize, fftSpectrum.data());
    
    // Выполнение IFFT (заглушка)
    // В реальной реализации:
    // fftwf_execute(ifftPlan);
    
    // Копирование результата
    std::copy(fftBuffer.begin(), fftBuffer.begin() + fftSize, output);
    
    // Применение окна синтеза
    applyWindowToBuffer(output, synthesisWindow);
}

void FFTEngine::performSTFT(const float* input, std::complex<float>* output, int frameIndex)
{
    if (!isPrepared)
        return;
    
    // Заглушка для STFT
    // В реальной реализации:
    // 1. Извлечение кадра с учетом hop size
    // 2. Применение окна
    // 3. Выполнение FFT
    
    std::fill(output, output + fftSize, std::complex<float>(0.0f, 0.0f));
}

void FFTEngine::performISTFT(const std::complex<float>* input, float* output, int frameIndex)
{
    if (!isPrepared)
        return;
    
    // Заглушка для ISTFT
    // В реальной реализации:
    // 1. Выполнение IFFT
    // 2. Применение окна синтеза
    // 3. Overlap-add в выходной буфер
    
    std::fill(output, output + fftSize, 0.0f);
}

//==============================================================================
void FFTEngine::setParameters(const Parameters& newParams)
{
    params = newParams;
    
    // Обновление FFT размера если изменился
    if (params.fftSize != fftSize)
    {
        cleanupFFT();
        initializeFFT();
    }
    
    // Обновление hop size
    hopSize = static_cast<int>(params.fftSize * params.overlap);
    
    // Обновление окон
    initializeWindows();
}

//==============================================================================
void FFTEngine::createWindow(std::vector<float>& window, int size, int type)
{
    window.resize(size);
    
    switch (type)
    {
        case 0: // Hann
            for (int i = 0; i < size; ++i)
            {
                window[i] = 0.5f - 0.5f * MathUtils::fastCos(MathUtils::TWO_PI * i / (size - 1));
            }
            break;
            
        case 1: // Hamming
            for (int i = 0; i < size; ++i)
            {
                window[i] = 0.54f - 0.46f * MathUtils::fastCos(MathUtils::TWO_PI * i / (size - 1));
            }
            break;
            
        case 2: // Blackman
            for (int i = 0; i < size; ++i)
            {
                window[i] = 0.42f - 0.5f * MathUtils::fastCos(MathUtils::TWO_PI * i / (size - 1)) + 
                            0.08f * MathUtils::fastCos(2.0f * MathUtils::TWO_PI * i / (size - 1));
            }
            break;
            
        default:
            // По умолчанию Hann
            for (int i = 0; i < size; ++i)
            {
                window[i] = 0.5f - 0.5f * MathUtils::fastCos(MathUtils::TWO_PI * i / (size - 1));
            }
            break;
    }
}

void FFTEngine::applyWindow(float* buffer, int size)
{
    if (size != static_cast<int>(window.size()))
        return;
    
    for (int i = 0; i < size; ++i)
    {
        buffer[i] *= window[i];
    }
}

void FFTEngine::getMagnitude(const std::complex<float>* spectrum, float* magnitude, int numBins)
{
    for (int i = 0; i < numBins; ++i)
    {
        magnitude[i] = std::abs(spectrum[i]);
    }
}

void FFTEngine::getPhase(const std::complex<float>* spectrum, float* phase, int numBins)
{
    for (int i = 0; i < numBins; ++i)
    {
        phase[i] = std::arg(spectrum[i]);
    }
}

void FFTEngine::setMagnitudeAndPhase(std::complex<float>* spectrum, const float* magnitude, 
                                    const float* phase, int numBins)
{
    for (int i = 0; i < numBins; ++i)
    {
        spectrum[i] = std::polar(magnitude[i], phase[i]);
    }
}

//==============================================================================
void FFTEngine::initializeFFT()
{
    // Создание FFT планов (заглушка)
    // В реальной реализации:
    // fftPlan = fftwf_plan_dft_r2c_1d(fftSize, fftBuffer.data(), 
    //                                 reinterpret_cast<fftwf_complex*>(fftSpectrum.data()), FFTW_ESTIMATE);
    // ifftPlan = fftwf_plan_dft_c2r_1d(fftSize, reinterpret_cast<fftwf_complex*>(fftSpectrum.data()), 
    //                                  fftBuffer.data(), FFTW_ESTIMATE);
    
    // Инициализация буферов
    fftBuffer.resize(fftSize);
    fftSpectrum.resize(fftSize);
}

void FFTEngine::initializeWindows()
{
    window.resize(fftSize);
    synthesisWindow.resize(fftSize);
    
    // Создание окон
    createWindow(window, fftSize, params.windowType);
    createWindow(synthesisWindow, fftSize, params.windowType);
}

void FFTEngine::initializeBuffers()
{
    // Overlap-add буферы
    outputBuffer.resize(fftSize * 2);
    overlapBuffer.resize(fftSize);
    
    // Инициализация
    std::fill(outputBuffer.begin(), outputBuffer.end(), 0.0f);
    std::fill(overlapBuffer.begin(), overlapBuffer.end(), 0.0f);
    
    outputIndex = 0;
}

void FFTEngine::normalizeSpectrum(std::complex<float>* spectrum, int numBins)
{
    float scale = 1.0f / std::sqrt(static_cast<float>(numBins));
    
    for (int i = 0; i < numBins; ++i)
    {
        spectrum[i] *= scale;
    }
}

void FFTEngine::applyWindowToBuffer(float* buffer, const std::vector<float>& window)
{
    if (buffer == nullptr || window.empty())
        return;
    
    int size = std::min(static_cast<int>(window.size()), fftSize);
    
    for (int i = 0; i < size; ++i)
    {
        buffer[i] *= window[i];
    }
}

void FFTEngine::cleanupFFT()
{
    // Очистка FFTW планов (заглушка)
    // В реальной реализации:
    // if (fftPlan) fftwf_destroy_plan(fftPlan);
    // if (ifftPlan) fftwf_destroy_plan(ifftPlan);
    
    fftPlan = nullptr;
    ifftPlan = nullptr;
} 