// Renders the editor to a PNG without a host: NFBrutalShot out.png [width] [grDb] [slay 0..1]
#define NFBRUTAL_NO_ENTRY 1
#include "PluginProcessor.h"
int main(int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    BrutalAudioProcessor p;
    const int w = argc > 2 ? atoi(argv[2]) : 810;
    p.grDb.store(argc > 3 ? (float) atof(argv[3]) : 0.f);
    if (argc > 4) if (auto* s = p.apvts.getParameter("slay")) s->setValueNotifyingHost((float) atof(argv[4]));
    std::unique_ptr<juce::AudioProcessorEditor> ed(p.createEditor());
    ed->setSize(w, (int) std::lround(w * 270.0 / 810.0));
    auto img = ed->createComponentSnapshot(ed->getLocalBounds(), true, 2.0f);
    juce::File f(argv[1]); f.deleteFile(); juce::FileOutputStream os(f); juce::PNGImageFormat png; png.writeImageToStream(img, os);
    return 0;
}
