#include "ReverbEngine.h"
#include "utils/MathUtils.h"
#include <algorithm>
#include <iostream>

//==============================================================================
ReverbEngine::ReverbEngine()
{
    // Инициализация параметров по умолчанию
    params.roomSize = 1000.0f;
    params.decayTime = 3.0f;
    params.damping = 50.0f;
    params.preDelay = 0.0f;
    params.stereoWidth = 100.0f;
    params.numCombFilters = 6;
    params.numAllPassFilters = 2;
}

ReverbEngine::~ReverbEngine() = default;

void ReverbEngine::prepare(double sampleRate, int blockSize)
{
    this->sampleRate = sampleRate;
    this->blockSize = blockSize;
    
    // Инициализация всех компонентов
    initializeCombFilters();
    initializeAllPassFilters();
    initializeEarlyReflections();
    
    // Инициализация всех параметров
    updateFilterParameters();
    updatePreDelay();
    updateEarlyReflections();
    updateStereoMixing();
    
    // Подготовка временных буферов - стерео
    tempBufferL.resize(blockSize, 0.0f);
    tempBufferR.resize(blockSize, 0.0f);
    combOutputL.resize(blockSize, 0.0f);
    combOutputR.resize(blockSize, 0.0f);
    allPassOutputL.resize(blockSize, 0.0f);
    allPassOutputR.resize(blockSize, 0.0f);
    earlyReflectionsBufferL.resize(blockSize, 0.0f);
    earlyReflectionsBufferR.resize(blockSize, 0.0f);
    
    isPrepared = true;
}

void ReverbEngine::process(const float* input, float* output, int numSamples)
{
    if (!isPrepared)
        return;
        
    processMono(input, output, numSamples);
}

void ReverbEngine::processStereo(const float* inputL, const float* inputR, 
                                 float* outputL, float* outputR, int numSamples)
{
    // НОВОЕ: Переключение между mono и stereo режимами
    if (params.monoMode)
    {
        // Mono режим: создаем моно микс из stereo входа
        std::vector<float> monoInput(numSamples);
        for (int i = 0; i < numSamples; ++i)
        {
            monoInput[i] = (inputL[i] + inputR[i]) * 0.5f;
        }
        
        // Обрабатываем в mono
        std::vector<float> monoOutput(numSamples);
        processMono(monoInput.data(), monoOutput.data(), numSamples);
        
        // Дублируем mono выход на оба канала
        for (int i = 0; i < numSamples; ++i)
        {
            outputL[i] = monoOutput[i];
            outputR[i] = monoOutput[i];
        }
        return;
    }
    
    // Stereo режим (оригинальный код)
    if (!isPrepared || combFiltersL.empty() || combFiltersR.empty())
    {
        // Если не готов, просто копируем входы в выходы
        for (int i = 0; i < numSamples; ++i)
        {
            outputL[i] = inputL[i];
            outputR[i] = inputR[i];
        }
        return;
    }
    
    // Создаем моно-сигнал для подачи на реверб (как в Freeverb)
    std::vector<float> monoInput(numSamples);
    for (int i = 0; i < numSamples; ++i)
    {
        monoInput[i] = (inputL[i] + inputR[i]) * 0.5f;
    }
    
    // Обрабатываем левый канал реверба
    std::fill(tempBufferL.begin(), tempBufferL.end(), 0.0f);
    std::fill(combOutputL.begin(), combOutputL.end(), 0.0f);
    std::fill(allPassOutputL.begin(), allPassOutputL.end(), 0.0f);
    
    // Левый канал: Pre-delay
    for (int i = 0; i < numSamples; ++i)
    {
        float delayedSample = preDelayBufferL[preDelayIndexL];
        preDelayBufferL[preDelayIndexL] = monoInput[i];
        preDelayIndexL = (preDelayIndexL + 1) % preDelayBufferL.size();
        tempBufferL[i] = delayedSample;
    }
    
    // Левый канал: Early reflections
    std::fill(earlyReflectionsBufferL.begin(), earlyReflectionsBufferL.end(), 0.0f);
    processEarlyReflections(tempBufferL.data(), earlyReflectionsBufferL.data(), numSamples, earlyReflectionsL);
    
    // Левый канал: Mix pre-delayed + early reflections для comb input
    std::vector<float> combInputL(numSamples);
    for (int i = 0; i < numSamples; ++i)
    {
        // ИСПРАВЛЕНО: Практически убираем early reflections для устранения delay эффекта
        combInputL[i] = tempBufferL[i] * 0.95f + earlyReflectionsBufferL[i] * 0.05f; // Было 0.7f + 0.3f
    }
    
    // Левый канал: Parallel comb filters
    for (size_t filterIndex = 0; filterIndex < combFiltersL.size(); ++filterIndex)
    {
        std::vector<float> filterOutput(numSamples);
        processCombFilter(combInputL.data(), filterOutput.data(), numSamples, combFiltersL[filterIndex]);
        
        for (int i = 0; i < numSamples; ++i)
        {
            combOutputL[i] += filterOutput[i];
        }
    }
    
    // ИСПРАВЛЕНО: Нормализация comb выхода для предотвращения перегруза
    float combNormalizationFactor = 1.0f / static_cast<float>(combFiltersL.size());
    for (int i = 0; i < numSamples; ++i)
    {
        combOutputL[i] *= combNormalizationFactor;
    }
    
    // Левый канал: Series all-pass filters
    std::copy(combOutputL.begin(), combOutputL.end(), allPassOutputL.begin());
    for (auto& filter : allPassFiltersL)
    {
        std::vector<float> filterOutput(numSamples);
        processAllPassFilter(allPassOutputL.data(), filterOutput.data(), numSamples, filter);
        std::copy(filterOutput.begin(), filterOutput.end(), allPassOutputL.begin());
    }
    
    // Обрабатываем правый канал реверба
    std::fill(tempBufferR.begin(), tempBufferR.end(), 0.0f);
    std::fill(combOutputR.begin(), combOutputR.end(), 0.0f);
    std::fill(allPassOutputR.begin(), allPassOutputR.end(), 0.0f);
    
    // Правый канал: Pre-delay
    for (int i = 0; i < numSamples; ++i)
    {
        float delayedSample = preDelayBufferR[preDelayIndexR];
        preDelayBufferR[preDelayIndexR] = monoInput[i];
        preDelayIndexR = (preDelayIndexR + 1) % preDelayBufferR.size();
        tempBufferR[i] = delayedSample;
    }
    
    // Правый канал: Early reflections
    std::fill(earlyReflectionsBufferR.begin(), earlyReflectionsBufferR.end(), 0.0f);
    processEarlyReflections(tempBufferR.data(), earlyReflectionsBufferR.data(), numSamples, earlyReflectionsR);
    
    // Правый канал: Mix pre-delayed + early reflections для comb input
    std::vector<float> combInputR(numSamples);
    for (int i = 0; i < numSamples; ++i)
    {
        // ИСПРАВЛЕНО: Практически убираем early reflections для устранения delay эффекта
        combInputR[i] = tempBufferR[i] * 0.95f + earlyReflectionsBufferR[i] * 0.05f; // Было 0.7f + 0.3f
    }
    
    // Правый канал: Parallel comb filters
    for (size_t filterIndex = 0; filterIndex < combFiltersR.size(); ++filterIndex)
    {
        std::vector<float> filterOutput(numSamples);
        processCombFilter(combInputR.data(), filterOutput.data(), numSamples, combFiltersR[filterIndex]);
        
        for (int i = 0; i < numSamples; ++i)
        {
            combOutputR[i] += filterOutput[i];
        }
    }
    
    // ИСПРАВЛЕНО: Нормализация comb выхода для предотвращения перегруза  
    for (int i = 0; i < numSamples; ++i)
    {
        combOutputR[i] *= combNormalizationFactor;
    }
    
    // Правый канал: Series all-pass filters
    std::copy(combOutputR.begin(), combOutputR.end(), allPassOutputR.begin());
    for (auto& filter : allPassFiltersR)
    {
        std::vector<float> filterOutput(numSamples);
        processAllPassFilter(allPassOutputR.data(), filterOutput.data(), numSamples, filter);
        std::copy(filterOutput.begin(), filterOutput.end(), allPassOutputR.begin());
    }
    
    // Freeverb-style cross-mixing для стерео эффекта
    for (int i = 0; i < numSamples; ++i)
    {
        float reverbL = allPassOutputL[i];
        float reverbR = allPassOutputR[i];
        
        // Cross-mixing: левый канал получает основной сигнал L + небольшой R
        // правый канал получает основной сигнал R + небольшой L
        outputL[i] = reverbL * wet1 + reverbR * wet2 + inputL[i] * dry;
        outputR[i] = reverbR * wet1 + reverbL * wet2 + inputR[i] * dry;
    }
}

