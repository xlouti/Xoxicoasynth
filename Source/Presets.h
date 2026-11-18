#pragma once
#include "Params.h"
#include "FactoryLevels.h"
#include <map>

// Factory presets and the sound generator.
namespace xs
{
using Patch = std::map<juce::String, float>;

struct Preset
{
    juce::String name;
    int category;      // index into categories
    Patch values;
};

// The neutral starting point every preset and roll is built on.
inline Patch initPatch()
{
    Patch p;
    for (auto& d : paramDefs())
        if (d.affectsSound) p[d.id] = d.def;
    return p;
}

inline Patch merge (Patch base, const Patch& over)
{
    for (auto& kv : over) base[kv.first] = kv.second;
    return base;
}

// ---- category templates (the "centre" sound of each category)
inline Patch categoryBase (int cat)
{
    switch (cat)
    {
        case 0: return merge (initPatch(), {   // Rage Lead
            { "a_wave", 2 }, { "a_uni", 7 }, { "a_detune", 0.38f }, { "a_width", 0.9f }, { "a_level", 0.8f },
            { "b_on", 1 }, { "b_wave", 2.6f }, { "b_oct", 1 }, { "b_uni", 5 }, { "b_detune", 0.3f }, { "b_level", 0.45f },
            { "sub_level", 0.15f }, { "flt_type", 1 }, { "flt_cutoff", 7500 }, { "flt_res", 0.15f }, { "flt_drive", 0.25f },
            { "flt_env", 0.15f }, { "flt_key", 0.4f },
            { "e1_a", 3 }, { "e1_d", 500 }, { "e1_s", 0.85f }, { "e1_r", 180 },
            { "e2_a", 1 }, { "e2_d", 350 }, { "e2_s", 0.35f }, { "e2_r", 200 },
            { "penv_amt", 0.5f }, { "penv_decay", 40 }, { "lfo_dest", 0 }, { "lfo_rate", 5.2f }, { "lfo_amt", 0.08f },
            { "dist_on", 1 }, { "dist_type", 0 }, { "dist_drive", 0.38f }, { "dist_tone", 0.75f }, { "dist_mix", 0.8f },
            { "dly_on", 1 }, { "dly_div", 6 }, { "dly_fb", 0.3f }, { "dly_mix", 0.16f },
            { "rev_on", 1 }, { "rev_size", 0.75f }, { "rev_mix", 0.26f }, { "master_vol", -4 } });
        case 1: return merge (initPatch(), {   // Plugg Bell
            { "a_wave", 0.4f }, { "a_oct", 1 }, { "a_uni", 1 }, { "a_level", 0.85f },
            { "b_on", 1 }, { "b_wave", 7 }, { "b_oct", 1 }, { "b_level", 0.4f },
            { "flt_type", 0 }, { "flt_cutoff", 9000 }, { "flt_res", 0.05f }, { "flt_env", 0.2f },
            { "e1_a", 1 }, { "e1_d", 650 }, { "e1_s", 0.0f }, { "e1_r", 550 },
            { "e2_a", 0 }, { "e2_d", 250 }, { "e2_s", 0.0f }, { "e2_r", 300 },
            { "penv_amt", 0.4f }, { "penv_decay", 12 }, { "lfo_dest", 0 }, { "lfo_rate", 5.5f }, { "lfo_amt", 0.06f },
            { "vel_sens", 0.6f },
            { "cho_on", 1 }, { "cho_rate", 0.5f }, { "cho_depth", 0.45f }, { "cho_mix", 0.4f },
            { "dly_on", 1 }, { "dly_div", 6 }, { "dly_fb", 0.38f }, { "dly_mix", 0.24f }, { "dly_pp", 1 },
            { "rev_on", 1 }, { "rev_size", 0.82f }, { "rev_damp", 0.45f }, { "rev_mix", 0.38f }, { "master_vol", -2 } });
        case 2: return merge (initPatch(), {   // 808
            { "a_wave", 0.15f }, { "a_oct", -1 }, { "a_uni", 1 }, { "a_level", 0.95f },
            { "sub_level", 0.15f }, { "sub_oct", 0 },
            { "flt_type", 0 }, { "flt_cutoff", 3500 }, { "flt_res", 0.05f }, { "flt_drive", 0.3f }, { "flt_key", 0.0f },
            { "e1_a", 1 }, { "e1_d", 1800 }, { "e1_s", 0.7f }, { "e1_r", 260 },
            { "penv_amt", 7 }, { "penv_decay", 38 },
            { "voice_mode", 2 }, { "glide", 110 }, { "vel_sens", 0.2f },
            { "dist_on", 1 }, { "dist_type", 0 }, { "dist_drive", 0.4f }, { "dist_tone", 0.55f }, { "dist_mix", 0.85f },
            { "master_vol", -3 } });
        case 3: return merge (initPatch(), {   // Dark Pad
            { "a_wave", 2.3f }, { "a_uni", 7 }, { "a_detune", 0.45f }, { "a_width", 1.0f }, { "a_level", 0.7f },
            { "b_on", 1 }, { "b_wave", 4.5f }, { "b_oct", -1 }, { "b_uni", 3 }, { "b_detune", 0.3f }, { "b_level", 0.45f },
            { "noise_level", 0.04f }, { "noise_color", 0.3f },
            { "flt_type", 1 }, { "flt_cutoff", 1200 }, { "flt_res", 0.22f }, { "flt_env", 0.2f }, { "flt_key", 0.25f },
            { "e1_a", 700 }, { "e1_d", 2000 }, { "e1_s", 0.85f }, { "e1_r", 1800 },
            { "e2_a", 1200 }, { "e2_d", 2500 }, { "e2_s", 0.6f }, { "e2_r", 1800 },
            { "lfo_shape", 1 }, { "lfo_dest", 1 }, { "lfo_rate", 0.25f }, { "lfo_amt", 0.3f },
            { "cho_on", 1 }, { "cho_rate", 0.3f }, { "cho_depth", 0.6f }, { "cho_mix", 0.5f },
            { "rev_on", 1 }, { "rev_size", 0.93f }, { "rev_damp", 0.55f }, { "rev_mix", 0.48f }, { "master_vol", -3 } });
        case 4: return merge (initPatch(), {  // Pluck
            { "a_wave", 2.2f }, { "a_uni", 3 }, { "a_detune", 0.15f }, { "a_level", 0.8f },
            { "b_on", 1 }, { "b_wave", 3.4f }, { "b_oct", 1 }, { "b_level", 0.3f },
            { "flt_type", 1 }, { "flt_cutoff", 600 }, { "flt_res", 0.25f }, { "flt_env", 0.7f }, { "flt_key", 0.5f },
            { "e1_a", 1 }, { "e1_d", 380 }, { "e1_s", 0.0f }, { "e1_r", 260 },
            { "e2_a", 0 }, { "e2_d", 200 }, { "e2_s", 0.0f }, { "e2_r", 200 },
            { "dly_on", 1 }, { "dly_div", 6 }, { "dly_fb", 0.33f }, { "dly_mix", 0.22f },
            { "rev_on", 1 }, { "rev_size", 0.65f }, { "rev_mix", 0.22f }, { "master_vol", -2 } });
        default: return merge (initPatch(), {  // drums / one-shots / loops / fx / vox
            { "a_wave", 0 }, { "a_uni", 1 }, { "a_level", 0.8f }, { "a_width", 0.0f },
            { "flt_type", 0 }, { "flt_cutoff", 20000 }, { "flt_res", 0.0f }, { "flt_key", 0.0f },
            { "e1_a", 0 }, { "e1_d", 300 }, { "e1_s", 0.0f }, { "e1_r", 120 },
            { "e2_a", 0 }, { "e2_d", 200 }, { "e2_s", 0.0f }, { "e2_r", 150 },
            { "vel_sens", 0.35f }, { "one_shot", 1 }, { "master_vol", 0 } });
    }
}

// Sound types that are pitched instruments: their export file name carries the key.
inline bool isTonalType (int t) { return t <= stPluck || t == stSubBass || t == stVox; }

inline std::vector<Preset> buildFactoryPresets()
{
    std::vector<Preset> v;
    auto add = [&v] (int cat, const char* name, const Patch& over) { v.push_back ({ name, cat, merge (categoryBase (cat), over) }); };

    // ---------------- Rage Lead
    add (0, "Glokk Siren", { { "a_detune", 0.45f }, { "b_semi", 7 }, { "b_oct", 0 }, { "lfo_dest", 0 }, { "lfo_rate", 6.5f }, { "lfo_amt", 0.14f }, { "rev_mix", 0.32f } });
    add (0, "Mosh Pit Saw", { { "dist_type", 3 }, { "dist_drive", 0.62f }, { "flt_cutoff", 5200 }, { "flt_drive", 0.45f }, { "b_oct", -1 }, { "rev_mix", 0.18f } });
    add (0, "Spaz Supersaw", { { "a_detune", 0.55f }, { "b_detune", 0.5f }, { "b_uni", 7 }, { "cho_on", 1 }, { "cho_mix", 0.3f }, { "flt_cutoff", 11000 } });
    add (0, "Vamp Stack", { { "a_wave", 3 }, { "b_wave", 2 }, { "b_oct", -1 }, { "sub_level", 0.3f }, { "dist_type", 1 }, { "dist_drive", 0.45f }, { "rev_size", 0.88f } });
    add (0, "Pit Alarm", { { "a_wave", 4.2f }, { "a_uni", 5 }, { "b_wave", 5 }, { "b_semi", 12 }, { "lfo_dest", 2 }, { "lfo_sync", 1 }, { "lfo_div", 9 }, { "lfo_amt", 0.35f } });
    add (0, "Blown Speaker", { { "dist_type", 2 }, { "dist_drive", 0.55f }, { "dist_tone", 0.6f }, { "flt_cutoff", 4200 }, { "flt_res", 0.3f }, { "dly_mix", 0.1f } });
    add (0, "Night Riot", { { "a_wave", 6 }, { "b_wave", 2.4f }, { "flt_cutoff", 3000 }, { "flt_env", 0.45f }, { "e2_d", 600 }, { "penv_amt", 2 }, { "rev_mix", 0.34f } });
    add (0, "Rage Octaves", { { "b_oct", 1 }, { "b_level", 0.65f }, { "sub_level", 0.35f }, { "a_uni", 5 }, { "dist_drive", 0.3f }, { "dly_div", 8 } });

    // ---------------- Plugg Bell
    add (1, "Plugg Glass", { { "b_level", 0.55f }, { "rev_mix", 0.42f } });
    add (1, "Candy Bell", { { "a_wave", 1 }, { "b_oct", 2 }, { "b_level", 0.3f }, { "e1_d", 500 }, { "cho_mix", 0.5f } });
    add (1, "Lean Keys", { { "a_wave", 0.8f }, { "a_oct", 0 }, { "e1_s", 0.25f }, { "e1_d", 900 }, { "lfo_amt", 0.1f }, { "lfo_rate", 4.5f }, { "flt_cutoff", 5000 } });
    add (1, "Cloud Chime", { { "b_wave", 6.6f }, { "b_semi", 7 }, { "dly_div", 8 }, { "dly_fb", 0.5f }, { "rev_size", 0.92f }, { "rev_mix", 0.5f } });
    add (1, "Plugg Music Box", { { "a_oct", 2 }, { "b_oct", 2 }, { "e1_d", 380 }, { "e1_r", 400 }, { "penv_amt", 1 }, { "penv_decay", 8 } });
    add (1, "Soft Mallet", { { "a_wave", 0 }, { "b_wave", 1 }, { "b_level", 0.25f }, { "flt_cutoff", 3500 }, { "e1_d", 750 }, { "cho_mix", 0.25f } });
    add (1, "Dreamy Kalimba", { { "a_wave", 0.2f }, { "b_wave", 7 }, { "b_semi", 12 }, { "b_oct", 0 }, { "e1_d", 420 }, { "dly_div", 5 }, { "dly_fb", 0.45f } });
    add (1, "Pink Haze Bell", { { "a_uni", 3 }, { "a_detune", 0.12f }, { "dist_on", 1 }, { "dist_drive", 0.15f }, { "dist_mix", 0.4f }, { "cho_mix", 0.6f }, { "rev_mix", 0.45f } });

    // ---------------- 808
    add (2, "Glide 808", {});
    add (2, "Rage Boom", { { "dist_drive", 0.55f }, { "dist_type", 1 }, { "penv_amt", 10 }, { "e1_d", 2500 }, { "glide", 90 } });
    add (2, "Mosh 808 Dist", { { "dist_type", 3 }, { "dist_drive", 0.68f }, { "flt_cutoff", 5000 }, { "flt_drive", 0.5f }, { "sub_level", 0.25f } });
    add (2, "Clean Sub", { { "dist_on", 0 }, { "flt_drive", 0.0f }, { "penv_amt", 3 }, { "a_wave", 0 }, { "sub_level", 0.0f } });
    add (2, "Punch Knock", { { "penv_amt", 14 }, { "penv_decay", 25 }, { "e1_d", 700 }, { "e1_s", 0.35f }, { "dist_drive", 0.5f } });
    add (2, "Long Slide", { { "glide", 220 }, { "e1_d", 4000 }, { "e1_s", 0.85f }, { "e1_r", 400 } });
    add (2, "Fuzz 808", { { "dist_type", 2 }, { "dist_drive", 0.45f }, { "dist_tone", 0.45f }, { "b_on", 1 }, { "b_wave", 2 }, { "b_oct", -1 }, { "b_level", 0.15f } });
    add (2, "Plugg Low", { { "dist_drive", 0.2f }, { "dist_mix", 0.5f }, { "flt_cutoff", 1600 }, { "penv_amt", 4 }, { "e1_d", 2200 }, { "glide", 140 } });

    // ---------------- Dark Pad
    add (3, "Void Pad", {});
    add (3, "Night Drive", { { "lfo_sync", 1 }, { "lfo_div", 2 }, { "lfo_amt", 0.45f }, { "flt_cutoff", 900 }, { "dly_on", 1 }, { "dly_div", 9 }, { "dly_mix", 0.15f } });
    add (3, "Hollow Choir", { { "a_wave", 6 }, { "b_wave", 6.5f }, { "flt_type", 4 }, { "flt_cutoff", 1500 }, { "flt_res", 0.3f }, { "cho_mix", 0.6f } });
    add (3, "Smoke Room", { { "noise_level", 0.12f }, { "noise_color", 0.5f }, { "flt_cutoff", 800 }, { "rev_mix", 0.6f } });
    add (3, "Underworld", { { "a_oct", -1 }, { "b_oct", -2 }, { "sub_level", 0.3f }, { "flt_cutoff", 650 }, { "dist_on", 1 }, { "dist_drive", 0.25f }, { "dist_mix", 0.4f } });
    add (3, "Cold Strings", { { "a_wave", 2 }, { "b_wave", 2 }, { "b_oct", 0 }, { "b_fine", 9 }, { "flt_cutoff", 2400 }, { "e1_a", 400 }, { "lfo_dest", 0 }, { "lfo_rate", 4.8f }, { "lfo_amt", 0.07f } });
    add (3, "Gloom Swell", { { "e1_a", 2200 }, { "flt_env", 0.45f }, { "e2_a", 2500 }, { "flt_cutoff", 500 } });
    add (3, "Black Ice", { { "a_wave", 7 }, { "a_uni", 5 }, { "b_wave", 1 }, { "b_oct", 1 }, { "flt_type", 0 }, { "flt_cutoff", 4000 }, { "rev_size", 0.98f } });

    // ---------------- Pluck
    add (4, "Rage Pluck", { { "a_uni", 5 }, { "a_detune", 0.3f }, { "dist_on", 1 }, { "dist_drive", 0.3f }, { "dist_mix", 0.6f } });
    add (4, "Plugg Pluck", { { "a_wave", 1.2f }, { "b_wave", 7 }, { "flt_cutoff", 1200 }, { "flt_env", 0.5f }, { "cho_on", 1 }, { "cho_mix", 0.35f } });
    add (4, "Glass Pick", { { "a_wave", 7 }, { "a_uni", 1 }, { "b_wave", 0 }, { "b_oct", 2 }, { "flt_cutoff", 2000 }, { "e1_d", 300 } });
    add (4, "Saw Stab", { { "a_uni", 7 }, { "a_detune", 0.4f }, { "e1_d", 220 }, { "e2_d", 140 }, { "rev_mix", 0.3f } });
    add (4, "Bounce Pluck", { { "penv_amt", 12 }, { "penv_decay", 15 }, { "flt_res", 0.4f }, { "dly_div", 5 } });
    add (4, "Dark Pick", { { "flt_cutoff", 300 }, { "flt_env", 0.55f }, { "b_oct", -1 }, { "sub_level", 0.2f }, { "rev_mix", 0.3f } });
    add (4, "Rubber Pluck", { { "flt_res", 0.6f }, { "flt_env", 0.6f }, { "e2_d", 300 }, { "a_wave", 3 }, { "a_uni", 1 } });
    add (4, "Sparkle Pluck", { { "a_oct", 1 }, { "b_oct", 2 }, { "flt_cutoff", 1500 }, { "cho_on", 1 }, { "cho_mix", 0.4f }, { "rev_mix", 0.35f } });
    // loudness-matched master levels (generated by: XoxicoaSynthAudit calibrate Source/FactoryLevels.h)
    for (auto& p : v)
        if (auto it = factoryMasterDb().find (p.name); it != factoryMasterDb().end())
            p.values["master_vol"] = it->second;
    return v;
}

inline const std::vector<Preset>& factoryPresets()
{
    static const auto v = buildFactoryPresets();
    return v;
}

// ---------------- Generator
// Every roll starts from the category template, then randomises inside hand-tuned
// "safe" ranges. Wildness widens each range toward a "wild" range and lets
// choices pick from a wider set, so a low roll is always usable and a high
// roll is still musical.
class Generator
{
public:
    static Patch generate (int cat, float wild, juce::Random& rng)
    {
        Generator g (rng, wild);
        Patch p = categoryBase (cat);
        switch (cat)
        {
            case 0: g.rageLead (p); break;
            case 1: g.pluggBell (p); break;
            case 2: g.eight08 (p); break;
            case 3: g.darkPad (p); break;
            case 4: g.pluck (p); break;
            case stKick: g.kick (p); break;
            case stSnare: g.snare (p); break;
            case stClap: g.clap (p); break;
            case stHiHat: g.hat (p, 0); break;
            case stOpenHat: g.hat (p, 1); break;
            case stCrash: g.hat (p, 2); break;
            case stPerc: g.perc (p); break;
            case stSnap: g.snap (p); break;
            case stSubBass: g.subBass (p); break;
            case stPercLoop: g.percLoop (p); break;
            case stFX: g.fx (p); break;
            case stVox: g.vox (p); break;
            default: break;
        }
        return p;
    }

