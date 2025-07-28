#include "ReverbEditor.h"
#include "../core/Version.h"

// ============================================================================
// ModernKnobLookAndFeel Implementation
// ============================================================================

void ModernKnobLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle, juce::Slider& slider)
{
    auto radius = juce::jmin(width, height) / 2.0f - 4.0f;
    auto centreX = x + width * 0.5f;
    auto centreY = y + height * 0.5f;
    auto rx = centreX - radius;
    auto ry = centreY - radius;
    auto rw = radius * 2.0f;
    auto angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

    // Внешняя тень
    g.setColour(juce::Colour(0x25000000));
    g.fillEllipse(rx + 2, ry + 3, rw, rw);

    // Основной корпус ручки
    juce::ColourGradient knobGradient(
        juce::Colour(0xff5a5a5a), centreX, ry,
        juce::Colour(0xff2a2a2a), centreX, ry + rw, false);
    knobGradient.addColour(0.3, juce::Colour(0xff4a4a4a));
    knobGradient.addColour(0.7, juce::Colour(0xff353535));
    g.setGradientFill(knobGradient);
    g.fillEllipse(rx, ry, rw, rw);

    // Внутренний ободок
    auto innerRadius = radius * 0.9f;
    auto innerRx = centreX - innerRadius;
    auto innerRy = centreY - innerRadius;
    auto innerRw = innerRadius * 2.0f;
    
    juce::ColourGradient innerGradient(
        juce::Colour(0xff6a6a6a), centreX, innerRy,
        juce::Colour(0xff3a3a3a), centreX, innerRy + innerRw, false);
    g.setGradientFill(innerGradient);
    g.fillEllipse(innerRx, innerRy, innerRw, innerRw);

    // Верхний блик
    g.setColour(juce::Colour(0x40ffffff));
    auto glareRadius = radius * 0.6f;
    g.fillEllipse(centreX - glareRadius, centreY - glareRadius * 1.2f, 
                  glareRadius * 2.0f, glareRadius * 0.8f);

    // Центральная область
    auto centerRadius = radius * 0.2f;
    g.setColour(juce::Colour(0xff2a2a2a));
    g.fillEllipse(centreX - centerRadius, centreY - centerRadius, 
                  centerRadius * 2.0f, centerRadius * 2.0f);

    // Указатель
    auto pointerLength = radius * 0.7f;
    auto pointerThickness = 3.0f;
    
    g.setColour(juce::Colour(0xff000000));
    g.drawLine(centreX + 1, centreY + 1,
               centreX + (pointerLength + 1) * std::cos(angle - juce::MathConstants<float>::halfPi),
               centreY + (pointerLength + 1) * std::sin(angle - juce::MathConstants<float>::halfPi),
               pointerThickness);
    
    g.setColour(juce::Colours::white);
    g.drawLine(centreX, centreY,
               centreX + pointerLength * std::cos(angle - juce::MathConstants<float>::halfPi),
               centreY + pointerLength * std::sin(angle - juce::MathConstants<float>::halfPi),
               pointerThickness);

    // Обводка
    g.setColour(juce::Colour(0xff1a1a1a));
    g.drawEllipse(rx, ry, rw, rw, 1.0f);
}



// ============================================================================
// ReverbEditor Implementation
// ============================================================================

ReverbEditor::ReverbEditor(ReverbProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setSize(800, 500);
    
    // Настройка слайдеров
    roomSizeSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    roomSizeSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    roomSizeSlider.setRange(10.0, 10000.0, 10.0);
    roomSizeSlider.setValue(5005.0);
    roomSizeSlider.setLookAndFeel(&knobLookAndFeel);
    addAndMakeVisible(roomSizeSlider);
    
    decayTimeSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    decayTimeSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    decayTimeSlider.setRange(0.1, 20.0, 0.1);
    decayTimeSlider.setValue(10.05);
    decayTimeSlider.setLookAndFeel(&knobLookAndFeel);
    addAndMakeVisible(decayTimeSlider);
    
    dryWetSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    dryWetSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    dryWetSlider.setRange(0.0, 100.0, 1.0);
    dryWetSlider.setValue(50.0);
    dryWetSlider.setLookAndFeel(&knobLookAndFeel);
    addAndMakeVisible(dryWetSlider);
    
    stereoWidthSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    stereoWidthSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    stereoWidthSlider.setRange(0.0, 200.0, 1.0);
    stereoWidthSlider.setValue(100.0);
    stereoWidthSlider.setLookAndFeel(&knobLookAndFeel);
    addAndMakeVisible(stereoWidthSlider);
    
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
    versionLabel.setColour(juce::Label::textColourId, juce::Colour(0x80FFFFFF));
    addAndMakeVisible(versionLabel);
    
    // Настройка Labels для подписей слайдеров
    roomSizeLabel.setText("Room Size", juce::dontSendNotification);
    roomSizeLabel.setFont(juce::Font(juce::FontOptions().withHeight(14.0f)));
    roomSizeLabel.setJustificationType(juce::Justification::centred);
    roomSizeLabel.setColour(juce::Label::textColourId, juce::Colour(0xffe0e0e0));
    addAndMakeVisible(roomSizeLabel);
    
    decayTimeLabel.setText("Decay Time", juce::dontSendNotification);
    decayTimeLabel.setFont(juce::Font(juce::FontOptions().withHeight(14.0f)));
    decayTimeLabel.setJustificationType(juce::Justification::centred);
    decayTimeLabel.setColour(juce::Label::textColourId, juce::Colour(0xffe0e0e0));
    addAndMakeVisible(decayTimeLabel);
    
    dryWetLabel.setText("Dry/Wet", juce::dontSendNotification);
    dryWetLabel.setFont(juce::Font(juce::FontOptions().withHeight(14.0f)));
    dryWetLabel.setJustificationType(juce::Justification::centred);
    dryWetLabel.setColour(juce::Label::textColourId, juce::Colour(0xffe0e0e0));
    addAndMakeVisible(dryWetLabel);
    
    stereoWidthLabel.setText("Stereo Width", juce::dontSendNotification);
    stereoWidthLabel.setFont(juce::Font(juce::FontOptions().withHeight(14.0f)));
    stereoWidthLabel.setJustificationType(juce::Justification::centred);
    stereoWidthLabel.setColour(juce::Label::textColourId, juce::Colour(0xffe0e0e0));
    addAndMakeVisible(stereoWidthLabel);
}

