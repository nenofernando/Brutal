// NF Brutal - DSP core (header only, no JUCE dependency so it can be used by the offline test tool).
// Structure (measured from the reference behaviour, see ~/NF-GRD-Match/FINDINGS.md):
//   dry -> Input gain -> Body (low shelf cut) -> peak limiter (soft-ratio, threshold/knee from Slay)
//       -> make-up Gain -> odd-harmonic saturator -> Lofi band-pass (optional) -> Mix with dry
#pragma once
#include <cmath>
#include <algorithm>
#include <cstring>

namespace brutal
{
constexpr double kPi = 3.14159265358979323846;

// ---- parameter mapping (normalised 0..1 <-> real units) -------------------------------------------------
struct Real { float inputDb = 0.f, gainDb = 27.1f, slay = 1.f, bodyDb = -3.5f, mix = 1.f; bool lofi = false, on = true; };

inline float lerpTab(const float* xs, const float* ys, int n, float x)
{
    if (x <= xs[0]) return ys[0];
    for (int i = 1; i < n; ++i)
        if (x <= xs[i]) { const float t = (x - xs[i - 1]) / (xs[i] - xs[i - 1]); return ys[i - 1] + t * (ys[i] - ys[i - 1]); }
    return ys[n - 1];
}
// Gain knob taper (normalised -> dB), measured at 3 kHz, small signal
inline float gainNormToDb(float n)
{
    static const float xs[] = {0, .05f, .1f, .15f, .2f, .25f, .3f, .35f, .4f, .45f, .5f, .55f, .6f, .65f, .7f, .75f, .8f, .85f, .9f, .95f, 1.f};
    static const float ys[] = {0, .9f, 2.0f, 3.4f, 4.8f, 6.4f, 8.0f, 9.7f, 11.5f, 13.3f, 15.1f, 17.1f, 19.0f, 21.0f, 23.1f, 25.1f, 27.2f, 29.4f, 31.6f, 33.8f, 36.f};
    return lerpTab(xs, ys, 21, n);
}
inline float gainDbToNorm(float d)
{
    static const float xs[] = {0, .05f, .1f, .15f, .2f, .25f, .3f, .35f, .4f, .45f, .5f, .55f, .6f, .65f, .7f, .75f, .8f, .85f, .9f, .95f, 1.f};
    static const float ys[] = {0, .9f, 2.0f, 3.4f, 4.8f, 6.4f, 8.0f, 9.7f, 11.5f, 13.3f, 15.1f, 17.1f, 19.0f, 21.0f, 23.1f, 25.1f, 27.2f, 29.4f, 31.6f, 33.8f, 36.f};
    return lerpTab(ys, xs, 21, d);
}
inline float inputNormToDb(float n) { return -96.f + 120.f * n; }
inline float bodyNormToDb(float n) { return -10.f + 10.f * n; }

// Slay tables (fit of the static curves, s = 0..1 in steps of 0.1): make-up gain ramp, limiter threshold (dBFS, pre-gain) and ln(knee exponent)
struct SlayShape { float geffDb, pDb, k; };
struct SlayTables
{
    // knots every 0.05 of Slay (21 values); make-up ramp is -3 dB at 0 and rises 69.3 dB per unit until it reaches the Gain knob
    float geff[21] = {-3.04f, 0.42f, 3.89f, 7.35f, 10.82f, 14.29f, 17.75f, 21.21f, 24.68f, 28.14f, 31.61f, 35.07f, 38.54f, 42.01f, 45.47f, 48.94f, 52.4f, 55.86f, 59.33f, 62.8f, 66.26f};
    float pdb[21] = {34.f, 25.35f, 1.05f, -7.13f, -13.49f, -17.19f, -21.09f, -24.64f, -28.54f, -30.92f, -31.25f, -31.15f, -30.91f, -30.63f, -30.38f, -30.32f, -30.28f, -30.24f, -30.22f, -30.21f, -30.2f};
    float lk[21] = {0.206f, -0.823f, -0.403f, -0.232f, 0.015f, 0.02f, 0.053f, 0.072f, 0.133f, 0.262f, 0.778f, 1.346f, 1.86f, 2.226f, 2.281f, 2.463f, 2.866f, 3.116f, 3.538f, 4.032f, 4.382f};
};
inline SlayShape slayShape(const SlayTables& t, float s, float gainDb)
{
    static float xs[21]; static bool init = false;
    if (!init) { for (int i = 0; i < 21; ++i) xs[i] = (float) i * 0.05f; init = true; }
    SlayShape r;
    r.geffDb = std::min(gainDb, lerpTab(xs, t.geff, 21, s));
    r.pDb = lerpTab(xs, t.pdb, 21, s);
    r.k = std::exp(lerpTab(xs, t.lk, 21, s));
    return r;
}

// ---- small filters ---------------------------------------------------------------------------------------
struct OnePole
{
    float a = 0.f, z = 0.f;
    void setFc(double fc, double sr) { a = (float) (1.0 - std::exp(-2.0 * kPi * fc / sr)); }
    float lp(float x) { z += a * (x - z); return z; }
    void reset() { z = 0.f; }
};
struct Biquad
{
    float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
    void setLowpass(double fc, double q, double sr)
    {
        const double w = 2 * kPi * fc / sr, c = std::cos(w), al = std::sin(w) / (2 * q), a0 = 1 + al;
        b0 = (float) ((1 - c) / 2 / a0); b1 = (float) ((1 - c) / a0); b2 = b0; a1 = (float) (-2 * c / a0); a2 = (float) ((1 - al) / a0);
    }
    // analog biquad (b2 s^2 + b1 s + b0) / (a2 s^2 + a1 s + a0), bilinear transform without pre-warp
    void setAnalog(double nb2, double nb1, double nb0, double da2, double da1, double da0, double sr)
    {
        const double k = 2.0 * sr, k2 = k * k;
        const double B0 = nb2 * k2 + nb1 * k + nb0, B1 = 2.0 * nb0 - 2.0 * nb2 * k2, B2 = nb2 * k2 - nb1 * k + nb0;
        const double A0 = da2 * k2 + da1 * k + da0, A1 = 2.0 * da0 - 2.0 * da2 * k2, A2 = da2 * k2 - da1 * k + da0;
        b0 = (float) (B0 / A0); b1 = (float) (B1 / A0); b2 = (float) (B2 / A0); a1 = (float) (A1 / A0); a2 = (float) (A2 / A0);
    }
    float process(float x) { const float y = b0 * x + z1; z1 = b1 * x - a1 * y + z2; z2 = b2 * x - a2 * y; return y; }
    void reset() { z1 = z2 = 0.f; }
};

// ---- processor ---------------------------------------------------------------------------------------------
class Engine
{
public:
    void prepare(double sampleRate)
    {
        sr = sampleRate;
        for (int c = 0; c < 2; ++c) { body[c].setFc(bodyFc, sr); det[c].setFc(detFc, sr);
            // Lofi: 4th-order response fitted (magnitude and phase) to the measured reference, two analog biquads mapped with the bilinear transform
            lofiA[c].setAnalog(0.04525249468, 8859.233511, 4.602520233e8, 1.0, 26117.96787, 4.906903912e8, sr);
            lofiB[c].setAnalog(1.0, 13644.60101, -1659.606917, 1.0, 12763.37816, 3.747137054e7, sr); }
        atk = (float) (1.0 - std::exp(-1.0 / (attackMs * 0.001 * sr)));
        relPerSample = (float) (relDbPerSec / sr); holdSamples = (int) (holdMs * 0.001 * sr);
        smoothA = (float) (1.0 - std::exp(-1.0 / (0.02 * sr)));
        reset(); first = true;
    }
    void reset()
    {
        for (int c = 0; c < 2; ++c) { body[c].reset(); det[c].reset(); lofiA[c].reset(); lofiB[c].reset(); }
        grDb = 0.f; holdLeft = 0;
    }
    void setParams(const Real& r) { target = r; if (first) { cur = r; first = false; } }
    float currentGrDb() const { return grDb; }

