#include "ReverbEditor.h"
#include "../core/Version.h"

ReverbEditor::ReverbEditor(ReverbProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setSize(600, 400); // Уменьшаем высоту
    
    // Создание слайдеров для параметров
    addAndMakeVisible(roomSizeSlider);
    addAndMakeVisible(decayTimeSlider);
    addAndMakeVisible(dryWetSlider);
    addAndMakeVisible(stereoWidthSlider);
    
    // Настройка слайдеров - убираем текстовые поля
    roomSizeSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    roomSizeSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0); // Убираем текстовое поле
    roomSizeSlider.setRange(10.0, 10000.0, 10.0);
    roomSizeSlider.setValue(5005.0); // Середина диапазона для 12 часов
    
    decayTimeSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    decayTimeSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0); // Убираем текстовое поле
    decayTimeSlider.setRange(0.1, 20.0, 0.1);
    decayTimeSlider.setValue(10.05); // Середина диапазона для 12 часов
    
    dryWetSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    dryWetSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0); // Убираем текстовое поле
    dryWetSlider.setRange(0.0, 100.0, 1.0);
    dryWetSlider.setValue(50.0); // Середина диапазона для 12 часов
    
    stereoWidthSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    stereoWidthSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0); // Убираем текстовое поле
    stereoWidthSlider.setRange(0.0, 200.0, 1.0);
    stereoWidthSlider.setValue(100.0); // Середина диапазона для 12 часов
    
    // Подключение к параметрам процессора
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
    g.drawFittedText("REVERBIX", getLocalBounds().removeFromTop(50), juce::Justification::centred, 1);
    
    g.setFont(juce::Font(juce::FontOptions().withHeight(12.0f)));
    g.drawFittedText("by SonicMakers", getLocalBounds().removeFromTop(50).translated(0, 30), juce::Justification::centred, 1);
    
    // Подписи под слайдерами
    g.setFont(juce::Font(juce::FontOptions().withHeight(12.0f)));
    g.drawText("Room Size", roomSizeSlider.getBounds().translated(0, 80), juce::Justification::centred);
    g.drawText("Decay Time", decayTimeSlider.getBounds().translated(0, 80), juce::Justification::centred);
    g.drawText("Dry/Wet", dryWetSlider.getBounds().translated(0, 80), juce::Justification::centred);
    g.drawText("Stereo Width", stereoWidthSlider.getBounds().translated(0, 80), juce::Justification::centred);
}

void ReverbEditor::resized()
{
    auto bounds = getLocalBounds().reduced(20);
    bounds.removeFromTop(50); // Место для заголовка
    
    const int sliderWidth = 120;
    const int sliderHeight = 120;
    const int spacing = 40;
    
    // Размещаем все слайдеры в один горизонтальный ряд внизу
    auto bottomRow = bounds.removeFromBottom(sliderHeight + 30); // +30 для подписей
    
    // Распределяем слайдеры равномерно по ширине
    int totalWidth = sliderWidth * 4 + spacing * 3;
    int startX = (bounds.getWidth() - totalWidth) / 2;
    
    roomSizeSlider.setBounds(startX, bottomRow.getY(), sliderWidth, sliderHeight);
    startX += sliderWidth + spacing;
    
    decayTimeSlider.setBounds(startX, bottomRow.getY(), sliderWidth, sliderHeight);
    startX += sliderWidth + spacing;
    
    dryWetSlider.setBounds(startX, bottomRow.getY(), sliderWidth, sliderHeight);
    startX += sliderWidth + spacing;
    
    stereoWidthSlider.setBounds(startX, bottomRow.getY(), sliderWidth, sliderHeight);
    
    // Размещение версии в правом нижнем углу
    versionLabel.setBounds(getWidth() - 80, getHeight() - 25, 70, 20);
} 