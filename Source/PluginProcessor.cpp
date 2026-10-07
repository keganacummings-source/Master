#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "FxCatalog.h"
#include "ChainMix.h"
#include <cmath>

namespace
{
inline float loadParam(const std::atomic<float>* p, float fallback = 0.f) noexcept
{
    return p != nullptr ? p->load(std::memory_order_relaxed) : fallback;
}

inline float safeMix(float v) noexcept
{
    return juce::jlimit(0.f, 1.f, v);
}
}

juce::String KyotoAudioProcessor::slotId(int i, const char* tail)
{
    return "s" + juce::String(i + 1).paddedLeft('0', 2) + tail;
}

juce::String KyotoAudioProcessor::chainLevelId(int chain)
{
    return "chain" + juce::String(chain + 1) + "Level";
}

juce::var KyotoAudioProcessor::exportChainLevels() const
{
    auto* state = new juce::DynamicObject();
    state->setProperty("enabled", apvts.getRawParameterValue("perChainLevels")->load() >= 0.5f);
    juce::Array<juce::var> levels;
    for (int i = 0; i < kMaxChains; ++i)
        levels.add(apvts.getRawParameterValue(chainLevelId(i))->load());
    state->setProperty("levels", levels);
    return juce::var(state);
}

void KyotoAudioProcessor::restoreChainLevels(const juce::var& state)
{
    auto* object = state.getDynamicObject();
    auto* levels = object != nullptr ? object->getProperty("levels").getArray() : nullptr;
    auto set = [this](const juce::String& id, float value)
    {
        if (auto* p = apvts.getParameter(id))
        {
            p->beginChangeGesture();
            p->setValueNotifyingHost(p->convertTo0to1(value));
            p->endChangeGesture();
        }
    };
    // Older module files have no mixer settings: preserve their original balance.
    set("perChainLevels", object != nullptr && (bool) object->getProperty("enabled") ? 1.f : 0.f);
    for (int i = 0; i < kMaxChains; ++i)
    {
        const float value = levels != nullptr && i < levels->size() ? (float) levels->getReference(i) : 1.f;
        set(chainLevelId(i), std::isfinite(value) ? safeMix(value) : 1.f);
    }
}

KyotoAudioProcessor::KyotoAudioProcessor()
    : AudioProcessor(BusesProperties()
          .withInput("Input", juce::AudioChannelSet::stereo(), true)
          .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "STATE", createLayout(isFx()))
{
    uiState.setProperty("grid", 0, nullptr);
    uiState.setProperty("free", 0, nullptr);
    uiState.setProperty("name", "untitled", nullptr);
    uiState.setProperty("theme", "trippah", nullptr);
    uiState.setProperty("playgroundMode", 1, nullptr);
    uiState.setProperty("playgroundWidth", 800, nullptr);
    uiState.setProperty("playgroundHeight", 800, nullptr);
    uiState.setProperty("aspectRatio", "1:1", nullptr);
    uiState.setProperty("bodyDesign", "Bare Frame", nullptr);
}

KyotoAudioProcessor::~KyotoAudioProcessor() = default;

bool KyotoAudioProcessor::isFx() const
{
#if KYOTO_IS_FX
    return true;
#else
    return false;
#endif
}

const juce::String KyotoAudioProcessor::getName() const
{
    return isFx() ? "KYOTRIPPAH FX" : "KYOTO";
}

bool KyotoAudioProcessor::acceptsMidi() const { return ! isFx(); }

