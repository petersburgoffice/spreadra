# Reverbix Plugin Architecture

## Обзор проекта

Reverbix Plugin — это профессиональный аудио-плагин, реализующий высококачественную реверберацию для трех основных форматов:
- **VST3** (Virtual Studio Technology 3) - для DAW на Windows, macOS, Linux
- **AU** (Audio Unit) - для Logic Pro, GarageBand, других macOS приложений  
- **AAX** (Avid Audio eXtension) - для Pro Tools

## Принцип работы Shimmer-эффекта

### Алгоритмическая схема
```
Input Signal
    ↓
    ┌───────────────── Signal Splitter ─────────────────┐
    │                                                   │
    ├── Dry Path (прямой путь)                          │
    │   └── Unprocessed Signal                          │
    │                                                   │
    └── Wet Path (обработанный путь)                    │
        ↓                                               │
        Input Attenuation (10% of input) ← Новое!       │
        ↓                                               │
        Delay Line (с feedback > 100%)                  │
        ↓                                               │
        Pitch Shifter (+12 semitones / +1 octave)       │
        ↓                                               │
        Filter Bank (опционально)                       │
        ↓                                               │
        Reverb (Hall/Cathedral algorithm)               │
        ↓                                               │
        Wet Signal Normalization                        │
        │                                               │
        └───────────────────────────────────────────────┘
                                ↓
                    ┌─── Dry/Wet Mixer ───┐
                    │                     │
                    │  Dry Gain + Wet Gain │
                    │                     │
                    └─────────┬───────────┘
                              ↓
                        Output Signal
```

### Ключевые компоненты DSP

1. **Delay Line с Feedback**
   - Реализация: Circular Buffer
   - Feedback range: 0% - 150%
   - Delay time: 10ms - 2000ms
   - Алгоритм: Schroeder's feedback delay network

2. **Pitch Shifter**
   - Метод: Phase Vocoder / FFT-based
   - Range: -24 to +24 semitones
   - Default: +12 semitones (+1 octave)
   - Overlap-add synthesis

3. **Reverb Algorithm**
   - Основа: Schroeder's "Natural Sounding Artificial Reverberation" (1961)
   - Структура: Parallel Comb Filters + Series All-Pass Filters
   - Room size: 10m² - 10000m²
   - Decay time: 0.1s - 20s

## Архитектура плагина

### 1. Основные слои архитектуры

```
┌─────────────────────────────────────────────────────────────┐
│                    Host Interface Layer                     │
├─────────────────────────────────────────────────────────────┤
│  VST3 Wrapper  │  AU Wrapper  │  AAX Wrapper  │  Standalone │
├─────────────────────────────────────────────────────────────┤
│                    Plugin Core Layer                        │
├─────────────────────────────────────────────────────────────┤
│  Parameter Manager  │  Audio Processor  │  GUI Controller   │
├─────────────────────────────────────────────────────────────┤
│                    DSP Engine Layer                         │
├─────────────────────────────────────────────────────────────┤
│  Shimmer Algorithm  │  Delay Engine  │  Pitch Shifter      │
│  Reverb Engine      │  Filter Bank   │  Metering System    │
├─────────────────────────────────────────────────────────────┤
│                    Utility Layer                            │
├─────────────────────────────────────────────────────────────┤
│  Math Utils  │  Buffer Management  │  FFT Library  │  GUI   │
└─────────────────────────────────────────────────────────────┘
```

### 2. Детальная архитектура компонентов

#### 2.1 Plugin Core Layer

**Parameter Manager**
```cpp
class ParameterManager {
    // Параметры плагина
    struct Parameters {
        float delayTime;      // 10ms - 2000ms
        float feedback;       // 0% - 150%
        float pitchShift;     // -24 to +24 semitones
        float reverbSize;     // 10m² - 10000m²
        float reverbDecay;    // 0.1s - 20s
        float dryWet;         // 0% - 100%
        float diffusion;      // 0% - 100%
        float modulation;     // 0% - 100%
    };
    
    // Автоматизация параметров
    // Preset management
    // Parameter smoothing
};
```