ReverbEditor::~ReverbEditor() = default;

void ReverbEditor::paint(juce::Graphics& g)
{
    // Основной фон с градиентом
    juce::ColourGradient bgGradient(
        juce::Colour(0xff1a1a1a), 0, 0,
        juce::Colour(0xff0a0a0a), 0, (float)getHeight(), false);
    g.setGradientFill(bgGradient);
    g.fillAll();

    // Верхняя панель с градиентом
    auto topPanel = getLocalBounds().removeFromTop(120).toFloat();
    juce::ColourGradient topGradient(
        juce::Colour(0xff2a2a2a), topPanel.getX(), topPanel.getY(),
        juce::Colour(0xff1a1a1a), topPanel.getX(), topPanel.getBottom(), false);
    g.setGradientFill(topGradient);
    g.fillRoundedRectangle(topPanel, 8.0f);

    // Логотип - современный шестиугольник
    juce::Path hex;
    float logoX = 60.0f, logoY = 60.0f, logoR = 30.0f;
    for (int i = 0; i < 6; ++i) {
        float angle = juce::MathConstants<float>::twoPi * i / 6.0f - juce::MathConstants<float>::halfPi;
        float px = logoX + logoR * std::cos(angle);
        float py = logoY + logoR * std::sin(angle);
        if (i == 0) hex.startNewSubPath(px, py);
        else hex.lineTo(px, py);
    }
    hex.closeSubPath();
    
    // Градиент для логотипа
    juce::ColourGradient logoGradient(
        juce::Colour(0xffffffff), logoX - logoR, logoY - logoR,
        juce::Colour(0xffe0e0e0), logoX + logoR, logoY + logoR, false);
    g.setGradientFill(logoGradient);
    g.fillPath(hex);
    
    // Обводка логотипа
    g.setColour(juce::Colour(0xffd0d0d0));
    g.strokePath(hex, juce::PathStrokeType(2.0f));

    // Текст Reverbix с современной типографикой
    g.setFont(juce::Font(juce::FontOptions().withHeight(42.0f)));
    juce::ColourGradient textGradient(
        juce::Colour(0xffffffff), 110, 30,
        juce::Colour(0xffe0e0e0), 110, 80, false);
    g.setGradientFill(textGradient);
    g.drawFittedText("Reverbix", 110, 30, 250, 60, juce::Justification::left, 1);

    // Подзаголовок
    g.setFont(juce::Font(juce::FontOptions().withHeight(14.0f)));
    g.setColour(juce::Colour(0xffa0a0a0));
    g.drawFittedText("Professional Reverb", 110, 85, 250, 20, juce::Justification::left, 1);
}

void ReverbEditor::resized()
{
    auto bounds = getLocalBounds().reduced(40, 160);
    bounds.removeFromTop(20);

    const int sliderWidth = 120;
    const int sliderHeight = 120;
    const int spacing = (bounds.getWidth() - sliderWidth * 4) / 3;
    int y = bounds.getY() + 20;
    int x = bounds.getX();

    roomSizeSlider.setBounds(x, y, sliderWidth, sliderHeight);
    roomSizeLabel.setBounds(x, y + sliderHeight + 5, sliderWidth, 20);
    x += sliderWidth + spacing;
    
    decayTimeSlider.setBounds(x, y, sliderWidth, sliderHeight);
    decayTimeLabel.setBounds(x, y + sliderHeight + 5, sliderWidth, 20);
    x += sliderWidth + spacing;
    
    stereoWidthSlider.setBounds(x, y, sliderWidth, sliderHeight);
    stereoWidthLabel.setBounds(x, y + sliderHeight + 5, sliderWidth, 20);
    x += sliderWidth + spacing;
    
    dryWetSlider.setBounds(x, y, sliderWidth, sliderHeight);
    dryWetLabel.setBounds(x, y + sliderHeight + 5, sliderWidth, 20);

    // Версия — в правом нижнем углу
    versionLabel.setBounds(getWidth() - 80, getHeight() - 25, 70, 20);
} 