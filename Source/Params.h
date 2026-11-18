#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

// Every parameter of Xoxicoa Synth lives here: IDs, ranges, defaults and the
// choice lists. The engine, the presets, the generator and the UI all use these.
namespace xs
{
inline const juce::StringArray waveNames { "Sine", "Triangle", "Saw", "Square", "Pulse", "Thin Pulse", "Hollow", "Glass" };
inline const juce::StringArray filterTypes { "LP 12", "LP 24", "HP 12", "HP 24", "BP" };
inline const juce::StringArray lfoShapes { "Sine", "Triangle", "Saw", "Square", "S&H" };
inline const juce::StringArray lfoDests { "Pitch", "Cutoff", "Wave A", "Wave B", "Volume", "Pan", "Vowel" };
inline const juce::StringArray lfoDivs { "4 Bar", "2 Bar", "1 Bar", "1/2", "1/4", "1/8D", "1/4T", "1/8", "1/8T", "1/16", "1/16T", "1/32" };
inline constexpr double lfoDivBeats[] { 16.0, 8.0, 4.0, 2.0, 1.0, 0.75, 2.0 / 3.0, 0.5, 1.0 / 3.0, 0.25, 1.0 / 6.0, 0.125 };
inline const juce::StringArray delayDivs { "1/32", "1/16T", "1/16", "1/16D", "1/8T", "1/8", "1/8D", "1/4T", "1/4", "1/4D", "1/2" };
inline constexpr double delayDivBeats[] { 0.125, 1.0 / 6.0, 0.25, 0.375, 1.0 / 3.0, 0.5, 0.75, 2.0 / 3.0, 1.0, 1.5, 2.0 };
inline const juce::StringArray voiceModes { "Poly", "Mono", "Legato" };
inline const juce::StringArray distTypes { "Soft", "Hard", "Fold", "Mosh" };
inline const juce::StringArray subOctaves { "-1 Oct", "-2 Oct" };
inline const juce::StringArray categories { "Rage Lead", "Plugg Bell", "808", "Dark Pad", "Pluck",
                                            "Kick", "Snare", "Clap", "Hi Hat", "Open Hat", "Crash", "Perc", "Snap",
                                            "Sub Bass", "Perc Loop", "FX", "Vox Chant" };
// sound types (index into categories)
enum SoundType { stRageLead, stPluggBell, st808, stDarkPad, stPluck, stKick, stSnare, stClap, stHiHat, stOpenHat,
                 stCrash, stPerc, stSnap, stSubBass, stPercLoop, stFX, stVox, stNumTypes };
inline const juce::StringArray seqRates { "1/8", "1/16", "1/16T", "1/32" };
inline constexpr double seqRateBeats[] { 0.5, 0.25, 1.0 / 6.0, 0.125 };
inline const juce::StringArray vowelNames { "A", "E", "I", "O", "U" };

enum class Kind { Float, Int, Choice, Bool };

struct ParamDef
{
    const char* id;
    const char* name;
    Kind kind;
    float min, max, def;
    float skewCentre = 0.0f;           // >0: skew the range so this value sits mid-knob
    const char* unit = "";
    const juce::StringArray* choices = nullptr;
    bool affectsSound = true;          // false for generator controls
};

// clang-format off
inline const std::vector<ParamDef>& paramDefs()
{
    static const std::vector<ParamDef> defs {
        // Oscillator A
        { "a_on",     "Osc A On",       Kind::Bool,   0, 1, 1 },
        { "a_wave",   "Osc A Wave",     Kind::Float,  0, 7, 2 },
        { "a_oct",    "Osc A Octave",   Kind::Int,   -3, 3, 0 },
        { "a_semi",   "Osc A Semi",     Kind::Int,  -12, 12, 0 },
        { "a_fine",   "Osc A Fine",     Kind::Float,-100, 100, 0, 0, "ct" },
        { "a_level",  "Osc A Level",    Kind::Float,  0, 1, 0.8f },
        { "a_uni",    "Osc A Unison",   Kind::Int,    1, 7, 1 },
        { "a_detune", "Osc A Detune",   Kind::Float,  0, 1, 0.25f },
        { "a_width",  "Osc A Width",    Kind::Float,  0, 1, 0.7f },
        // Oscillator B
        { "b_on",     "Osc B On",       Kind::Bool,   0, 1, 0 },
        { "b_wave",   "Osc B Wave",     Kind::Float,  0, 7, 3 },
        { "b_oct",    "Osc B Octave",   Kind::Int,   -3, 3, 0 },
        { "b_semi",   "Osc B Semi",     Kind::Int,  -12, 12, 0 },
        { "b_fine",   "Osc B Fine",     Kind::Float,-100, 100, 0, 0, "ct" },
        { "b_level",  "Osc B Level",    Kind::Float,  0, 1, 0.6f },
        { "b_uni",    "Osc B Unison",   Kind::Int,    1, 7, 1 },
        { "b_detune", "Osc B Detune",   Kind::Float,  0, 1, 0.25f },
        { "b_width",  "Osc B Width",    Kind::Float,  0, 1, 0.7f },
        // Sub + noise
        { "sub_level","Sub Level",      Kind::Float,  0, 1, 0 },
        { "sub_oct",  "Sub Octave",     Kind::Choice, 0, 1, 0, 0, "", &subOctaves },
        { "sub_shape","Sub Shape",      Kind::Float,  0, 1, 0 },
        { "noise_level","Noise Level",  Kind::Float,  0, 1, 0 },
        { "noise_color","Noise Colour", Kind::Float,  0, 1, 0.6f },
        // Filter
        { "flt_type", "Filter Type",    Kind::Choice, 0, 4, 1, 0, "", &filterTypes },
        { "flt_cutoff","Cutoff",        Kind::Float, 20, 20000, 20000, 1000, "Hz" },
        { "flt_res",  "Resonance",      Kind::Float,  0, 1, 0.1f },
        { "flt_drive","Filter Drive",   Kind::Float,  0, 1, 0 },
        { "flt_env",  "Filter Env Amt", Kind::Float, -1, 1, 0 },
        { "flt_key",  "Key Track",      Kind::Float,  0, 1, 0.3f },
        // Envelopes (ms)
        { "e1_a", "Amp Attack",  Kind::Float, 0, 5000, 2,    200, "ms" },
        { "e1_d", "Amp Decay",   Kind::Float, 1, 8000, 400,  400, "ms" },
        { "e1_s", "Amp Sustain", Kind::Float, 0, 1, 0.8f },
        { "e1_r", "Amp Release", Kind::Float, 1, 8000, 150,  400, "ms" },
        { "e2_a", "Env2 Attack", Kind::Float, 0, 5000, 2,    200, "ms" },
        { "e2_d", "Env2 Decay",  Kind::Float, 1, 8000, 400,  400, "ms" },
        { "e2_s", "Env2 Sustain",Kind::Float, 0, 1, 0.3f },
        { "e2_r", "Env2 Release",Kind::Float, 1, 8000, 300,  400, "ms" },
        // Pitch envelope
        { "penv_amt",   "Pitch Env Amt",   Kind::Float, -24, 48, 0, 0, "st" },
        { "penv_decay", "Pitch Env Decay", Kind::Float, 1, 1000, 40, 80, "ms" },
        // LFO
        { "lfo_shape", "LFO Shape",  Kind::Choice, 0, 4, 0, 0, "", &lfoShapes },
        { "lfo_rate",  "LFO Rate",   Kind::Float, 0.05f, 20, 2, 2, "Hz" },
        { "lfo_sync",  "LFO Sync",   Kind::Bool,  0, 1, 0 },
        { "lfo_div",   "LFO Division", Kind::Choice, 0, 11, 4, 0, "", &lfoDivs },
        { "lfo_dest",  "LFO Dest",   Kind::Choice, 0, 5, 1, 0, "", &lfoDests },
        { "lfo_amt",   "LFO Amount", Kind::Float, 0, 1, 0 },
        // Voicing
        { "voice_mode","Voice Mode", Kind::Choice, 0, 2, 0, 0, "", &voiceModes },
        { "glide",     "Glide",      Kind::Float, 0, 2000, 0, 150, "ms" },
        { "bend",      "Bend Range", Kind::Int,   1, 24, 2, 0, "st" },
        { "vel_sens",  "Velocity",   Kind::Float, 0, 1, 0.5f },
        // FX: distortion
        { "dist_on",   "Dist On",    Kind::Bool,  0, 1, 0 },
        { "dist_type", "Dist Type",  Kind::Choice, 0, 3, 0, 0, "", &distTypes },
        { "dist_drive","Dist Drive", Kind::Float, 0, 1, 0.4f },
        { "dist_tone", "Dist Tone",  Kind::Float, 0, 1, 0.7f },
        { "dist_mix",  "Dist Mix",   Kind::Float, 0, 1, 1 },
        // chorus
        { "cho_on",    "Chorus On",  Kind::Bool,  0, 1, 0 },
        { "cho_rate",  "Chorus Rate",Kind::Float, 0.05f, 5, 0.6f, 0.8f, "Hz" },
        { "cho_depth", "Chorus Depth",Kind::Float,0, 1, 0.5f },
        { "cho_mix",   "Chorus Mix", Kind::Float, 0, 1, 0.4f },
        // delay
        { "dly_on",    "Delay On",   Kind::Bool,  0, 1, 0 },
        { "dly_div",   "Delay Time", Kind::Choice, 0, 10, 6, 0, "", &delayDivs },
        { "dly_fb",    "Delay Feedback", Kind::Float, 0, 0.95f, 0.35f },
        { "dly_tone",  "Delay Tone", Kind::Float, 0, 1, 0.6f },
        { "dly_mix",   "Delay Mix",  Kind::Float, 0, 1, 0.25f },
        { "dly_pp",    "Ping Pong",  Kind::Bool,  0, 1, 1 },
        // reverb
        { "rev_on",    "Reverb On",  Kind::Bool,  0, 1, 0 },
        { "rev_size",  "Reverb Size",Kind::Float, 0, 1, 0.7f },
        { "rev_damp",  "Reverb Damp",Kind::Float, 0, 1, 0.4f },
        { "rev_width", "Reverb Width",Kind::Float,0, 1, 1 },
        { "rev_mix",   "Reverb Mix", Kind::Float, 0, 1, 0.25f },
        // master
        { "master_vol","Master",     Kind::Float, -36, 12, -3, 0, "dB" },
        { "limiter",   "Limiter",    Kind::Bool,  0, 1, 1 },
        // generator (do not change the sound directly)
        { "gen_cat",   "Generator Category", Kind::Choice, 0, 4, 0, 0, "", &categories, false },
        { "gen_wild",  "Wildness",   Kind::Float, 0, 1, 0.35f, 0, "", nullptr, false },
        // ---- v1.1 drum / vox / loop engine (appended so v1.0 automation indices stay put)
        { "noise_decay","Noise Decay", Kind::Float, 0, 2000, 0, 120, "ms" },   // 0 = noise follows the amp env
        { "ring",      "Ring Mod",   Kind::Float, 0, 1, 0 },
        { "burst",     "Claps",      Kind::Int,   0, 6, 0 },
        { "burst_gap", "Clap Gap",   Kind::Float, 3, 40, 10, 0, "ms" },
        { "one_shot",  "One Shot",   Kind::Bool,  0, 1, 0 },
        { "vox_on",    "Vox On",     Kind::Bool,  0, 1, 0 },
        { "vox_vowel", "Vowel",      Kind::Float, 0, 4, 0 },
        { "vox_env",   "Vowel Env",  Kind::Float, -1, 1, 0 },
        { "vox_mix",   "Vox Mix",    Kind::Float, 0, 1, 1 },
        { "seq_on",    "Loop On",    Kind::Bool,  0, 1, 0 },
        { "seq_pat",   "Loop Pattern", Kind::Int, 0, 65535, 4369 },
        { "seq_rate",  "Loop Rate",  Kind::Choice, 0, 3, 1, 0, "", &seqRates },
        { "seq_vary",  "Loop Vary",  Kind::Float, 0, 1, 0.3f },
        { "seq_swing", "Swing",      Kind::Float, 0, 1, 0 },
        { "boost",     "Boost",      Kind::Float, 0, 24, 0, 0, "dB" },     // pre-FX gain: drives the distortion, lifts quiet hits
        // export (do not change the sound)
        { "exp_note",  "Export Note", Kind::Int, 24, 84, 36, 0, "", nullptr, false },
        { "exp_bpm",   "Export BPM", Kind::Int, 60, 200, 150, 0, "", nullptr, false },
    };
    return defs;
}
// clang-format on

inline const ParamDef* findDef (const juce::String& id)
{
    for (auto& d : paramDefs())
        if (id == d.id)
            return &d;
    return nullptr;
}

inline juce::NormalisableRange<float> rangeFor (const ParamDef& d)
{
    if (d.kind == Kind::Int || d.kind == Kind::Choice || d.kind == Kind::Bool)
        return { d.min, d.max, 1.0f };
    juce::NormalisableRange<float> r (d.min, d.max);
    if (d.skewCentre > 0.0f)
        r.setSkewForCentre (d.skewCentre);
    return r;
}

inline juce::String formatValue (const ParamDef& d, float v)
{
    if (d.kind == Kind::Bool)   return v > 0.5f ? "On" : "Off";
    if (d.kind == Kind::Choice) return (*d.choices)[juce::jlimit (0, d.choices->size() - 1, (int) std::round (v))];
    const juce::String unit (d.unit);
    if (d.kind == Kind::Int)
    {
        const int i = (int) std::round (v);
        if (juce::String (d.id) == "exp_note") return juce::MidiMessage::getMidiNoteName (i, true, true, 4);
        if (juce::String (d.id) == "exp_bpm")  return juce::String (i) + " BPM";
        if (juce::String (d.id) == "seq_pat")  { juce::String t; for (int b = 0; b < 16; ++b) t << ((i >> b) & 1 ? "x" : "-"); return t; }
        if (juce::String (d.id) == "burst")    return i == 0 ? juce::String ("Off") : juce::String (i) + "x";
        if (juce::String (d.id).endsWith ("_oct") || juce::String (d.id).endsWith ("_semi"))
            return (i > 0 ? "+" : "") + juce::String (i);
        if (juce::String (d.id).endsWith ("_uni")) return juce::String (i) + (i == 1 ? " voice" : " voices");
        return juce::String (i) + (unit.isNotEmpty() ? " " + unit : juce::String());
    }
    if (juce::String (d.id) == "noise_decay" && v < 0.5f) return "Amp Env";
    if (juce::String (d.id) == "vox_vowel")
    {
        const int i = (int) std::floor (v);
        const float f = v - (float) i;
        if (f < 0.05f || i >= 4) return vowelNames[juce::jlimit (0, 4, (int) std::round (v))];
        return vowelNames[i] + " > " + vowelNames[i + 1] + " " + juce::String (juce::roundToInt (f * 100)) + "%";
    }
    if (juce::String (d.id).endsWith ("_wave"))
    {
        const int i = (int) std::floor (v);
        const float f = v - (float) i;
        if (f < 0.04f || i >= 7) return waveNames[juce::jlimit (0, 7, (int) std::round (v))];
        return waveNames[i] + " > " + waveNames[i + 1] + " " + juce::String (juce::roundToInt (f * 100)) + "%";
    }
    if (unit == "Hz")  return v >= 1000.0f ? juce::String (v / 1000.0f, 2) + " kHz" : juce::String (v, v < 10 ? 2 : 0) + " Hz";
    if (unit == "ms")  return v >= 1000.0f ? juce::String (v / 1000.0f, 2) + " s" : juce::String (v, v < 10 ? 1 : 0) + " ms";
    if (unit == "dB")  return juce::String (v, 1) + " dB";
    if (unit == "st")  return (v > 0 ? "+" : "") + juce::String (v, 1) + " st";
    if (unit == "ct")  return (v > 0 ? "+" : "") + juce::String (juce::roundToInt (v)) + " ct";
    if (d.min < 0.0f)  return (v > 0 ? "+" : "") + juce::String (juce::roundToInt (v * 100)) + "%";
    return juce::String (juce::roundToInt (v * 100)) + "%";
}

inline juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    for (auto& d : paramDefs())
    {
        const juce::ParameterID pid { d.id, 1 };
        if (d.kind == Kind::Bool)
            layout.add (std::make_unique<juce::AudioParameterBool> (pid, d.name, d.def > 0.5f));
        else if (d.kind == Kind::Choice)
            layout.add (std::make_unique<juce::AudioParameterChoice> (pid, d.name, *d.choices, (int) d.def));
        else if (d.kind == Kind::Int)
            layout.add (std::make_unique<juce::AudioParameterInt> (pid, d.name, (int) d.min, (int) d.max, (int) d.def,
                juce::AudioParameterIntAttributes().withStringFromValueFunction ([&d] (int v, int) { return formatValue (d, (float) v); })));
        else
            layout.add (std::make_unique<juce::AudioParameterFloat> (pid, d.name, rangeFor (d), d.def,
                juce::AudioParameterFloatAttributes().withStringFromValueFunction ([&d] (float v, int) { return formatValue (d, v); })));
    }
    return layout;
}
} // namespace xs
