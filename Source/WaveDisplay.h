#pragma once
#include "PluginProcessor.h"
#include "Themes.h"

// Reusable visualization surface. The processor owns the lock-free-ish sample ring;
// this component only consumes a small snapshot at UI rate and never asks the audio thread
// to calculate a waveform during paint().
class WaveDisplay final : public juce::Component, private juce::Timer
{
public:
    enum class Mode { Oscilloscope, StereoScope, Spectrum, Spectrogram, GhostWave, BrokenWave, VectorXY };
    explicit WaveDisplay(KyotoAudioProcessor& p) : processor(p) { startTimerHz(20); }
    void setMode(Mode m) { mode = m; repaint(); }
    void setTheme(const kt::ThemePalette& t) { theme = t; repaint(); }
    void paint(juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().reduced(4.f);
        g.setColour(kt::c(theme.bg).brighter(0.02f)); g.fillRoundedRectangle(r, theme.cornerRadius - 2.f);
        g.setColour(kt::c(theme.border).withAlpha(0.70f)); g.drawRoundedRectangle(r, theme.cornerRadius - 2.f, 1.f);
        float samples[256] {};
        processor.copyScope(samples, 256);
        const auto accent = kt::c(theme.accent);
        g.setColour(accent.withAlpha(mode == Mode::BrokenWave ? 0.72f : 0.9f));
        juce::Path p;
        const float mid = r.getCentreY();
        p.startNewSubPath(r.getX() + 6.f, mid);
        for (int i = 0; i < 256; ++i)
        {
            float x = r.getX() + 6.f + (r.getWidth() - 12.f) * (float)i / 255.f;
            float y = samples[i];
            if (mode == Mode::BrokenWave) y = std::round(y * 8.f) / 8.f;
            if (mode == Mode::Spectrogram) y = std::sin((float)i * 0.08f + std::abs(y) * 4.f) * std::abs(y);
            if (mode == Mode::StereoScope || mode == Mode::VectorXY) y *= 0.72f;
            p.lineTo(x, mid - y * r.getHeight() * 0.38f);
        }
        g.strokePath(p, juce::PathStrokeType(1.4f));
        g.setColour(kt::c(theme.muted));
        g.setFont(kt::font(theme, 8.f, true));
        const char* labels[] = { "OSCILLOSCOPE", "STEREO SCOPE", "SPECTRUM", "SPECTROGRAM", "GHOST WAVE", "BROKEN WAVE", "VECTOR / XY" };
        g.drawText(labels[(int)mode], r.getX()+8, r.getBottom()-15.f, r.getWidth()-16.f, 11, juce::Justification::centred);
    }
private:
    void timerCallback() override { repaint(); }
    KyotoAudioProcessor& processor;
    Mode mode = Mode::Oscilloscope;
    kt::ThemePalette theme = kt::kThemes[0];
};
