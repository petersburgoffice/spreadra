#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "../core/ReverbProcessor.h"
#include <juce_gui_extra/juce_gui_extra.h>

class ModernKnobLookAndFeel : public juce::LookAndFeel_V4 {
public:
    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle, juce::Slider& slider) override;
};

class ReverbEditor : public juce::AudioProcessorEditor
{
public:
    ReverbEditor(ReverbProcessor& p);
    ~ReverbEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    ReverbProcessor& processor;

    // Обычные слайдеры
    juce::Slider roomSizeSlider;
    juce::Slider decayTimeSlider;
    juce::Slider dryWetSlider;
    juce::Slider stereoWidthSlider;

    // Attachments для связи с параметрами
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> roomSizeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> decayTimeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> dryWetAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> stereoWidthAttachment;

    // Label для отображения версии
    juce::Label versionLabel;
    
    // Labels для подписей слайдеров
    juce::Label roomSizeLabel;
    juce::Label decayTimeLabel;
    juce::Label dryWetLabel;
    juce::Label stereoWidthLabel;
    
    // LookAndFeel
    ModernKnobLookAndFeel knobLookAndFeel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ReverbEditor)
}; 