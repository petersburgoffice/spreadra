# DSP Algorithms for Shimmer Effect

## Обзор алгоритмов

Shimmer-эффект основан на комбинации трех основных DSP алгоритмов:
1. **Feedback Delay Network** - создает длительную петлю обратной связи
2. **Phase Vocoder Pitch Shifting** - сдвигает высоту тона без изменения темпа
3. **Schroeder's Reverb Algorithm** - создает пространственную реверберацию

## 1. Feedback Delay Network (FDN)

### Теоретические основы

FDN основан на работе Schroeder (1961) и представляет собой сеть параллельных линий задержки с обратной связью.

### Математическая модель

```
y[n] = x[n] + Σ(aᵢ * y[n - dᵢ])
```

где:
- `y[n]` - выходной сигнал
- `x[n]` - входной сигнал  
- `aᵢ` - коэффициенты обратной связи
- `dᵢ` - времена задержки
- `i` - индекс линии задержки

### Реализация

```cpp
class FeedbackDelayNetwork {
private:
    struct DelayLine {
        std::vector<float> buffer;
        size_t writeIndex;
        size_t delayTime;
        float feedback;
    };
    
    std::vector<DelayLine> delayLines;
    std::vector<float> feedbackMatrix;
    
public:
    void process(float* input, float* output, int numSamples) {
        for (int sample = 0; sample < numSamples; ++sample) {
            float inputSample = input[sample];
            float outputSample = inputSample;
            
            // Process each delay line
            for (size_t i = 0; i < delayLines.size(); ++i) {
                // Read from delay line
                size_t readIndex = (delayLines[i].writeIndex - delayLines[i].delayTime + 
                                   delayLines[i].buffer.size()) % delayLines[i].buffer.size();
                float delayed = delayLines[i].buffer[readIndex];
                
                // Apply feedback matrix
                for (size_t j = 0; j < delayLines.size(); ++j) {
                    delayed += feedbackMatrix[i * delayLines.size() + j] * 
                              delayLines[j].buffer[readIndex];
                }
                
                // Write to delay line
                delayLines[i].buffer[delayLines[i].writeIndex] = delayed;
                delayLines[i].writeIndex = (delayLines[i].writeIndex + 1) % 
                                          delayLines[i].buffer.size();
                
                outputSample += delayed * delayLines[i].feedback;
            }
            
            output[sample] = outputSample;
        }
    }
};
```

### Оптимизация

1. **Hadamard Matrix** для feedback matrix обеспечивает максимальную диффузию
2. **Prime delay times** предотвращают резонансы
3. **Circular buffer** для эффективного использования памяти

## 2. Phase Vocoder Pitch Shifting

### Теоретические основы

Phase vocoder основан на кратковременном преобразовании Фурье (STFT) и позволяет изменять высоту тона без изменения темпа.

### Алгоритм

#### Шаг 1: STFT Analysis
```cpp
void performSTFT(const float* input, std::complex<float>* spectrum, int frame) {
    // Apply window function
    for (int i = 0; i < fftSize; ++i) {
        windowedBuffer[i] = input[frame * hopSize + i] * window[i];
    }
    
    // Perform FFT
    fftEngine.performForwardFFT(windowedBuffer, spectrum);
}
```

#### Шаг 2: Phase Unwrapping
```cpp
void unwrapPhase(std::complex<float>* spectrum, float* phase, float* previousPhase) {
    for (int bin = 0; bin < fftSize/2; ++bin) {
        // Calculate phase difference
        float currentPhase = std::arg(spectrum[bin]);
        float phaseDiff = currentPhase - previousPhase[bin];
        
        // Expected phase increment
        float expectedPhase = 2.0f * M_PI * bin * hopSize / fftSize;
        
        // Phase unwrapping
        float phaseError = phaseDiff - expectedPhase;
        phaseError = std::fmod(phaseError + M_PI, 2.0f * M_PI) - M_PI;
        
        // Update phase
        phase[bin] = previousPhase[bin] + expectedPhase + phaseError;
        previousPhase[bin] = phase[bin];
    }
}
```

#### Шаг 3: Pitch Shifting
```cpp
void shiftPitch(float* phase, float pitchShiftRatio) {
    for (int bin = 0; bin < fftSize/2; ++bin) {
        // Apply pitch shift to phase
        phase[bin] *= pitchShiftRatio;
        
        // Handle frequency wrapping
        if (bin * pitchShiftRatio >= fftSize/2) {
            phase[bin] = 0.0f; // Zero out out-of-range frequencies
        }
    }
}
```