juce::AudioProcessorValueTreeState::ParameterLayout KyotoAudioProcessor::createLayout(bool fx)
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    if (! fx)
    {
        layout.add(std::make_unique<juce::AudioParameterInt>(juce::ParameterID { "osc", 1 }, "Oscillator", 0, 2, 0));
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "cutoff", 1 }, "Cutoff", 0.f, 1.f, 0.62f));
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "res", 1 }, "Resonance", 0.f, 1.f, 0.18f));
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "attack", 1 }, "Attack", 0.f, 1.f, 0.05f));
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "decay", 1 }, "Decay", 0.f, 1.f, 0.28f));
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "sustain", 1 }, "Sustain", 0.f, 1.f, 0.65f));
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "release", 1 }, "Release", 0.f, 1.f, 0.35f));
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "sub", 1 }, "Sub Oscillator", 0.f, 1.f, 0.22f));
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "noise", 1 }, "Noise", 0.f, 1.f, 0.f));
    }

    const int n = fx ? kMaxSlots : 16;
    for (int i = 0; i < n; ++i)
    {
        layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID { slotId(i, "on"), 1 }, "Slot " + juce::String(i + 1) + " On", false));
        layout.add(std::make_unique<juce::AudioParameterInt>(juce::ParameterID { slotId(i, "type"), 1 }, "Slot " + juce::String(i + 1) + " Type", 0, kBreakType, 0));
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { slotId(i, "amt"), 1 }, "Slot " + juce::String(i + 1) + " Amount", 0.f, 1.f, 0.45f));
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { slotId(i, "tone"), 1 }, "Slot " + juce::String(i + 1) + " Tone", 0.f, 1.f, 0.5f));
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { slotId(i, "mot"), 1 }, "Slot " + juce::String(i + 1) + " Motion", 0.f, 1.f, 0.35f));
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { slotId(i, "mix"), 1 }, "Slot " + juce::String(i + 1) + " Mix", 0.f, 1.f, 0.4f));
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { slotId(i, "shp"), 1 }, "Slot " + juce::String(i + 1) + " Shape", 0.f, 1.f, 0.5f));
    }
    // Append new parameters so existing parameter order and type ranges stay stable.
    layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID { "perChainLevels", 1 }, "Per-chain levels", true));
    for (int i = 0; i < kMaxChains; ++i)
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { chainLevelId(i), 1 },
            "Chain " + juce::String(i + 1) + " Level", 0.f, 1.f, 1.f));
    return layout;
}

void KyotoAudioProcessor::cacheParameters()
{
    perChainLevelsParam = apvts.getRawParameterValue("perChainLevels");
    for (int i = 0; i < kMaxChains; ++i)
        chainLevelParams[i] = apvts.getRawParameterValue(chainLevelId(i));
    oscParam = apvts.getRawParameterValue("osc");
    cutoffParam = apvts.getRawParameterValue("cutoff");
    attackParam = apvts.getRawParameterValue("attack");
    decayParam = apvts.getRawParameterValue("decay");
    sustainParam = apvts.getRawParameterValue("sustain");
    releaseParam = apvts.getRawParameterValue("release");
    subParam = apvts.getRawParameterValue("sub");
    noiseParam = apvts.getRawParameterValue("noise");

    for (auto& p : slotParams)
        p = {};
    for (int i = 0; i < slotCount(); ++i)
    {
        auto& p = slotParams[i];
        const auto prefix = "s" + juce::String(i + 1).paddedLeft('0', 2);
        p.on = apvts.getRawParameterValue(prefix + "on");
        p.type = apvts.getRawParameterValue(prefix + "type");
        p.amount = apvts.getRawParameterValue(prefix + "amt");
        p.tone = apvts.getRawParameterValue(prefix + "tone");
        p.motion = apvts.getRawParameterValue(prefix + "mot");
        p.mix = apvts.getRawParameterValue(prefix + "mix");
        p.shape = apvts.getRawParameterValue(prefix + "shp");
    }
}

void KyotoAudioProcessor::prepareToPlay(double sampleRate, int)
{
    sampleRateHz = sampleRate > 0.0 ? sampleRate : 44100.0;
    maxDelaySamples = juce::jmax(2048, (int) std::round(sampleRateHz * 0.75));

    for (auto& s : slotDsp)
    {
        s = {};
        s.delay[0].assign((size_t) maxDelaySamples, 0.f);
        s.delay[1].assign((size_t) maxDelaySamples, 0.f);
    }
    for (auto& v : voices)
        v = {};
    cacheParameters();
}

