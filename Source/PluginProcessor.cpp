#include "PluginProcessor.h"
#include "PorradaEditor.h"
using namespace juce;

PorradaProcessor::PorradaProcessor()
    : AudioProcessor(BusesProperties().withInput("Input", AudioChannelSet::stereo(), true).withOutput("Output", AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "BRUTAL", layout()) {}

AudioProcessorValueTreeState::ParameterLayout PorradaProcessor::layout()
{
    using R = NormalisableRange<float>;
    AudioProcessorValueTreeState::ParameterLayout l;
    auto pct = [](float v, int) { return String(roundToInt(v * 100.f)) + " %"; };
    auto dB1 = [](float v, int) { return String(v, 1) + " dB"; };
    l.add(std::make_unique<AudioParameterFloat>(ParameterID{"slay", 1}, "Slay", R(0.f, 1.f), 1.f, AudioParameterFloatAttributes().withStringFromValueFunction(pct)));
    l.add(std::make_unique<AudioParameterFloat>(ParameterID{"input", 1}, "Input", R(-96.f, 24.f, 0.1f), 0.f, AudioParameterFloatAttributes().withStringFromValueFunction(dB1)));
    R gainRange(0.f, 36.f, [](float, float, float n) { return porrada::gainNormToDb(n); }, [](float, float, float v) { return porrada::gainDbToNorm(v); });
    l.add(std::make_unique<AudioParameterFloat>(ParameterID{"gain", 1}, "Gain", gainRange, porrada::gainNormToDb(0.794418f), AudioParameterFloatAttributes().withStringFromValueFunction(dB1)));
    l.add(std::make_unique<AudioParameterFloat>(ParameterID{"body", 1}, "Body", R(-10.f, 0.f, 0.1f), -3.5f, AudioParameterFloatAttributes().withStringFromValueFunction(dB1)));
    l.add(std::make_unique<AudioParameterFloat>(ParameterID{"mix", 1}, "Mix", R(0.f, 1.f), 1.f, AudioParameterFloatAttributes().withStringFromValueFunction(pct)));
    l.add(std::make_unique<AudioParameterBool>(ParameterID{"lofi", 1}, "Lofi", false));
    l.add(std::make_unique<AudioParameterBool>(ParameterID{"power", 1}, "On", true));
    return l;
}

void PorradaProcessor::processBlock(AudioBuffer<float>& b, MidiBuffer&)
{
    ScopedNoDenormals nd;
    porrada::Real r;
    r.slay = *apvts.getRawParameterValue("slay"); r.inputDb = *apvts.getRawParameterValue("input"); r.gainDb = *apvts.getRawParameterValue("gain");
    r.bodyDb = *apvts.getRawParameterValue("body"); r.mix = *apvts.getRawParameterValue("mix");
    r.lofi = *apvts.getRawParameterValue("lofi") > 0.5f; r.on = *apvts.getRawParameterValue("power") > 0.5f;
    engine.setParams(r);
    if (b.getNumChannels() >= 2) engine.process(b.getWritePointer(0), b.getWritePointer(1), b.getNumSamples());
    else if (b.getNumChannels() == 1) engine.process(b.getWritePointer(0), nullptr, b.getNumSamples());
    grDb.store(r.on ? engine.currentGrDb() : 0.f);
}

AudioProcessorEditor* PorradaProcessor::createEditor() { return new nfporradaui::PorradaEditor(*this); }

#ifndef NFPORRADA_NO_ENTRY
AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new PorradaProcessor(); }
#endif
