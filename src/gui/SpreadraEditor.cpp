#include "SpreadraEditor.h"
#include "../core/Version.h"
#include "BinaryData.h"

void CustomRotarySliderLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                                     float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                                                     juce::Slider& slider)
{
    auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat().reduced(8);
    auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
    auto toAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    auto lineW = juce::jmin(2.0f, radius * 0.4f);
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
// SpreadraEditor Implementation
// ============================================================================

SpreadraEditor::SpreadraEditor(SpreadraProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setSize(739, 350);
    
    // Create custom look and feel objects with specific colors
    dryWetLookAndFeel = std::make_unique<CustomRotarySliderLookAndFeel>(juce::Colour(0xff2196F3)); // Blue
    stereoWidthLookAndFeel = std::make_unique<CustomRotarySliderLookAndFeel>(juce::Colour(0xffE0E0E0)); // Light Gray
    


    // Setup Dry/Wet slider
    dryWetSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    dryWetSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    dryWetSlider.setRange(0.0, 100.0, 1.0);
    dryWetSlider.setValue(50.0);
    dryWetSlider.setLookAndFeel(dryWetLookAndFeel.get());
    addAndMakeVisible(dryWetSlider);

    // Setup Stereo Width slider
    stereoWidthSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    stereoWidthSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    stereoWidthSlider.setRange(0.0, 200.0, 1.0);
    stereoWidthSlider.setValue(100.0);
    stereoWidthSlider.setLookAndFeel(stereoWidthLookAndFeel.get());
    addAndMakeVisible(stereoWidthSlider);
    
    // Подключение к параметрам процессора
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
    backgroundImage = juce::ImageCache::getFromMemory(BinaryData::UI_png, BinaryData::UI_pngSize);
}

SpreadraEditor::~SpreadraEditor()
{
    dryWetSlider.setLookAndFeel(nullptr);
    stereoWidthSlider.setLookAndFeel(nullptr);
}

void SpreadraEditor::paint(juce::Graphics& g)
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
}

void SpreadraEditor::resized()
{
    auto bounds = getLocalBounds().reduced(40, 40);

    const int sliderWidth = 120;
    const int sliderHeight = 120;
    const int spacing = 213;
    
    int y = bounds.getY() + 103;
    int x = bounds.getX() + bounds.getWidth() / 4 - 1; // Center the two sliders

    stereoWidthSlider.setBounds(x, y, sliderWidth, sliderHeight);
    x += spacing;

    dryWetSlider.setBounds(x, y, sliderWidth, sliderHeight);

    // Версия — в правом нижнем углу
    versionLabel.setBounds(getWidth() - 80, getHeight() - 25, 70, 20);
} 