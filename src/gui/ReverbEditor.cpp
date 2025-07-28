#include "ReverbEditor.h"
#include "../core/Version.h"

void KnobLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle, juce::Slider& slider)
{
    auto radius = juce::jmin(width, height) / 2.0f - 6.0f;
    auto centreX = x + width * 0.5f;
    auto centreY = y + height * 0.5f;
    auto angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

    // Внешняя тень
    g.setColour(juce::Colour(0x40000000));
    g.fillEllipse(centreX - radius + 3, centreY - radius + 8, radius * 2.0f, radius * 1.5f);

    // Внешний ободок с градиентом
    juce::ColourGradient outerRimGradient(
        juce::Colour(0xffe0e0e0), centreX, centreY - radius,
        juce::Colour(0xff2a2a2a), centreX, centreY + radius, true);
    outerRimGradient.addColour(0.3, juce::Colour(0xff4a4a4a));
    outerRimGradient.addColour(0.7, juce::Colour(0xff1a1a1a));
    outerRimGradient.addColour(0.9, juce::Colour(0xffe0e0e0).withAlpha(0.8f));
    g.setGradientFill(outerRimGradient);
    g.fillEllipse(centreX - radius, centreY - radius, radius * 2.0f, radius * 2.0f);

    // Внутренний ободок (более темный)
    float innerRimR = radius * 0.92f;
    juce::ColourGradient innerRimGradient(
        juce::Colour(0xff3a3a3a), centreX, centreY - innerRimR,
        juce::Colour(0xff0a0a0a), centreX, centreY + innerRimR, false);
    g.setGradientFill(innerRimGradient);
    g.fillEllipse(centreX - innerRimR, centreY - innerRimR, innerRimR * 2.0f, innerRimR * 2.0f);

    // Основная поверхность ручки
    float bodyR = radius * 0.85f;
    juce::ColourGradient bodyGradient(
        juce::Colour(0xff5a5a5a), centreX, centreY - bodyR * 0.8f,
        juce::Colour(0xff1a1a1a), centreX, centreY + bodyR, false);
    bodyGradient.addColour(0.4, juce::Colour(0xff3a3a3a));
    g.setGradientFill(bodyGradient);
    g.fillEllipse(centreX - bodyR, centreY - bodyR, bodyR * 2.0f, bodyR * 2.0f);

    // Верхний блик
    g.setColour(juce::Colour(0x99ffffff));
    g.fillEllipse(centreX - bodyR * 0.7f, centreY - bodyR * 0.7f, bodyR * 1.4f, bodyR * 0.6f);

    // Центральная впадина
    float centerR = bodyR * 0.3f;
    juce::ColourGradient centerGradient(
        juce::Colour(0xff2a2a2a), centreX, centreY - centerR,
        juce::Colour(0xff0a0a0a), centreX, centreY + centerR, false);
    g.setGradientFill(centerGradient);
    g.fillEllipse(centreX - centerR, centreY - centerR, centerR * 2.0f, centerR * 2.0f);

    // Указатель (более стильный)
    g.setColour(juce::Colours::white);
    float pointerLength = bodyR * 0.75f;
    float pointerThickness = 3.0f;
    float startFrac = 0.25f;
    
    juce::Path pointerPath;
    pointerPath.startNewSubPath(
        centreX + bodyR * startFrac * std::cos(angle - juce::MathConstants<float>::halfPi),
        centreY + bodyR * startFrac * std::sin(angle - juce::MathConstants<float>::halfPi));
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

ReverbEditor::ReverbEditor(ReverbProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setSize(700, 340); // Ближе к референсу
    setLookAndFeel(&knobLookAndFeel);
    
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
    roomSizeSlider.setLookAndFeel(&knobLookAndFeel);
    
    decayTimeSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    decayTimeSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0); // Убираем текстовое поле
    decayTimeSlider.setRange(0.1, 20.0, 0.1);
    decayTimeSlider.setValue(10.05); // Середина диапазона для 12 часов
    decayTimeSlider.setLookAndFeel(&knobLookAndFeel);
    
    dryWetSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    dryWetSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0); // Убираем текстовое поле
    dryWetSlider.setRange(0.0, 100.0, 1.0);
    dryWetSlider.setValue(50.0); // Середина диапазона для 12 часов
    dryWetSlider.setLookAndFeel(&knobLookAndFeel);
    
    stereoWidthSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    stereoWidthSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0); // Убираем текстовое поле
    stereoWidthSlider.setRange(0.0, 200.0, 1.0);
    stereoWidthSlider.setValue(100.0); // Середина диапазона для 12 часов
    stereoWidthSlider.setLookAndFeel(&knobLookAndFeel);
    
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
    // Основной фон с глубоким градиентом
    juce::ColourGradient bgGradient(
        juce::Colour(0xff2a2a2a), 0, 0,
        juce::Colour(0xff1a1a1a), 0, (float)getHeight(), false);
    g.setGradientFill(bgGradient);
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 8.0f);

    // Внутренняя тень для глубины
    g.setColour(juce::Colour(0x20000000));
    g.fillRoundedRectangle(getLocalBounds().reduced(2).toFloat(), 6.0f);

    // Основная панель с градиентом
    auto panelBounds = getLocalBounds().reduced(8).toFloat();
    juce::ColourGradient panelGradient(
        juce::Colour(0xff3a3a3a), panelBounds.getX(), panelBounds.getY(),
        juce::Colour(0xff2a2a2a), panelBounds.getX(), panelBounds.getBottom(), false);
    g.setGradientFill(panelGradient);
    g.fillRoundedRectangle(panelBounds, 6.0f);

    // Верхний блик панели
    g.setColour(juce::Colour(0x15ffffff));
    g.fillRoundedRectangle(panelBounds.removeFromBottom(panelBounds.getHeight() * 0.5f), 6.0f);

    // Логотип - улучшенный шестиугольник с градиентом
    juce::Path hex;
    float logoX = 40.0f, logoY = 40.0f, logoR = 24.0f;
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

    // Текст Reverbix с улучшенной типографикой
    g.setFont(juce::Font(juce::FontOptions().withHeight(38.0f)));
    juce::ColourGradient textGradient(
        juce::Colour(0xffffffff), 80, 20,
        juce::Colour(0xffe0e0e0), 80, 60, false);
    g.setGradientFill(textGradient);
    g.drawFittedText("Reverbix", 80, 18, 220, 48, juce::Justification::left, 1);

    // Подписи под ручками с улучшенной типографикой
    g.setFont(juce::Font(juce::FontOptions().withHeight(16.0f)));
    g.setColour(juce::Colour(0xffe0e0e0));
    auto r1 = roomSizeSlider.getBounds();
    auto r2 = decayTimeSlider.getBounds();
    auto r3 = stereoWidthSlider.getBounds();
    auto r4 = dryWetSlider.getBounds();
    g.drawFittedText("Room size", r1.getX(), r1.getBottom() + 12, r1.getWidth(), 24, juce::Justification::centred, 1);
    g.drawFittedText("Decay time", r2.getX(), r2.getBottom() + 12, r2.getWidth(), 24, juce::Justification::centred, 1);
    g.drawFittedText("Stereo Width", r3.getX(), r3.getBottom() + 12, r3.getWidth(), 24, juce::Justification::centred, 1);
    g.drawFittedText("Dry/Wet", r4.getX(), r4.getBottom() + 12, r4.getWidth(), 24, juce::Justification::centred, 1);
}

void ReverbEditor::resized()
{
    auto bounds = getLocalBounds().reduced(36);
    bounds.removeFromTop(60); // Место для заголовка и логотипа

    const int sliderWidth = 120;
    const int sliderHeight = 120;
    const int spacing = (bounds.getWidth() - sliderWidth * 4) / 3;
    int y = bounds.getY() + 20;
    int x = bounds.getX();

    roomSizeSlider.setBounds(x, y, sliderWidth, sliderHeight);
    x += sliderWidth + spacing;
    decayTimeSlider.setBounds(x, y, sliderWidth, sliderHeight);
    x += sliderWidth + spacing;
    stereoWidthSlider.setBounds(x, y, sliderWidth, sliderHeight);
    x += sliderWidth + spacing;
    dryWetSlider.setBounds(x, y, sliderWidth, sliderHeight);

    // Версия — в правом нижнем углу
    versionLabel.setBounds(getWidth() - 80, getHeight() - 25, 70, 20);
} 