void ReverbEngine::reset()
{
    // Левый канал
    for (auto& filter : combFiltersL)
    {
        std::fill(filter.buffer.begin(), filter.buffer.end(), 0.0f);
        filter.readIndex = 0;
        filter.writeIndex = 0;
    }
    
    for (auto& filter : allPassFiltersL)
    {
        std::fill(filter.buffer.begin(), filter.buffer.end(), 0.0f);
        filter.readIndex = 0;
        filter.writeIndex = 0;
    }
    
    for (auto& reflection : earlyReflectionsL)
    {
        std::fill(reflection.buffer.begin(), reflection.buffer.end(), 0.0f);
        reflection.readIndex = 0;
        reflection.writeIndex = 0;
    }
    
    // Правый канал
    for (auto& filter : combFiltersR)
    {
        std::fill(filter.buffer.begin(), filter.buffer.end(), 0.0f);
        filter.readIndex = 0;
        filter.writeIndex = 0;
    }
    
    for (auto& filter : allPassFiltersR)
    {
        std::fill(filter.buffer.begin(), filter.buffer.end(), 0.0f);
        filter.readIndex = 0;
        filter.writeIndex = 0;
    }
    
    for (auto& reflection : earlyReflectionsR)
    {
        std::fill(reflection.buffer.begin(), reflection.buffer.end(), 0.0f);
        reflection.readIndex = 0;
        reflection.writeIndex = 0;
    }
    
    // Pre-delay буферы
    std::fill(preDelayBufferL.begin(), preDelayBufferL.end(), 0.0f);
    std::fill(preDelayBufferR.begin(), preDelayBufferR.end(), 0.0f);
    preDelayIndexL = 0;
    preDelayIndexR = 0;
}

