#pragma once
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <vector>
#include <cmath>

namespace xs
{
// Band-limited wavetable bank: 8 morphable frames, each stored at 21 half-octave
// "mip" levels so nothing aliases from sub-bass to the top of the keyboard.
class WavetableBank
{
public:
    static constexpr int kSize = 2048, kFrames = 8, kLevels = 21;

    static const WavetableBank& get()
    {
        static const WavetableBank bank;
        return bank;
    }

    // Brightest level whose top harmonic stays under Nyquist for this frequency.
    int levelFor (float freqHz, float sampleRate) const noexcept
    {
        const float limit = 0.48f * sampleRate;
        for (int k = 0; k < kLevels; ++k)
            if ((float) harmonics[(size_t) k] * freqHz < limit)
                return k;
        return kLevels - 1;
    }

    // phase in [0,1). position in [0,7] morphs between frames.
    inline float read (float position, int level, float phase) const noexcept
    {
        const int f0 = juce::jlimit (0, kFrames - 1, (int) position);
        const int f1 = juce::jmin (f0 + 1, kFrames - 1);
        const float fr = juce::jlimit (0.0f, 1.0f, position - (float) f0);
        const float idx = phase * (float) kSize;
        const int i0 = juce::jmin ((int) idx, kSize - 1);
        const float t = idx - (float) i0;
        const float* a = table (f0, level) + i0;
        const float va = a[0] + t * (a[1] - a[0]);
        if (fr <= 0.0f || f0 == f1) return va;
        const float* b = table (f1, level) + i0;
        const float vb = b[0] + t * (b[1] - b[0]);
        return va + fr * (vb - va);
    }

    inline float readFrame (int frame, int level, float phase) const noexcept
    {
        const float idx = phase * (float) kSize;
        const int i0 = juce::jmin ((int) idx, kSize - 1);
        const float t = idx - (float) i0;
        const float* a = table (frame, level) + i0;
        return a[0] + t * (a[1] - a[0]);
    }

private:
    std::vector<float> data;           // [frame][level][kSize + 1]
    std::array<int, kLevels> harmonics {};

    const float* table (int frame, int level) const noexcept
    {
        return data.data() + ((size_t) frame * kLevels + (size_t) level) * (kSize + 1);
    }

    // Amplitude of harmonic n for each frame (sine terms, cosine terms).
    static void spectrum (int frame, int n, double& s, double& c)
    {
        const double pi = juce::MathConstants<double>::pi;
        s = c = 0.0;
        switch (frame)
        {
            case 0: s = n == 1 ? 1.0 : 0.0; break;                                             // sine
            case 1: if (n % 2) s = (8.0 / (pi * pi)) * (((n - 1) / 2) % 2 ? -1.0 : 1.0) / (n * (double) n); break; // triangle
            case 2: s = (2.0 / pi) * (n % 2 ? 1.0 : -1.0) / n; break;                          // saw
            case 3: if (n % 2) s = (4.0 / pi) / n; break;                                      // square
            case 4: c = (2.0 / (n * pi)) * std::sin (pi * n * 0.25); break;                   // 25% pulse
            case 5: c = (2.0 / (n * pi)) * std::sin (pi * n * 0.10); break;                   // 10% pulse
            case 6:                                                                            // hollow: square + formant bump
                if (n % 2) s = (1.0 / n) * (1.0 + 4.0 * std::exp (-std::pow ((n - 9.0) / 3.5, 2.0)));
                else       s = 0.25 * (1.0 / n) * std::exp (-std::pow ((n - 6.0) / 2.0, 2.0));
                break;
            case 7:                                                                            // glass / bell partials
            {
                static const int   hs[] { 1, 2, 3, 5, 7, 9, 12, 16, 21 };
                static const float as[] { 1.0f, 0.35f, 0.15f, 0.45f, 0.32f, 0.12f, 0.22f, 0.12f, 0.08f };
                for (int i = 0; i < 9; ++i) if (hs[i] == n) s = as[i];
                break;
            }
            default: break;
        }
    }

    WavetableBank()
    {
        data.assign ((size_t) kFrames * kLevels * (kSize + 1), 0.0f);
        for (int k = 0; k < kLevels; ++k)
            harmonics[(size_t) k] = juce::jmax (1, (int) std::floor (1024.0 * std::pow (2.0, -0.5 * k)));

        juce::dsp::FFT fft (11); // 2048
        std::vector<float> buf (kSize * 2);
        for (int f = 0; f < kFrames; ++f)
        {
            float norm = 1.0f;
            for (int k = 0; k < kLevels; ++k)
            {
                std::fill (buf.begin(), buf.end(), 0.0f);
                auto* cpx = reinterpret_cast<std::complex<float>*> (buf.data());
                for (int n = 1; n <= harmonics[(size_t) k] && n < kSize / 2; ++n)
                {
                    double s, c;
                    spectrum (f, n, s, c);
                    // x[t] = sum c*cos + s*sin  ->  X[n] = (c - i s) * N/2 (inverse is unscaled in JUCE)
                    cpx[n] = { (float) (c * 0.5), (float) (-s * 0.5) };
                }
                fft.performRealOnlyInverseTransform (buf.data());
                float* dst = data.data() + ((size_t) f * kLevels + (size_t) k) * (kSize + 1);
                // JUCE's inverse divides by N; undo that.
                for (int i = 0; i < kSize; ++i) dst[i] = buf[(size_t) i] * (float) kSize;
                if (k == 0)
                {
                    float peak = 0.0f;
                    for (int i = 0; i < kSize; ++i) peak = juce::jmax (peak, std::abs (dst[i]));
                    norm = peak > 0.0f ? 0.95f / peak : 1.0f;
                }
                // DC removal + normalisation (same gain for every level of a frame)
                double dc = 0.0;
                for (int i = 0; i < kSize; ++i) dc += dst[i];
                dc /= kSize;
                for (int i = 0; i < kSize; ++i) dst[i] = (float) ((dst[i] - dc) * norm);
                dst[kSize] = dst[0];
            }
        }
    }
};
} // namespace xs
