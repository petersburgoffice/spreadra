#include "FilterBank.h"
#include "utils/MathUtils.h"
#include <algorithm>

//==============================================================================
FilterBank::FilterBank()
{
    // Инициализация параметров по умолчанию
    params.lowPassFreq = 20000.0f;
    params.highPassFreq = 20.0f;
    params.bandPassFreq = 1000.0f;
    params.bandPassQ = 1.0f;
    params.allPassFreq = 1000.0f;
    params.allPassQ = 1.0f;
    params.enableLowPass = false;
    params.enableHighPass = false;
    params.enableBandPass = false;
    params.enableAllPass = false;
}

FilterBank::~FilterBank()
{
}

//==============================================================================
void FilterBank::prepare(double sampleRate, int blockSize)
{
    this->sampleRate = sampleRate;
    this->blockSize = blockSize;
    
    // Инициализация фильтров
    initializeFilters();
    
    // Инициализация временных буферов
    tempBuffer.resize(blockSize);
    
    isPrepared = true;
}

void FilterBank::reset()
{
    // Сброс всех фильтров
    if (lowPassFilter) lowPassFilter->reset();
    if (highPassFilter) highPassFilter->reset();
    if (bandPassFilter) bandPassFilter->reset();
    if (allPassFilter) allPassFilter->reset();
    
    // Очистка временных буферов
    std::fill(tempBuffer.begin(), tempBuffer.end(), 0.0f);
}

//==============================================================================
void FilterBank::process(const float* input, float* output, int numSamples)
{
    if (!isPrepared || numSamples > blockSize)
        return;
    
    // Копирование входного сигнала
    std::copy(input, input + numSamples, output);
    
    // Применение фильтров в цепочке
    if (params.enableLowPass && lowPassFilter)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            output[i] = lowPassFilter->process(output[i]);
        }
    }
    
    if (params.enableHighPass && highPassFilter)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            output[i] = highPassFilter->process(output[i]);
        }
    }
    
    if (params.enableBandPass && bandPassFilter)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            output[i] = bandPassFilter->process(output[i]);
        }
    }
    
    if (params.enableAllPass && allPassFilter)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            output[i] = allPassFilter->process(output[i]);
        }
    }
}