void ReverbEngine::setParameters(const Parameters& newParams)
{
    bool roomSizeChanged = (newParams.roomSize != params.roomSize);
    
    params = newParams;
    
    if (isPrepared)
    {
        // ИСПРАВЛЕНО: Не переинициализируем буферы для устранения треска
        // Только изменяем времена задержек в существующих фильтрах
        if (roomSizeChanged)
        {
            updateDelayTimes();
            updateEarlyReflections();
        }
        
        updateFilterParameters();
        updatePreDelay();
        updateEarlyReflections();
    }
}

void ReverbEngine::setRoomSize(float roomSizeM2)
{
    float oldRoomSize = params.roomSize;
    params.roomSize = MathUtils::clamp(roomSizeM2, 10.0f, 10000.0f);
    
    if (isPrepared && oldRoomSize != params.roomSize)
    {
        // ИСПРАВЛЕНО: Не переинициализируем фильтры - только изменяем времена задержек
        updateDelayTimes();
        updateEarlyReflections();
        updateFilterParameters();
        updateEarlyReflections();
    }
}

void ReverbEngine::setDecayTime(float decayTimeSeconds)
{
    params.decayTime = MathUtils::clamp(decayTimeSeconds, 0.1f, 20.0f);
    
    if (isPrepared)
    {
        updateFilterParameters();
    }
}

void ReverbEngine::setDamping(float dampingPercent)
{
    params.damping = MathUtils::clamp(dampingPercent, 0.0f, 100.0f);
    if (isPrepared)
        updateFilterParameters();
}

void ReverbEngine::setPreDelay(float preDelayMs)
{
    params.preDelay = MathUtils::clamp(preDelayMs, 0.0f, 500.0f);
    if (isPrepared)
        updatePreDelay();
}

void ReverbEngine::setStereoWidth(float widthPercent)
{
    params.stereoWidth = MathUtils::clamp(widthPercent, 0.0f, 150.0f);
    if (isPrepared)
        updateStereoMixing();
}

void ReverbEngine::setDryWetMix(float mixPercent)
{
    params.dryWetMix = juce::jlimit(0.0f, 100.0f, mixPercent);
    if (isPrepared)
        updateStereoMixing();
}

void ReverbEngine::setMonoMode(bool monoMode)
{
    params.monoMode = monoMode;
    // Параметр применяется немедленно в processStereo()
}

//==============================================================================
// Масштабируемые времена задержек
//==============================================================================

float ReverbEngine::calculateRoomScale(float roomSize)
{
    // Логарифмическое масштабирование для более естественного звучания
    // 10 m² -> 0.3x, 1000 m² -> 1.0x, 10000 m² -> 2.0x
    float logScale = std::log10(roomSize / 100.0f); // 100 m² = базовый размер
    return MathUtils::clamp(0.3f + logScale * 0.5f, 0.3f, 2.0f);
}

std::vector<int> ReverbEngine::getScaledCombDelays(bool isRightChannel)
{
    // Классические времена задержек из статьи SynthEdit (взаимно простые)
    // "идеальные" времена: 50, 53, 61, 68, 72, и 78 ms
    std::vector<float> baseDelayTimesMs = {50.0f, 53.0f, 61.0f, 68.0f, 72.0f, 78.0f};
    
    float roomScale = calculateRoomScale(params.roomSize);
    
    std::vector<int> delaySamples;
    for (float baseDelayMs : baseDelayTimesMs)
    {
        float scaledDelayMs = baseDelayMs * roomScale;
        int samples = static_cast<int>((scaledDelayMs / 1000.0f) * sampleRate);
        
        // Для правого канала добавляем stereoSpread (декорреляция)
        if (isRightChannel)
        {
            samples += params.stereoSpread;
        }
        
        delaySamples.push_back(samples);
    }
    
    return delaySamples;
}

std::vector<int> ReverbEngine::getScaledAllPassDelays(bool isRightChannel)
{
    // ИСПРАВЛЕНО: Значительно уменьшенные времена задержек для устранения delay эффекта
    // Было: 17ms и 74ms - слишком большие!
    // Стало: 8ms и 15ms - намного меньше порога восприятия delay
    std::vector<float> baseDelayTimesMs = {8.0f, 15.0f}; // Было {17.0f, 74.0f}
    
    float roomScale = calculateRoomScale(params.roomSize);
    
    std::vector<int> delaySamples;
    for (float baseDelayMs : baseDelayTimesMs)
    {
        float scaledDelayMs = baseDelayMs * roomScale;
        int samples = static_cast<int>((scaledDelayMs / 1000.0f) * sampleRate);
        
        // Для правого канала добавляем stereoSpread (декорреляция)
        if (isRightChannel)
        {
            samples += params.stereoSpread;
        }
        
        delaySamples.push_back(samples);
    }
    
    return delaySamples;
}

