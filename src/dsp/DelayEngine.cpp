#include "DelayEngine.h"
#include "utils/MathUtils.h"
#include <algorithm>

//==============================================================================
DelayEngine::DelayEngine()
{
    // Инициализация параметров по умолчанию
    params.delayTime = 500.0f;       // 500ms задержка
    params.feedback = 50.0f;         // 50% feedback (не 120%!)
    params.diffusion = 50.0f;
    params.modulation = 0.0f;
    params.modulationDepth = 0.0f;
    params.numDelayLines = 4;
}

DelayEngine::~DelayEngine()
{
}

//==============================================================================
void DelayEngine::prepare(double sampleRate, int blockSize)
{
    this->sampleRate = sampleRate;
    this->blockSize = blockSize;
    
    // Инициализация линий задержки
    initializeDelayLines();
    
    // Инициализация матрицы обратной связи
    initializeFeedbackMatrix();
    
    // Инициализация временных буферов
    tempBuffer.resize(blockSize);
    modulationBuffer.resize(blockSize);
    
    isPrepared = true;
}

void DelayEngine::reset()
{
    // Сброс всех линий задержки
    for (auto& line : delayLines)
    {
        std::fill(line.buffer.begin(), line.buffer.end(), 0.0f);
        line.writeIndex = 0;
        line.modulationPhase = 0.0f;
    }
    
    // Очистка временных буферов
    std::fill(tempBuffer.begin(), tempBuffer.end(), 0.0f);
    std::fill(modulationBuffer.begin(), modulationBuffer.end(), 0.0f);
}

//==============================================================================
void DelayEngine::process(const float* input, float* output, int numSamples)
{
    if (!isPrepared || numSamples > blockSize)
        return;
    
    // Если нет линий задержки, просто копируем входной сигнал
    if (delayLines.empty())
    {
        std::copy(input, input + numSamples, output);
        return;
    }
    
    // Используем первую линию задержки для простого delay эффекта
    auto& line = delayLines[0];
    
    // ✅ FEEDBACK DELAY АЛГОРИТМ (исправленный):
    
    for (int sample = 0; sample < numSamples; ++sample)
    {
        // 1. Читаем задержанный сигнал из буфера 
        size_t readIndex = (line.writeIndex - line.delayTime + line.buffer.size()) % line.buffer.size();
        float delayedSignal = line.buffer[readIndex];
        
        // 2. Применяем feedback: входной + задержанный * feedback коэффициент
        float feedbackGain = line.feedback; // Уже ограничен в setFeedback()
        float inputWithFeedback = input[sample] + (delayedSignal * feedbackGain);
        
        // 3. Записываем смешанный сигнал в буфер
        line.buffer[line.writeIndex] = inputWithFeedback;
        
        // 4. Выходной сигнал = только задержанный сигнал (wet)
        output[sample] = delayedSignal;
        
        // 5. Обновляем индекс записи
        line.writeIndex = (line.writeIndex + 1) % line.buffer.size();
    }
}

void DelayEngine::processStereo(const float* inputL, const float* inputR, 
                               float* outputL, float* outputR, int numSamples)
{
    // ИСПРАВЛЕНО: Используем разные delay линии для L и R каналов с feedback
    if (!isPrepared || numSamples > blockSize)
        return;
    
    // Если нет линий задержки, просто копируем входные сигналы
    if (delayLines.empty())
    {
        std::copy(inputL, inputL + numSamples, outputL);
        std::copy(inputR, inputR + numSamples, outputR);
        return;
    }
    
    // Используем первую линию для левого канала
    auto& lineL = delayLines[0];
    
    // Обработка левого канала с feedback
    for (int sample = 0; sample < numSamples; ++sample)
    {
        size_t readIndex = (lineL.writeIndex - lineL.delayTime + lineL.buffer.size()) % lineL.buffer.size();
        float delayedSignal = lineL.buffer[readIndex];
        
        // Применяем feedback
        float feedbackGain = lineL.feedback;
        float inputWithFeedback = inputL[sample] + (delayedSignal * feedbackGain);
        
        lineL.buffer[lineL.writeIndex] = inputWithFeedback;
        outputL[sample] = delayedSignal;
        lineL.writeIndex = (lineL.writeIndex + 1) % lineL.buffer.size();
    }
    
    // Для правого канала используем вторую линию (если есть)
    if (delayLines.size() > 1)
    {
        auto& lineR = delayLines[1];
        
        // Обработка правого канала с feedback
        for (int sample = 0; sample < numSamples; ++sample)
        {
            size_t readIndex = (lineR.writeIndex - lineR.delayTime + lineR.buffer.size()) % lineR.buffer.size();
            float delayedSignal = lineR.buffer[readIndex];
            
            // Применяем feedback
            float feedbackGain = lineR.feedback;
            float inputWithFeedback = inputR[sample] + (delayedSignal * feedbackGain);
            
            lineR.buffer[lineR.writeIndex] = inputWithFeedback;
            outputR[sample] = delayedSignal;
            lineR.writeIndex = (lineR.writeIndex + 1) % lineR.buffer.size();
        }
    }
    else
    {
        // Если только одна линия, просто копируем левый канал на правый
        std::copy(outputL, outputL + numSamples, outputR);
    }
}

//==============================================================================
void DelayEngine::setParameters(const Parameters& newParams)
{
    params = newParams;
    updateDelayTimes();
    updateModulation();
}

void DelayEngine::setDelayTime(float delayTimeMs)
{
    params.delayTime = MathUtils::clamp(delayTimeMs, 10.0f, 2000.0f);
    updateDelayTimes();
}