    // in-place, stereo (or mono if R == nullptr)
    void process(float* L, float* R, int n)
    {
        for (int i = 0; i < n; ++i)
        {
            smooth();
            const float dryL = L[i], dryR = R ? R[i] : L[i];
            if (!cur.on) continue;
            const float inG = std::pow(10.f, cur.inputDb / 20.f);
            const float k = bodyK;
            float x[2] = {dryL * inG, dryR * inG};
            if (cur.lofi) for (int c = 0; c < (R ? 2 : 1); ++c) x[c] = lofiB[c].process(lofiA[c].process(x[c]));   // Lofi band-pass sits BEFORE the limiter
            if (bodyPos == 0) for (int c = 0; c < (R ? 2 : 1); ++c) x[c] = x[c] + (k - 1.f) * body[c].lp(x[c]);   // low shelf (pre limiter)
            float d[2] = {x[0], x[1]};                                                     // detector: fixed low-shelf cut (-4.2 dB, 164 Hz)
            for (int c = 0; c < (R ? 2 : 1); ++c) d[c] = d[c] + (detK - 1.f) * det[c].lp(d[c]);
            const float p = std::max(std::fabs(d[0]), R ? std::fabs(d[1]) : 0.f);
            // gain computer (soft ratio limiter)
            float gt = 1.f;
            if (p > 1e-9f)
            {
                const float e = sh.k * std::log(p / pLin);                          // soft limiter: lim = p / (1+(p/P)^k)^(1/k)
                const float sp = e > 30.f ? e : std::log1p(std::exp(e));            // softplus, stable for large k
                gt = std::min(1.f, std::exp(-sp / sh.k));
            }
            const float tgtGr = -20.f * std::log10(std::max(gt, 1e-6f));
            if (tgtGr > grDb) { grDb += (tgtGr - grDb) * atk; holdLeft = holdSamples; }
            else if (tgtGr >= grDb - 0.05f) holdLeft = holdSamples;            // peak refreshes the hold
            else if (holdLeft > 0) --holdLeft;                                  // hold, then linear-in-dB release
            else grDb = std::max(tgtGr, grDb - relPerSample);
            const float lg = std::pow(10.f, (sh.geffDb - grDb) / 20.f);
            float w[2];
            for (int c = 0; c < (R ? 2 : 1); ++c)
            {
                float u = x[c] * lg;
                if (bodyPos == 1) u = u + (k - 1.f) * body[c].lp(u);
                float y = u / std::pow(1.f + std::pow(std::fabs(u) / satA, satN), 1.f / satN);
                if (bodyPos == 2) y = y + (k - 1.f) * body[c].lp(y);
                
                w[c] = y;
            }
            const float m = cur.mix;
            L[i] = dryL * (1.f - m) + w[0] * m;
            if (R) R[i] = dryR * (1.f - m) + w[1] * m;
        }
    }