#### Шаг 4: Synthesis
```cpp
void synthesize(float* output, const std::complex<float>* spectrum, const float* phase) {
    // Reconstruct spectrum with new phase
    std::complex<float> reconstructedSpectrum[fftSize];
    for (int bin = 0; bin < fftSize/2; ++bin) {
        float magnitude = std::abs(spectrum[bin]);
        reconstructedSpectrum[bin] = std::polar(magnitude, phase[bin]);
        reconstructedSpectrum[fftSize - bin - 1] = std::conj(reconstructedSpectrum[bin]);
    }
    
    // Perform IFFT
    fftEngine.performInverseFFT(reconstructedSpectrum, output);
    
    // Apply window and overlap-add
    for (int i = 0; i < fftSize; ++i) {
        output[i] *= window[i];
        outputBuffer[outputIndex + i] += output[i];
    }
}
```

### Формантное сохранение

```cpp
class FormantPreservation {
private:
    std::vector<float> formantEnvelope;
    std::vector<float> shiftedFormantEnvelope;
    
public:
    void preserveFormants(std::complex<float>* spectrum, float pitchShiftRatio) {
        // Calculate formant envelope
        calculateFormantEnvelope(spectrum);
        
        // Shift formant envelope
        shiftFormantEnvelope(pitchShiftRatio);
        
        // Apply formant correction
        for (int bin = 0; bin < fftSize/2; ++bin) {
            float correction = shiftedFormantEnvelope[bin] / formantEnvelope[bin];
            spectrum[bin] *= correction;
        }
    }
};
```

## 3. Schroeder's Reverb Algorithm

### Теоретические основы

Алгоритм Schroeder (1961) основан на параллельных comb-фильтрах и последовательных all-pass фильтрах.

### Структура алгоритма

```
Input → Early Reflections → Parallel Comb Filters → Series All-Pass Filters → Output
```

### Comb Filter Implementation

```cpp
class CombFilter {
private:
    std::vector<float> buffer;
    size_t writeIndex;
    size_t delayTime;
    float decayTime;
    float sampleRate;
    
public:
    float process(float input) {
        // Read from delay line
        size_t readIndex = (writeIndex - delayTime + buffer.size()) % buffer.size();
        float delayed = buffer[readIndex];
        
        // Calculate feedback coefficient
        float feedback = std::exp(-3.0f * delayTime / (decayTime * sampleRate));
        
        // Process
        float output = input + feedback * delayed;
        
        // Write to buffer
        buffer[writeIndex] = output;
        writeIndex = (writeIndex + 1) % buffer.size();
        
        return delayed;
    }
};
```

### All-Pass Filter Implementation

```cpp
class AllPassFilter {
private:
    std::vector<float> buffer;
    size_t writeIndex;
    size_t delayTime;
    float feedback;
    
public:
    float process(float input) {
        // Read from delay line
        size_t readIndex = (writeIndex - delayTime + buffer.size()) % buffer.size();
        float delayed = buffer[readIndex];
        
        // All-pass filter equation: y[n] = -g*x[n] + x[n-d] + g*y[n-d]
        float output = -feedback * input + delayed;
        
        // Write to buffer
        buffer[writeIndex] = input + feedback * output;
        writeIndex = (writeIndex + 1) % buffer.size();
        
        return output;
    }
};
```

### Early Reflections

```cpp
class EarlyReflectionEngine {
private:
    struct Reflection {
        float delay;
        float amplitude;
        float pan;
    };
    
    std::vector<Reflection> reflections;
    std::vector<std::vector<float>> delayBuffers;
    
public:
    void process(float* input, float* outputL, float* outputR, int numSamples) {
        for (int sample = 0; sample < numSamples; ++sample) {
            float left = 0.0f, right = 0.0f;
            
            for (size_t i = 0; i < reflections.size(); ++i) {
                size_t readIndex = (writeIndices[i] - reflections[i].delay + 
                                   delayBuffers[i].size()) % delayBuffers[i].size();
                float delayed = delayBuffers[i][readIndex];
                
                // Apply panning
                left += delayed * reflections[i].amplitude * (1.0f - reflections[i].pan);
                right += delayed * reflections[i].amplitude * reflections[i].pan;
                
                // Write to buffer
                delayBuffers[i][writeIndices[i]] = input[sample];
                writeIndices[i] = (writeIndices[i] + 1) % delayBuffers[i].size();
            }
            
            outputL[sample] = left;
            outputR[sample] = right;
        }
    }
};
```

## 4. Интеграция алгоритмов

### Shimmer Pipeline