**Audio Processor**
```cpp
class ShimmerProcessor {
    // Основной аудио-процессор
    void processBlock(float* input, float* output, int numSamples);
    
    // Автоматическое определение моно/стерео
    if (totalNumInputChannels == 1 && totalNumOutputChannels == 1) {
        // Моно обработка
        shimmerAlgorithm.process(input, output, numSamples);
    } else if (totalNumInputChannels == 2 && totalNumOutputChannels == 2) {
        // Стерео обработка
        shimmerAlgorithm.processStereo(inputL, inputR, outputL, outputR, numSamples);
    }
    
    // Компоненты DSP
    DelayEngine delayEngine;
    PitchShifter pitchShifter;
    ReverbEngine reverbEngine;
    FilterBank filterBank;
    
    // Управление состоянием
    void prepareToPlay(double sampleRate, int blockSize);
    void releaseResources();
};
```

#### 2.2 DSP Engine Layer

**Shimmer Algorithm (Splitter/Mixer Architecture)**
```cpp
class ShimmerAlgorithm {
    // Основной DSP алгоритм с splitter/mixer архитектурой
    void process(const float* input, float* output, int numSamples);
    void processStereo(const float* inputL, const float* inputR, 
                      float* outputL, float* outputR, int numSamples);
    
    // Внутренние методы обработки
    void processMono(const float* input, float* output, int numSamples);
    void processStereoInternal(const float* inputL, const float* inputR, 
                              float* outputL, float* outputR, int numSamples);
    
    // DSP компоненты
    DelayEngine delayEngine;
    PitchShifter pitchShifter;
    ReverbEngine reverbEngine;
    FilterBank filterBank;
    
    // Временные буферы для обработки
    std::vector<float> tempBuffer1, tempBuffer2, tempBuffer3;
    std::vector<float> tempBufferL, tempBufferR;  // Стерео буферы
    
    // Splitter/Mixer логика
    struct DryWetLogic {
        // Разделение сигнала на dry и wet пути
        // Dry Path: Input (без обработки)
        // Wet Path: Input → Delay → Pitch → Filter → Reverb → Normalization
        
        // Нормализация wet сигнала
        float normalizeWetSignal(const float* wetSignal, int numSamples);
        
        // Смешивание dry и wet сигналов
        void mixDryWet(const float* drySignal, const float* wetSignal, 
                      float* output, float dryWetPercent, int numSamples);
    };
};
```

**Dry/Wet Logic Implementation**
```cpp
// Константа ослабления для wet path (10% от входного сигнала)
const float WET_INPUT_ATTENUATION = 0.1f;

// Ослабление входного сигнала для wet path
for (int i = 0; i < numSamples; ++i) {
    wetInput[i] = input[i] * WET_INPUT_ATTENUATION;
}

// Нормализация wet сигнала для предотвращения перегрузки
float maxWetLevel = 0.0f;
for (int i = 0; i < numSamples; ++i) {
    maxWetLevel = std::max(maxWetLevel, std::abs(wetSignal[i]));
}

float wetNormalization = (maxWetLevel > 0.0f) ? 
    std::min(1.0f / maxWetLevel, 2.0f) : 1.0f;

// Смешивание с правильной логикой
float dryGain = (100.0f - dryWetPercent) / 100.0f;  // 0% = 100% dry
float wetGain = dryWetPercent / 100.0f;              // 100% = 100% wet

output[i] = dryGain * drySignal[i] + wetGain * normalizedWet[i];
```

**Delay Engine**
```cpp
class DelayEngine {
    // Circular buffer для задержки
    CircularBuffer<float> delayBuffer;
    
    // Feedback network (Schroeder's algorithm)
    struct FeedbackNetwork {
        std::vector<CombFilter> combFilters;
        std::vector<AllPassFilter> allPassFilters;
    };
    
    // Параметры
    float delayTime;
    float feedback;
    float diffusion;
    
    void process(float* input, float* output, int numSamples);
};
```

**Pitch Shifter**
```cpp
class PitchShifter {
    // FFT-based pitch shifting
    FFTEngine fftEngine;
    
    // Phase vocoder implementation
    struct PhaseVocoder {
        std::vector<std::complex<float>> spectrum;
        std::vector<float> phase;
        std::vector<float> previousPhase;
    };
    
    // Параметры
    float pitchShift;  // в полутонах
    float formantPreservation;
    
    void process(float* input, float* output, int numSamples);
};
```