void KyotoAudioProcessor::releaseResources() {}

bool KyotoAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    const auto in = layouts.getMainInputChannelSet();
    if (in.isDisabled())
        return true;
    return in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo();
}

void KyotoAudioProcessor::noteOn(int note, float vel)
{
    Voice* slot = nullptr;
    for (auto& v : voices)
        if (! v.on || v.note == note) { slot = &v; break; }
    if (slot == nullptr) slot = &voices[0];
    slot->on = true;
    slot->note = note;
    slot->vel = juce::jlimit(0.05f, 1.f, vel);
    slot->stage = 0;
    slot->env = 0.f;
    slot->phase = 0.f;
    slot->sub = 0.f;
}

void KyotoAudioProcessor::noteOff(int note)
{
    for (auto& v : voices)
        if (v.on && v.note == note && v.stage < 3)
            v.stage = 3;
}

float KyotoAudioProcessor::renderVoice(Voice& v)
{
    if (! v.on) return 0.f;
    const float atk = 0.001f + loadParam(attackParam) * 0.8f;
    const float dec = 0.01f + loadParam(decayParam) * 1.2f;
    const float sus = loadParam(sustainParam);
    const float rel = 0.02f + loadParam(releaseParam) * 1.5f;
    const float dt = 1.f / (float) sampleRateHz;

    if (v.stage == 0)
    {
        v.env += dt / atk;
        if (v.env >= 1.f) { v.env = 1.f; v.stage = 1; }
    }
    else if (v.stage == 1)
    {
        v.env -= dt / dec * (1.f - sus);
        if (v.env <= sus) { v.env = sus; v.stage = 2; }
    }
    else if (v.stage == 3)
    {
        v.env -= dt / rel;
        if (v.env <= 0.f) { v.env = 0.f; v.on = false; return 0.f; }
    }

    const float freq = 440.f * std::pow(2.f, (v.note - 69) / 12.f);
    const float inc = freq / (float) sampleRateHz;
    v.phase += inc; if (v.phase >= 1.f) v.phase -= 1.f;
    v.sub += inc * 0.5f; if (v.sub >= 1.f) v.sub -= 1.f;
    const int osc = juce::jlimit(0, 2, (int) std::lround(loadParam(oscParam)));
    float wave = v.phase * 2.f - 1.f;
    if (osc == 1) wave = v.phase < 0.5f ? 1.f : -1.f;
    if (osc == 2) wave = std::sin(v.phase * juce::MathConstants<float>::twoPi);
    const float sub = std::sin(v.sub * juce::MathConstants<float>::twoPi) * loadParam(subParam);
    const float noise = (noiseRng.nextFloat() * 2.f - 1.f) * loadParam(noiseParam);
    return (wave * 0.35f + sub * 0.25f + noise * 0.15f) * v.env * v.vel;
}

