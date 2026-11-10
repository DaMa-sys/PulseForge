#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <cmath>
#include <array>

namespace PulseForge
{

//==============================================================================
// Lightweight 6-Voice Virtual Analog Preview Synth with Mono Legato Glide & VCF
//==============================================================================
class PreviewSynth
{
public:
    void prepare (double newSampleRate)
    {
        sampleRate = (newSampleRate > 1000.0) ? newSampleRate : 44100.0;
        reset();
    }

    void reset()
    {
        for (auto& v : voices)
            v = Voice{};
    }

    void setEnabled (bool shouldEnable) noexcept { enabled = shouldEnable; }
    bool isEnabled() const noexcept { return enabled; }

    void setCutModCC (int ccVal) noexcept
    {
        ccCutoffNorm = juce::jlimit (0.0f, 1.0f, static_cast<float> (ccVal) / 127.0f);
    }

    void noteOn (int midiNote, int velocity, bool isLegatoSlide)
    {
        if (!enabled)
            return;

        const float targetFreq = 440.0f * std::pow (2.0f, (static_cast<float> (midiNote) - 69.0f) / 12.0f);
        const float velNorm = juce::jlimit (0.05f, 1.0f, static_cast<float> (velocity) / 127.0f);
        const bool isAccented = (velocity >= 112);

        // If legato slide requested, find an already-active voice to glide its pitch without retriggering amp env
        if (isLegatoSlide)
        {
            for (auto& v : voices)
            {
                if (v.active && !v.releasing)
                {
                    v.midiNote = midiNote;
                    v.targetFreq = targetFreq;
                    v.glideCoeff = 0.9982f; // ~55ms analog RC portamento
                    if (isAccented)
                    {
                        v.filterEnv = juce::jmin (1.0f, v.filterEnv + 0.55f);
                        v.accentBoost = 0.45f;
                    }
                    return;
                }
            }
        }

        // Otherwise allocate a free or oldest voice
        int bestIdx = 0;
        float lowestEnv = 99.0f;
        for (int i = 0; i < kNumVoices; ++i)
        {
            if (!voices[static_cast<size_t>(i)].active)
            {
                bestIdx = i;
                break;
            }
            if (voices[static_cast<size_t>(i)].ampEnv < lowestEnv)
            {
                lowestEnv = voices[static_cast<size_t>(i)].ampEnv;
                bestIdx = i;
            }
        }

        auto& v = voices[static_cast<size_t>(bestIdx)];
        if (!v.active || v.currentFreq < 20.0f)
            v.currentFreq = targetFreq;

        v.active = true;
        v.releasing = false;
        v.midiNote = midiNote;
        v.targetFreq = targetFreq;
        v.glideCoeff = isLegatoSlide ? 0.9980f : 0.9850f; // Crisp tiny slew on normal notes, smooth slide on legato
        v.velocity = velNorm;
        v.ampEnv = juce::jmax (v.ampEnv, 0.85f);
        v.filterEnv = 1.0f;
        v.accentBoost = isAccented ? 0.48f : 0.0f;
    }

    void noteOff (int midiNote)
    {
        for (auto& v : voices)
        {
            if (v.active && v.midiNote == midiNote)
                v.releasing = true;
        }
    }

    void allNotesOff()
    {
        for (auto& v : voices)
            v.releasing = true;
    }

    void render (juce::AudioBuffer<float>& buffer, int startSample, int numSamples)
    {
        if (!enabled)
            return;

        float* left  = buffer.getWritePointer (0, startSample);
        float* right = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1, startSample) : nullptr;

        const float invSr = static_cast<float> (1.0 / sampleRate);
        const float ampDecay = std::exp (-invSr / 0.35f);
        const float ampRelease = std::exp (-invSr / 0.065f);
        const float filtDecay = std::exp (-invSr / 0.14f);

        for (int s = 0; s < numSamples; ++s)
        {
            float mixL = 0.0f;
            float mixR = 0.0f;

            for (int i = 0; i < kNumVoices; ++i)
            {
                auto& v = voices[static_cast<size_t>(i)];
                if (!v.active)
                    continue;

                // RC Portamento / Slide
                v.currentFreq = v.currentFreq * v.glideCoeff + v.targetFreq * (1.0f - v.glideCoeff);

                // Envelopes
                if (v.releasing)
                {
                    v.ampEnv *= ampRelease;
                    if (v.ampEnv < 0.0005f)
                    {
                        v.active = false;
                        v.ampEnv = 0.0f;
                        continue;
                    }
                }
                else
                {
                    v.ampEnv = 0.25f + (v.ampEnv - 0.25f) * ampDecay;
                }
                v.filterEnv *= filtDecay;

                // Dual Oscillator: Sawtooth + Sub-octave Square
                const float dt = v.currentFreq * invSr;
                v.phase += dt;
                if (v.phase >= 1.0f) v.phase -= 1.0f;

                v.subPhase += dt * 0.5f;
                if (v.subPhase >= 1.0f) v.subPhase -= 1.0f;

                const float saw = 2.0f * v.phase - 1.0f;
                const float subSq = (v.subPhase < 0.5f) ? 0.55f : -0.55f;
                const float rawOsc = 0.68f * saw + 0.32f * subSq;

                // 2-pole resonant diode/ladder style lowpass filter
                float cutoffHz = 140.0f
                               + ccCutoffNorm * 2200.0f
                               + (v.filterEnv * (1600.0f + v.accentBoost * 3600.0f));
                cutoffHz = juce::jlimit (80.0f, static_cast<float> (sampleRate * 0.42), cutoffHz);

                const float f = juce::jlimit (0.01f, 0.85f, 2.0f * std::sin (juce::MathConstants<float>::pi * cutoffHz * invSr));
                const float q = 0.65f + v.accentBoost * 0.65f; // Squelchy resonance on accented steps

                const float hp = rawOsc - v.lp1 - q * v.bp1;
                v.bp1 += f * hp;
                v.lp1 += f * v.bp1;
                // Soft analog saturation & NaN guard inside filter loop
                if (!std::isfinite (v.bp1) || !std::isfinite (v.lp1))
                {
                    v.bp1 = 0.0f;
                    v.lp1 = 0.0f;
                }
                v.bp1 = std::tanh (v.bp1 * 1.15f);

                const float sig = std::tanh (v.lp1 * (1.4f + v.accentBoost)) * v.ampEnv * v.velocity * 0.22f;
                const float pan = (i % 2 == 0) ? -0.15f : 0.15f;
                mixL += sig * (1.0f - pan);
                mixR += sig * (1.0f + pan);
            }

            left[s] += mixL;
            if (right != nullptr)
                right[s] += mixR;
        }
    }

private:
    static constexpr int kNumVoices = 6;

    struct Voice
    {
        bool active = false;
        bool releasing = false;
        int midiNote = 60;
        float currentFreq = 261.63f;
        float targetFreq = 261.63f;
        float glideCoeff = 0.99f;
        float phase = 0.0f;
        float subPhase = 0.0f;
        float velocity = 0.8f;
        float ampEnv = 0.0f;
        float filterEnv = 0.0f;
        float accentBoost = 0.0f;
        float lp1 = 0.0f;
        float bp1 = 0.0f;
    };

    std::array<Voice, kNumVoices> voices;
    double sampleRate = 44100.0;
    bool enabled = true;
    float ccCutoffNorm = 0.5f;
};

} // namespace PulseForge
