#include "SpreadraProcessor.h"
#include "../gui/SpreadraEditor.h"
 #include <algorithm>

//==============================================================================
SpreadraProcessor::SpreadraProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, juce::Identifier("SpreadraParameters"), createParameterLayout())
{
    // Инициализация файлового логгера
    Logger::getInstance().initialize("Spreadra");
    SHIMMER_LOG_INFO("SpreadraProcessor initialized");
    
    // Инициализация параметров
    updateParameters();
}

SpreadraProcessor::~SpreadraProcessor()
{
    // SHIMMER_LOG_INFO("SpreadraProcessor shutting down");
    // Logger::getInstance().shutdown();
}

//==============================================================================
void SpreadraProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // SHIMMER_LOG_INFO("Preparing to play: sampleRate=" + juce::String(sampleRate) + 
    //                  ", samplesPerBlock=" + juce::String(samplesPerBlock));
    
    reverbAlgorithm.prepare(sampleRate, samplesPerBlock);
    tempBuffer.setSize(getTotalNumOutputChannels(), samplesPerBlock);
    
    // Обновление метрик
    latencyMs = reverbAlgorithm.getLatency();
    
    // SHIMMER_LOG_INFO("Spreadra ready: latency=" + juce::String(latencyMs, 2) + "ms");
}

void SpreadraProcessor::releaseResources()
{
    reverbAlgorithm.reset();
    tempBuffer.setSize(0, 0);
}

bool SpreadraProcessor::isBusesLayoutSupported(const BusesLayout& busesLayout) const
{
    // Поддержка моно и стерео
    if (busesLayout.getMainOutputChannelSet() != juce::AudioChannelSet::mono() &&
        busesLayout.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    
    if (busesLayout.getMainOutputChannelSet() != busesLayout.getMainInputChannelSet())
        return false;
    
    return true;
}

void SpreadraProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    
    const int totalNumInputChannels = getTotalNumInputChannels();
    const int totalNumOutputChannels = getTotalNumOutputChannels();
    const int numSamples = buffer.getNumSamples();
    
    // Очистка неиспользуемых каналов
    for (int i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, numSamples);
    
    // Обновление параметров если изменились
    updateParameters();
    
    // УПРОЩЕНО: всегда используем стерео обработку
    const float* inputL = buffer.getReadPointer(0);
    const float* inputR = (totalNumInputChannels > 1) ? buffer.getReadPointer(1) : inputL; // Дублируем L если моно
    
    float* outputL = buffer.getWritePointer(0);
    float* outputR = (totalNumOutputChannels > 1) ? buffer.getWritePointer(1) : outputL; // Используем L если моно выход
    
    reverbAlgorithm.processStereo(inputL, inputR, outputL, outputR, numSamples);
    
    // Обновление метрик производительности
    cpuUsage = reverbAlgorithm.getCpuUsage();
}

//==============================================================================
juce::AudioProcessorEditor* SpreadraProcessor::createEditor()
{
    return new SpreadraEditor(*this);
}

bool SpreadraProcessor::hasEditor() const
{
    return true;
}

//==============================================================================
const juce::String SpreadraProcessor::getName() const
{
    return JucePlugin_Name;
}

bool SpreadraProcessor::acceptsMidi() const
{
    return false;
}

bool SpreadraProcessor::producesMidi() const
{
    return false;
}

bool SpreadraProcessor::isMidiEffect() const
{
    return false;
}

double SpreadraProcessor::getTailLengthSeconds() const
{
    float currentDecayTime = parameters.getRawParameterValue("decayTime")->load();
    float roomSize = parameters.getRawParameterValue("roomSize")->load();
    
    // Учитываем и decay time, и room size
    float calculatedTail = currentDecayTime + (roomSize / 10000.0f) * 5.0f;
    
    return std::clamp(static_cast<double>(calculatedTail), 1.0, 25.0);
}

//==============================================================================
int SpreadraProcessor::getNumPrograms()
{
    return 1;
}

int SpreadraProcessor::getCurrentProgram()
{
    return 0;
}

void SpreadraProcessor::setCurrentProgram(int index)
{
    juce::ignoreUnused(index);
}

const juce::String SpreadraProcessor::getProgramName(int index)
{
    juce::ignoreUnused(index);
    return {};
}

void SpreadraProcessor::changeProgramName(int index, const juce::String& newName)
{
    juce::ignoreUnused(index, newName);
}

//==============================================================================
void SpreadraProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void SpreadraProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName(parameters.state.getType()))
            parameters.replaceState(juce::ValueTree::fromXml(*xmlState));
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout SpreadraProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    
    // Spreadra parameters
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "dryWet", "Dry/Wet", juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f), 50.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 0) + "%"; }));
    
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "stereoWidth", "Stereo Width", juce::NormalisableRange<float>(0.0f, 200.0f, 1.0f), 100.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 0) + "%"; }));
    
    return { params.begin(), params.end() };
}

void SpreadraProcessor::updateParameters()
{
    // Получение параметров из AudioProcessorValueTreeState
    float dryWet = parameters.getRawParameterValue("dryWet")->load();
    float stereoWidth = parameters.getRawParameterValue("stereoWidth")->load();
    
    // Обновление параметров Spreadra-ядра
    reverbAlgorithm.setDryWet(dryWet);
    reverbAlgorithm.setStereoWidth(stereoWidth);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SpreadraProcessor();
} 