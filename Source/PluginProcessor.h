#pragma once
// NF Brutal - plug-in wrapper (VST3 / AU). DSP, parameters, presets and editor live in Source/Shared (also used by the native Mixa insert).
#include <juce_audio_processors/juce_audio_processors.h>
#include "Shared/BrutalCore.h"
#ifndef BRUTAL_LICENSE_ENFORCE
 #define BRUTAL_LICENSE_ENFORCE 0   // set by CMake (option BRUTAL_LICENSE)
#endif
#if BRUTAL_LICENSE_ENFORCE
 #include "License/NFBRLicense.h"
#endif

class BrutalAudioProcessor final : public juce::AudioProcessor
{
public:
    BrutalAudioProcessor();
    ~BrutalAudioProcessor() override;
    const juce::String getName() const override { return "NF Brutal"; }
    void prepareToPlay(double sr, int) override { core.prepare(sr); }
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
    void getStateInformation(juce::MemoryBlock& d) override { core.getState(d); }
    void setStateInformation(const void* d, int n) override { core.setState(d, n); }
    bool isBusesLayoutSupported(const BusesLayout& l) const override
    { return l.getMainInputChannelSet() == l.getMainOutputChannelSet() && (l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo() || l.getMainOutputChannelSet() == juce::AudioChannelSet::mono()); }

    // ---- licence: without a valid key the plug-in is muted and the editor shows the activation window (CMake option BRUTAL_LICENSE) ----
    bool isLicensed() const { return licensed.load(); }
    void setLicensed(bool b) { licensed.store(b); }

    nfbrutal::BrutalCore core { *this };
    juce::AudioProcessorValueTreeState& apvts = core.apvts;
    std::atomic<float>& grDb = core.grDb;
private:
    std::atomic<bool> licensed { !BRUTAL_LICENSE_ENFORCE };
   #if BRUTAL_LICENSE_ENFORCE
    // Unlicensed: look every 2 s for a key. Licensed: about once a week the server is asked again (2 computers per licence, revoked keys);
    // up to 30 days offline are tolerated.
    struct LicenceWatch : juce::Thread
    {
        explicit LicenceWatch(BrutalAudioProcessor& o) : juce::Thread("NF Brutal licence"), owner(o) {}
        void run() override
        {
            while (!threadShouldExit())
            {
                if (!owner.licensed.load()) { if (NFBRLicense::isLicensed()) owner.licensed.store(true); wait(2000); continue; }
                if (NFBRLicense::needsOnlineCheck())
                {
                    const auto r = NFBRLicense::activateOnline(NFBRLicense::keyFile().loadFileAsString());
                    if (threadShouldExit()) return;
                    if (r.state == NFBRLicense::Online::Ok) NFBRLicense::writeStamp();
                    else if (r.state == NFBRLicense::Online::Refused) { NFBRLicense::clearLicence(); owner.licensed.store(false); }
                    else if (!NFBRLicense::withinOfflineGrace()) owner.licensed.store(false);
                }
                wait(60 * 1000);
            }
        }
        BrutalAudioProcessor& owner;
    };
    LicenceWatch licenceWatch { *this };
   #endif
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BrutalAudioProcessor)
};