    // MIDI note the export / audition plays. Tonal types get a random key in a useful register.
    static int exportNote (int cat, juce::Random& rng)
    {
        switch (cat)
        {
            case st808: case stSubBass: return 28 + rng.nextInt (12);   // E1 .. D#2
            case stVox:                 return 50 + rng.nextInt (12);   // D3 .. C#4
            case stDarkPad: case stPluck: case stRageLead: case stPluggBell: return 55 + rng.nextInt (12);
            default:                    return 48;                      // drums are tuned with the oscillator Octave/Semi
        }
    }

private:
    juce::Random& r;
    float w;
    Generator (juce::Random& rr, float wild) : r (rr), w (juce::jlimit (0.0f, 1.0f, wild)) {}

    float rnd() { return r.nextFloat(); }
    // safe range [lo,hi], widened toward [wlo,whi] by wildness
    float range (float lo, float hi, float wlo, float whi, bool logScale = false)
    {
        const float a = lo + (wlo - lo) * w, b = hi + (whi - hi) * w;
        const float t = rnd();
        if (logScale && a > 0.0f && b > 0.0f) return a * std::pow (b / a, t);
        return a + (b - a) * t;
    }
    float pick (std::initializer_list<float> safe, std::initializer_list<float> wildSet = {})
    {
        const bool useWild = wildSet.size() > 0 && rnd() < w * 0.8f;
        const auto& set = useWild ? wildSet : safe;
        return *(set.begin() + juce::jmin ((int) set.size() - 1, (int) (rnd() * (float) set.size())));
    }
    bool chance (float safeP, float wildP) { return rnd() < safeP + (wildP - safeP) * w; }

