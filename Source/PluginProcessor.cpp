#include "PluginProcessor.h"
#include "Shared/BrutalEditor.h"
#include "NFBrutalBinaryData.h"
#if BRUTAL_LICENSE_ENFORCE
 #include "License/BrutalLicenseOverlay.h"
#endif

BrutalAudioProcessor::BrutalAudioProcessor()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true).withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
   #if BRUTAL_LICENSE_ENFORCE
    licensed.store(NFBRLicense::isLicensed());
    licenceWatch.startThread();
   #endif
}

void BrutalAudioProcessor::processBlock(juce::AudioBuffer<float>& b, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals nd;
   #if BRUTAL_LICENSE_ENFORCE
    if (!licensed.load()) { b.clear(); return; }   // unlicensed: silence (the host's bypass never reaches here, so it still passes audio through)
   #endif
    core.process(b);
}

BrutalAudioProcessor::~BrutalAudioProcessor()
{
   #if BRUTAL_LICENSE_ENFORCE
    licenceWatch.stopThread(10000);
   #endif
}

juce::AudioProcessorEditor* BrutalAudioProcessor::createEditor()
{
    auto* ed = new nfbrutalui::BrutalEditor(*this, core, juce::ImageCache::getFromMemory(NFBrutalBinaryData::NF_Audio_Tools_Logo_png, NFBrutalBinaryData::NF_Audio_Tools_Logo_pngSize));
   #if BRUTAL_LICENSE_ENFORCE
    auto overlay = std::make_unique<BrutalLicenseOverlay>();
    overlay->isLicensed = [this] { return isLicensed(); };
    overlay->onActivated = [this] { setLicensed(true); };
    overlay->setVisible(!isLicensed());
    ed->installOverlay(std::move(overlay));
    ed->resized();
   #endif
    return ed;
}

#ifndef NFBRUTAL_NO_ENTRY
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new BrutalAudioProcessor(); }
#endif