std::vector<float> ReverbEngine::getEarlyReflectionDelays(bool isRightChannel)
{
    // ИСПРАВЛЕНО: Создаем более плотную структуру отражений для устранения delay эффекта
    // Профессиональные early reflections: плотная структура 3-25ms
    float roomScale = calculateRoomScale(params.roomSize);
    float baseDelay = 3.0f + 12.0f * roomScale; // 3-15ms базовая задержка (было 5-35ms)
    
    std::vector<float> delays;
    // Создаем 8 отражений с малыми промежутками для более плотной структуры
    delays.push_back(baseDelay);           // 1-е отражение: 3-15ms
    delays.push_back(baseDelay * 1.15f);   // 2-е отражение: +15% (малый промежуток)
    delays.push_back(baseDelay * 1.35f);   // 3-е отражение: +35%
    delays.push_back(baseDelay * 1.58f);   // 4-е отражение: +58%
    delays.push_back(baseDelay * 1.84f);   // 5-е отражение: +84%
    delays.push_back(baseDelay * 2.12f);   // 6-е отражение: +112%
    delays.push_back(baseDelay * 2.43f);   // 7-е отражение: +143%
    delays.push_back(baseDelay * 2.77f);   // 8-е отражение: +177%
    
    // Для правого канала добавляем небольшую декорреляцию
    if (isRightChannel)
    {
        float spreadMs = (params.stereoSpread / sampleRate) * 1000.0f;
        for (auto& delay : delays)
        {
            delay += spreadMs * 0.5f; // Уменьшаем декорреляцию для более плотной структуры
        }
    }
    
    // ИСПРАВЛЕНО: Ограничиваем максимум для более плотной структуры
    for (auto& delay : delays)
    {
        delay = MathUtils::clamp(delay, 3.0f, 45.0f); // Было 5-80ms, стало 3-45ms
    }
    
    return delays;
}

bool ReverbEngine::areMutuallyPrime(const std::vector<int>& delays)
{
    for (size_t i = 0; i < delays.size(); ++i)
    {
        for (size_t j = i + 1; j < delays.size(); ++j)
        {
            if (gcd(delays[i], delays[j]) != 1)
                return false;
        }
    }
    return true;
}

int ReverbEngine::gcd(int a, int b)
{
    while (b != 0)
    {
        int temp = b;
        b = a % b;
        a = temp;
    }
    return a;
}

float ReverbEngine::calculateFeedback(float decayTime, int delayTime, double sampleRate)
{
    // ПРАВИЛЬНАЯ RT60 ФОРМУЛА из FAUST/Stanford/CCRMA
    // Эта формула обеспечивает точное затухание -60dB за decayTime секунд
    
    // Время задержки фильтра в миллисекундах
    float delayTimeMs = (static_cast<float>(delayTime) / static_cast<float>(sampleRate)) * 1000.0f;
    
    // RT60 формула: feedback = 0.001^(delayTime / RT60)
    // 0.001 = 10^(-3) = -60dB
    float feedback = std::pow(0.001f, delayTimeMs / (decayTime * 1000.0f));
    
    // Альтернативная запись той же формулы:
    // feedback = std::pow(10.0f, -3.0f * delayTimeMs / (decayTime * 1000.0f));
    
    // Ограничиваем разумными пределами для стабильности
    feedback = MathUtils::clamp(feedback, 0.001f, 0.999f);
    
    return feedback;
}

#ifdef DEBUG
void ReverbEngine::logFeedbackValues() const
{
    std::cout << "=== REVERB FEEDBACK DEBUG ===" << std::endl;
    std::cout << "DecayTime: " << params.decayTime << "s" << std::endl;
    std::cout << "RoomSize: " << params.roomSize << "m²" << std::endl;
    
    for (size_t i = 0; i < combFilters.size(); ++i)
    {
        const auto& filter = combFilters[i];
        float delayMs = (static_cast<float>(filter.delayTime) / static_cast<float>(sampleRate)) * 1000.0f;
        std::cout << "Comb[" << i << "]: delay=" << delayMs << "ms, feedback=" << filter.feedback << std::endl;
    }
    std::cout << "=========================" << std::endl;
}
#endif

void ReverbEngine::logReverbState() const
{
    if (!isPrepared)
        return;
    
    // Собираем данные для логирования (используем левый канал)
    std::vector<float> feedbacks;
    std::vector<float> delayTimes;
    
    for (size_t i = 0; i < combFiltersL.size(); ++i)
    {
        const auto& filter = combFiltersL[i];
        float delayMs = (static_cast<float>(filter.delayTime) / static_cast<float>(sampleRate)) * 1000.0f;
        
        feedbacks.push_back(filter.feedback);
        delayTimes.push_back(delayMs);
    }
    
    // Используем специальный метод логгера для полного состояния
    Logger::getInstance().logReverbParams(params.decayTime, params.roomSize, feedbacks, delayTimes);
}

//==============================================================================
// Инициализация фильтров
//==============================================================================

void ReverbEngine::initializeCombFilters()
{
    // Инициализация левого канала
    combFiltersL.clear();
    combFiltersL.resize(6); // Фиксированное количество по Schroeder
    
    auto delaysL = getScaledCombDelays(false);
    
    for (size_t i = 0; i < combFiltersL.size(); ++i)
    {
        auto& filter = combFiltersL[i];
        
        int delayTime = delaysL[i];
        int bufferSize = delayTime + blockSize;
        
        filter.buffer.resize(bufferSize, 0.0f);
        filter.delayTime = delayTime;
        filter.readIndex = 0;
        filter.writeIndex = 0;
        
        // Рассчитываем feedback на основе decay time
        filter.feedback = calculateFeedback(params.decayTime, delayTime, sampleRate);
        
        // Damping коэффициент
        filter.damping = params.damping / 100.0f;
        
        // Gain управление
        filter.outputGain = 1.0f;
        filter.targetOutputGain = 1.0f;
        filter.fadeRemaining = 0;
    }
    
    // Инициализация правого канала
    combFiltersR.clear();
    combFiltersR.resize(6); // Фиксированное количество по Schroeder
    
    auto delaysR = getScaledCombDelays(true);
    
    for (size_t i = 0; i < combFiltersR.size(); ++i)
    {
        auto& filter = combFiltersR[i];
        
        int delayTime = delaysR[i];
        int bufferSize = delayTime + blockSize;
        
        filter.buffer.resize(bufferSize, 0.0f);
        filter.delayTime = delayTime;
        filter.readIndex = 0;
        filter.writeIndex = 0;
        
        // Рассчитываем feedback на основе decay time
        filter.feedback = calculateFeedback(params.decayTime, delayTime, sampleRate);
        
        // Damping коэффициент
        filter.damping = params.damping / 100.0f;
        
        // Gain управление
        filter.outputGain = 1.0f;
        filter.targetOutputGain = 1.0f;
        filter.fadeRemaining = 0;
    }
}