    void rageLead (Patch& p)
    {
        p["a_wave"] = range (1.7f, 2.6f, 0.8f, 6.5f);
        p["a_uni"] = pick ({ 5, 7, 7 }, { 3, 4, 5, 6, 7 });
        p["a_detune"] = range (0.28f, 0.5f, 0.12f, 0.8f);
        p["a_width"] = range (0.7f, 1.0f, 0.3f, 1.0f);
        p["b_on"] = chance (0.85f, 0.7f) ? 1.0f : 0.0f;
        p["b_wave"] = range (1.8f, 3.2f, 0.0f, 7.0f);
        p["b_oct"] = pick ({ 1, 0, -1 }, { 1, 0, -1, 2 });
        p["b_semi"] = pick ({ 0, 0, 0, 7 }, { 0, 3, 5, 7, -5, 12 });
        p["b_uni"] = pick ({ 3, 5, 7 }, { 1, 3, 5, 7 });
        p["b_detune"] = range (0.15f, 0.45f, 0.05f, 0.8f);
        p["b_level"] = range (0.35f, 0.65f, 0.2f, 0.8f);
        p["sub_level"] = range (0.0f, 0.3f, 0.0f, 0.5f);
        p["noise_level"] = range (0.0f, 0.04f, 0.0f, 0.15f);
        p["flt_type"] = pick ({ 1, 1, 0 }, { 0, 1, 4 });
        p["flt_cutoff"] = range (3500.0f, 12000.0f, 1200.0f, 20000.0f, true);
        p["flt_res"] = range (0.05f, 0.3f, 0.0f, 0.6f);
        p["flt_drive"] = range (0.1f, 0.4f, 0.0f, 0.8f);
        p["flt_env"] = range (0.0f, 0.3f, -0.1f, 0.7f);
        p["e1_a"] = range (1.0f, 8.0f, 0.0f, 40.0f);
        p["e1_d"] = range (250.0f, 900.0f, 100.0f, 2000.0f, true);
        p["e1_s"] = range (0.65f, 1.0f, 0.3f, 1.0f);
        p["e1_r"] = range (100.0f, 300.0f, 40.0f, 900.0f, true);
        p["e2_d"] = range (150.0f, 600.0f, 60.0f, 1500.0f, true);
        p["penv_amt"] = chance (0.25f, 0.5f) ? range (1.0f, 3.0f, -12.0f, 12.0f) : 0.0f;
        p["penv_decay"] = range (20.0f, 80.0f, 10.0f, 300.0f, true);
        const int lfoMode = (int) pick ({ 0, 0, 9 }, { 0, 1, 2, 4, 9 }); // 9 = off
        p["lfo_amt"] = 0.0f;
        if (lfoMode != 9)
        {
            p["lfo_dest"] = (float) lfoMode;
            p["lfo_shape"] = pick ({ 0 }, { 0, 1, 3, 4 });
            if (lfoMode == 0) { p["lfo_rate"] = range (4.5f, 6.5f, 3.0f, 8.0f); p["lfo_amt"] = range (0.04f, 0.12f, 0.02f, 0.25f); p["lfo_sync"] = 0; }
            else { p["lfo_sync"] = 1; p["lfo_div"] = pick ({ 7, 9 }, { 4, 7, 9, 10, 11 }); p["lfo_amt"] = range (0.15f, 0.35f, 0.1f, 0.7f); }
        }
        p["voice_mode"] = chance (0.1f, 0.35f) ? 2.0f : 0.0f;
        p["glide"] = p["voice_mode"] > 0 ? range (30.0f, 90.0f, 10.0f, 250.0f) : 0.0f;
        p["dist_on"] = chance (0.85f, 0.9f) ? 1.0f : 0.0f;
        p["dist_type"] = pick ({ 0, 0, 3 }, { 0, 1, 2, 3 });
        p["dist_drive"] = range (0.2f, 0.5f, 0.1f, 0.85f);
        p["dist_tone"] = range (0.6f, 0.9f, 0.35f, 1.0f);
        p["dist_mix"] = range (0.55f, 1.0f, 0.3f, 1.0f);
        p["cho_on"] = chance (0.3f, 0.5f) ? 1.0f : 0.0f;
        p["cho_mix"] = range (0.2f, 0.4f, 0.1f, 0.7f);
        p["dly_on"] = chance (0.7f, 0.8f) ? 1.0f : 0.0f;
        p["dly_div"] = pick ({ 6, 8, 5 }, { 3, 4, 5, 6, 8, 9 });
        p["dly_fb"] = range (0.2f, 0.4f, 0.1f, 0.65f);
        p["dly_mix"] = range (0.1f, 0.22f, 0.05f, 0.4f);
        p["rev_on"] = 1;
        p["rev_size"] = range (0.6f, 0.88f, 0.4f, 0.98f);
        p["rev_mix"] = range (0.18f, 0.34f, 0.1f, 0.55f);
    }

