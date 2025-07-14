#include "ReverbAlgorithm.h"
#include "utils/MathUtils.h"
#include <algorithm>

//==============================================================================
ReverbAlgorithm::ReverbAlgorithm()
{
    // Инициализация параметров по умолчанию
    params.roomSize = 1000.0f;
    params.decayTime = 3.0f;
    params.dryWet = 50.0f;
}

ReverbAlgorithm::~ReverbAlgorithm()
{
}

//==============================================================================
void ReverbAlgorithm::prepare(double sampleRate, int blockSize)
{
    this->sampleRate = sampleRate;
    this->blockSize = blockSize;
    
    // Подготовка DSP компонентов
    reverbEngine.prepare(sampleRate, blockSize);
    filterBank.prepare(sampleRate, blockSize);
    
    // Инициализация временных буферов
    tempBuffer1.resize(blockSize);
    tempBuffer2.resize(blockSize);
    tempBuffer3.resize(blockSize);
    tempBufferL.resize(blockSize);
    tempBufferR.resize(blockSize);
    
    // Обновление параметров DSP
    updateDSPParameters();
    
    isPrepared = true;
}

void ReverbAlgorithm::reset()
{
    reverbEngine.reset();
    filterBank.reset();
    
    // Очистка буферов
    std::fill(tempBuffer1.begin(), tempBuffer1.end(), 0.0f);
    std::fill(tempBuffer2.begin(), tempBuffer2.end(), 0.0f);
    std::fill(tempBuffer3.begin(), tempBuffer3.end(), 0.0f);
    std::fill(tempBufferL.begin(), tempBufferL.end(), 0.0f);
    std::fill(tempBufferR.begin(), tempBufferR.end(), 0.0f);
}

//==============================================================================
void ReverbAlgorithm::process(const float* input, float* output, int numSamples)
{
    if (!isPrepared || numSamples > blockSize)
        return;
    
    // УПРОЩЕНО: всегда используем стерео обработку (дублируем моно на оба канала)
    processStereoInternal(input, input, output, tempBuffer1.data(), numSamples);
    
    // Берем только левый канал как результат моно
    // (правый канал в tempBuffer1 игнорируется)
}

void ReverbAlgorithm::processStereo(const float* inputL, const float* inputR, 
                                    float* outputL, float* outputR, int numSamples)
{
    if (!isPrepared || numSamples > blockSize)
        return;
    
    // Копирование входных сигналов
    std::copy(inputL, inputL + numSamples, tempBufferL.data());
    std::copy(inputR, inputR + numSamples, tempBufferR.data());
    
    // Обработка через DSP chain
    processStereoInternal(tempBufferL.data(), tempBufferR.data(), 
                         outputL, outputR, numSamples);
}

//==============================================================================
void ReverbAlgorithm::setParameters(const Parameters& newParams)
{
    params = newParams;
    updateDSPParameters();
}

void ReverbAlgorithm::setRoomSize(float roomSizeM2)
{
    params.roomSize = MathUtils::clamp(roomSizeM2, 10.0f, 10000.0f);
    reverbEngine.setRoomSize(params.roomSize);
}

void ReverbAlgorithm::setDecayTime(float decayTimeSeconds)
{
    params.decayTime = MathUtils::clamp(decayTimeSeconds, 0.1f, 20.0f);
    reverbEngine.setDecayTime(params.decayTime);
}

void ReverbAlgorithm::setDryWet(float dryWetPercent)
{
    params.dryWet = MathUtils::clamp(dryWetPercent, 0.0f, 100.0f);
}

void ReverbAlgorithm::setStereoWidth(float stereoWidthPercent)
{
    params.stereoWidth = MathUtils::clamp(stereoWidthPercent, 0.0f, 200.0f);
    reverbEngine.setStereoWidth(params.stereoWidth);
}

//==============================================================================
float ReverbAlgorithm::getCpuUsage() const
{
    // Простая оценка CPU usage на основе сложности алгоритмов
    float reverbUsage = 0.4f;
    float filterUsage = 0.05f;
    
    return reverbUsage + filterUsage;
}

float ReverbAlgorithm::getLatency() const
{
    // Расчет общей задержки
    float reverbLatency = 10.0f; // Минимальная задержка реверберации
    
    return reverbLatency;
}

void ReverbAlgorithm::getSpectrum(float* spectrum, int numBins)
{
    // Заполнение спектра нулями (заглушка)
    std::fill(spectrum, spectrum + numBins, 0.0f);
}

//==============================================================================
void ReverbAlgorithm::updateDSPParameters()
{
    if (!isPrepared)
        return;
    
    // Обновление параметров reverbix engine
    ReverbEngine::Parameters reverbParams;
    reverbParams.roomSize = params.roomSize;
    reverbParams.decayTime = params.decayTime;
    reverbParams.damping = params.damping;
    reverbParams.preDelay = params.preDelay;
    reverbParams.stereoWidth = params.stereoWidth;
    reverbEngine.setParameters(reverbParams);
    
    // Обновление параметров filter bank
    FilterBank::Parameters filterParams;
    filterParams.lowPassFreq = params.lowPassFreq;
    filterParams.highPassFreq = params.highPassFreq;
    filterParams.enableLowPass = params.enableLowPass;
    filterParams.enableHighPass = params.enableHighPass;
    filterBank.setParameters(filterParams);
}

//==============================================================================

void ReverbAlgorithm::processStereoInternal(const float* inputL, const float* inputR, 
                                            float* outputL, float* outputR, int numSamples)
{
    // Если dry/wet = 0%, только dry сигнал (оптимизация)
    if (params.dryWet <= 0.0f)
    {
        std::copy(inputL, inputL + numSamples, outputL);
        std::copy(inputR, inputR + numSamples, outputR);
        return;
    }
    
    // Обрабатываем wet сигнал через ReverbixEngine
    std::copy(inputL, inputL + numSamples, tempBufferL.data());
    std::copy(inputR, inputR + numSamples, tempBufferR.data());
    reverbEngine.processStereo(tempBufferL.data(), tempBufferR.data(), 
                              tempBuffer1.data(), tempBuffer2.data(), numSamples);
    
    // Простой микс dry/wet
    float dryMixGain = (100.0f - params.dryWet) / 100.0f;
    float wetMixGain = params.dryWet / 100.0f;
    
    for (int i = 0; i < numSamples; ++i)
    {
        float drySignalL = inputL[i];
        float drySignalR = inputR[i];
        float wetSignalL = tempBuffer1[i];
        float wetSignalR = tempBuffer2[i];
        outputL[i] = dryMixGain * drySignalL + wetMixGain * wetSignalL;
        outputR[i] = dryMixGain * drySignalR + wetMixGain * wetSignalR;
    }
    
    // Mid-Side преобразование для управления стерео шириной
    float widthFactor = params.stereoWidth / 100.0f; // 0.0 - 2.0
    
    for (int i = 0; i < numSamples; ++i)
    {
        // M/S кодирование
        float mid = (outputL[i] + outputR[i]) * 0.5f;
        float side = (outputL[i] - outputR[i]) * 0.5f;
        
        // Применение ширины к side сигналу
        side *= widthFactor;
        
        // M/S декодирование
        outputL[i] = mid + side;
        outputR[i] = mid - side;
    }
} 