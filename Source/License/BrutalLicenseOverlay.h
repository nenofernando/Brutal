#pragma once
// Activation window shown over the NF Brutal editor until a valid licence key is on this computer (plug-in only; the native Mixa insert has none).
#include <juce_gui_basics/juce_gui_basics.h>
#include "NFBRLicense.h"

class BrutalLicenseOverlay final : public juce::Component, private juce::Timer
{
public:
    std::function<bool()> isLicensed;          // asked about twice a second: the overlay hides itself when another instance activated
    std::function<void()> onActivated;
    BrutalLicenseOverlay()
    {
        using namespace juce;
        keyBox.setMultiLine(false);
        keyBox.setTextToShowWhenEmpty("Cole aqui a sua chave de acesso / Paste your access key here", Colours::grey);
        keyBox.setColour(TextEditor::backgroundColourId, Colour(0xff0b0c0f)); keyBox.setColour(TextEditor::textColourId, Colours::white);
        keyBox.setColour(TextEditor::outlineColourId, Colour(0xff5a2a18)); keyBox.setColour(TextEditor::focusedOutlineColourId, Colour(0xffff5a1f));
        keyBox.onReturnKey = [this] { activate(); }; addAndMakeVisible(keyBox);
        button.setColour(TextButton::buttonColourId, Colour(0xffff5a1f)); button.setColour(TextButton::textColourOffId, Colours::white);
        button.onClick = [this] { activate(); }; addAndMakeVisible(button);
        startTimerHz(2);
    }
    void resized() override
    {
        auto c = getLocalBounds().getCentre();
        keyBox.setBounds(juce::Rectangle<int>(juce::jmax(260, getWidth() * 3 / 5), 26).withCentre({ c.x, c.y + 4 }));
        button.setBounds(juce::Rectangle<int>(120, 26).withCentre({ c.x, c.y + 42 }));
    }
    void paint(juce::Graphics& g) override
    {
        using namespace juce;
        g.fillAll(Colour(0xf2101114)); auto c = getLocalBounds().getCentre();
        g.setColour(Colours::white); g.setFont(FontOptions(18.0f, Font::bold));
        g.drawText("Ativar o NF Brutal / Activate NF Brutal", getLocalBounds().withHeight(30).withCentre({ c.x, c.y - 52 }), Justification::centred);
        g.setFont(FontOptions(12.0f)); g.setColour(Colour(0xffff9a3c));
        g.drawText("Cole a chave que voce recebeu apos a compra. / Paste the key you received after purchase.", getLocalBounds().withHeight(20).withCentre({ c.x, c.y - 28 }), Justification::centred);
        g.setColour(message.isEmpty() ? Colours::transparentBlack : Colour(0xffff8a80));
        g.drawText(message, getLocalBounds().withHeight(20).withCentre({ c.x, c.y + 72 }), Justification::centred);
    }
private:
    void timerCallback() override { if (isLicensed && isLicensed() && isVisible()) setVisible(false); else if (isLicensed && !isLicensed() && !isVisible()) { setVisible(true); toFront(false); } }
    void activate()
    {
        if (busy) return;
        const auto key = keyBox.getText().removeCharacters(" \t\r\n");
        if (key.isEmpty()) { message = "Cole a chave primeiro / Paste the key first."; repaint(); return; }
        const bool shortCode = NFBRLicense::looksShort(key);
        if (!shortCode) { auto info = NFBRLicense::check(key); if (!info.valid) { message = info.message; repaint(); return; } }
        busy = true; message = "Verificando... / Checking..."; button.setEnabled(false); repaint();
        juce::Component::SafePointer<BrutalLicenseOverlay> safe(this);
        juce::Thread::launch([safe, key]
        {
            const auto r = NFBRLicense::activateOnline(key);   // registers this computer (max 2 per licence); a short code comes back as the signed long key
            juce::MessageManager::callAsync([safe, key, r]
            {
                if (safe == nullptr) return;
                safe->busy = false; safe->button.setEnabled(true);
                if (r.state != NFBRLicense::Online::Ok) { safe->message = r.message; safe->repaint(); return; }
                const auto toStore = r.longKey.isNotEmpty() ? r.longKey : key;
                if (!NFBRLicense::check(toStore).valid) { safe->message = "The server answered with a key this plug-in does not accept."; safe->repaint(); return; }
                NFBRLicense::keyFile().getParentDirectory().createDirectory();
                NFBRLicense::keyFile().replaceWithText(toStore.removeCharacters(" \t\r\n"));
                NFBRLicense::writeStamp();
                if (safe->onActivated) safe->onActivated();
                safe->setVisible(false);
            });
        });
    }
    juce::TextEditor keyBox; juce::TextButton button { "Ativar / Activate" }; juce::String message; bool busy = false;
};
