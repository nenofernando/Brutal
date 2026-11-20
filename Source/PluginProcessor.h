#pragma once
// NF Porrada - plug-in wrapper. The DSP lives in PorradaDSP.h, the UI in PorradaEditor.h.
#include <juce_audio_processors/juce_audio_processors.h>
#include "PorradaDSP.h"

class PorradaProcessor : public juce::AudioProcessor
{
public:
    PorradaProcessor();
    static juce::AudioProcessorValueTreeState::ParameterLayout layout();

    const juce::String getName() const override { return "NF Brutal"; }
    void prepareToPlay(double sr, int) override { engine.prepare(sr); }
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    double getTailLengthSeconds() const override { return 0.0; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock& d) override { if (auto xml = apvts.copyState().createXml()) copyXmlToBinary(*xml, d); }
    void setStateInformation(const void* d, int n) override { if (auto xml = getXmlFromBinary(d, n)) apvts.replaceState(juce::ValueTree::fromXml(*xml)); }
    bool isBusesLayoutSupported(const BusesLayout& l) const override
    { return l.getMainInputChannelSet() == l.getMainOutputChannelSet() && (l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo() || l.getMainOutputChannelSet() == juce::AudioChannelSet::mono()); }

    juce::AudioProcessorValueTreeState apvts;
    std::atomic<float> grDb { 0.f };     // gain reduction for the meter
private:
    porrada::Engine engine;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PorradaProcessor)
};