    void pluggBell (Patch& p)
    {
        p["a_wave"] = pick ({ 0, 0.3f, 0.7f, 1 }, { 0, 1, 6.5f, 7 }) + range (0.0f, 0.1f, 0.0f, 0.4f);
        p["a_oct"] = pick ({ 1, 1, 2 }, { 0, 1, 2 });
        p["a_uni"] = pick ({ 1, 1, 2 }, { 1, 2, 3 });
        p["a_detune"] = range (0.0f, 0.12f, 0.0f, 0.35f);
        p["b_on"] = chance (0.8f, 0.8f) ? 1.0f : 0.0f;
        p["b_wave"] = pick ({ 7, 7, 0 }, { 0, 1, 6, 7 });
        p["b_oct"] = pick ({ 1, 2 }, { 0, 1, 2 });
        p["b_semi"] = pick ({ 0, 0, 7, 12 }, { 0, 3, 5, 7, 12 });
        p["b_level"] = range (0.25f, 0.55f, 0.15f, 0.75f);
        p["flt_type"] = 0;
        p["flt_cutoff"] = range (4000.0f, 12000.0f, 1500.0f, 20000.0f, true);
        p["flt_res"] = range (0.0f, 0.15f, 0.0f, 0.4f);
        p["flt_env"] = range (0.0f, 0.3f, 0.0f, 0.6f);
        p["e1_a"] = range (0.0f, 3.0f, 0.0f, 15.0f);
        p["e1_d"] = range (350.0f, 900.0f, 150.0f, 1800.0f, true);
        p["e1_s"] = range (0.0f, 0.15f, 0.0f, 0.4f);
        p["e1_r"] = range (300.0f, 750.0f, 150.0f, 1500.0f, true);
        p["penv_amt"] = range (0.0f, 0.8f, -2.0f, 5.0f);
        p["penv_decay"] = range (6.0f, 20.0f, 3.0f, 60.0f);
        p["lfo_dest"] = 0; p["lfo_sync"] = 0;
        p["lfo_rate"] = range (4.5f, 6.5f, 3.0f, 8.0f);
        p["lfo_amt"] = range (0.0f, 0.09f, 0.0f, 0.18f);
        p["dist_on"] = chance (0.1f, 0.4f) ? 1.0f : 0.0f;
        p["dist_drive"] = range (0.1f, 0.25f, 0.05f, 0.5f);
        p["dist_mix"] = range (0.3f, 0.5f, 0.2f, 0.8f);
        p["cho_on"] = chance (0.8f, 0.7f) ? 1.0f : 0.0f;
        p["cho_mix"] = range (0.3f, 0.55f, 0.15f, 0.75f);
        p["cho_depth"] = range (0.3f, 0.6f, 0.1f, 0.9f);
        p["dly_on"] = chance (0.85f, 0.8f) ? 1.0f : 0.0f;
        p["dly_div"] = pick ({ 6, 8, 5 }, { 4, 5, 6, 7, 8, 9 });
        p["dly_fb"] = range (0.3f, 0.5f, 0.15f, 0.7f);
        p["dly_mix"] = range (0.18f, 0.32f, 0.1f, 0.45f);
        p["rev_on"] = 1;
        p["rev_size"] = range (0.75f, 0.92f, 0.5f, 0.99f);
        p["rev_mix"] = range (0.3f, 0.48f, 0.15f, 0.65f);
    }

    void eight08 (Patch& p)
    {
        p["a_wave"] = range (0.0f, 0.4f, 0.0f, 1.3f);
        p["a_oct"] = -1;
        p["b_on"] = chance (0.05f, 0.35f) ? 1.0f : 0.0f;
        p["b_wave"] = range (1.0f, 2.5f, 0.0f, 6.0f);
        p["b_oct"] = pick ({ -1, 0 }, { -1, 0, 1 });
        p["b_level"] = range (0.08f, 0.2f, 0.05f, 0.4f);
        p["sub_level"] = range (0.0f, 0.3f, 0.0f, 0.5f);
        p["flt_type"] = 0;
        p["flt_cutoff"] = range (700.0f, 2800.0f, 400.0f, 12000.0f, true);   // low enough that Cutoff / Reso / Env shape the drive
        p["flt_res"] = range (0.05f, 0.2f, 0.0f, 0.4f);
        p["flt_drive"] = range (0.3f, 0.6f, 0.1f, 0.85f);
        p["flt_env"] = range (0.1f, 0.35f, 0.0f, 0.6f);
        p["e2_d"] = range (120.0f, 400.0f, 60.0f, 900.0f, true);
        p["e1_d"] = range (900.0f, 2800.0f, 300.0f, 5000.0f, true);
        p["e1_s"] = range (0.5f, 0.9f, 0.1f, 1.0f);
        p["e1_r"] = range (150.0f, 450.0f, 60.0f, 900.0f, true);
        p["penv_amt"] = range (4.0f, 12.0f, 0.0f, 24.0f);
        p["penv_decay"] = range (22.0f, 65.0f, 10.0f, 150.0f, true);
        p["voice_mode"] = 2;
        p["glide"] = range (70.0f, 170.0f, 30.0f, 350.0f);
        p["dist_on"] = chance (0.85f, 0.95f) ? 1.0f : 0.0f;
        p["dist_type"] = pick ({ 0, 0, 1, 3 }, { 0, 1, 2, 3 });
        p["dist_drive"] = range (0.25f, 0.55f, 0.1f, 0.85f);
        p["dist_tone"] = range (0.4f, 0.75f, 0.25f, 1.0f);
        p["dist_mix"] = range (0.6f, 1.0f, 0.4f, 1.0f);
        p["rev_on"] = chance (0.0f, 0.3f) ? 1.0f : 0.0f;
        p["rev_mix"] = range (0.05f, 0.1f, 0.05f, 0.2f);
    }

    void darkPad (Patch& p)
    {
        p["a_wave"] = range (1.5f, 3.5f, 0.0f, 7.0f);
        p["a_uni"] = pick ({ 5, 7 }, { 3, 5, 7 });
        p["a_detune"] = range (0.3f, 0.55f, 0.15f, 0.85f);
        p["b_wave"] = range (3.5f, 6.0f, 0.0f, 7.0f);
        p["b_oct"] = pick ({ -1, 0 }, { -2, -1, 0, 1 });
        p["b_semi"] = pick ({ 0, 0, 7 }, { 0, 3, 5, 7 });
        p["b_level"] = range (0.3f, 0.55f, 0.15f, 0.75f);
        p["noise_level"] = range (0.0f, 0.08f, 0.0f, 0.2f);
        p["flt_type"] = pick ({ 1, 1, 0 }, { 0, 1, 4 });
        p["flt_cutoff"] = range (600.0f, 2200.0f, 380.0f, 6000.0f, true);
        p["flt_res"] = range (0.1f, 0.32f, 0.0f, 0.6f);
        p["flt_env"] = range (0.1f, 0.35f, -0.2f, 0.6f);
        p["e1_a"] = range (300.0f, 1500.0f, 80.0f, 3500.0f, true);
        p["e1_r"] = range (1000.0f, 2800.0f, 400.0f, 6000.0f, true);
        p["e2_a"] = range (500.0f, 2000.0f, 100.0f, 4000.0f, true);
        p["lfo_dest"] = pick ({ 1, 1, 2 }, { 1, 2, 3, 5 });
        p["lfo_shape"] = pick ({ 0, 1 }, { 0, 1, 4 });
        p["lfo_rate"] = range (0.1f, 0.6f, 0.05f, 3.0f, true);
        p["lfo_amt"] = range (0.15f, 0.4f, 0.05f, 0.75f);
        p["cho_on"] = 1;
        p["cho_mix"] = range (0.35f, 0.65f, 0.2f, 0.85f);
        p["dly_on"] = chance (0.2f, 0.5f) ? 1.0f : 0.0f;
        p["dly_div"] = pick ({ 8, 9 }, { 6, 8, 9, 10 });
        p["dly_mix"] = range (0.1f, 0.2f, 0.05f, 0.35f);
        p["rev_size"] = range (0.85f, 0.97f, 0.6f, 1.0f);
        p["rev_mix"] = range (0.38f, 0.58f, 0.2f, 0.75f);
    }

