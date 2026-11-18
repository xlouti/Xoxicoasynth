# Xoxicoa Synth

A polyphonic wavetable / virtual-analog synth with a built-in sound **generator**, made for rage / mosh
(150-170 BPM) and pluggnb (140-160 BPM). Formats: VST3, AU (instrument) and Standalone. It is a universal
macOS binary for Apple Silicon and Intel, built with JUCE 8.0.4 and CMake.

## Engine
- **Osc A / Osc B**: 8-frame morphable wavetable (Sine > Triangle > Saw > Square > Pulse > Thin Pulse > Hollow > Glass),
  band-limited at 21 half-octave levels so it doesn't alias. Each has Octave, Semi, Fine, Level, Unison (1-7),
  Detune and Stereo Width.
- **Sub** (-1 / -2 oct, sine to square shape) and **Noise** (colour from dark to white).
- **Filter**: LP12 / LP24 / HP12 / HP24 / BP with zero-delay-feedback SVF, Cutoff, Resonance, Drive, Env amount (bipolar) and Key tracking.
- **Amp ADSR** and **Mod ADSR** (routed to the filter). Click-free: attack starts from the current level, with a 1 ms minimum attack and a 4 ms minimum release.
- **Pitch envelope** (-24 to +48 st, 1-1000 ms decay) for 808 punch and knock.
- **LFO**: Sine / Tri / Saw / Square / S&H. Free rate or tempo sync (4 bars to 1/32, triplets and dotted).
  Destinations: Pitch, Cutoff, Wave A, Wave B, Volume, Pan.
- **Voice**: Poly (16 voices) / Mono / Legato, Glide, Bend range and Velocity sensitivity.
  Legato with overlapping notes gives FL-style 808 slides with no retrigger.
- **FX**:
  - Distortion: Soft / Hard / Fold / Mosh, oversampled 2x, with Drive, Tone and Mix.
  - Chorus.
  - Tempo-synced ping-pong Delay. Tone darkens the echoes on the left half of the knob and thins them on the right half.
  - Reverb with Size, Damp, Width and Mix.
- **Master**: volume plus a 1.5 ms look-ahead limiter with a -0.5 dBFS ceiling.

## Generator
Pick a category (Rage Lead, Plugg Bell, 808, Dark Pad, Pluck), set **Wild**, then hit **GENERATE**.
Every roll starts from a hand-tuned category template and randomises inside musical "safe" ranges.
Wild widens those ranges toward hand-set "wild" limits. Each roll is then rendered offline in about 50 ms
and loudness-matched, so rolls land at a consistent level.

## Presets
There are 40 factory presets, 8 per category, all loudness-matched. User presets save and load as `.xoxpreset`
files in `~/Music/Xoxicoa Synth/Presets`. Click the preset name to browse by category, or use < and > to step through.

## Controls
- Drag up or right to turn a knob up. Fast drags move further.
- Hold Shift for fine control.
- Double-click (or Alt-click) to reset to default.
- The mouse wheel works on knobs and selectors.
- Hovering shows the value.

## Build
- **Mac (CI)**: run the GitHub workflow `.github/workflows/build-mac.yml`, which uses `macos-15-intel` and Xcode 16.4 to produce a universal binary with a macOS 10.15 deployment target (Catalina).
- **Local**: `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build --config Release`
- **Tests**: configure with `-DXS_BUILD_TESTS=ON`, then run:
  - `XoxicoaSynthAudit audit|safety|demos|calibrate <path>`
  - `XoxicoaSynthUISnapshot out.png`

  Latest reports are in `tests/results/`.

## Licence note
JUCE 8 is AGPLv3 or commercial. Sort out a JUCE licence before you sell or distribute closed-source builds.
