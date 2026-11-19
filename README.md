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
- **808 / Width (v1.2)**: Punch (fast pitch drop of up to +24 st plus up to +4 dB of attack on every new hit, never on a legato slide),
  Click (a kick-style click / knock layer: a 3.6 kHz > 150 Hz sweep plus a noise tick), Low Keep (clean sub weight: a pure sine at the note's fundamental, following glides, pitch env and punch, blended
  under the sound while the distorted / FX signal is turned down so the sum keeps its headroom) and Width (mid/side,
  0 = mono to 2 = extra wide).
- **LFO**: Sine / Tri / Saw / Square / S&H. Free rate or tempo sync (4 bars to 1/32, triplets and dotted).
  Destinations: Pitch, Cutoff, Wave A, Wave B, Volume, Pan, Vowel. Fade In (v1.2, 0-3 s) delays the LFO on every new note (flute / lead vibrato).
- **Voice**: Poly (16 voices) / Mono / Legato, Glide, Bend range and Velocity sensitivity.
  Legato with overlapping notes gives FL-style 808 slides with no retrigger.
- **FX**:
  - Distortion: Soft / Hard / Fold / Mosh / Tube / Rage (v1.2: Tube = biased warm even harmonics, Rage = tube into a hard clipper), oversampled 2x, with Drive, Tone and Mix.
  - Chorus.
  - Tempo-synced ping-pong Delay. Tone darkens the echoes on the left half of the knob and thins them on the right half.
  - Reverb with Size, Damp, Width and Mix.
- **Master**: volume plus a 1.5 ms look-ahead limiter with a -0.5 dBFS ceiling.

- **Drum section (v1.1)**: Noise Decay (gives the noise its own envelope: kick click, snare wires, hat sizzle),
  Ring (Osc A x Osc B ring mod for metallic hats, crashes, bells), Claps (0-6 pre-hits) + Clap Gap,
  Boost (0-24 dB pre-FX gain that also drives the distortion), One Shot (ignore note-off so hits ring out).
- **Vox Formant (v1.1)**: 3-band vowel filter (A-E-I-O-U morph), Vowel Env (Mod Env moves the vowel: "hey", "ay-oh"), Mix.
- **Loop (v1.1)**: hold a note and a 16-step pattern retriggers the sound in tempo: clickable step grid,
  Rate (1/8, 1/16, 1/16T, 1/32), Vary (per-step pitch + accent variation) and Swing.

## Generator
Pick a sound type, set **Wild**, then hit **GENERATE**. Types: Rage Lead, Plugg Bell, 808, Dark Pad, Pluck,
Kick, Snare, Clap, Hi Hat, Open Hat, Crash, Perc, Snap, Sub Bass, Perc Loop, FX, Vox Chant, and (v1.2)
808 Clean Sub, 808 Distorted Rage, 808 Punchy Pluggnb, 808 Spinz Glide, 808 Long Boom, Soft Bell, Soft Pluck,
Airy Pad, Flute / Whistle, Pluggnb Keys, Music Box and Glass Lead. The type menu is grouped
(Synths / 808s and Bass / Light Synths / Drums / Loops, FX and Vox).
Drums, loops, FX and vox chants are built from the synth's own oscillators, noise, filter, envelopes and FX
(plus the v1.1 drum / vox / loop sections), so every normal knob keeps shaping a generated sound and
MIDI plays it chromatically. One-shots are peak-normalised to -1 dBFS before the limiter.
Every roll starts from a hand-tuned category template and randomises inside musical "safe" ranges.
Wild widens those ranges toward hand-set "wild" limits. Each roll is then rendered offline in about 50 ms
and loudness-matched, so rolls land at a consistent level.

## Export (v1.1)
- **PLAY** (or click the waveform) auditions the sound at the export **Note**.
- **SAVE WAV** writes a 24-bit 44.1 kHz WAV to `~/Music/Xoxicoa Kit Gen/<Folder>/`, e.g. `Xoxicoa 808 07 (F#).wav`,
  `Xoxicoa Kick 03.wav`, `Xoxicoa Perc Loop 02 150 BPM.wav`. Folders: 808s, Kicks, Snares, Claps, Hi Hats, Open Hats,
  Crashes, Percs, Snaps, Sub Basses, Foley and Percussion Loops, FX, Vox Chants, Synth One Shots.
- **GEN 10** generates and saves 10 new variations of the selected type.
- **Drag** the waveform into FL Studio (or Finder) to drop the WAV.
- **BPM** sets the export tempo for loops (exactly 16 steps, tail wrapped to the start so it loops seamlessly).

## Presets
There are 40 factory synth presets, 8 per category, all loudness-matched. User presets save and load as `.xoxpreset`
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
  - `XoxicoaSynthKitGenTest examples <dir> <report> | knobs <report> | shot <png> <type> | export <type> <n>`

  Latest reports are in `tests/results/`.

## Licence note
JUCE 8 is AGPLv3 or commercial. Sort out a JUCE licence before you sell or distribute closed-source builds.