    void pluck (Patch& p)
    {
        p["a_wave"] = range (1.8f, 3.2f, 0.0f, 7.0f);
        p["a_uni"] = pick ({ 1, 3, 3 }, { 1, 3, 5, 7 });
        p["a_detune"] = range (0.05f, 0.22f, 0.0f, 0.55f);
        p["b_on"] = chance (0.7f, 0.7f) ? 1.0f : 0.0f;
        p["b_wave"] = range (2.5f, 5.0f, 0.0f, 7.0f);
        p["b_oct"] = pick ({ 0, 1 }, { -1, 0, 1, 2 });
        p["b_level"] = range (0.2f, 0.45f, 0.1f, 0.7f);
        p["flt_type"] = pick ({ 1, 0 }, { 0, 1 });
        p["flt_cutoff"] = range (350.0f, 1300.0f, 260.0f, 4000.0f, true);
        p["flt_res"] = range (0.12f, 0.4f, 0.0f, 0.7f);
        p["flt_env"] = range (0.45f, 0.85f, 0.25f, 1.0f);
        p["e1_d"] = range (200.0f, 520.0f, 90.0f, 1100.0f, true);
        p["e1_r"] = range (150.0f, 380.0f, 60.0f, 900.0f, true);
        p["e2_d"] = range (110.0f, 320.0f, 40.0f, 700.0f, true);
        p["penv_amt"] = chance (0.15f, 0.5f) ? range (2.0f, 8.0f, -12.0f, 24.0f) : 0.0f;
        p["penv_decay"] = range (10.0f, 30.0f, 5.0f, 90.0f);
        p["dist_on"] = chance (0.2f, 0.5f) ? 1.0f : 0.0f;
        p["dist_drive"] = range (0.15f, 0.35f, 0.1f, 0.7f);
        p["dist_mix"] = range (0.3f, 0.6f, 0.2f, 1.0f);
        p["cho_on"] = chance (0.3f, 0.5f) ? 1.0f : 0.0f;
        p["dly_on"] = chance (0.8f, 0.8f) ? 1.0f : 0.0f;
        p["dly_div"] = pick ({ 6, 5 }, { 3, 4, 5, 6, 8 });
        p["dly_fb"] = range (0.25f, 0.45f, 0.1f, 0.65f);
        p["dly_mix"] = range (0.16f, 0.28f, 0.08f, 0.4f);
        p["rev_mix"] = range (0.15f, 0.3f, 0.08f, 0.5f);
    }

    // ======================= drums / one-shots (v1.1) =======================
    // Every type is built only from the synth's own modules, so all the normal knobs keep shaping it.
    float pickf (std::initializer_list<float> s) { return *(s.begin() + juce::jmin ((int) s.size() - 1, (int) (rnd() * (float) s.size()))); }

    void kick (Patch& p)
    {
        p["a_wave"] = range (0.0f, 0.35f, 0.0f, 1.2f);                 // sine -> triangle for more knock
        p["a_oct"] = -1;
        p["a_semi"] = std::round (range (-9.0f, -2.0f, -12.0f, 3.0f));  // 41-58 Hz body
        p["a_level"] = 0.9f;
        p["sub_level"] = chance (0.3f, 0.5f) ? range (0.1f, 0.3f, 0.0f, 0.5f) : 0.0f;
        p["noise_level"] = range (0.15f, 0.45f, 0.0f, 0.8f);            // beater click
        p["noise_color"] = range (0.7f, 1.0f, 0.4f, 1.0f);
        p["noise_decay"] = range (4.0f, 14.0f, 2.0f, 40.0f, true);
        p["flt_type"] = 0;
        p["flt_cutoff"] = range (5000.0f, 14000.0f, 1500.0f, 20000.0f, true);
        p["flt_res"] = range (0.0f, 0.15f, 0.0f, 0.4f);
        p["flt_drive"] = range (0.1f, 0.4f, 0.0f, 0.8f);
        p["flt_env"] = range (0.0f, 0.3f, 0.0f, 0.6f);
        p["e2_d"] = range (30.0f, 90.0f, 15.0f, 250.0f, true);
        p["e1_d"] = range (260.0f, 650.0f, 150.0f, 1400.0f, true);
        p["e1_r"] = range (60.0f, 140.0f, 30.0f, 400.0f, true);
        p["penv_amt"] = range (24.0f, 40.0f, 12.0f, 48.0f);
        p["penv_decay"] = range (22.0f, 60.0f, 10.0f, 140.0f, true);
        p["dist_on"] = chance (0.6f, 0.85f) ? 1.0f : 0.0f;
        p["dist_type"] = pick ({ 0, 0, 1 }, { 0, 1, 2, 3 });
        p["dist_drive"] = range (0.15f, 0.45f, 0.05f, 0.85f);
        p["dist_tone"] = range (0.45f, 0.8f, 0.2f, 1.0f);
        p["dist_mix"] = range (0.6f, 1.0f, 0.3f, 1.0f);
    }

    void snare (Patch& p)
    {
        p["a_wave"] = range (0.0f, 1.0f, 0.0f, 2.5f);
        p["a_semi"] = std::round (range (4.0f, 11.0f, -2.0f, 14.0f));   // 165-245 Hz body
        p["a_level"] = range (0.7f, 0.95f, 0.4f, 1.0f);
        p["b_on"] = chance (0.35f, 0.6f) ? 1.0f : 0.0f;                  // second drum mode
        p["b_wave"] = range (0.0f, 1.0f, 0.0f, 3.0f);
        p["b_semi"] = std::round (range (7.0f, 10.0f, 3.0f, 12.0f)); p["b_fine"] = range (-40.0f, 40.0f, -100.0f, 100.0f);
        p["b_level"] = range (0.2f, 0.4f, 0.1f, 0.6f);
        p["noise_level"] = range (0.4f, 0.7f, 0.25f, 1.0f);
        p["noise_color"] = range (0.6f, 0.95f, 0.3f, 1.0f);
        p["noise_decay"] = range (160.0f, 360.0f, 70.0f, 650.0f, true);
        p["flt_type"] = pick ({ 0, 0, 4 }, { 0, 2, 4 });
        p["flt_cutoff"] = p["flt_type"] > 3 ? range (1800.0f, 3500.0f, 900.0f, 6000.0f, true) : range (5000.0f, 13000.0f, 2500.0f, 20000.0f, true);
        p["flt_res"] = range (0.05f, 0.25f, 0.0f, 0.5f);
        p["flt_drive"] = range (0.1f, 0.35f, 0.0f, 0.7f);
        p["e1_d"] = range (120.0f, 240.0f, 60.0f, 400.0f, true);
        p["e1_r"] = range (80.0f, 200.0f, 40.0f, 400.0f, true);
        p["penv_amt"] = range (5.0f, 14.0f, 0.0f, 24.0f);
        p["penv_decay"] = range (15.0f, 45.0f, 8.0f, 90.0f, true);
        p["dist_on"] = chance (0.45f, 0.8f) ? 1.0f : 0.0f;
        p["dist_type"] = pick ({ 0, 1 }, { 0, 1, 2, 3 });
        p["dist_drive"] = range (0.15f, 0.4f, 0.05f, 0.8f);
        p["dist_tone"] = range (0.55f, 0.9f, 0.3f, 1.0f);
        p["rev_on"] = chance (0.35f, 0.6f) ? 1.0f : 0.0f;
        p["rev_size"] = range (0.3f, 0.55f, 0.1f, 0.8f);
        p["rev_mix"] = range (0.08f, 0.18f, 0.05f, 0.3f);
    }