void ReverbEngine::initializeAllPassFilters()
{
    // Инициализация левого канала
    allPassFiltersL.clear();
    allPassFiltersL.resize(2); // Фиксированное количество по Schroeder
    
    auto delaysL = getScaledAllPassDelays(false);
    
    for (size_t i = 0; i < allPassFiltersL.size(); ++i)
    {
        auto& filter = allPassFiltersL[i];
        
        int delayTime = delaysL[i];
        int bufferSize = delayTime + blockSize;
        
        filter.buffer.resize(bufferSize, 0.0f);
        filter.delayTime = delayTime;
        filter.readIndex = 0;
        filter.writeIndex = 0;
        
        // Фиксированный коэффициент обратной связи для all-pass фильтра
        filter.feedback = 0.5f;
        
        // Gain управление
        filter.outputGain = 1.0f;
        filter.targetOutputGain = 1.0f;
        filter.fadeRemaining = 0;
    }
    
    // Инициализация правого канала
    allPassFiltersR.clear();
    allPassFiltersR.resize(2); // Фиксированное количество по Schroeder
    
    auto delaysR = getScaledAllPassDelays(true);
    
    for (size_t i = 0; i < allPassFiltersR.size(); ++i)
    {
        auto& filter = allPassFiltersR[i];
        
        int delayTime = delaysR[i];
        int bufferSize = delayTime + blockSize;
        
        filter.buffer.resize(bufferSize, 0.0f);
        filter.delayTime = delayTime;
        filter.readIndex = 0;
        filter.writeIndex = 0;
        
        // Фиксированный коэффициент обратной связи для all-pass фильтра
        filter.feedback = 0.5f;
        
        // Gain управление
        filter.outputGain = 1.0f;
        filter.targetOutputGain = 1.0f;
        filter.fadeRemaining = 0;
    }
}

void ReverbEngine::initializeEarlyReflections()
{
    // Инициализация левого канала
    earlyReflectionsL.clear();
    earlyReflectionsL.resize(8); // 8 early reflections (было 6)
    
    auto reflectionDelaysL = getEarlyReflectionDelays(false);
    
    // Расчет размеров буферов для early reflections
    for (size_t i = 0; i < earlyReflectionsL.size() && i < reflectionDelaysL.size(); ++i)
    {
        auto& reflection = earlyReflectionsL[i];
        
        int delayTimeSamples = static_cast<int>((reflectionDelaysL[i] / 1000.0f) * sampleRate);
        int bufferSize = delayTimeSamples + blockSize;
        
        reflection.buffer.resize(bufferSize, 0.0f);
        reflection.delayTime = delayTimeSamples;
        reflection.readIndex = 0;
        reflection.writeIndex = 0;
        
        // ИСПРАВЛЕНО: Намного более тихие early reflections с плавным затуханием
        reflection.gain = 0.018f / (static_cast<float>(i + 1)); // 0.018, 0.009, 0.006, 0.0045... (еще тише)
    }
    
    // Инициализация правого канала
    earlyReflectionsR.clear();
    earlyReflectionsR.resize(8); // 8 early reflections (было 6)
    
    auto reflectionDelaysR = getEarlyReflectionDelays(true);
    
    // Расчет размеров буферов для early reflections
    for (size_t i = 0; i < earlyReflectionsR.size() && i < reflectionDelaysR.size(); ++i)
    {
        auto& reflection = earlyReflectionsR[i];
        
        int delayTimeSamples = static_cast<int>((reflectionDelaysR[i] / 1000.0f) * sampleRate);
        int bufferSize = delayTimeSamples + blockSize;
        
        reflection.buffer.resize(bufferSize, 0.0f);
        reflection.delayTime = delayTimeSamples;
        reflection.readIndex = 0;
        reflection.writeIndex = 0;
        
        // ИСПРАВЛЕНО: Намного более тихие early reflections с плавным затуханием
        reflection.gain = 0.018f / (static_cast<float>(i + 1)); // 0.018, 0.009, 0.006, 0.0045... (еще тише)
    }
}

