#pragma once
// NF Brutal core: parameters, state, meter and block processing with NO plug-in wrapper.
// Shared (identical file) by the NF Brutal plug-in (licensed) and by the native insert inside Mixa Studio (covered by the Mixa licence).
// The wrappers only forward prepare / processBlock / state to this class. Edit it in the NF Brutal repo, then run tools/sync_to_mixa.sh.
#include <juce_audio_processors/juce_audio_processors.h>
#include "BrutalDSP.h"

namespace nfbrutal
{
inline constexpr const char* kStateType = "NF_BRUTAL_STATE";
#ifdef JucePlugin_VersionString
inline constexpr const char* kVersion = JucePlugin_VersionString;
#else
inline constexpr const char* kVersion = "1.0.0";
#endif

class BrutalCore
{
public:
    explicit BrutalCore(juce::AudioProcessor& owner) : apvts(owner, nullptr, kStateType, createParameters())
    {
        auto get = [this](const char* id) { return apvts.getRawParameterValue(id); };
        pSlay = get("slay"); pInput = get("input"); pGain = get("gain"); pBody = get("body"); pMix = get("mix"); pLofi = get("lofi"); pPower = get("power");
    }

    void prepare(double sr) { engine.prepare(sr); }

    void process(juce::AudioBuffer<float>& b)
    {
        brutal::Real r;
        r.slay = pSlay->load(); r.inputDb = pInput->load(); r.gainDb = pGain->load(); r.bodyDb = pBody->load(); r.mix = pMix->load();
        r.lofi = pLofi->load() > 0.5f; r.on = pPower->load() > 0.5f;
        engine.setParams(r);
        if (b.getNumChannels() >= 2) engine.process(b.getWritePointer(0), b.getWritePointer(1), b.getNumSamples());
        else if (b.getNumChannels() == 1) engine.process(b.getWritePointer(0), nullptr, b.getNumSamples());
        grDb.store(r.on ? engine.currentGrDb() : 0.f);
    }

    void getState(juce::MemoryBlock& dest) { if (auto xml = apvts.copyState().createXml()) juce::AudioProcessor::copyXmlToBinary(*xml, dest); }
    void setState(const void* data, int size)
    {
        if (auto xml = juce::AudioProcessor::getXmlFromBinary(data, size); xml && xml->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xml));
    }

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameters()
    {
        using R = juce::NormalisableRange<float>;
        using ID = juce::ParameterID;
        using Attr = juce::AudioParameterFloatAttributes;
        juce::AudioProcessorValueTreeState::ParameterLayout l;
        auto pct = [](float v, int) { return juce::String(juce::roundToInt(v * 100.f)) + " %"; };
        auto dB1 = [](float v, int) { return juce::String(v, 1) + " dB"; };
        l.add(std::make_unique<juce::AudioParameterFloat>(ID{"slay", 1}, "Slay", R(0.f, 1.f), 1.f, Attr().withStringFromValueFunction(pct)));
        l.add(std::make_unique<juce::AudioParameterFloat>(ID{"input", 1}, "Input", R(-96.f, 24.f, 0.1f), 0.f, Attr().withStringFromValueFunction(dB1)));
        R gainRange(0.f, 36.f, [](float, float, float n) { return brutal::gainNormToDb(n); }, [](float, float, float v) { return brutal::gainDbToNorm(v); });
        l.add(std::make_unique<juce::AudioParameterFloat>(ID{"gain", 1}, "Gain", gainRange, brutal::gainNormToDb(0.794418f), Attr().withStringFromValueFunction(dB1)));
        l.add(std::make_unique<juce::AudioParameterFloat>(ID{"body", 1}, "Body", R(-10.f, 0.f, 0.1f), -3.5f, Attr().withStringFromValueFunction(dB1)));
        l.add(std::make_unique<juce::AudioParameterFloat>(ID{"mix", 1}, "Mix", R(0.f, 1.f), 1.f, Attr().withStringFromValueFunction(pct)));
        l.add(std::make_unique<juce::AudioParameterBool>(ID{"lofi", 1}, "Lofi", false));
        l.add(std::make_unique<juce::AudioParameterBool>(ID{"power", 1}, "On", true));
        return l;
    }

    juce::AudioProcessorValueTreeState apvts;
    std::atomic<float> grDb { 0.f };      // gain reduction (dB, >= 0) for the meter

private:
    brutal::Engine engine;
    std::atomic<float> *pSlay = nullptr, *pInput = nullptr, *pGain = nullptr, *pBody = nullptr, *pMix = nullptr, *pLofi = nullptr, *pPower = nullptr;
};
}
