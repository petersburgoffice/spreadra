#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "../core/SpreadraProcessor.h"
#include <juce_gui_extra/juce_gui_extra.h>

class CustomRotarySliderLookAndFeel : public juce::LookAndFeel_V4
{
public:
    CustomRotarySliderLookAndFeel(juce::Colour ringColour) : ringColour(ringColour) {}
    
    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                         float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                         juce::Slider& slider) override;

private:
    juce::Colour ringColour;
};

class SpreadraEditor : public juce::AudioProcessorEditor
{
public:
    SpreadraEditor(SpreadraProcessor& p);
    ~SpreadraEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    SpreadraProcessor& processor;

    // Обычные слайдеры
    juce::Slider dryWetSlider;
    juce::Slider stereoWidthSlider;

    // Attachments для связи с параметрами
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> dryWetAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> stereoWidthAttachment;

    // Label для отображения версии
    juce::Label versionLabel;
    
    // Labels для подписей слайдеров
    juce::Label dryWetLabel;
    juce::Label stereoWidthLabel;
    
    // Custom look and feel objects с разными цветами
    std::unique_ptr<CustomRotarySliderLookAndFeel> dryWetLookAndFeel;
    std::unique_ptr<CustomRotarySliderLookAndFeel> stereoWidthLookAndFeel;
    
    // Фоновое изображение
    juce::Image backgroundImage;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpreadraEditor)
}; 