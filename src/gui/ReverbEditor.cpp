#include "ReverbEditor.h"
#include "../core/Version.h"
#include "BinaryData.h"

void CustomRotarySliderLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                                     float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                                                     juce::Slider& slider)
{
    auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat().reduced(8);
    auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
    auto toAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    auto lineW = juce::jmin(6.0f, radius * 0.4f);
    auto arcRadius = radius - lineW * 0.5f;

    // Draw the background arc (full circle)
    juce::Path backgroundArc;
    backgroundArc.addCentredArc(bounds.getCentreX(),
                               bounds.getCentreY(),
                               arcRadius,
                               arcRadius,
                               0.0f,
                               rotaryStartAngle,
                               rotaryEndAngle,
                               true);

    g.setColour(juce::Colour(0xff2a2a2a));
    g.strokePath(backgroundArc, juce::PathStrokeType(lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Draw the value arc
    if (slider.isEnabled())
    {
        juce::Path valueArc;
        valueArc.addCentredArc(bounds.getCentreX(),
                              bounds.getCentreY(),
                              arcRadius,
                              arcRadius,
                              0.0f,
                              rotaryStartAngle,
                              toAngle,
                              true);

        g.setColour(ringColour);
        g.strokePath(valueArc, juce::PathStrokeType(lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // Draw the knob with consistent size
    auto knobRadius = radius - lineW * 1.8f;
    auto knobBounds = juce::Rectangle<float>(knobRadius * 2.0f, knobRadius * 2.0f).withCentre(bounds.getCentre());
    
    // Knob shadow
    auto shadowBounds = knobBounds.translated(1, 2);
    g.setColour(juce::Colour(0x30000000));
    g.fillEllipse(shadowBounds);
    
    // Knob gradient from light to dark (simulating light from top-left)
    juce::ColourGradient knobGradient(juce::Colour(0xff4a4a4a), knobBounds.getTopLeft(),
                                     juce::Colour(0xff2a2a2a), knobBounds.getBottomRight(), false);
    g.setGradientFill(knobGradient);
    g.fillEllipse(knobBounds);

    // Knob border
    g.setColour(juce::Colour(0xff1a1a1a));
    g.drawEllipse(knobBounds, 1.0f);

    // Draw the pointer with consistent style
    juce::Path pointer;
    auto pointerLength = knobRadius * 0.7f;
    auto pointerThickness = 2.0f;
    pointer.addRectangle(-pointerThickness * 0.5f, -knobRadius + pointerLength * 0.4f, pointerThickness, pointerLength);
    
    pointer.applyTransform(juce::AffineTransform::rotation(toAngle).translated(bounds.getCentreX(), bounds.getCentreY()));
    
    g.setColour(ringColour.brighter(0.4f));
    g.fillPath(pointer);
}

// ============================================================================
// ReverbEditor Implementation
// ============================================================================

ReverbEditor::ReverbEditor(ReverbProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setSize(800, 500);
    
    // Create custom look and feel objects with specific colors
    roomSizeLookAndFeel = std::make_unique<CustomRotarySliderLookAndFeel>(juce::Colour(0xff00BCD4)); // Cyan/Teal
    decayTimeLookAndFeel = std::make_unique<CustomRotarySliderLookAndFeel>(juce::Colour(0xffFF9800)); // Orange
    dryWetLookAndFeel = std::make_unique<CustomRotarySliderLookAndFeel>(juce::Colour(0xff2196F3)); // Blue
    stereoWidthLookAndFeel = std::make_unique<CustomRotarySliderLookAndFeel>(juce::Colour(0xffE0E0E0)); // Light Gray
    
    // Setup Room Size slider
    roomSizeSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    roomSizeSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    roomSizeSlider.setRange(10.0, 10000.0, 10.0);
    roomSizeSlider.setValue(5005.0);
    roomSizeSlider.setLookAndFeel(roomSizeLookAndFeel.get());
    addAndMakeVisible(roomSizeSlider);

    roomSizeLabel.setText("ROOM SIZE", juce::dontSendNotification);
    roomSizeLabel.setFont(juce::Font(14.0f, juce::Font::plain));
    roomSizeLabel.setJustificationType(juce::Justification::centred);
    roomSizeLabel.setColour(juce::Label::textColourId, juce::Colour(0xff00BCD4));
    addAndMakeVisible(roomSizeLabel);

    // Setup Decay Time slider
    decayTimeSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    decayTimeSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    decayTimeSlider.setRange(0.1, 20.0, 0.1);
    decayTimeSlider.setValue(10.05);
    decayTimeSlider.setLookAndFeel(decayTimeLookAndFeel.get());
    addAndMakeVisible(decayTimeSlider);

    decayTimeLabel.setText("DECAY TIME", juce::dontSendNotification);
    decayTimeLabel.setFont(juce::Font(14.0f, juce::Font::plain));
    decayTimeLabel.setJustificationType(juce::Justification::centred);
    decayTimeLabel.setColour(juce::Label::textColourId, juce::Colour(0xffFF9800));
    addAndMakeVisible(decayTimeLabel);

    // Setup Dry/Wet slider
    dryWetSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    dryWetSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    dryWetSlider.setRange(0.0, 100.0, 1.0);
    dryWetSlider.setValue(50.0);
    dryWetSlider.setLookAndFeel(dryWetLookAndFeel.get());
    addAndMakeVisible(dryWetSlider);

    dryWetLabel.setText("DRY/WET", juce::dontSendNotification);
    dryWetLabel.setFont(juce::Font(14.0f, juce::Font::plain));
    dryWetLabel.setJustificationType(juce::Justification::centred);
    dryWetLabel.setColour(juce::Label::textColourId, juce::Colour(0xff2196F3));
    addAndMakeVisible(dryWetLabel);

    // Setup Stereo Width slider
    stereoWidthSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    stereoWidthSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    stereoWidthSlider.setRange(0.0, 200.0, 1.0);
    stereoWidthSlider.setValue(100.0);
    stereoWidthSlider.setLookAndFeel(stereoWidthLookAndFeel.get());
    addAndMakeVisible(stereoWidthSlider);

    stereoWidthLabel.setText("STEREO WIDTH", juce::dontSendNotification);
    stereoWidthLabel.setFont(juce::Font(14.0f, juce::Font::plain));
    stereoWidthLabel.setJustificationType(juce::Justification::centred);
    stereoWidthLabel.setColour(juce::Label::textColourId, juce::Colour(0xffE0E0E0));
    addAndMakeVisible(stereoWidthLabel);
    
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
    
    // Загрузка фонового изображения из binary resources
    backgroundImage = juce::ImageCache::getFromMemory(BinaryData::bg_png, BinaryData::bg_pngSize);
}

ReverbEditor::~ReverbEditor()
{
    roomSizeSlider.setLookAndFeel(nullptr);
    decayTimeSlider.setLookAndFeel(nullptr);
    dryWetSlider.setLookAndFeel(nullptr);
    stereoWidthSlider.setLookAndFeel(nullptr);
}

void ReverbEditor::paint(juce::Graphics& g)
{
    // Отрисовка фонового изображения
    if (backgroundImage.isValid())
    {
        g.drawImage(backgroundImage, getLocalBounds().toFloat(), 
                   juce::RectanglePlacement::fillDestination);
    }
    else
    {
        // Резервный градиентный фон, если изображение не загружено
        juce::ColourGradient bgGradient(
            juce::Colour(0xff1a1a1a), 0, 0,
            juce::Colour(0xff0a0a0a), 0, (float)getHeight(), false);
        g.setGradientFill(bgGradient);
        g.fillAll();
    }

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