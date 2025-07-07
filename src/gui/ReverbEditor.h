#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "../core/ReverbProcessor.h"

/**
 * @brief Базовый редактор (GUI) для Reverb плагина
 * 
 * Реализует JUCE AudioProcessorEditor и связывает параметры с GUI.
 */
class ReverbEditor : public juce::AudioProcessorEditor
{
public:
    ReverbEditor(ReverbProcessor& p);
    ~ReverbEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    ReverbProcessor& processor;

    // Слайдеры для параметров
    juce::Slider pitchShiftSlider;
    juce::Slider roomSizeSlider;
    juce::Slider decayTimeSlider;
    juce::Slider dryWetSlider;
    juce::Slider stereoWidthSlider;

    // Кнопка для переключения mono/stereo
    juce::ToggleButton monoModeButton;

    // Attachments для связи с параметрами
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> pitchShiftAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> roomSizeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> decayTimeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> dryWetAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> stereoWidthAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> monoModeAttachment;

    // Label для отображения версии
    juce::Label versionLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ReverbEditor)
}; 