    void clap (Patch& p)
    {
        p["a_on"] = 0;
        p["noise_level"] = 1.0f;
        p["noise_color"] = range (0.65f, 1.0f, 0.4f, 1.0f);
        p["burst"] = pick ({ 2, 3, 3, 4 }, { 1, 2, 3, 4, 5, 6 });
        p["burst_gap"] = range (7.0f, 12.0f, 4.0f, 22.0f);
        p["flt_type"] = 4;
        p["flt_cutoff"] = range (900.0f, 1700.0f, 500.0f, 3500.0f, true);
        p["flt_res"] = range (0.15f, 0.4f, 0.0f, 0.65f);
        p["flt_env"] = range (0.0f, 0.2f, -0.2f, 0.5f);
        p["e2_d"] = range (40.0f, 120.0f, 20.0f, 300.0f, true);
        p["e1_d"] = range (120.0f, 260.0f, 70.0f, 500.0f, true);
        p["e1_r"] = range (100.0f, 220.0f, 50.0f, 400.0f, true);
        p["dist_on"] = chance (0.35f, 0.7f) ? 1.0f : 0.0f;
        p["dist_type"] = pick ({ 0, 1 }, { 0, 1, 3 });
        p["dist_drive"] = range (0.15f, 0.35f, 0.05f, 0.7f);
        p["rev_on"] = chance (0.7f, 0.8f) ? 1.0f : 0.0f;
        p["rev_size"] = range (0.3f, 0.6f, 0.15f, 0.85f);
        p["rev_mix"] = range (0.12f, 0.25f, 0.05f, 0.4f);
    }

    // kind 0 closed hat, 1 open hat, 2 crash
    void hat (Patch& p, int kind)
    {
        p["a_wave"] = range (2.8f, 3.2f, 1.5f, 5.5f);                   // square-ish metal partials
        p["a_oct"] = pick ({ 2, 3 }, { 1, 2, 3 });
        p["a_semi"] = std::round (range (0.0f, 7.0f, -12.0f, 12.0f));
        p["a_level"] = 0.8f;
        p["b_on"] = 1;
        p["b_wave"] = range (2.8f, 3.2f, 1.5f, 5.5f);
        p["b_oct"] = p["a_oct"];
        p["b_semi"] = std::round (range (4.0f, 11.0f, 1.0f, 12.0f));
        p["b_fine"] = range (-50.0f, 50.0f, -100.0f, 100.0f);          // inharmonic ratio
        p["b_level"] = 0.8f;
        p["ring"] = range (0.55f, 0.95f, 0.2f, 1.0f);
        p["noise_level"] = range (0.35f, 0.7f, 0.15f, 1.0f);
        p["noise_color"] = range (0.8f, 1.0f, 0.5f, 1.0f);
        p["flt_type"] = pick ({ 3, 3, 2 }, { 2, 3, 4 });
        p["flt_res"] = range (0.05f, 0.3f, 0.0f, 0.55f);
        p["vel_sens"] = 0.5f;
        if (kind == 0)
        {
            p["flt_cutoff"] = range (6500.0f, 10000.0f, 3500.0f, 13000.0f, true);
            p["e1_d"] = range (35.0f, 90.0f, 20.0f, 160.0f, true);
            p["e1_r"] = range (30.0f, 70.0f, 15.0f, 150.0f, true);
        }
        else if (kind == 1)
        {
            p["flt_cutoff"] = range (5500.0f, 9000.0f, 3000.0f, 12000.0f, true);
            p["e1_d"] = range (260.0f, 650.0f, 150.0f, 1100.0f, true);
            p["e1_r"] = range (150.0f, 350.0f, 80.0f, 600.0f, true);
        }
        else
        {
            p["noise_level"] = range (0.6f, 1.0f, 0.3f, 1.0f);
            p["flt_type"] = pick ({ 2, 3 }, { 2, 3, 4 });
            p["flt_cutoff"] = range (3500.0f, 6500.0f, 1800.0f, 9000.0f, true);
            p["flt_env"] = range (0.1f, 0.3f, 0.0f, 0.5f);
            p["e2_d"] = range (150.0f, 400.0f, 60.0f, 900.0f, true);
            p["e1_d"] = range (1400.0f, 3200.0f, 700.0f, 5000.0f, true);
            p["e1_r"] = range (500.0f, 1200.0f, 250.0f, 2500.0f, true);
            p["a_uni"] = pick ({ 3 }, { 1, 3, 5 }); p["a_detune"] = range (0.2f, 0.5f, 0.0f, 0.8f); p["a_width"] = 0.8f;
            p["cho_on"] = chance (0.4f, 0.6f) ? 1.0f : 0.0f; p["cho_mix"] = range (0.2f, 0.4f, 0.1f, 0.6f);
            p["rev_on"] = chance (0.5f, 0.7f) ? 1.0f : 0.0f; p["rev_size"] = range (0.5f, 0.8f, 0.3f, 0.95f); p["rev_mix"] = range (0.1f, 0.2f, 0.05f, 0.35f);
        }
        p["dist_on"] = chance (0.2f, 0.5f) ? 1.0f : 0.0f;
        p["dist_type"] = pick ({ 0, 1 }, { 0, 1, 2, 3 });
        p["dist_drive"] = range (0.1f, 0.3f, 0.05f, 0.6f);
        p["dist_tone"] = range (0.8f, 1.0f, 0.5f, 1.0f);
    }

    void perc (Patch& p)
    {
        const int kind = (int) pickf ({ 0, 1, 2, 3 });   // 0 rim/wood, 1 tom/bongo, 2 cowbell/bell, 3 zap blip
        p["a_wave"] = kind == 2 ? range (2.8f, 3.4f, 1.0f, 6.0f) : range (0.0f, 1.0f, 0.0f, 3.0f);
        p["a_oct"] = kind == 1 ? 0.0f : pickf ({ 1, 2 });
        p["a_semi"] = std::round (range (-5.0f, 7.0f, -12.0f, 12.0f));
        p["a_level"] = 0.85f;
        if (kind == 2 || chance (0.2f, 0.4f))
        {
            p["b_on"] = 1; p["b_wave"] = p["a_wave"]; p["b_oct"] = p["a_oct"];
            p["b_semi"] = pickf ({ 5, 7, 8, 11 }); p["b_fine"] = range (-30.0f, 30.0f, -80.0f, 80.0f); p["b_level"] = 0.7f;
            p["ring"] = kind == 2 ? range (0.2f, 0.6f, 0.0f, 1.0f) : range (0.0f, 0.4f, 0.0f, 1.0f);
        }
        p["noise_level"] = range (0.1f, 0.35f, 0.0f, 0.7f);
        p["noise_color"] = range (0.6f, 1.0f, 0.3f, 1.0f);
        p["noise_decay"] = range (5.0f, 25.0f, 2.0f, 80.0f, true);
        p["flt_type"] = pick ({ 0, 4, 1 }, { 0, 1, 2, 4 });
        p["flt_cutoff"] = range (2500.0f, 9000.0f, 800.0f, 18000.0f, true);
        p["flt_res"] = range (0.1f, 0.4f, 0.0f, 0.75f);
        p["flt_env"] = range (0.0f, 0.4f, -0.3f, 0.8f);
        p["e2_d"] = range (20.0f, 80.0f, 10.0f, 200.0f, true);
        p["e1_d"] = kind == 1 ? range (150.0f, 380.0f, 80.0f, 700.0f, true) : range (50.0f, 180.0f, 25.0f, 400.0f, true);
        p["e1_r"] = range (40.0f, 120.0f, 20.0f, 300.0f, true);
        p["penv_amt"] = kind == 3 ? range (18.0f, 36.0f, 8.0f, 48.0f) : range (2.0f, 12.0f, -12.0f, 24.0f);
        p["penv_decay"] = range (8.0f, 40.0f, 4.0f, 120.0f, true);
        p["dist_on"] = chance (0.3f, 0.6f) ? 1.0f : 0.0f;
        p["dist_type"] = pick ({ 0, 1 }, { 0, 1, 2, 3 });
        p["dist_drive"] = range (0.15f, 0.4f, 0.05f, 0.8f);
        p["dly_on"] = chance (0.15f, 0.4f) ? 1.0f : 0.0f; p["dly_div"] = pick ({ 5, 6 }, { 2, 4, 5, 6, 8 }); p["dly_mix"] = range (0.1f, 0.2f, 0.05f, 0.35f);
        p["rev_on"] = chance (0.35f, 0.6f) ? 1.0f : 0.0f; p["rev_size"] = range (0.4f, 0.7f, 0.2f, 0.9f); p["rev_mix"] = range (0.08f, 0.2f, 0.05f, 0.35f);
    }