void KyotoAudioProcessor::applySlotStereo(int slot, float& left, float& right)
{
    const auto& cfg = blockConfig[slot];
    if (! cfg.on) return;

    const int type = cfg.type;
    if (type >= kt::kFxCount) return;

    const int fam = kt::kFx[type].family;
    const float amount = cfg.amount;
    const float tone = cfg.tone;
    const float motion = cfg.motion;
    const float mix = cfg.mix;
    const float shape = cfg.shape;
    auto& d = slotDsp[slot];

    float wetL = left, wetR = right;
    if (fam == 4)
    {
        const float c = juce::jlimit(0.002f, 0.45f, 0.006f + tone * 0.30f);
        d.lp[0] += c * (left - d.lp[0]);
        d.lp[1] += c * (right - d.lp[1]);
        if ((type & 1) == 0) { wetL = d.lp[0]; wetR = d.lp[1]; }
        else { wetL = left - d.lp[0]; wetR = right - d.lp[1]; }
    }
    else if (fam == 0 || fam == 1)
    {
        const int n = maxDelaySamples;
        const int taps = juce::jlimit(1, n - 1, (int) ((0.012f + motion * (fam == 1 ? 0.62f : 0.30f)) * (float) sampleRateHz));
        const int read = (d.write + n - taps) % n;
        wetL = d.delay[0][(size_t) read];
        wetR = d.delay[1][(size_t) read];
        const float fb = juce::jlimit(0.f, 0.88f, amount * (fam == 1 ? 0.80f : 0.68f));
        d.delay[0][(size_t) d.write] = left + wetL * fb;
        d.delay[1][(size_t) d.write] = right + wetR * fb;
        d.write = (d.write + 1) % n;
    }
    else if (fam == 2 || fam == 3)
    {
        d.lfo += (0.05f + motion * 8.f) / (float) sampleRateHz;
        if (d.lfo >= 1.f) d.lfo -= std::floor(d.lfo);
        const float l = std::sin(d.lfo * juce::MathConstants<float>::twoPi);
        if (fam == 2)
        {
            const float width = amount * (0.25f + shape * 0.75f);
            wetL = left * (1.f + l * width);
            wetR = right * (1.f - l * width);
        }
        else
        {
            const float c = 0.01f + tone * 0.22f;
            d.bp[0] += c * ((left * (0.5f + 0.5f * l)) - d.bp[0]);
            d.bp[1] += c * ((right * (0.5f - 0.5f * l)) - d.bp[1]);
            wetL = d.bp[0]; wetR = d.bp[1];
        }
    }
    else if (fam == 6)
    {
        const float thr = 0.08f + (1.f - amount) * 0.82f;
        const float drive = 1.f + shape * 7.f;
        auto comp = [thr, drive](float x)
        {
            const float ax = std::abs(x);
            if (ax <= thr) return x;
            return std::copysign(thr + (ax - thr) / drive, x);
        };
        wetL = comp(left); wetR = comp(right);
    }
    else
    {
        const float k = 1.f + amount * (2.f + shape * 10.f);
        wetL = std::tanh(left * k);
        wetR = std::tanh(right * k);
        const float c = 0.02f + tone * 0.28f;
        d.lp[0] += c * (wetL - d.lp[0]);
        d.lp[1] += c * (wetR - d.lp[1]);
        wetL = d.lp[0]; wetR = d.lp[1];
    }

    const float over = slotOvermax[slot] <= 0.f ? 1.f : slotOvermax[slot];
    const float wet = juce::jlimit(0.f, 1.f, (0.15f + mix * 0.85f) * juce::jmin(1.6f, over));
    const float drive = juce::jmax(1.f, over);
    left = left * (1.f - wet) + std::tanh(wetL * drive) * wet;
    right = right * (1.f - wet) + std::tanh(wetR * drive) * wet;
}

void KyotoAudioProcessor::rebuildActiveSlots() noexcept
{
    activeSlotCount = 0;
    anyActiveSlot = false;
    const int nSlots = slotCount();
    for (int s = 0; s < nSlots; ++s)
    {
        if (! blockConfig[s].on)
            continue;
        // Hard cap: never process more than kMaxSlots live stages even if
        // a pathological custom expansion tried to fill everything.
        if (activeSlotCount >= kMaxSlots)
            break;
        activeSlots[activeSlotCount++] = s;
        anyActiveSlot = true;
    }
}

void KyotoAudioProcessor::processChain(float& left, float& right, float original)
{
    juce::ignoreUnused(original);
    if (! anyActiveSlot)
    {
        if (blockPerChainLevels)
        {
            left *= blockChainLevels[0];
            right *= blockChainLevels[0];
        }
        return;
    }

    float dryL = left, dryR = right;
    float segmentL = left, segmentR = right;
    kt::ChainMix branches;
    float chainMix = 1.f;
    int chain = 0;

    // Walk only the compact active list — O(active) not O(maxSlots) per sample.
    for (int ai = 0; ai < activeSlotCount; ++ai)
    {
        const int s = activeSlots[ai];
        const auto& cfg = blockConfig[s];
        const int type = cfg.type;

        if (type == kMixType)
        {
            chainMix = cfg.amount;
            continue;
        }
        if (type == kBreakType)
        {
            branches.add(segmentL, segmentR, blockPerChainLevels ? blockChainLevels[chain] : 1.f);
            ++chain;
            segmentL = dryL;
            segmentR = dryR;
            continue;
        }

        left = segmentL;
        right = segmentR;
        applySlotStereo(s, left, right);
        segmentL = left;
        segmentR = right;
    }

    branches.add(segmentL, segmentR, blockPerChainLevels ? blockChainLevels[chain] : 1.f);
    branches.output(left, right, blockPerChainLevels);

    left = dryL + (left - dryL) * chainMix;
    right = dryR + (right - dryR) * chainMix;
}