void DelayEngine::setFeedback(float feedbackPercent)
{
    // Ограничиваем feedback на разумные значения для избежания взрыва сигнала
    params.feedback = MathUtils::clamp(feedbackPercent, 0.0f, 95.0f);
    
    // Обновление коэффициентов обратной связи для всех линий
    for (auto& line : delayLines)
    {
        line.feedback = params.feedback / 100.0f;
    }
}

void DelayEngine::setDiffusion(float diffusionPercent)
{
    params.diffusion = MathUtils::clamp(diffusionPercent, 0.0f, 100.0f);
}

void DelayEngine::setModulation(float modulationHz)
{
    params.modulation = MathUtils::clamp(modulationHz, 0.0f, 10.0f);
    updateModulation();
}

void DelayEngine::setModulationDepth(float depthMs)
{
    params.modulationDepth = MathUtils::clamp(depthMs, 0.0f, 50.0f);
    updateModulation();
}

//==============================================================================
void DelayEngine::setDelayTimes(const std::vector<float>& delayTimes)
{
    if (delayTimes.size() != delayLines.size())
        return;
    
    for (size_t i = 0; i < delayLines.size(); ++i)
    {
        delayLines[i].delayTime = MathUtils::calculateDelaySamples(delayTimes[i], sampleRate);
    }
}

void DelayEngine::setFeedbackMatrix(const std::vector<float>& matrix)
{
    if (matrix.size() != feedbackMatrix.size())
        return;
    
    feedbackMatrix = matrix;
}

void DelayEngine::enableModulation(bool enabled)
{
    for (auto& line : delayLines)
    {
        line.modulationEnabled = enabled;
    }
}

//==============================================================================
void DelayEngine::initializeDelayLines()
{
    delayLines.clear();
    delayLines.resize(params.numDelayLines);
    
    // Расчет размеров буферов для максимальной задержки
    int maxDelaySamples = MathUtils::calculateDelaySamples(2000.0f, sampleRate);
    int bufferSize = maxDelaySamples + blockSize; // Добавляем запас
    
    // Инициализация каждой линии задержки
    for (auto& line : delayLines)
    {
        line.buffer.resize(bufferSize, 0.0f);
        line.writeIndex = 0;
        line.delayTime = 0;
        line.feedback = params.feedback / 100.0f;
        line.modulationPhase = 0.0f;
        line.modulationIncrement = 0.0f;
        line.modulationDepth = 0.0f;
        line.modulationEnabled = false;
    }
    
    // Установка начальных времен задержки
    updateDelayTimes();
}

void DelayEngine::initializeFeedbackMatrix()
{
    int size = static_cast<int>(delayLines.size());
    feedbackMatrix.resize(size * size);
    
    // Создание Hadamard матрицы для максимальной диффузии
    auto hadamard = createHadamardMatrix(size);
    feedbackMatrix = hadamard;
}

void DelayEngine::updateDelayTimes()
{
    if (delayLines.empty())
        return;
    
    // ИСПРАВЛЕНО: Все линии имеют одинаковое время задержки
    // (для избежания биений между левым и правым каналом)
    int baseDelaySamples = MathUtils::calculateDelaySamples(params.delayTime, sampleRate);
    
    for (size_t i = 0; i < delayLines.size(); ++i)
    {
        // Все линии имеют одинаковое время задержки
        delayLines[i].delayTime = static_cast<size_t>(baseDelaySamples);
    }
}

void DelayEngine::updateModulation()
{
    float modulationIncrement = MathUtils::TWO_PI * params.modulation / sampleRate;
    
    for (auto& line : delayLines)
    {
        line.modulationIncrement = modulationIncrement;
        line.modulationDepth = MathUtils::calculateDelaySamples(params.modulationDepth, sampleRate);
    }
}

//==============================================================================
float DelayEngine::getModulatedDelayTime(size_t lineIndex)
{
    if (lineIndex >= delayLines.size())
        return 0.0f;
    
    auto& line = delayLines[lineIndex];
    if (!line.modulationEnabled)
        return static_cast<float>(line.delayTime);
    
    // Синусоидальная модуляция
    float modulation = line.modulationDepth * MathUtils::fastSin(line.modulationPhase);
    return static_cast<float>(line.delayTime) + modulation;
}

void DelayEngine::updateModulationPhase()
{
    for (auto& line : delayLines)
    {
        line.modulationPhase += line.modulationIncrement;
        line.modulationPhase = MathUtils::wrapPhase(line.modulationPhase);
    }
}

//==============================================================================
std::vector<float> DelayEngine::createHadamardMatrix(int size)
{
    std::vector<float> matrix(size * size);
    
    // Простая реализация Hadamard матрицы
    if (size == 1)
    {
        matrix[0] = 1.0f;
    }
    else if (size == 2)
    {
        matrix[0] = 1.0f; matrix[1] = 1.0f;
        matrix[2] = 1.0f; matrix[3] = -1.0f;
    }
    else
    {
        // Для больших размеров используем простую диагональную матрицу
        for (int i = 0; i < size; ++i)
        {
            for (int j = 0; j < size; ++j)
            {
                matrix[i * size + j] = (i == j) ? 0.7f : 0.0f;
            }
        }
    }
    
    return matrix;
}

bool DelayEngine::isPrime(int n)
{
    return MathUtils::isPrime(n);
}

std::vector<int> DelayEngine::generatePrimeDelays(int count, int minDelay, int maxDelay)
{
    return MathUtils::generatePrimeDelays(count, minDelay, maxDelay);
} 