    void snap (Patch& p)
    {
        p["a_wave"] = 0; p["a_oct"] = 2; p["a_semi"] = std::round (range (0.0f, 9.0f, -6.0f, 12.0f));
        p["a_level"] = range (0.15f, 0.35f, 0.0f, 0.6f);
        p["penv_amt"] = range (12.0f, 24.0f, 0.0f, 36.0f); p["penv_decay"] = range (4.0f, 12.0f, 2.0f, 30.0f);
        p["noise_level"] = 1.0f;
        p["noise_color"] = range (0.75f, 1.0f, 0.5f, 1.0f);
        p["burst"] = pick ({ 1, 1, 2 }, { 0, 1, 2, 3 });
        p["burst_gap"] = range (4.0f, 8.0f, 3.0f, 14.0f);
        p["flt_type"] = 4;
        p["flt_cutoff"] = range (1600.0f, 3200.0f, 900.0f, 5500.0f, true);
        p["flt_res"] = range (0.3f, 0.6f, 0.1f, 0.8f);
        p["e1_d"] = range (45.0f, 110.0f, 25.0f, 220.0f, true);
        p["e1_r"] = range (40.0f, 90.0f, 20.0f, 200.0f, true);
        p["dist_on"] = chance (0.2f, 0.5f) ? 1.0f : 0.0f; p["dist_drive"] = range (0.1f, 0.3f, 0.05f, 0.6f);
        p["rev_on"] = chance (0.75f, 0.8f) ? 1.0f : 0.0f;
        p["rev_size"] = range (0.4f, 0.75f, 0.2f, 0.95f);
        p["rev_mix"] = range (0.15f, 0.3f, 0.05f, 0.45f);
    }

    void subBass (Patch& p)
    {
        p["one_shot"] = 0;
        p["a_wave"] = range (0.0f, 0.15f, 0.0f, 0.8f);
        p["a_oct"] = -1;
        p["sub_level"] = range (0.15f, 0.4f, 0.0f, 0.6f);
        p["sub_shape"] = range (0.0f, 0.1f, 0.0f, 0.4f);
        p["flt_type"] = pick ({ 0, 1 }, { 0, 1 });
        p["flt_cutoff"] = range (350.0f, 1200.0f, 150.0f, 4000.0f, true);
        p["flt_res"] = range (0.0f, 0.15f, 0.0f, 0.4f);
        p["flt_drive"] = range (0.0f, 0.25f, 0.0f, 0.6f);
        p["flt_env"] = range (0.0f, 0.2f, 0.0f, 0.5f);
        p["e1_a"] = range (2.0f, 12.0f, 0.0f, 60.0f);
        p["e1_d"] = range (600.0f, 1500.0f, 300.0f, 3000.0f, true);
        p["e1_s"] = range (0.75f, 1.0f, 0.4f, 1.0f);
        p["e1_r"] = range (120.0f, 350.0f, 60.0f, 800.0f, true);
        p["e2_d"] = range (100.0f, 300.0f, 50.0f, 800.0f, true);
        p["penv_amt"] = range (0.0f, 3.0f, 0.0f, 12.0f); p["penv_decay"] = range (15.0f, 40.0f, 8.0f, 90.0f);
        p["voice_mode"] = 2;
        p["glide"] = range (40.0f, 120.0f, 15.0f, 300.0f);
        p["vel_sens"] = 0.2f;
        p["dist_on"] = chance (0.5f, 0.8f) ? 1.0f : 0.0f;
        p["dist_type"] = 0;
        p["dist_drive"] = range (0.05f, 0.25f, 0.0f, 0.55f);
        p["dist_tone"] = range (0.3f, 0.6f, 0.2f, 0.9f);
        p["dist_mix"] = range (0.4f, 0.8f, 0.2f, 1.0f);
        p["lfo_dest"] = 4; p["lfo_sync"] = 1; p["lfo_div"] = pick ({ 7, 9 }, { 4, 7, 9 });
        p["lfo_amt"] = chance (0.0f, 0.3f) ? range (0.2f, 0.5f, 0.1f, 0.8f) : 0.0f;   // optional wobble
    }

    void percLoop (Patch& p)
    {
        perc (p);
        p["seq_on"] = 1;
        p["seq_rate"] = pick ({ 1, 1, 1, 2 }, { 0, 1, 2, 3 });
        const float density = range (0.35f, 0.6f, 0.2f, 0.85f);
        int pat = 1;                                             // always hit the downbeat
        for (int i = 1; i < 16; ++i)
        {
            const float bias = (i % 4 == 0) ? 0.25f : ((i % 2 == 0) ? 0.08f : -0.05f);
            if (rnd() < density + bias) pat |= 1 << i;
        }
        p["seq_pat"] = (float) pat;
        p["seq_vary"] = range (0.3f, 0.7f, 0.1f, 1.0f);
        p["seq_swing"] = chance (0.4f, 0.5f) ? range (0.1f, 0.35f, 0.0f, 0.6f) : 0.0f;
        p["e1_d"] = range (40.0f, 150.0f, 20.0f, 300.0f, true);
        p["noise_level"] = range (0.2f, 0.6f, 0.05f, 1.0f);       // foley grit
        p["noise_decay"] = range (10.0f, 60.0f, 3.0f, 150.0f, true);
        p["flt_type"] = pick ({ 4, 0 }, { 0, 2, 4 });
        p["flt_cutoff"] = range (900.0f, 5000.0f, 400.0f, 12000.0f, true);
        p["flt_res"] = range (0.2f, 0.5f, 0.0f, 0.8f);
        p["dly_on"] = chance (0.35f, 0.6f) ? 1.0f : 0.0f;
    }

