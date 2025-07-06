#include "PitchShifter.h"
#include "utils/MathUtils.h"
#include <algorithm>

//==============================================================================
PitchShifter::PitchShifter()
{
    // Инициализация параметров по умолчанию
    params.pitchShift = 12.0f;
    params.formantPreservation = 80.0f;
    params.quality = 1.0f;
    params.fftSize = 2048;
    params.hopSize = 0.25f;
    
    // Инициализация FFT планов
    fftPlan = nullptr;
    ifftPlan = nullptr;
}

PitchShifter::~PitchShifter()
{
    cleanupFFT();
}

//==============================================================================
void PitchShifter::prepare(double sampleRate, int blockSize)
{
    this->sampleRate = sampleRate;
    this->blockSize = blockSize;
    
    // Инициализация FFT
    initializeFFT();
    
    // Инициализация окон
    initializeWindows();
    
    // Инициализация буферов
    initializeBuffers();
    
    isPrepared = true;
}

void PitchShifter::reset()
{
    // Очистка буферов
    std::fill(fftBuffer.begin(), fftBuffer.end(), 0.0f);
    std::fill(fftSpectrum.begin(), fftSpectrum.end(), std::complex<float>(0.0f, 0.0f));
    std::fill(outputBuffer.begin(), outputBuffer.end(), 0.0f);
    std::fill(overlapBuffer.begin(), overlapBuffer.end(), 0.0f);
    std::fill(previousPhase.begin(), previousPhase.end(), 0.0f);
    std::fill(phaseAccumulator.begin(), phaseAccumulator.end(), 0.0f);
    
    outputIndex = 0;
}

//==============================================================================
void PitchShifter::process(const float* input, float* output, int numSamples)
{
    if (!isPrepared || numSamples > blockSize)
        return;
    
    // Простая заглушка - копирование входного сигнала
    std::copy(input, input + numSamples, output);
    
    // TODO: Реализовать полный phase vocoder алгоритм
    // 1. STFT analysis
    // 2. Phase unwrapping
    // 3. Pitch shifting
    // 4. Formant preservation
    // 5. ISTFT synthesis
}

void PitchShifter::processStereo(const float* inputL, const float* inputR, 
                                float* outputL, float* outputR, int numSamples)
{
    // ЭКСТРЕННОЕ ИСПРАВЛЕНИЕ: Избегаем интерференции стерео каналов
    // Проблема: оба канала используют одни и те же FFT буферы
    // Решение: Обрабатываем как МОНО, затем копируем на оба канала
    
    // Создаем моно сигнал (среднее L+R)
    std::vector<float> monoInput(numSamples);
    std::vector<float> monoOutput(numSamples);
    
    for (int i = 0; i < numSamples; ++i)
    {
        monoInput[i] = (inputL[i] + inputR[i]) * 0.5f;
    }
    
    // Обрабатываем как моно
    process(monoInput.data(), monoOutput.data(), numSamples);
    
    // Копируем результат на оба канала
    std::copy(monoOutput.begin(), monoOutput.end(), outputL);
    std::copy(monoOutput.begin(), monoOutput.end(), outputR);
}

//==============================================================================
void PitchShifter::setParameters(const Parameters& newParams)
{
    params = newParams;
    
    // Обновление FFT размера если изменился
    if (params.fftSize != fftSize)
    {
        cleanupFFT();
        initializeFFT();
    }
    
    // Обновление hop size
    hopSize = static_cast<int>(params.fftSize * params.hopSize);
    overlap = static_cast<int>(params.fftSize / hopSize);
}

void PitchShifter::setPitchShift(float semitones)
{
    params.pitchShift = MathUtils::clamp(semitones, -24.0f, 24.0f);
}

void PitchShifter::setFormantPreservation(float percent)
{
    params.formantPreservation = MathUtils::clamp(percent, 0.0f, 100.0f);
}

void PitchShifter::setQuality(float quality)
{
    params.quality = MathUtils::clamp(quality, 0.5f, 1.0f);
}

void PitchShifter::setFFTSize(int size)
{
    // Проверка что размер является степенью 2
    if (size != 512 && size != 1024 && size != 2048 && size != 4096)
        return;
    
    params.fftSize = size;
    cleanupFFT();
    initializeFFT();
}

//==============================================================================
void PitchShifter::enableFormantPreservation(bool enabled)
{
    formantPreservationEnabled = enabled;
}

void PitchShifter::setWindowType(int windowType)
{
    if (windowType >= 0 && windowType <= 2)
    {
        createWindow(window, fftSize, windowType);
        createWindow(synthesisWindow, fftSize, windowType);
    }
}

//==============================================================================
void PitchShifter::initializeFFT()
{
    fftSize = params.fftSize;
    hopSize = static_cast<int>(fftSize * params.hopSize);
    overlap = static_cast<int>(fftSize / hopSize);
    
    // Создание FFT планов
    fftBuffer.resize(fftSize);
    fftSpectrum.resize(fftSize);
    
    // Создание FFTW планов (заглушка)
    // В реальной реализации здесь будет:
    // fftPlan = fftwf_plan_dft_r2c_1d(fftSize, fftBuffer.data(), 
    //                                 reinterpret_cast<fftwf_complex*>(fftSpectrum.data()), FFTW_ESTIMATE);
    // ifftPlan = fftwf_plan_dft_c2r_1d(fftSize, reinterpret_cast<fftwf_complex*>(fftSpectrum.data()), 
    //                                  fftBuffer.data(), FFTW_ESTIMATE);
}

