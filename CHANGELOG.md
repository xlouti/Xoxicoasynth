# Changelog

## 1.2.0
- 808s play at the real note pitch (v1.1 played them an octave low, at 20-39 Hz).
- New Punch, Click, Low Keep (clean sine sub weight that follows glides and the pitch envelope) and Width controls (808 / WIDTH panel), plus an LFO Fade In knob.
- New distortion types: Tube and Rage.
- New generator types:
  - 808 Clean Sub, 808 Distorted Rage, 808 Punchy Pluggnb, 808 Spinz Glide, 808 Long Boom.
  - Soft Bell, Soft Pluck, Airy Pad, Flute / Whistle, Pluggnb Keys, Music Box, Glass Lead.
- 24 new factory presets; the existing 808 presets moved onto the new engine. Factory levels were recalibrated.
- Grouped sound-type menu.
- Tests:
  - New `XoxicoaSynthMeasure` tool (808 metrics plus demo patterns).
  - The parameter audit gained contexts for the drum, vox and loop knobs, and now checks 3 loop patterns instead of all 65536.
  - The knob audit covers the new types, and renders of slightly different lengths no longer read as a fake 35 dB side-channel change.

## 1.1.0
- Drum, vox and loop sound types, export, and the Catalina weak-link fix.