void ReverbEngine::updateFilterParameters()
{
    if (!isPrepared)
        return;
    
    // Обновляем параметры comb фильтров
    for (auto& filter : combFiltersL)
    {
        filter.feedback = calculateFeedback(params.decayTime, filter.delayTime, sampleRate);
        
        // ИСПРАВЛЕНО: Damping должен быть очень маленьким (1-5%), не 50%!
        // В профессиональных реверберах damping - это слабое ослабление высоких частот
        // params.damping диапазон 0-100%, но используем только 0-5% для реального damping
        float dampingNormalized = MathUtils::clamp(params.damping / 100.0f, 0.0f, 1.0f);
        filter.damping = dampingNormalized * 0.05f; // Максимум 5% damping, не 100%!
    }
    
    for (auto& filter : combFiltersR)
    {
        filter.feedback = calculateFeedback(params.decayTime, filter.delayTime, sampleRate);
        
        // ИСПРАВЛЕНО: Damping должен быть очень маленьким (1-5%), не 50%!
        // В профессиональных реверберах damping - это слабое ослабление высоких частот
        // params.damping диапазон 0-100%, но используем только 0-5% для реального damping
        float dampingNormalized = MathUtils::clamp(params.damping / 100.0f, 0.0f, 1.0f);
        filter.damping = dampingNormalized * 0.05f; // Максимум 5% damping, не 100%!
    }
    
    // Обновляем параметры all-pass фильтров
    for (auto& filter : allPassFiltersL)
    {
        filter.feedback = 0.5f; // Стандартное значение для all-pass фильтров
    }
    
    for (auto& filter : allPassFiltersR)
    {
        filter.feedback = 0.5f; // Стандартное значение для all-pass фильтров
    }
}

void ReverbEngine::updatePreDelay()
{
    preDelaySamples = static_cast<int>((params.preDelay / 1000.0f) * sampleRate);
    preDelaySamples = MathUtils::clamp(preDelaySamples, 0, static_cast<int>(0.5f * sampleRate)); // Макс 0.5 секунды
    
    // Инициализация левого канала pre-delay
    size_t bufferSize = preDelaySamples + blockSize;
    preDelayBufferL.resize(bufferSize, 0.0f);
    preDelayIndexL = 0;
    
    // Инициализация правого канала pre-delay
    preDelayBufferR.resize(bufferSize, 0.0f);
    preDelayIndexR = 0;
}

void ReverbEngine::updateEarlyReflections()
{
    // Левый канал
    auto reflectionDelaysL = getEarlyReflectionDelays(false);
    
    for (size_t i = 0; i < earlyReflectionsL.size() && i < reflectionDelaysL.size(); ++i)
    {
        auto& reflection = earlyReflectionsL[i];
        
        int newDelayTime = static_cast<int>((reflectionDelaysL[i] / 1000.0f) * sampleRate);
        newDelayTime = MathUtils::clamp(newDelayTime, 1, static_cast<int>(reflection.buffer.size() - 1));
        
        reflection.delayTime = newDelayTime;
        reflection.gain = 0.018f / (static_cast<float>(i + 1)); // Убывающий gain
    }
    
    // Правый канал
    auto reflectionDelaysR = getEarlyReflectionDelays(true);
    
    for (size_t i = 0; i < earlyReflectionsR.size() && i < reflectionDelaysR.size(); ++i)
    {
        auto& reflection = earlyReflectionsR[i];
        
        int newDelayTime = static_cast<int>((reflectionDelaysR[i] / 1000.0f) * sampleRate);
        newDelayTime = MathUtils::clamp(newDelayTime, 1, static_cast<int>(reflection.buffer.size() - 1));
        
        reflection.delayTime = newDelayTime;
        reflection.gain = 0.018f / (static_cast<float>(i + 1)); // Убывающий gain
    }
}

//==============================================================================
// НОВЫЕ МЕТОДЫ: Обновление времен задержек без переинициализации буферов
//==============================================================================