**Reverb Engine**
```cpp
class ReverbEngine {
    // Schroeder's reverb algorithm
    struct SchroederReverb {
        // Parallel comb filters
        std::vector<CombFilter> combFilters;
        
        // Series all-pass filters
        std::vector<AllPassFilter> allPassFilters;
        
        // Early reflections
        EarlyReflectionEngine earlyReflections;
    };
    
    // Параметры
    float roomSize;
    float decayTime;
    float diffusion;
    float damping;
    
    void process(float* input, float* output, int numSamples);
};
```

### 3. Форматы плагинов

#### 3.1 VST3 Implementation
```cpp
class ShimmerVST3 : public juce::AudioProcessor {
    // JUCE-based VST3 wrapper
    // Автоматическая генерация VST3 интерфейса
    // Parameter automation
    // MIDI support
};
```

#### 3.2 AU Implementation
```cpp
class ShimmerAU : public juce::AudioProcessor {
    // JUCE-based AU wrapper
    // Audio Unit v3 support
    // Parameter validation
    // Preset management
};
```

#### 3.3 AAX Implementation
```cpp
class ShimmerAAX : public AAX_CEffectParameters {
    // Avid AAX wrapper
    // Pro Tools integration
    // AAX-specific features
    // Parameter persistence
};
```

### 4. Dry/Wet Behavior and Technical Details

#### 4.1 Поведение по позициям

| Dry/Wet | Dry Signal | Wet Signal | Результат |
|---------|------------|------------|-----------|
| 0%      | 100%       | 0%         | Только чистый сигнал |
| 10%     | 90%        | 10%        | 90% чистого + 10% обработанного |
| 50%     | 50%        | 50%        | Равный микс |
| 90%     | 10%        | 90%        | 10% чистого + 90% обработанного |
| 100%    | 0%         | 100%       | Только обработанный сигнал |

#### 4.2 Преимущества splitter/mixer архитектуры

1. **Предотвращение перегрузки**: Wet сигнал нормализуется перед смешиванием
2. **Входное ослабление**: Wet path получает только 10% от входного сигнала
3. **Правильный баланс**: Dry и wet сигналы имеют сопоставимые уровни
4. **Интуитивное поведение**: 1% dry/wet действительно означает 1% обработанного сигнала
5. **Защита от клиппинга**: Ограничение максимального усиления до 2x

#### 4.3 Технические детали

- **Входное ослабление**: Wet path получает 10% от входного сигнала (WET_INPUT_ATTENUATION = 0.1f)
- **Нормализация**: Автоматическая, основана на максимальном уровне wet сигнала
- **Ограничение усиления**: Максимум 2x для предотвращения искажений
- **Обработка по блокам**: Нормализация вычисляется для каждого блока сэмплов
- **Стерео поддержка**: Отдельная нормализация для левого и правого каналов

### 5. GUI Architecture

**Modern UI Framework**
```cpp
class ShimmerGUI : public juce::AudioProcessorEditor {
    // Основные компоненты интерфейса
    struct GUIComponents {
        // Кнопки управления
        juce::Slider delayTimeSlider;
        juce::Slider feedbackSlider;
        juce::Slider pitchShiftSlider;
        juce::Slider reverbSizeSlider;
        juce::Slider reverbDecaySlider;
        juce::Slider dryWetSlider;
        
        // Визуализация
        juce::Component spectrumAnalyzer;
        juce::Component reverbVisualizer;
        
        // Preset management
        juce::ComboBox presetSelector;
        juce::Button savePresetButton;
    };
    
    // Responsive design
    // Dark/Light themes
    // High DPI support
};
```

## Технические требования

### Системные требования
- **CPU**: Intel i5 / AMD Ryzen 5 или выше
- **RAM**: 4GB минимум, 8GB рекомендуется
- **OS**: Windows 10+, macOS 10.14+, Linux (Ubuntu 18.04+)
- **DAW**: Поддержка VST3, AU, AAX хостов

