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
    auto angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

    // Внешняя тень
    g.setColour(juce::Colour(0x30000000));
    g.fillEllipse(centreX - radius + 2, centreY - radius + 4, radius * 2.0f, radius * 2.0f);

    // Основной круг с градиентом
    juce::ColourGradient mainGradient(
        juce::Colour(0xff4a4a4a), centreX, centreY - radius,
        juce::Colour(0xff1a1a1a), centreX, centreY + radius, true);
    mainGradient.addColour(0.3, juce::Colour(0xff3a3a3a));
    mainGradient.addColour(0.7, juce::Colour(0xff2a2a2a));
    g.setGradientFill(mainGradient);
    g.fillEllipse(centreX - radius, centreY - radius, radius * 2.0f, radius * 2.0f);

    // Внутренний круг (более светлый)
    float innerRadius = radius * 0.85f;
    juce::ColourGradient innerGradient(
        juce::Colour(0xff6a6a6a), centreX, centreY - innerRadius * 0.8f,
        juce::Colour(0xff2a2a2a), centreX, centreY + innerRadius, false);
    innerGradient.addColour(0.4, juce::Colour(0xff4a4a4a));
    g.setGradientFill(innerGradient);
    g.fillEllipse(centreX - innerRadius, centreY - innerRadius, innerRadius * 2.0f, innerRadius * 2.0f);

    // Верхний блик
    g.setColour(juce::Colour(0x40ffffff));
    g.fillEllipse(centreX - innerRadius * 0.7f, centreY - innerRadius * 0.7f, innerRadius * 1.4f, innerRadius * 0.5f);

    // Центральная впадина
    float centerRadius = innerRadius * 0.25f;
    juce::ColourGradient centerGradient(
        juce::Colour(0xff3a3a3a), centreX, centreY - centerRadius,
        juce::Colour(0xff1a1a1a), centreX, centreY + centerRadius, false);
    g.setGradientFill(centerGradient);
    g.fillEllipse(centreX - centerRadius, centreY - centerRadius, centerRadius * 2.0f, centerRadius * 2.0f);

    // Указатель
    g.setColour(juce::Colours::white);
    float pointerLength = innerRadius * 0.7f;
    float pointerThickness = 2.5f;
    float startFrac = 0.2f;
    
    juce::Path pointerPath;
    pointerPath.startNewSubPath(
        centreX + innerRadius * startFrac * std::cos(angle - juce::MathConstants<float>::halfPi),
        centreY + innerRadius * startFrac * std::sin(angle - juce::MathConstants<float>::halfPi));
    pointerPath.lineTo(
        centreX + pointerLength * std::cos(angle - juce::MathConstants<float>::halfPi),
        centreY + pointerLength * std::sin(angle - juce::MathConstants<float>::halfPi));
    
    // Тень указателя
    g.setColour(juce::Colour(0x40000000));
    juce::Path shadowPath = pointerPath;
    shadowPath.applyTransform(juce::AffineTransform::translation(1, 1));
    g.strokePath(shadowPath, juce::PathStrokeType(pointerThickness, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    
    // Сам указатель
    g.setColour(juce::Colours::white);
    g.strokePath(pointerPath, juce::PathStrokeType(pointerThickness, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void ModernKnobLookAndFeel::drawLabel(juce::Graphics& g, juce::Label& label)
{
    g.setColour(juce::Colour(0xffe0e0e0));
    g.setFont(juce::Font(juce::FontOptions().withHeight(12.0f)));
    g.drawFittedText(label.getText(), label.getLocalBounds(), juce::Justification::centred, 1);
}

// ============================================================================
// AnimatedSlider Implementation
// ============================================================================

AnimatedSlider::AnimatedSlider(const juce::String& labelText)
    : label(labelText)
{
    setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    setLookAndFeel(&lookAndFeel);
    
    addAndMakeVisible(valueLabel);
    valueLabel.setJustificationType(juce::Justification::centred);
    valueLabel.setColour(juce::Label::textColourId, juce::Colour(0xffe0e0e0));
    valueLabel.setFont(juce::Font(juce::FontOptions().withHeight(10.0f)));
    
    startTimerHz(30);
}

AnimatedSlider::~AnimatedSlider()
{
    stopTimer();
}

void AnimatedSlider::paint(juce::Graphics& g)
{
    // Рисуем свечение если активен
    if (glowIntensity > 0.0f)
    {
        auto bounds = getLocalBounds().toFloat();
        g.setColour(juce::Colour::fromFloatRGBA(1.0f, 1.0f, 1.0f, glowIntensity * 0.3f));
        g.fillEllipse(bounds);
    }
    
    // Рисуем слайдер через LookAndFeel
    lookAndFeel.drawRotarySlider(g, 0, 0, getWidth(), getHeight(), 
                                (float)getValue() / (getMaximum() - getMinimum()), 
                                juce::MathConstants<float>::pi * 1.2f, 
                                juce::MathConstants<float>::pi * 2.8f, *this);
}

void AnimatedSlider::resized()
{
    auto bounds = getLocalBounds();
    auto knobBounds = bounds.removeFromTop(bounds.getHeight() * 0.8f);
    juce::Slider::setBounds(knobBounds);
    
    valueLabel.setBounds(bounds);
}

void AnimatedSlider::timerCallback()
{
    animationPhase += 0.1f;
    if (animationPhase > juce::MathConstants<float>::twoPi)
        animationPhase -= juce::MathConstants<float>::twoPi;
    
    // Плавное изменение свечения
    glowIntensity = 0.5f + 0.3f * std::sin(animationPhase);
    repaint();
}

// ============================================================================
// ReverbEditor Implementation
// ============================================================================

ReverbEditor::ReverbEditor(ReverbProcessor& p)
    : AudioProcessorEditor(&p), processor(p),
      roomSizeSlider("Room Size"),
      decayTimeSlider("Decay Time"),
      dryWetSlider("Dry/Wet"),
      stereoWidthSlider("Stereo Width")
{
    setSize(800, 500);
    
    // Настройка диапазонов
    roomSizeSlider.setRange(10.0, 10000.0, 10.0);
    roomSizeSlider.setValue(5005.0);
    
    decayTimeSlider.setRange(0.1, 20.0, 0.1);
    decayTimeSlider.setValue(10.05);
    
    dryWetSlider.setRange(0.0, 100.0, 1.0);
    dryWetSlider.setValue(50.0);
    
    stereoWidthSlider.setRange(0.0, 200.0, 1.0);
    stereoWidthSlider.setValue(100.0);
    
    // Добавление слайдеров
    addAndMakeVisible(roomSizeSlider);
    addAndMakeVisible(decayTimeSlider);
    addAndMakeVisible(dryWetSlider);
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
    
    // Запуск анимации
    startTimerHz(30);
}

ReverbEditor::~ReverbEditor()
{
    stopTimer();
}

void ReverbEditor::paint(juce::Graphics& g)
{
    // Основной фон с анимированным градиентом
    juce::ColourGradient bgGradient(
        juce::Colour(0xff1a1a1a), 0, 0,
        juce::Colour(0xff0a0a0a), 0, (float)getHeight(), false);
    bgGradient.addColour(0.5f + 0.1f * std::sin(backgroundPhase), juce::Colour(0xff151515));
    g.setGradientFill(bgGradient);
    g.fillAll();

    // Верхняя панель с градиентом
    auto topPanel = getLocalBounds().removeFromTop(120).toFloat();
    juce::ColourGradient topGradient(
        juce::Colour(0xff2a2a2a), topPanel.getX(), topPanel.getY(),
        juce::Colour(0xff1a1a1a), topPanel.getX(), topPanel.getBottom(), false);
    g.setGradientFill(topGradient);
    g.fillRoundedRectangle(topPanel, 8.0f);

    // Логотип - современный шестиугольник с анимацией
    juce::Path hex;
    float logoX = 60.0f, logoY = 60.0f, logoR = 30.0f + 2.0f * std::sin(logoGlow);
    for (int i = 0; i < 6; ++i) {
        float angle = juce::MathConstants<float>::twoPi * i / 6.0f - juce::MathConstants<float>::halfPi;
        float px = logoX + logoR * std::cos(angle);
        float py = logoY + logoR * std::sin(angle);
        if (i == 0) hex.startNewSubPath(px, py);
        else hex.lineTo(px, py);
    }
    hex.closeSubPath();
    
    // Градиент для логотипа с анимацией
    juce::ColourGradient logoGradient(
        juce::Colour(0xffffffff), logoX - logoR, logoY - logoR,
        juce::Colour(0xffe0e0e0), logoX + logoR, logoY + logoR, false);
    logoGradient.addColour(0.5f, juce::Colour(0xfff0f0f0));
    g.setGradientFill(logoGradient);
    g.fillPath(hex);
    
    // Обводка логотипа с свечением
    g.setColour(juce::Colour(0xffd0d0d0).withAlpha(0.8f + 0.2f * std::sin(logoGlow)));
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

    // Основная панель для слайдеров с анимацией
    auto mainPanel = getLocalBounds().reduced(20, 140).toFloat();
    juce::ColourGradient panelGradient(
        juce::Colour(0xff3a3a3a), mainPanel.getX(), mainPanel.getY(),
        juce::Colour(0xff2a2a2a), mainPanel.getX(), mainPanel.getBottom(), false);
    panelGradient.addColour(0.3f + 0.1f * std::sin(backgroundPhase), juce::Colour(0xff323232));
    g.setGradientFill(panelGradient);
    g.fillRoundedRectangle(mainPanel, 12.0f);

    // Внутренняя тень панели
    g.setColour(juce::Colour(0x20000000));
    g.fillRoundedRectangle(mainPanel.reduced(2), 10.0f);

    // Верхний блик панели
    g.setColour(juce::Colour(0x15ffffff));
    g.fillRoundedRectangle(mainPanel.removeFromBottom(mainPanel.getHeight() * 0.5f), 10.0f);
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

void ReverbEditor::timerCallback()
{
    backgroundPhase += 0.02f;
    if (backgroundPhase > juce::MathConstants<float>::twoPi)
        backgroundPhase -= juce::MathConstants<float>::twoPi;
    
    logoGlow += 0.05f;
    if (logoGlow > juce::MathConstants<float>::twoPi)
        logoGlow -= juce::MathConstants<float>::twoPi;
    
    repaint();
} 