void KyotoAudioProcessor::setHardwareColour(int, float)
{
    // Intentionally empty: shells, modules and cosmetic parts never touch the sound.
    // Only effects the user places (and wires) are in the signal path.
    hardwareFx.store(-1, std::memory_order_relaxed);
    hardwareAmt.store(0.f, std::memory_order_relaxed);
}

void KyotoAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    if (! isFx())
    {
        for (const auto meta : midi)
        {
            const auto msg = meta.getMessage();
            if (msg.isNoteOn()) noteOn(msg.getNoteNumber(), msg.getFloatVelocity());
            else if (msg.isNoteOff()) noteOff(msg.getNoteNumber());
        }
    }

    const int nIn = getTotalNumInputChannels();
    const int nOut = getTotalNumOutputChannels();
    const int n = buffer.getNumSamples();
    // Snapshot all slot parameters once per block (not per sample).
    for (int s = 0; s < slotCount(); ++s)
    {
        const auto& p = slotParams[s];
        auto& cfg = blockConfig[s];
        cfg.on = loadParam(p.on) >= 0.5f;
        cfg.type = juce::jlimit(0, kBreakType, (int) std::lround(loadParam(p.type)));
        cfg.amount = safeMix(loadParam(p.amount));
        cfg.tone = safeMix(loadParam(p.tone));
        cfg.motion = safeMix(loadParam(p.motion));
        cfg.mix = safeMix(loadParam(p.mix));
        cfg.shape = safeMix(loadParam(p.shape));
    }
    // Build the compact active-slot list once. Extreme custom-FX expansions
    // that fill many slots still only walk the live ones per sample.
    rebuildActiveSlots();
    blockPerChainLevels = loadParam(perChainLevelsParam, 1.f) >= 0.5f;
    for (int chain = 0; chain < kMaxChains; ++chain)
        blockChainLevels[chain] = safeMix(loadParam(chainLevelParams[chain], 1.f));
    auto* writeL = nOut > 0 ? buffer.getWritePointer(0) : nullptr;
    auto* writeR = nOut > 1 ? buffer.getWritePointer(1) : writeL;
    const auto* readL = nIn > 0 ? buffer.getReadPointer(0) : nullptr;
    const auto* readR = nIn > 1 ? buffer.getReadPointer(1) : readL;

    for (int i = 0; i < n; ++i)
    {
        float synth = 0.f;
        if (! isFx())
        {
            for (auto& v : voices) synth += renderVoice(v);
            const float cut = 0.02f + loadParam(cutoffParam) * 0.5f;
            slotDsp[0].hp[0] += cut * (synth - slotDsp[0].hp[0]);
            synth = slotDsp[0].hp[0];
        }

        float left = synth + (readL != nullptr ? readL[i] : 0.f);
        float right = synth + (readR != nullptr ? readR[i] : left);

        int sp = samplePos.load(std::memory_order_relaxed);
        if (sp >= 0)
        {
            float sampleValue = 0.f;
            {
                juce::ScopedLock lock(sampleLock);
                if (sp < (int) sample.size()) sampleValue = sample[(size_t) sp];
                else sp = -2;
            }
            if (sp >= 0)
            {
                const double ratio = sampleRateFile / sampleRateHz;
                samplePos.store(sp + juce::jmax(1, (int) std::round(ratio)), std::memory_order_relaxed);
                left += sampleValue * 0.8f;
                right += sampleValue * 0.8f;
            }
            else
                samplePos.store(-1, std::memory_order_relaxed);
        }

        const float original = left;
        processChain(left, right, original);
        left = std::isfinite(left) ? std::tanh(left * 0.92f) * 0.92f : 0.f;
        right = std::isfinite(right) ? std::tanh(right * 0.92f) * 0.92f : 0.f;

        if (writeL != nullptr) writeL[i] = left;
        if (nOut > 1 && writeR != nullptr) writeR[i] = right;
        const int w = scopeWrite.load(std::memory_order_relaxed);
        scope[w] = left;
        scopeWrite.store((w + 1) % scopeN, std::memory_order_relaxed);
    }

    for (int ch = nOut; ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear(ch, 0, n);
}

