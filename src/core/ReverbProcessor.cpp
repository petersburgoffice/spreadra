#include "ReverbProcessor.h"
#include "../gui/ReverbEditor.h"
 #include <algorithm>

//==============================================================================
ReverbProcessor::ReverbProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, juce::Identifier("ReverbParameters"), createParameterLayout())
{
    // Инициализация файлового логгера
    Logger::getInstance().initialize("Reverbix");
    SHIMMER_LOG_INFO("ReverbixProcessor initialized");
    
    // Инициализация параметров
    updateParameters();
}

ReverbProcessor::~ReverbProcessor()
{
    // SHIMMER_LOG_INFO("ReverbixProcessor shutting down");
    // Logger::getInstance().shutdown();
}

//==============================================================================
void ReverbProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // SHIMMER_LOG_INFO("Preparing to play: sampleRate=" + juce::String(sampleRate) + 
    //                  ", samplesPerBlock=" + juce::String(samplesPerBlock));
    
    reverbAlgorithm.prepare(sampleRate, samplesPerBlock);
    tempBuffer.setSize(getTotalNumOutputChannels(), samplesPerBlock);
    
    // Обновление метрик
    latencyMs = reverbAlgorithm.getLatency();
    
    // SHIMMER_LOG_INFO("Reverbix ready: latency=" + juce::String(latencyMs, 2) + "ms");
}

void ReverbProcessor::releaseResources()
{
    reverbAlgorithm.reset();
    tempBuffer.setSize(0, 0);
}

bool ReverbProcessor::isBusesLayoutSupported(const BusesLayout& busesLayout) const
{
    // Поддержка моно и стерео
    if (busesLayout.getMainOutputChannelSet() != juce::AudioChannelSet::mono() &&
        busesLayout.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    
    if (busesLayout.getMainOutputChannelSet() != busesLayout.getMainInputChannelSet())
        return false;
    
    return true;
}

void ReverbProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
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
juce::AudioProcessorEditor* ReverbProcessor::createEditor()
{
    return new ReverbEditor(*this);
}

bool ReverbProcessor::hasEditor() const
{
    return true;
}

//==============================================================================
const juce::String ReverbProcessor::getName() const
{
    return JucePlugin_Name;
}

bool ReverbProcessor::acceptsMidi() const
{
    return false;
}

bool ReverbProcessor::producesMidi() const
{
    return false;
}

bool ReverbProcessor::isMidiEffect() const
{
    return false;
}

double ReverbProcessor::getTailLengthSeconds() const
{
    float currentDecayTime = parameters.getRawParameterValue("decayTime")->load();
    float roomSize = parameters.getRawParameterValue("roomSize")->load();
    
    // Учитываем и decay time, и room size
    float calculatedTail = currentDecayTime + (roomSize / 10000.0f) * 5.0f;
    
    return std::clamp(static_cast<double>(calculatedTail), 1.0, 25.0);
}

//==============================================================================
int ReverbProcessor::getNumPrograms()
{
    return 1;
}

int ReverbProcessor::getCurrentProgram()
{
    return 0;
}

void ReverbProcessor::setCurrentProgram(int index)
{
    juce::ignoreUnused(index);
}

const juce::String ReverbProcessor::getProgramName(int index)
{
    juce::ignoreUnused(index);
    return {};
}

void ReverbProcessor::changeProgramName(int index, const juce::String& newName)
{
    juce::ignoreUnused(index, newName);
}

//==============================================================================
void ReverbProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void ReverbProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName(parameters.state.getType()))
            parameters.replaceState(juce::ValueTree::fromXml(*xmlState));
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout ReverbProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    
    // Reverbix parameters
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "roomSize", "Room Size", juce::NormalisableRange<float>(10.0f, 10000.0f, 10.0f), 5005.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 0) + " m²"; }));
    
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "decayTime", "Decay Time", juce::NormalisableRange<float>(0.1f, 20.0f, 0.1f), 10.05f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + " s"; }));
    
    // Mix parameters
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "dryWet", "Dry/Wet", juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f), 50.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 0) + "%"; }));
    
    // Stereo parameters  
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "stereoWidth", "Stereo Width", juce::NormalisableRange<float>(0.0f, 200.0f, 1.0f), 100.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 0) + "%"; }));
    
    return { params.begin(), params.end() };
}

void ReverbProcessor::updateParameters()
{
    // Получение параметров из AudioProcessorValueTreeState
    float roomSize = parameters.getRawParameterValue("roomSize")->load();
    float decayTime = parameters.getRawParameterValue("decayTime")->load();
    float dryWet = parameters.getRawParameterValue("dryWet")->load();
    float stereoWidth = parameters.getRawParameterValue("stereoWidth")->load();
    
    // Обновление параметров reverbix-ядра
    reverbAlgorithm.setRoomSize(roomSize);
    reverbAlgorithm.setDecayTime(decayTime);
    reverbAlgorithm.setDryWet(dryWet);
    reverbAlgorithm.setStereoWidth(stereoWidth);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ReverbProcessor();
} 