```cpp
class ShimmerAlgorithm {
private:
    FeedbackDelayNetwork fdn;
    PhaseVocoder pitchShifter;
    SchroederReverb reverb;
    EarlyReflectionEngine earlyReflections;
    
public:
    void process(float* input, float* output, int numSamples) {
        // Temporary buffers
        std::vector<float> fdnOutput(numSamples);
        std::vector<float> pitchShifted(numSamples);
        std::vector<float> reverbOutput(numSamples);
        
        // Step 1: Feedback Delay Network
        fdn.process(input, fdnOutput.data(), numSamples);
        
        // Step 2: Pitch Shifting
        pitchShifter.process(fdnOutput.data(), pitchShifted.data(), numSamples);
        
        // Step 3: Early Reflections
        earlyReflections.process(pitchShifted.data(), 
                                reverbOutput.data(), 
                                reverbOutput.data(), 
                                numSamples);
        
        // Step 4: Schroeder Reverb
        reverb.process(reverbOutput.data(), reverbOutput.data(), numSamples);
        
        // Step 5: Mix dry and wet
        for (int i = 0; i < numSamples; ++i) {
            output[i] = dryWet * input[i] + (1.0f - dryWet) * reverbOutput[i];
        }
    }
};
```

## 5. Оптимизация производительности

### SIMD оптимизация

```cpp
#include <immintrin.h>

void processSIMD(float* input, float* output, int numSamples) {
    for (int i = 0; i < numSamples; i += 8) {
        __m256 inputVec = _mm256_load_ps(&input[i]);
        __m256 processedVec = processVector(inputVec);
        _mm256_store_ps(&output[i], processedVec);
    }
}
```

### Lock-free circular buffer

```cpp
template<typename T>
class LockFreeCircularBuffer {
private:
    std::atomic<size_t> writeIndex;
    std::atomic<size_t> readIndex;
    std::vector<T> buffer;
    
public:
    void write(const T& value) {
        size_t current = writeIndex.load(std::memory_order_relaxed);
        size_t next = (current + 1) % buffer.size();
        
        while (next == readIndex.load(std::memory_order_acquire)) {
            // Buffer full, wait
            std::this_thread::yield();
        }
        
        buffer[current] = value;
        writeIndex.store(next, std::memory_order_release);
    }
    
    T read() {
        size_t current = readIndex.load(std::memory_order_relaxed);
        
        while (current == writeIndex.load(std::memory_order_acquire)) {
            // Buffer empty, wait
            std::this_thread::yield();
        }
        
        T value = buffer[current];
        readIndex.store((current + 1) % buffer.size(), std::memory_order_release);
        return value;
    }
};
```

## 6. Качество звука

### Динамический диапазон

```cpp
class DynamicRangeProcessor {
private:
    float noiseFloor;
    float maxAmplitude;
    
public:
    void process(float* buffer, int numSamples) {
        for (int i = 0; i < numSamples; ++i) {
            // Apply noise gate
            if (std::abs(buffer[i]) < noiseFloor) {
                buffer[i] = 0.0f;
            }
            
            // Apply soft clipping
            if (std::abs(buffer[i]) > maxAmplitude) {
                buffer[i] = std::copysign(maxAmplitude, buffer[i]);
            }
        }
    }
};
```

### Частотная характеристика

```cpp
class FrequencyResponseCorrection {
private:
    std::vector<float> frequencyResponse;
    
public:
    void correctResponse(float* buffer, int numSamples) {
        // Apply frequency response correction
        for (int i = 0; i < numSamples; ++i) {
            float frequency = i * sampleRate / numSamples;
            float correction = getCorrection(frequency);
            buffer[i] *= correction;
        }
    }
};
```

## 7. Тестирование алгоритмов

### Unit тесты

```cpp
TEST(PitchShifterTest, BasicPitchShift) {
    PitchShifter shifter;
    std::vector<float> input = generateTestSignal();
    std::vector<float> output(input.size());
    
    shifter.setPitchShift(12.0f); // +1 octave
    shifter.process(input.data(), output.data(), input.size());
    
    // Verify pitch shift
    float fundamentalFreq = detectFundamentalFrequency(output);
    float expectedFreq = detectFundamentalFrequency(input) * 2.0f;
    EXPECT_NEAR(fundamentalFreq, expectedFreq, 1.0f);
}
```

### Performance benchmarks

```cpp
BENCHMARK(DelayEnginePerformance) {
    DelayEngine engine;
    std::vector<float> input(1024);
    std::vector<float> output(1024);
    
    for (auto _ : state) {
        engine.process(input.data(), output.data(), input.size());
    }
}
``` 