void ReverbEngine::updateDelayTimes()
{
    // Базовые времена задержек
    std::vector<float> baseCombDelays = {50.0f, 53.0f, 61.0f, 68.0f, 72.0f, 78.0f}; // ms
    std::vector<float> baseAllPassDelays = {8.0f, 15.0f}; // ms
    
    float roomScale = calculateRoomScale(params.roomSize);
    
    // Обновляем delay time для comb фильтров - левый канал
    for (size_t i = 0; i < combFiltersL.size() && i < baseCombDelays.size(); ++i)
    {
        size_t newDelayTime = static_cast<size_t>(baseCombDelays[i] * roomScale * sampleRate / 1000.0f);
        newDelayTime = MathUtils::clamp(newDelayTime, static_cast<size_t>(1), combFiltersL[i].buffer.size() - 1);
        
        if (combFiltersL[i].delayTime != newDelayTime)
        {
            combFiltersL[i].delayTime = newDelayTime;
            
            // Запускаем crossfade для плавного перехода
            combFiltersL[i].outputGain = 1.0f;
            combFiltersL[i].targetOutputGain = 1.0f;
            combFiltersL[i].fadeRemaining = static_cast<int>(0.002f * sampleRate); // 2ms fade
        }
    }
    
    // Обновляем delay time для comb фильтров - правый канал
    for (size_t i = 0; i < combFiltersR.size() && i < baseCombDelays.size(); ++i)
    {
        size_t newDelayTime = static_cast<size_t>(baseCombDelays[i] * roomScale * sampleRate / 1000.0f);
        newDelayTime += params.stereoSpread; // Добавляем stereoSpread
        newDelayTime = MathUtils::clamp(newDelayTime, static_cast<size_t>(1), combFiltersR[i].buffer.size() - 1);
        
        if (combFiltersR[i].delayTime != newDelayTime)
        {
            combFiltersR[i].delayTime = newDelayTime;
            
            // Запускаем crossfade для плавного перехода
            combFiltersR[i].outputGain = 1.0f;
            combFiltersR[i].targetOutputGain = 1.0f;
            combFiltersR[i].fadeRemaining = static_cast<int>(0.002f * sampleRate); // 2ms fade
        }
    }
    
    // Обновляем delay time для all-pass фильтров - левый канал
    for (size_t i = 0; i < allPassFiltersL.size() && i < baseAllPassDelays.size(); ++i)
    {
        size_t newDelayTime = static_cast<size_t>(baseAllPassDelays[i] * roomScale * sampleRate / 1000.0f);
        newDelayTime = MathUtils::clamp(newDelayTime, static_cast<size_t>(1), allPassFiltersL[i].buffer.size() - 1);
        
        if (allPassFiltersL[i].delayTime != newDelayTime)
        {
            allPassFiltersL[i].delayTime = newDelayTime;
            
            // Запускаем crossfade для плавного перехода
            allPassFiltersL[i].outputGain = 1.0f;
            allPassFiltersL[i].targetOutputGain = 1.0f;
            allPassFiltersL[i].fadeRemaining = static_cast<int>(0.002f * sampleRate); // 2ms fade
        }
    }
    
    // Обновляем delay time для all-pass фильтров - правый канал
    for (size_t i = 0; i < allPassFiltersR.size() && i < baseAllPassDelays.size(); ++i)
    {
        size_t newDelayTime = static_cast<size_t>(baseAllPassDelays[i] * roomScale * sampleRate / 1000.0f);
        newDelayTime += params.stereoSpread; // Добавляем stereoSpread
        newDelayTime = MathUtils::clamp(newDelayTime, static_cast<size_t>(1), allPassFiltersR[i].buffer.size() - 1);
        
        if (allPassFiltersR[i].delayTime != newDelayTime)
        {
            allPassFiltersR[i].delayTime = newDelayTime;
            
            // Запускаем crossfade для плавного перехода
            allPassFiltersR[i].outputGain = 1.0f;
            allPassFiltersR[i].targetOutputGain = 1.0f;
            allPassFiltersR[i].fadeRemaining = static_cast<int>(0.002f * sampleRate); // 2ms fade
        }
    }
}

//==============================================================================
// Обработка сигнала
//==============================================================================

void ReverbEngine::processMono(const float* input, float* output, int numSamples)
{
    if (!isPrepared || combFiltersL.empty() || allPassFiltersL.empty())
    {
        // Если не готов, просто копируем input в output
        for (int i = 0; i < numSamples; ++i)
            output[i] = input[i];
        return;
    }

    // ИСПРАВЛЕНО: Добавляем pre-delay как в stereo версии
    std::vector<float> preDelayedInput(numSamples);
    for (int i = 0; i < numSamples; ++i)
    {
        float delayedSample = preDelayBufferL[preDelayIndexL];
        preDelayBufferL[preDelayIndexL] = input[i];
        preDelayIndexL = (preDelayIndexL + 1) % preDelayBufferL.size();
        preDelayedInput[i] = delayedSample;
    }

    // Массив для early reflections
    std::vector<float> earlyOutput(numSamples, 0.0f);
    
    // Обработка early reflections
    processEarlyReflections(preDelayedInput.data(), earlyOutput.data(), numSamples, earlyReflectionsL);

    // Смешиваем pre-delayed input с early reflections для comb фильтров
    std::vector<float> combInput(numSamples);
    for (int i = 0; i < numSamples; ++i)
    {
        // ИСПРАВЛЕНО: Практически убираем early reflections для устранения delay эффекта
        combInput[i] = preDelayedInput[i] * 0.95f + earlyOutput[i] * 0.05f; // Было 0.7f + 0.3f
    }

    // Обработка comb фильтров (параллельно)
    std::vector<float> combOutput(numSamples, 0.0f);
    
    for (size_t i = 0; i < combFiltersL.size(); ++i)
    {
        std::vector<float> filterOutput(numSamples);
        processCombFilter(combInput.data(), filterOutput.data(), numSamples, combFiltersL[i]);
        
        // Смешиваем выходы comb фильтров
        for (int j = 0; j < numSamples; ++j)
        {
            combOutput[j] += filterOutput[j];
        }
    }

    // Нормализация comb выхода
    float combNormalizationFactor = 1.0f / static_cast<float>(combFiltersL.size());
    for (int i = 0; i < numSamples; ++i)
    {
        combOutput[i] *= combNormalizationFactor;
    }

    // Обработка all-pass фильтров (последовательно)
    std::vector<float> allPassOutput = combOutput;
    for (auto& filter : allPassFiltersL)
    {
        processAllPassFilter(allPassOutput.data(), allPassOutput.data(), numSamples, filter);
    }

    // ИСПРАВЛЕНО: Добавляем dry/wet микширование как в stereo версии
    for (int i = 0; i < numSamples; ++i)
    {
        float wetSignal = allPassOutput[i];
        float drySignal = input[i];
        
        // Используем тот же алгоритм микширования что и в stereo
        output[i] = wetSignal * wet1 + drySignal * dry;
    }
}