    double bodyFc = 305.0, detFc = 164.0; float satA = 1.07318f, satN = 2.87328f, detGainDb = -4.2f, pOffsetDb = 0.4f; float detK = 0.617f;
    SlayTables slayT;
    float attackMs = 0.10f, relDbPerSec = 298.f, holdMs = 13.8f; int bodyPos = 1;
private:
    
    void smooth()
    {
        auto s = [&](float& a, float b) { a += (b - a) * smoothA; };
        s(cur.inputDb, target.inputDb); s(cur.gainDb, target.gainDb); s(cur.slay, target.slay); s(cur.bodyDb, target.bodyDb); s(cur.mix, target.mix);
        cur.lofi = target.lofi; cur.on = target.on;
        sh = slayShape(slayT, cur.slay, cur.gainDb); pLin = std::pow(10.f, (sh.pDb + pOffsetDb) / 20.f); detK = std::pow(10.f, detGainDb / 20.f);
        bodyK = std::pow(10.f, cur.bodyDb / 20.f);
        { const float a = (float) (1.0 - std::exp(-2.0 * kPi * bodyFc / std::sqrt((double) bodyK) / sr)); body[0].a = a; body[1].a = a; }
    }
    double sr = 48000.0; Real cur, target; bool first = true;
    SlayShape sh{}; int holdLeft = 0, holdSamples = 600; float pLin = 0.03f, bodyK = 0.668f, grDb = 0.f, atk = 0.1f, relPerSample = 0.005f, smoothA = 0.001f;
    OnePole body[2], det[2]; Biquad lofiA[2], lofiB[2];
};
} // namespace brutal
