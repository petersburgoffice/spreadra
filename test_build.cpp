#include <juce_core/juce_core.h>
#include <juce_audio_processors/juce_audio_processors.h>

int main()
{
    juce::MessageManager::getInstance();
    juce::Logger::writeToLog("Shimmer plugin test build successful!");
    return 0;
} 