void FilterBank::processStereo(const float* inputL, const float* inputR, 
                              float* outputL, float* outputR, int numSamples)
{
    // ЭКСТРЕННОЕ ИСПРАВЛЕНИЕ: Избегаем интерференции стерео каналов
    // Проблема: оба канала используют одни и те же фильтры
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
void FilterBank::setParameters(const Parameters& newParams)
{
    params = newParams;
    updateFilterParameters();
}

void FilterBank::setLowPassFrequency(float frequency)
{
    params.lowPassFreq = MathUtils::clamp(frequency, 20.0f, 20000.0f);
    if (lowPassFilter)
    {
        lowPassFilter->setFrequency(params.lowPassFreq);
    }
}

void FilterBank::setHighPassFrequency(float frequency)
{
    params.highPassFreq = MathUtils::clamp(frequency, 20.0f, 20000.0f);
    if (highPassFilter)
    {
        highPassFilter->setFrequency(params.highPassFreq);
    }
}

void FilterBank::setBandPassFrequency(float frequency)
{
    params.bandPassFreq = MathUtils::clamp(frequency, 20.0f, 20000.0f);
    if (bandPassFilter)
    {
        bandPassFilter->setFrequency(params.bandPassFreq);
    }
}

void FilterBank::setBandPassQ(float q)
{
    params.bandPassQ = MathUtils::clamp(q, 0.1f, 10.0f);
    if (bandPassFilter)
    {
        bandPassFilter->setQ(params.bandPassQ);
    }
}

void FilterBank::setAllPassFrequency(float frequency)
{
    params.allPassFreq = MathUtils::clamp(frequency, 20.0f, 20000.0f);
    if (allPassFilter)
    {
        allPassFilter->setFrequency(params.allPassFreq);
    }
}

void FilterBank::setAllPassQ(float q)
{
    params.allPassQ = MathUtils::clamp(q, 0.1f, 10.0f);
    if (allPassFilter)
    {
        allPassFilter->setQ(params.allPassQ);
    }
}

//==============================================================================
void FilterBank::enableLowPass(bool enabled)
{
    params.enableLowPass = enabled;
}

void FilterBank::enableHighPass(bool enabled)
{
    params.enableHighPass = enabled;
}

void FilterBank::enableBandPass(bool enabled)
{
    params.enableBandPass = enabled;
}

void FilterBank::enableAllPass(bool enabled)
{
    params.enableAllPass = enabled;
}

//==============================================================================
void FilterBank::initializeFilters()
{
    // Создание фильтров
    lowPassFilter = std::make_unique<BiquadFilter>(BiquadFilter::Type::LowPass);
    highPassFilter = std::make_unique<BiquadFilter>(BiquadFilter::Type::HighPass);
    bandPassFilter = std::make_unique<BiquadFilter>(BiquadFilter::Type::BandPass);
    allPassFilter = std::make_unique<BiquadFilter>(BiquadFilter::Type::AllPass);
    
    // Подготовка фильтров
    lowPassFilter->prepare(sampleRate);
    highPassFilter->prepare(sampleRate);
    bandPassFilter->prepare(sampleRate);
    allPassFilter->prepare(sampleRate);
    
    // Установка начальных параметров
    updateFilterParameters();
}

void FilterBank::updateFilterParameters()
{
    if (!isPrepared)
        return;
    
    // Обновление параметров фильтров
    if (lowPassFilter)
    {
        lowPassFilter->setFrequency(params.lowPassFreq);
    }
    
    if (highPassFilter)
    {
        highPassFilter->setFrequency(params.highPassFreq);
    }
    
    if (bandPassFilter)
    {
        bandPassFilter->setFrequency(params.bandPassFreq);
        bandPassFilter->setQ(params.bandPassQ);
    }
    
    if (allPassFilter)
    {
        allPassFilter->setFrequency(params.allPassFreq);
        allPassFilter->setQ(params.allPassQ);
    }
}

//==============================================================================
// BiquadFilter реализация
FilterBank::BiquadFilter::BiquadFilter(Type type)
    : type(type)
{
}

FilterBank::BiquadFilter::~BiquadFilter()
{
}

void FilterBank::BiquadFilter::prepare(double sampleRate)
{
    this->sampleRate = sampleRate;
    calculateCoefficients();
    reset();
}

void FilterBank::BiquadFilter::reset()
{
    x1 = x2 = 0.0f;
    y1 = y2 = 0.0f;
}

float FilterBank::BiquadFilter::process(float input)
{
    // Direct Form II реализация
    float w = input - a1 * x1 - a2 * x2;
    float output = b0 * w + b1 * x1 + b2 * x2;
    
    x2 = x1;
    x1 = w;
    y2 = y1;
    y1 = output;
    
    return output;
}

void FilterBank::BiquadFilter::setFrequency(float frequency)
{
    this->frequency = MathUtils::clamp(frequency, 20.0f, sampleRate / 2.0f);
    calculateCoefficients();
}

void FilterBank::BiquadFilter::setQ(float q)
{
    this->q = MathUtils::clamp(q, 0.1f, 10.0f);
    calculateCoefficients();
}

void FilterBank::BiquadFilter::setType(Type newType)
{
    type = newType;
    calculateCoefficients();
}

void FilterBank::BiquadFilter::calculateCoefficients()
{
    float w0 = 2.0f * MathUtils::PI * frequency / sampleRate;
    float alpha = MathUtils::fastSin(w0) / (2.0f * q);
    float cosw0 = MathUtils::fastCos(w0);
    
    switch (type)
    {
        case Type::LowPass:
        {
            float b0_denom = 1.0f + alpha;
            b0 = (1.0f - cosw0) / 2.0f / b0_denom;
            b1 = (1.0f - cosw0) / b0_denom;
            b2 = (1.0f - cosw0) / 2.0f / b0_denom;
            a0 = 1.0f;
            a1 = -2.0f * cosw0 / b0_denom;
            a2 = (1.0f - alpha) / b0_denom;
            break;
        }
        
        case Type::HighPass:
        {
            float b0_denom = 1.0f + alpha;
            b0 = (1.0f + cosw0) / 2.0f / b0_denom;
            b1 = -(1.0f + cosw0) / b0_denom;
            b2 = (1.0f + cosw0) / 2.0f / b0_denom;
            a0 = 1.0f;
            a1 = -2.0f * cosw0 / b0_denom;
            a2 = (1.0f - alpha) / b0_denom;
            break;
        }
        
        case Type::BandPass:
        {
            float b0_denom = 1.0f + alpha;
            b0 = alpha / b0_denom;
            b1 = 0.0f;
            b2 = -alpha / b0_denom;
            a0 = 1.0f;
            a1 = -2.0f * cosw0 / b0_denom;
            a2 = (1.0f - alpha) / b0_denom;
            break;
        }
        
        case Type::AllPass:
        {
            float b0_denom = 1.0f + alpha;
            b0 = (1.0f - alpha) / b0_denom;
            b1 = -2.0f * cosw0 / b0_denom;
            b2 = (1.0f + alpha) / b0_denom;
            a0 = 1.0f;
            a1 = -2.0f * cosw0 / b0_denom;
            a2 = (1.0f - alpha) / b0_denom;
            break;
        }
    }
} 