void PitchShifter::initializeWindows()
{
    window.resize(fftSize);
    synthesisWindow.resize(fftSize);
    
    // Создание окон по умолчанию (Hann)
    createWindow(window, fftSize, 0);
    createWindow(synthesisWindow, fftSize, 0);
}

void PitchShifter::initializeBuffers()
{
    // Overlap-add буферы
    outputBuffer.resize(fftSize * 2);
    overlapBuffer.resize(fftSize);
    
    // Phase vocoder состояние
    previousPhase.resize(fftSize / 2);
    phaseAccumulator.resize(fftSize / 2);
    
    // Формантное сохранение
    formantEnvelope.resize(fftSize / 2);
    shiftedFormantEnvelope.resize(fftSize / 2);
    
    // Инициализация
    std::fill(outputBuffer.begin(), outputBuffer.end(), 0.0f);
    std::fill(overlapBuffer.begin(), overlapBuffer.end(), 0.0f);
    std::fill(previousPhase.begin(), previousPhase.end(), 0.0f);
    std::fill(phaseAccumulator.begin(), phaseAccumulator.end(), 0.0f);
    std::fill(formantEnvelope.begin(), formantEnvelope.end(), 0.0f);
    std::fill(shiftedFormantEnvelope.begin(), shiftedFormantEnvelope.end(), 0.0f);
    
    outputIndex = 0;
}

//==============================================================================
void PitchShifter::performSTFT(const float* input, std::complex<float>* spectrum, int frameIndex)
{
    // Заглушка для STFT
    // В реальной реализации:
    // 1. Применение окна к входному сигналу
    // 2. Выполнение FFT
    // 3. Сохранение спектра
    
    std::fill(spectrum, spectrum + fftSize, std::complex<float>(0.0f, 0.0f));
}

void PitchShifter::performISTFT(std::complex<float>* spectrum, float* output, int frameIndex)
{
    // Заглушка для ISTFT
    // В реальной реализации:
    // 1. Выполнение IFFT
    // 2. Применение окна синтеза
    // 3. Overlap-add
    
    std::fill(output, output + fftSize, 0.0f);
}

//==============================================================================
void PitchShifter::unwrapPhase(std::complex<float>* spectrum)
{
    // Заглушка для phase unwrapping
    // В реальной реализации:
    // 1. Вычисление разности фаз
    // 2. Phase unwrapping
    // 3. Обновление накопленной фазы
}

void PitchShifter::shiftPitch(std::complex<float>* spectrum)
{
    // Заглушка для pitch shifting
    // В реальной реализации:
    // 1. Применение pitch shift ratio к фазе
    // 2. Обработка частотного wrapping
}

void PitchShifter::preserveFormants(std::complex<float>* spectrum)
{
    // Заглушка для formant preservation
    // В реальной реализации:
    // 1. Вычисление формантной огибающей
    // 2. Сдвиг формантной огибающей
    // 3. Применение коррекции
}

//==============================================================================
void PitchShifter::calculateFormantEnvelope(const std::complex<float>* spectrum)
{
    // Заглушка для вычисления формантной огибающей
    std::fill(formantEnvelope.begin(), formantEnvelope.end(), 1.0f);
}

void PitchShifter::shiftFormantEnvelope(float pitchShiftRatio)
{
    // Заглушка для сдвига формантной огибающей
    std::fill(shiftedFormantEnvelope.begin(), shiftedFormantEnvelope.end(), 1.0f);
}

void PitchShifter::applyFormantCorrection(std::complex<float>* spectrum)
{
    // Заглушка для применения формантной коррекции
}

//==============================================================================
void PitchShifter::applyWindow(float* buffer, int size)
{
    if (size != static_cast<int>(window.size()))
        return;
    
    for (int i = 0; i < size; ++i)
    {
        buffer[i] *= window[i];
    }
}

void PitchShifter::createWindow(std::vector<float>& window, int size, int type)
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

float PitchShifter::semitonesToRatio(float semitones)
{
    return MathUtils::semitonesToRatio(semitones);
}

//==============================================================================
void PitchShifter::getMagnitude(const std::complex<float>* spectrum, float* magnitude, int numBins)
{
    for (int i = 0; i < numBins; ++i)
    {
        magnitude[i] = std::abs(spectrum[i]);
    }
}

void PitchShifter::getPhase(const std::complex<float>* spectrum, float* phase, int numBins)
{
    for (int i = 0; i < numBins; ++i)
    {
        phase[i] = std::arg(spectrum[i]);
    }
}

void PitchShifter::setMagnitudeAndPhase(std::complex<float>* spectrum, const float* magnitude, 
                                       const float* phase, int numBins)
{
    for (int i = 0; i < numBins; ++i)
    {
        spectrum[i] = std::polar(magnitude[i], phase[i]);
    }
}

//==============================================================================
void PitchShifter::cleanupFFT()
{
    // Очистка FFTW планов (заглушка)
    // В реальной реализации:
    // if (fftPlan) fftwf_destroy_plan(fftPlan);
    // if (ifftPlan) fftwf_destroy_plan(ifftPlan);
    
    fftPlan = nullptr;
    ifftPlan = nullptr;
} 