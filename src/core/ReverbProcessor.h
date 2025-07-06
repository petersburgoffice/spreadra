#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "../dsp/ReverbAlgorithm.h"
#include "ParameterManager.h"
#include "utils/Logger.h"

/**
 * @brief Основной аудио-процессор для Reverb плагина
 * 
 * Этот класс является центральным компонентом плагина, который:
 * - Управляет DSP алгоритмами
 * - Обрабатывает аудио данные
 * - Интегрируется с JUCE AudioProcessor
 * - Управляет параметрами плагина
 */
class ReverbProcessor : public juce::AudioProcessor
{
public:
    //==============================================================================
    ReverbProcessor();
    ~ReverbProcessor() override;

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& busesLayout) const override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    //==============================================================================
    // Параметры плагина
    juce::AudioProcessorValueTreeState& getValueTreeState() { return parameters; }
    
    // DSP компоненты
    ReverbAlgorithm& getReverbAlgorithm() { return reverbAlgorithm; }
    
    // Метрики производительности
    float getCpuUsage() const { return cpuUsage; }
    float getLatency() const { return latencyMs; }

private:
    //==============================================================================
    // JUCE AudioProcessorValueTreeState для параметров
    juce::AudioProcessorValueTreeState parameters;
    
    // DSP алгоритм
    ReverbAlgorithm reverbAlgorithm;
    
    // Параметры плагина
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    
    // Метрики производительности
    float cpuUsage = 0.0f;
    float latencyMs = 0.0f;
    
    // Временные буферы
    juce::AudioBuffer<float> tempBuffer;
    
    // Обработчики параметров
    void updateParameters();
    
    // JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ReverbProcessor)
}; 