    void fx (Patch& p)
    {
        const int kind = (int) pickf ({ 0, 1, 2, 3 });   // 0 riser, 1 downlifter, 2 laser zap, 3 impact boom
        p["one_shot"] = 1;
        p["a_wave"] = range (1.5f, 3.0f, 0.0f, 7.0f);
        p["a_uni"] = pick ({ 3, 5 }, { 1, 3, 5, 7 }); p["a_detune"] = range (0.2f, 0.5f, 0.0f, 0.9f); p["a_width"] = 0.9f;
        p["noise_level"] = range (0.2f, 0.5f, 0.0f, 0.9f); p["noise_color"] = range (0.5f, 1.0f, 0.2f, 1.0f);
        p["flt_type"] = pick ({ 1, 4 }, { 0, 1, 3, 4 });
        p["flt_res"] = range (0.2f, 0.5f, 0.0f, 0.8f);
        p["dly_on"] = 1; p["dly_div"] = pick ({ 6, 8 }, { 4, 5, 6, 8, 9 }); p["dly_fb"] = range (0.3f, 0.55f, 0.15f, 0.75f); p["dly_mix"] = range (0.15f, 0.3f, 0.1f, 0.45f);
        p["rev_on"] = 1; p["rev_size"] = range (0.75f, 0.95f, 0.5f, 1.0f); p["rev_mix"] = range (0.25f, 0.45f, 0.15f, 0.6f);
        p["dist_on"] = chance (0.4f, 0.7f) ? 1.0f : 0.0f; p["dist_drive"] = range (0.15f, 0.4f, 0.05f, 0.8f);
        if (kind == 0)
        {   // riser: pitch climbs into the note, filter opens
            p["penv_amt"] = -range (12.0f, 24.0f, 6.0f, 24.0f); p["penv_decay"] = range (500.0f, 1000.0f, 250.0f, 1000.0f);
            p["e1_a"] = range (900.0f, 2200.0f, 400.0f, 4000.0f, true); p["e1_d"] = 400; p["e1_s"] = 0.0f; p["e1_r"] = 300;
            p["flt_cutoff"] = range (300.0f, 900.0f, 150.0f, 2000.0f, true); p["flt_env"] = range (0.5f, 0.9f, 0.3f, 1.0f);
            p["e2_a"] = p["e1_a"]; p["e2_d"] = 500; p["e2_s"] = 0.0f;
        }
        else if (kind == 1)
        {   // downlifter: starts high, falls, filter closes
            p["penv_amt"] = range (12.0f, 36.0f, 6.0f, 48.0f); p["penv_decay"] = range (500.0f, 1000.0f, 250.0f, 1000.0f);
            p["e1_d"] = range (1200.0f, 2600.0f, 600.0f, 4500.0f, true); p["e1_r"] = 600;
            p["flt_cutoff"] = range (600.0f, 2000.0f, 300.0f, 5000.0f, true); p["flt_env"] = range (0.4f, 0.8f, 0.2f, 1.0f);
            p["e2_d"] = range (600.0f, 1500.0f, 300.0f, 3000.0f, true);
        }
        else if (kind == 2)
        {   // laser / zap
            p["a_uni"] = 1; p["noise_level"] = range (0.0f, 0.15f, 0.0f, 0.4f);
            p["a_oct"] = pickf ({ 1, 2 });
            p["penv_amt"] = range (24.0f, 48.0f, 12.0f, 48.0f); p["penv_decay"] = range (60.0f, 220.0f, 25.0f, 500.0f, true);
            p["e1_d"] = range (250.0f, 700.0f, 120.0f, 1500.0f, true); p["e1_r"] = 200;
            p["flt_cutoff"] = range (2500.0f, 9000.0f, 1000.0f, 20000.0f, true); p["flt_env"] = range (0.0f, 0.3f, -0.3f, 0.6f);
            p["lfo_dest"] = 0; p["lfo_rate"] = range (8.0f, 18.0f, 3.0f, 20.0f); p["lfo_amt"] = chance (0.3f, 0.6f) ? range (0.1f, 0.3f, 0.05f, 0.5f) : 0.0f;
        }
        else
        {   // impact: low boom + noise blast into a big room
            p["a_wave"] = range (0.0f, 0.4f, 0.0f, 1.5f); p["a_uni"] = 1; p["a_oct"] = -1; p["a_semi"] = std::round (range (-7.0f, 0.0f, -12.0f, 5.0f));
            p["penv_amt"] = range (24.0f, 40.0f, 12.0f, 48.0f); p["penv_decay"] = range (80.0f, 250.0f, 40.0f, 500.0f, true);
            p["noise_decay"] = range (200.0f, 700.0f, 80.0f, 1500.0f, true); p["noise_level"] = range (0.3f, 0.7f, 0.1f, 1.0f);
            p["e1_d"] = range (900.0f, 2000.0f, 500.0f, 4000.0f, true); p["e1_r"] = 600;
            p["flt_type"] = 0; p["flt_cutoff"] = range (2000.0f, 7000.0f, 800.0f, 15000.0f, true); p["flt_res"] = 0.1f;
            p["dly_on"] = 0; p["dist_on"] = 1; p["dist_drive"] = range (0.25f, 0.5f, 0.1f, 0.85f);
        }
    }

    void vox (Patch& p)
    {
        const int kind = (int) pickf ({ 0, 0, 1, 2 });   // 0 "hey"/"ay" shout, 1 "oh"/"ah" chant, 2 sliding "ay-oh"
        p["one_shot"] = 1;
        p["vox_on"] = 1; p["vox_mix"] = range (0.85f, 1.0f, 0.6f, 1.0f);
        p["a_wave"] = range (1.8f, 2.4f, 1.0f, 4.0f);                  // saw-like glottal buzz
        p["a_uni"] = pick ({ 3, 5 }, { 1, 3, 5, 7 }); p["a_detune"] = range (0.08f, 0.22f, 0.0f, 0.45f); p["a_width"] = range (0.6f, 1.0f, 0.2f, 1.0f);
        p["b_on"] = chance (0.5f, 0.6f) ? 1.0f : 0.0f; p["b_wave"] = 2.0f; p["b_oct"] = pick ({ -1, 1 }, { -1, 0, 1 }); p["b_uni"] = 3; p["b_detune"] = 0.2f; p["b_level"] = range (0.2f, 0.4f, 0.1f, 0.6f);
        p["flt_type"] = 0; p["flt_cutoff"] = range (3500.0f, 8000.0f, 1800.0f, 14000.0f, true); p["flt_key"] = 0.0f;
        p["noise_level"] = range (0.25f, 0.5f, 0.1f, 0.8f); p["noise_color"] = range (0.6f, 0.9f, 0.4f, 1.0f);   // the "h" breath
        p["noise_decay"] = range (25.0f, 70.0f, 10.0f, 150.0f, true);
        p["lfo_dest"] = 0; p["lfo_shape"] = 0; p["lfo_rate"] = range (4.5f, 6.0f, 3.0f, 7.5f); p["lfo_amt"] = range (0.0f, 0.08f, 0.0f, 0.15f);
        p["cho_on"] = chance (0.6f, 0.7f) ? 1.0f : 0.0f; p["cho_mix"] = range (0.25f, 0.45f, 0.1f, 0.6f);
        p["dly_on"] = chance (0.5f, 0.7f) ? 1.0f : 0.0f; p["dly_div"] = pick ({ 6, 8 }, { 4, 5, 6, 8, 9 }); p["dly_mix"] = range (0.12f, 0.25f, 0.05f, 0.4f);
        p["rev_on"] = 1; p["rev_size"] = range (0.55f, 0.85f, 0.3f, 0.97f); p["rev_mix"] = range (0.18f, 0.35f, 0.1f, 0.5f);
        p["dist_on"] = chance (0.3f, 0.6f) ? 1.0f : 0.0f; p["dist_drive"] = range (0.1f, 0.3f, 0.05f, 0.6f); p["dist_mix"] = 0.5f;
        if (kind == 0)
        {   // "hey"/"ay": E -> A open, punchy, scoop up in pitch
            p["vox_vowel"] = range (0.9f, 1.3f, 0.5f, 2.0f); p["vox_env"] = -range (0.15f, 0.25f, 0.05f, 0.4f);
            p["e2_a"] = range (60.0f, 140.0f, 30.0f, 250.0f); p["e2_d"] = 400; p["e2_s"] = 1.0f;
            p["e1_a"] = range (4.0f, 15.0f, 1.0f, 30.0f); p["e1_d"] = range (220.0f, 420.0f, 150.0f, 700.0f, true); p["e1_s"] = 0.0f; p["e1_r"] = 150;
            p["penv_amt"] = -range (2.0f, 5.0f, 0.0f, 9.0f); p["penv_decay"] = range (50.0f, 120.0f, 25.0f, 250.0f);
        }
        else if (kind == 1)
        {   // "oh"/"ah" chant: held vowel
            p["vox_vowel"] = pickf ({ 0.0f, 3.0f, 3.2f, 0.3f }); p["vox_env"] = range (-0.05f, 0.05f, -0.2f, 0.2f);
            p["e1_a"] = range (15.0f, 50.0f, 5.0f, 120.0f); p["e1_d"] = range (500.0f, 900.0f, 300.0f, 1500.0f, true); p["e1_s"] = range (0.0f, 0.3f, 0.0f, 0.6f); p["e1_r"] = 300;
            p["noise_level"] = range (0.05f, 0.15f, 0.0f, 0.3f);
            p["lfo_amt"] = range (0.04f, 0.1f, 0.0f, 0.18f);
        }
        else
        {   // "ay-oh": vowel glides A -> O over the note
            p["vox_vowel"] = range (0.0f, 0.3f, 0.0f, 1.0f); p["vox_env"] = range (0.6f, 0.75f, 0.3f, 1.0f);
            p["e2_a"] = range (180.0f, 350.0f, 80.0f, 600.0f); p["e2_d"] = 400; p["e2_s"] = 1.0f;
            p["e1_a"] = range (8.0f, 25.0f, 2.0f, 60.0f); p["e1_d"] = range (500.0f, 900.0f, 300.0f, 1400.0f, true); p["e1_s"] = 0.0f; p["e1_r"] = 250;
            p["penv_amt"] = range (0.0f, 2.0f, -3.0f, 5.0f); p["penv_decay"] = 200.0f;
        }
    }
};
} // namespace xs
