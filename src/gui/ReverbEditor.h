#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "../core/ReverbProcessor.h"
#include <juce_gui_extra/juce_gui_extra.h>

class ModernKnobLookAndFeel : public juce::LookAndFeel_V4 {
public:
    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle, juce::Slider& slider) override;
    void drawLabel(juce::Graphics& g, juce::Label& label) override;
};

class AnimatedSlider : public juce::Slider, public juce::Timer {
public:
    AnimatedSlider(const juce::String& labelText = "");
    ~AnimatedSlider() override;
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;
    
    void setGlowIntensity(float intensity) { glowIntensity = intensity; repaint(); }
    
private:
    juce::String label;
    juce::Label valueLabel;
    ModernKnobLookAndFeel lookAndFeel;
    float glowIntensity = 0.0f;
    float animationPhase = 0.0f;
};

class ReverbEditor : public juce::AudioProcessorEditor, public juce::Timer
{
public:
    ReverbEditor(ReverbProcessor& p);
    ~ReverbEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    ReverbProcessor& processor;

    // Анимированные слайдеры
    AnimatedSlider roomSizeSlider;
    AnimatedSlider decayTimeSlider;
    AnimatedSlider dryWetSlider;
    AnimatedSlider stereoWidthSlider;

    // Attachments для связи с параметрами
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> roomSizeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> decayTimeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> dryWetAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> stereoWidthAttachment;

    // Label для отображения версии
    juce::Label versionLabel;
    
    // Анимационные переменные
    float backgroundPhase = 0.0f;
    float logoGlow = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ReverbEditor)
}; 