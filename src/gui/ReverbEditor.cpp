#include "ReverbEditor.h"
#include "../core/Version.h"

ReverbEditor::ReverbEditor(ReverbProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setSize(600, 520);
    
    // Создание слайдеров для параметров
    addAndMakeVisible(pitchShiftSlider);
    addAndMakeVisible(roomSizeSlider);
    addAndMakeVisible(decayTimeSlider);
    addAndMakeVisible(dryWetSlider);
    addAndMakeVisible(stereoWidthSlider);
    
    // Настройка слайдеров
    pitchShiftSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    pitchShiftSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
    pitchShiftSlider.setRange(-24.0, 24.0, 0.1);
    pitchShiftSlider.setValue(12.0);
    
    roomSizeSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    roomSizeSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
    roomSizeSlider.setRange(10.0, 10000.0, 10.0);
    roomSizeSlider.setValue(1000.0);
    
    decayTimeSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    decayTimeSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
    decayTimeSlider.setRange(0.1, 20.0, 0.1);
    decayTimeSlider.setValue(3.0);
    
    dryWetSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    dryWetSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
    dryWetSlider.setRange(0.0, 100.0, 1.0);
    dryWetSlider.setValue(50.0);
    
    stereoWidthSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    stereoWidthSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
    stereoWidthSlider.setRange(0.0, 200.0, 1.0);
    stereoWidthSlider.setValue(100.0);
    
    // Подключение к параметрам процессора
    pitchShiftAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor.getValueTreeState(), "pitchShift", pitchShiftSlider);
    roomSizeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor.getValueTreeState(), "roomSize", roomSizeSlider);
    decayTimeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor.getValueTreeState(), "decayTime", decayTimeSlider);
    dryWetAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor.getValueTreeState(), "dryWet", dryWetSlider);
    stereoWidthAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor.getValueTreeState(), "stereoWidth", stereoWidthSlider);
    
    // Настройка label для версии
    versionLabel.setText("v" + juce::String(PLUGIN_VERSION_STRING), juce::dontSendNotification);
    versionLabel.setFont(juce::Font(juce::FontOptions().withHeight(10.0f)));
    versionLabel.setJustificationType(juce::Justification::centredRight);
    versionLabel.setColour(juce::Label::textColourId, juce::Colour(0x80FFFFFF)); // Semi-transparent white
    addAndMakeVisible(versionLabel);
}

ReverbEditor::~ReverbEditor() = default;

void ReverbEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1e1e1e));
    
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(juce::FontOptions().withHeight(24.0f)));
    g.drawFittedText("Reverb Audio Plugin", getLocalBounds().removeFromTop(50), juce::Justification::centred, 1);
    
    // Заголовки для слайдеров
    g.setFont(juce::Font(juce::FontOptions().withHeight(14.0f)));
    g.drawText("Pitch Shift", pitchShiftSlider.getBounds().translated(0, -25), juce::Justification::centred);
    g.drawText("Room Size", roomSizeSlider.getBounds().translated(0, -25), juce::Justification::centred);
    g.drawText("Decay Time", decayTimeSlider.getBounds().translated(0, -25), juce::Justification::centred);
    g.drawText("Dry/Wet", dryWetSlider.getBounds().translated(0, -25), juce::Justification::centred);
    g.drawText("Stereo Width", stereoWidthSlider.getBounds().translated(0, -25), juce::Justification::centred);
}

void ReverbEditor::resized()
{
    auto bounds = getLocalBounds().reduced(20);
    bounds.removeFromTop(50); // Место для заголовка
    
    const int sliderWidth = 100;
    const int sliderHeight = 120;
    const int spacing = 20;
    
    // Расположение слайдеров в три ряда
    auto topRow = bounds.removeFromTop(sliderHeight);
    auto middleRow = bounds.removeFromTop(sliderHeight);
    auto bottomRow = bounds.removeFromTop(sliderHeight);
    
    // Первый ряд
    pitchShiftSlider.setBounds(topRow.removeFromLeft(sliderWidth));
    topRow.removeFromLeft(spacing);
    
    roomSizeSlider.setBounds(topRow.removeFromLeft(sliderWidth));
    topRow.removeFromLeft(spacing);
    
    decayTimeSlider.setBounds(topRow.removeFromLeft(sliderWidth));
    
    // Второй ряд
    dryWetSlider.setBounds(middleRow.removeFromLeft(sliderWidth));
    middleRow.removeFromLeft(spacing);
    
    stereoWidthSlider.setBounds(middleRow.removeFromLeft(sliderWidth));
    
    // Размещение версии в правом нижнем углу
    versionLabel.setBounds(getWidth() - 80, getHeight() - 25, 70, 20);
} 