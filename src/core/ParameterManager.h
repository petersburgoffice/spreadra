#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>
#include <memory>

/**
 * @brief Менеджер параметров плагина
 * 
 * Управляет всеми параметрами плагина, включая:
 * - Автоматизацию параметров
 * - Сохранение/загрузку пресетов
 * - Сглаживание параметров
 * - Валидацию значений
 */
class ParameterManager
{
public:
    //==============================================================================
    ParameterManager();
    ~ParameterManager();

    //==============================================================================
    // Параметры плагина
    struct Parameters
    {
        // Delay parameters
        float delayTime = 500.0f;      // ms, 10-2000
        float feedback = 120.0f;       // %, 0-150
        float diffusion = 50.0f;       // %, 0-100
        float modulation = 0.0f;       // Hz, 0-10
        float modulationDepth = 0.0f;  // ms, 0-50
        
        // Pitch shift parameters
        float pitchShift = 12.0f;      // semitones, -24 to +24
        float formantPreservation = 80.0f; // %, 0-100
        float quality = 1.0f;          // 0.5-1.0
        
        // Reverb parameters
        float roomSize = 1000.0f;      // m², 10-10000
        float decayTime = 3.0f;        // seconds, 0.1-20
        float damping = 50.0f;         // %, 0-100
        float preDelay = 0.0f;         // ms, 0-500
        float stereoWidth = 100.0f;    // %, 0-150
        
        // Mix parameters
        float dryWet = 50.0f;          // %, 0-100
        float shimmerMix = 50.0f;      // %, 0-100
        
        // Filter parameters
        float lowPassFreq = 20000.0f;  // Hz, 20-20000
        float highPassFreq = 20.0f;    // Hz, 20-20000
        bool enableLowPass = false;
        bool enableHighPass = false;
    };

    //==============================================================================
    // Основные операции
    void setParameters(const Parameters& newParams);
    const Parameters& getParameters() const { return currentParams; }
    
    void setParameter(int parameterId, float value);
    float getParameter(int parameterId) const;
    
    //==============================================================================
    // Автоматизация
    void beginParameterChangeGesture(int parameterId);
    void endParameterChangeGesture(int parameterId);
    void setParameterNotifyingHost(int parameterId, float newValue);
    
    //==============================================================================
    // Сглаживание параметров
    void updateSmoothing();
    void resetSmoothing();
    
    //==============================================================================
    // Пресеты
    struct Preset
    {
        juce::String name;
        Parameters parameters;
        juce::String category;
        juce::String description;
    };
    
    void savePreset(const juce::String& name, const juce::String& category = "User");
    void loadPreset(const juce::String& name);
    void deletePreset(const juce::String& name);
    
    std::vector<Preset> getPresets() const;
    std::vector<Preset> getPresetsByCategory(const juce::String& category) const;
    
    //==============================================================================
    // Сериализация
    juce::ValueTree getState() const;
    void setState(const juce::ValueTree& state);
    
    juce::String getStateAsString() const;
    void setStateFromString(const juce::String& stateString);
    
    //==============================================================================
    // Валидация
    bool isValidParameter(int parameterId) const;
    bool isValidValue(int parameterId, float value) const;
    float clampValue(int parameterId, float value) const;
    
    //==============================================================================
    // Утилиты
    juce::String getParameterName(int parameterId) const;
    juce::String getParameterLabel(int parameterId) const;
    juce::String getParameterText(int parameterId, float value) const;
    
    float getParameterMinValue(int parameterId) const;
    float getParameterMaxValue(int parameterId) const;
    float getParameterDefaultValue(int parameterId) const;

private:
    //==============================================================================
    // Структура параметра
    struct ParameterInfo
    {
        int id;
        juce::String name;
        juce::String label;
        float minValue;
        float maxValue;
        float defaultValue;
        float currentValue;
        float targetValue;
        float smoothingRate;
        bool isSmoothing;
        bool isAutomated;
    };

    //==============================================================================
    // Состояние
    Parameters currentParams;
    Parameters targetParams;
    std::vector<ParameterInfo> parameters;
    
    // Пресеты
    std::vector<Preset> presets;
    
    // Сглаживание
    float smoothingRate = 0.1f;
    bool smoothingEnabled = true;

    //==============================================================================
    // Внутренние методы
    void initializeParameters();
    void updateParameter(int parameterId, float value);
    void updateSmoothingForParameter(int parameterId);
    
    // Пресеты
    void loadFactoryPresets();
    void savePresetsToFile();
    void loadPresetsFromFile();
    
    // Валидация
    bool isParameterInRange(int parameterId, float value) const;
    float normalizeValue(int parameterId, float value) const;
    float denormalizeValue(int parameterId, float normalizedValue) const;

    // JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ParameterManager)
}; 