void KyotoAudioProcessor::loadSample(juce::AudioBuffer<float> buffer, double fileRate)
{
    juce::ScopedLock lock(sampleLock);
    sample.clear();
    sample.reserve((size_t) buffer.getNumSamples());
    const int chs = juce::jmax(1, buffer.getNumChannels());
    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        float s = 0.f;
        for (int c = 0; c < chs; ++c) s += buffer.getSample(c, i);
        sample.push_back(s / (float) chs);
    }
    sampleRateFile = fileRate > 0.0 ? fileRate : sampleRateHz;
    samplePos.store(-1, std::memory_order_relaxed);
}

void KyotoAudioProcessor::triggerSample()
{
    if (! sample.empty()) samplePos.store(0, std::memory_order_relaxed);
}

double KyotoAudioProcessor::hostBpm() const
{
    if (auto* head = getPlayHead())
        if (auto pos = head->getPosition())
            if (auto bpm = pos->getBpm())
                if (*bpm > 1.0) return *bpm;
    return 120.0;
}

void KyotoAudioProcessor::setSlotOvermax(int slot, float over)
{
    if (slot >= 0 && slot < kMaxSlots) slotOvermax[slot] = juce::jlimit(0.25f, 4.f, over);
}

float KyotoAudioProcessor::getSlotOvermax(int slot) const
{
    if (slot < 0 || slot >= kMaxSlots) return 1.f;
    return slotOvermax[slot] <= 0.f ? 1.f : slotOvermax[slot];
}

void KyotoAudioProcessor::copyScope(float* dest, int n) const
{
    const int w = scopeWrite.load(std::memory_order_relaxed);
    for (int i = 0; i < n; ++i) dest[i] = scope[(w + i) % scopeN];
}

void KyotoAudioProcessor::getStateInformation(juce::MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml())
    {
        if (auto ui = uiState.createXml()) xml->addChildElement(ui.release());
        copyXmlToBinary(*xml, dest);
    }
}

void KyotoAudioProcessor::setStateInformation(const void* data, int size)
{
    if (auto xml = getXmlFromBinary(data, size))
    {
        auto tree = juce::ValueTree::fromXml(*xml);
        auto ui = tree.getChildWithName("ui");
        if (ui.isValid()) { uiState = ui; tree.removeChild(ui, nullptr); }
        // Backfill old DAW states explicitly rather than inheriting the current
        // session's gains. Old sessions retain the original balanced summing.
        auto ensureParameter = [&tree](const juce::String& id, float value)
        {
            if (! tree.getChildWithProperty("id", id).isValid())
            {
                juce::ValueTree parameter("PARAM");
                parameter.setProperty("id", id, nullptr);
                parameter.setProperty("value", value, nullptr);
                tree.appendChild(parameter, nullptr);
            }
        };
        ensureParameter("perChainLevels", 0.f);
        for (int i = 0; i < kMaxChains; ++i)
            ensureParameter(chainLevelId(i), 1.f);
        apvts.replaceState(tree);
        cacheParameters();
    }
}

juce::AudioProcessorEditor* KyotoAudioProcessor::createEditor() { return new KyotoAudioProcessorEditor(*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new KyotoAudioProcessor(); }
