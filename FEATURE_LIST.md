# Xoxicoa Synth v1.2.0: feature list (808 engine and light synths)

## Hard-hitting 808s
- **The 808 now plays at its real pitch.** v1.1 played the 808 an octave under the note, so A1 came out at 26-27 Hz, which is subsonic.
  Now A1 = 55 Hz, and the default export range E1..D#2 gives 41-78 Hz fundamentals.
- **Clean sine body** with Punch and Click (new **808 / WIDTH** panel):
  - **Punch**: on every new hit, a fast pitch drop (up to +24 st, 12-42 ms) plus up to +5.4 dB on the attack.
    Legato slides never re-punch.
  - **Click**: a kick-style knock layer (3.6 kHz > 150 Hz sweep plus a noise tick). It gets longer and harder as the knob goes up.
  - **Low Keep**: the band under 120 Hz is replaced with the clean sub, up to +2.6 dB. Drive then adds harmonics you can hear on a phone without muddying or eating the sub.
  - **Width**: mid/side, from 0 (mono, the 808 default) to 2 (extra wide, for light synths).
- **Two new distortion types** (oversampled 2x):
  - **Tube**: biased, with warm even harmonics. The 2nd harmonic is what laptop and phone speakers actually play.
  - **Rage**: tube into a hard clipper, giving dense odd and even harmonics.
  - The existing types are still there: Soft, Hard, Fold and Mosh.
- **Glide and length:**
  - Legato glide, from 70 ms up to 450-700 ms for Spinz.
  - Bend range of 12 st on Spinz.
  - Long decay/sustain/release up to 6-8 s on Long Boom. The export length follows the decay.
- **Gain staging:**
  - 808 rolls are loudness-matched to about -8 dB RMS.
  - The peak may hit the 1.5 ms look-ahead limiter by up to about 2-3 dB on the click only. The sub body is not limited, and no samples reach 0 dBFS (safety test).
- **New 808 sound types** in the GENERATE picker:
  - 808 Clean Sub
  - 808 Distorted Rage
  - 808 Punchy Pluggnb
  - 808 Spinz Glide
  - 808 Long Boom
  - The classic "808" type now rolls a random one of these styles.
- **New 808 factory presets (10):**
  - Pure Sine Sub, Deep Sine Glide
  - Distorted Rage, Spaz Mosh 808
  - Punchy Pluggnb, Plugg Knock
  - Spinz Glide, Slide Sauce
  - Long Boom, Stadium Boom
  - The 8 older 808 presets were moved onto the new engine as well.

## Light synths (same sound-type picker and GENERATE, no separate tab)
- **Soft Bell, Soft Pluck, Airy Pad, Flute / Whistle, Pluggnb Keys, Music Box (celesta), Glass Lead.**
- **Shared light template:**
  - Gentle low-pass with low resonance and no filter drive.
  - Distortion off, or a touch of Soft/Tube at most.
  - Chorus, Width 1.15-1.9, a tempo delay and a big soft reverb.
  - Soft attacks on pads and flutes.
- **LFO Fade In** (new, 0-3 s): delayed vibrato on each note for flutes, whistles and glass leads.
- **14 light factory presets:** Soft Bell, Cotton Bell, Soft Pluck, Feather Pluck, Airy Pad, Cloud Haze, Breath Flute, Night Whistle, Pluggnb Keys, Velvet Keys, Music Box, Celesta, Glass Lead, Crystal Line.
- **Loudness and export:**
  - Light types are loudness-matched where they are played (probed around C4, not C2).
  - They export to `~/Music/Xoxicoa Kit Gen/Light Synth One Shots/`. The new 808 types export to `808s/`.

## UI
- The window is still 1240 x 800, black and white, with the same style.
- **New 808 / WIDTH panel** (Punch, Click, Low Keep, Width). The scope is narrower so the panel fits.
- **Fade In knob** in the LFO panel.
- **Grouped type menu:** Synths / 808s and Bass / Light Synths / Drums / Loops, FX and Vox. Long names shrink to fit the selector.
- **Distortion selector** has Tube and Rage.

## Compatibility
- New parameters are appended at the end, so automation and saved projects from v1.1 keep their indexes.
- The macOS deployment target is still 10.15.