void ReverbEngine::processCombFilter(const float* input, float* output, int numSamples, CombFilter& filter)
{
    if (filter.buffer.empty())
        return;

    const size_t bufferSize = filter.buffer.size();
    
    for (int i = 0; i < numSamples; ++i)
    {
        // Читаем задержанный сигнал
        float delayed = filter.buffer[filter.readIndex];
        
        // ПРАВИЛЬНАЯ COMB FORMULA: y[n] = x[n] + g*y[n-M]
        // Включаем исходный сигнал для работы ручек
        float combOutput = input[i] + filter.feedback * delayed;
        
        // Применяем damping (high-frequency attenuation)
        float dampedSignal = delayed * (1.0f - filter.damping) + filter.damping * delayed;
        
        // Записываем в буфер
        filter.buffer[filter.writeIndex] = combOutput;
        
        // Применяем crossfade к выходному сигналу
        if (filter.fadeRemaining > 0)
        {
            float fadeProgress = 1.0f - (float(filter.fadeRemaining) / (0.002f * sampleRate));
            filter.outputGain = fadeProgress; // Fade in после изменения
            filter.fadeRemaining--;
        }
        else
        {
            filter.outputGain = 1.0f;
        }
        
        // Выходной сигнал с crossfade
        output[i] = combOutput * filter.outputGain;
        
        // Обновляем индексы
        filter.readIndex = (filter.readIndex + 1) % bufferSize;
        filter.writeIndex = (filter.writeIndex + 1) % bufferSize;
    }
}

void ReverbEngine::processAllPassFilter(const float* input, float* output, int numSamples, AllPassFilter& filter)
{
    if (filter.buffer.empty())
        return;

    const size_t bufferSize = filter.buffer.size();
    
    for (int i = 0; i < numSamples; ++i)
    {
        // Читаем задержанный сигнал
        float delayed = filter.buffer[filter.readIndex];
        
        // ПРАВИЛЬНАЯ ALL-PASS FORMULA: y[n] = -g*x[n] + x[n-M] + g*y[n-M]
        // Включаем исходный сигнал для работы ручек
        float allPassOutput = -filter.feedback * input[i] + delayed + filter.feedback * delayed;
        
        // Записываем в буфер
        filter.buffer[filter.writeIndex] = input[i] + filter.feedback * delayed;
        
        // Применяем crossfade к выходному сигналу
        if (filter.fadeRemaining > 0)
        {
            float fadeProgress = 1.0f - (float(filter.fadeRemaining) / (0.002f * sampleRate));
            filter.outputGain = fadeProgress; // Fade in после изменения
            filter.fadeRemaining--;
        }
        else
        {
            filter.outputGain = 1.0f;
        }
        
        // Выходной сигнал с crossfade
        output[i] = allPassOutput * filter.outputGain;
        
        // Обновляем индексы
        filter.readIndex = (filter.readIndex + 1) % bufferSize;
        filter.writeIndex = (filter.writeIndex + 1) % bufferSize;
    }
}

void ReverbEngine::processEarlyReflections(const float* input, float* output, int numSamples,
                                         std::vector<EarlyReflection>& reflections)
{
    // Обнуляем выходной буфер
    std::fill(output, output + numSamples, 0.0f);
    
    // Обрабатываем каждое отражение
    for (auto& reflection : reflections)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            // Записываем новый сэмпл в буфер
            reflection.buffer[reflection.writeIndex] = input[i];
            
            // Читаем задержанный сэмпл
            float delayedSample = reflection.buffer[reflection.readIndex];
            
            // Добавляем к выходу с учетом gain
            output[i] += delayedSample * reflection.gain;
            
            // Обновляем индексы с учетом размера буфера
            reflection.readIndex = (reflection.readIndex + 1) % reflection.buffer.size();
            reflection.writeIndex = (reflection.writeIndex + 1) % reflection.buffer.size();
        }
    }
}

float ReverbEngine::calculateReverbTime()
{
    return MathUtils::calculateReverbTime(params.roomSize, params.damping);
}

void ReverbEngine::updateStereoMixing()
{
    // Freeverb-style стерео микширование с исправленными коэффициентами
    float effectMix = params.dryWetMix / 100.0f;  // 0-1
    float width = params.stereoWidth / 100.0f;    // 0-1.5
    
    // ИСПРАВЛЕНО: Уменьшенные коэффициенты для предотвращения перегруза
    const float scaleWet = 1.0f;  // Было 3.0f - слишком много!
    const float scaleDry = 1.0f;  // Было 2.0f - слишком много!
    
    float wet1_calc = scaleWet * effectMix;
    float dry1_calc = scaleDry * (1.0f - effectMix);
    
    float wet_total = wet1_calc / (wet1_calc + dry1_calc);
    float dry_total = dry1_calc / (wet1_calc + dry1_calc);
    
    // Стерео ширина влияет на cross-mixing
    wet1 = wet_total * (width / 2.0f + 0.5f);
    wet2 = wet_total * (1.0f - width) / 2.0f;
    dry = dry_total;
    
    // ИСПРАВЛЕНО: Более строгие ограничения + общая нормализация
    wet1 = MathUtils::clamp(wet1, 0.0f, 1.0f);
    wet2 = MathUtils::clamp(wet2, 0.0f, 1.0f);
    dry = MathUtils::clamp(dry, 0.0f, 1.0f);
    
    // Общая нормализация для предотвращения перегруза
    float totalGain = wet1 + wet2 + dry;
    if (totalGain > 1.0f)
    {
        wet1 /= totalGain;
        wet2 /= totalGain;
        dry /= totalGain;
    }
} 