### Производительность
- **Latency**: < 10ms при 44.1kHz
- **CPU Usage**: < 5% на одном ядре
- **Memory**: < 50MB RAM
- **Sample Rates**: 44.1kHz - 192kHz

### Качество звука
- **Bit Depth**: 32-bit float internal processing
- **Dynamic Range**: > 120dB
- **THD+N**: < -100dB
- **Frequency Response**: 20Hz - 20kHz ±0.1dB

## Алгоритмы DSP

### 1. Schroeder's Reverb Algorithm
```cpp
// Parallel comb filters
for (int i = 0; i < numCombFilters; ++i) {
    output += combFilters[i].process(input, delayTimes[i], decayTimes[i]);
}

// Series all-pass filters
for (int i = 0; i < numAllPassFilters; ++i) {
    output = allPassFilters[i].process(output, delayTimes[i], feedback[i]);
}
```

### 2. Phase Vocoder Pitch Shifting
```cpp
// FFT analysis
fftEngine.performForwardFFT(input, spectrum);

// Phase unwrapping and modification
for (int bin = 0; bin < fftSize/2; ++bin) {
    float phaseDiff = phase[bin] - previousPhase[bin];
    float expectedPhase = 2.0f * M_PI * bin * hopSize / fftSize;
    float phaseError = phaseDiff - expectedPhase;
    
    // Phase correction
    phase[bin] += phaseError * pitchShiftRatio;
}

// IFFT synthesis
fftEngine.performInverseFFT(spectrum, output);
```

### 3. Feedback Delay Network
```cpp
// Circular buffer implementation
for (int sample = 0; sample < numSamples; ++sample) {
    float delayed = delayBuffer.read(delayTime);
    float feedbackSignal = delayed * feedback;
    
    output[sample] = input[sample] + feedbackSignal;
    delayBuffer.write(input[sample] + feedbackSignal);
}
```

## Структура проекта

```
Shimmer/
├── src/
│   ├── core/
│   │   ├── ShimmerProcessor.cpp
│   │   ├── ParameterManager.cpp
│   │   └── PluginState.cpp
│   ├── dsp/
│   │   ├── DelayEngine.cpp
│   │   ├── PitchShifter.cpp
│   │   ├── ReverbEngine.cpp
│   │   ├── FilterBank.cpp
│   │   └── FFTEngine.cpp
│   ├── gui/
│   │   ├── ShimmerGUI.cpp
│   │   ├── Components/
│   │   └── Themes/
│   ├── formats/
│   │   ├── VST3/
│   │   ├── AU/
│   │   └── AAX/
│   └── utils/
│       ├── MathUtils.cpp
│       ├── BufferManager.cpp
│       └── AudioUtils.cpp
├── tests/
│   ├── unit/
│   ├── integration/
│   └── performance/
├── docs/
│   ├── API.md
│   ├── DSP.md
│   └── GUI.md
├── presets/
├── resources/
└── build/
```

## Безопасность и стабильность

### Обработка ошибок
- Валидация входных параметров
- Graceful degradation при ошибках DSP
- Memory leak prevention
- Thread safety

### Тестирование
- Unit tests для всех DSP компонентов
- Integration tests для плагинов
- Performance benchmarks
- Stress testing

### Совместимость
- Backward compatibility
- Cross-platform testing
- DAW compatibility matrix
- Sample rate conversion handling

## Правила поддержания архитектуры

### Обязательные требования
1. **Единый источник истины**: Вся архитектурная документация должна быть в `ARCHITECTURE.md`
2. **Актуальность**: При изменении кода архитектура должна обновляться в том же коммите
3. **Детализация**: Все изменения в DSP логике должны отражаться в диаграммах
4. **Проверка**: Перед коммитом проверять соответствие кода и документации

### При изменении DSP логики
- Обновить диаграмму сигнального потока
- Добавить описание новых компонентов
- Обновить примеры кода
- Проверить соответствие моно/стерео версий

### При изменении dry/wet логики
- Обновить splitter/mixer диаграмму
- Проверить таблицу поведения
- Обновить технические детали
- Убедиться в корректности нормализации 