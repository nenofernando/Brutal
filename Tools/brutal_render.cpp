// Offline renderer: porrada_render in.wav out.wav [idx=norm ...]   (same indexes as the reference host: 0 slay,1 input,2 gain,3 body,4 mix,6 on,7 lofi)
#include "../Source/Shared/BrutalDSP.h"
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <string>
#include <cstdint>
static bool readWav(const char* p, std::vector<float>& L, std::vector<float>& R, int& sr)
{
    FILE* f = fopen(p, "rb"); if (!f) return false; std::vector<unsigned char> b; unsigned char t[65536]; size_t n;
    while ((n = fread(t, 1, sizeof t, f)) > 0) b.insert(b.end(), t, t + n); fclose(f);
    auto u32 = [&](size_t o) { return (uint32_t) b[o] | b[o + 1] << 8 | b[o + 2] << 16 | (uint32_t) b[o + 3] << 24; };
    auto u16 = [&](size_t o) { return (uint16_t) (b[o] | b[o + 1] << 8); };
    size_t o = 12; int ch = 2, bits = 32, fmt = 3; size_t dpos = 0, dlen = 0;
    while (o + 8 <= b.size()) { uint32_t id = u32(o), sz = u32(o + 4);
        if (id == 0x20746d66) { fmt = u16(o + 8); ch = u16(o + 10); sr = (int) u32(o + 12); bits = u16(o + 22); }
        if (id == 0x61746164) { dpos = o + 8; dlen = sz; break; } o += 8 + sz + (sz & 1); }
    if (fmt != 3 || bits != 32) return false;
    size_t frames = dlen / (4 * ch); L.resize(frames); R.resize(frames);
    for (size_t i = 0; i < frames; ++i) { float v[2]; for (int c = 0; c < ch && c < 2; ++c) memcpy(&v[c], &b[dpos + (i * ch + c) * 4], 4); L[i] = v[0]; R[i] = ch > 1 ? v[1] : v[0]; }
    return true;
}
static void writeWav(const char* p, const std::vector<float>& L, const std::vector<float>& R, int sr)
{
    FILE* f = fopen(p, "wb"); uint32_t dl = (uint32_t) (L.size() * 8), rl = 36 + dl, fs = 16, br = sr * 8, sr32 = sr; uint16_t fmt = 3, ch = 2, ba = 8, bits = 32;
    fwrite("RIFF", 1, 4, f); fwrite(&rl, 4, 1, f); fwrite("WAVEfmt ", 1, 8, f); fwrite(&fs, 4, 1, f); fwrite(&fmt, 2, 1, f); fwrite(&ch, 2, 1, f);
    fwrite(&sr32, 4, 1, f); fwrite(&br, 4, 1, f); fwrite(&ba, 2, 1, f); fwrite(&bits, 2, 1, f); fwrite("data", 1, 4, f); fwrite(&dl, 4, 1, f);
    for (size_t i = 0; i < L.size(); ++i) { fwrite(&L[i], 4, 1, f); fwrite(&R[i], 4, 1, f); } fclose(f);
}
int main(int argc, char** argv)
{
    if (argc < 3) return 1; std::vector<float> L, R; int sr = 48000; if (!readWav(argv[1], L, R, sr)) return 2;
    brutal::Real r; // defaults of the reference
    r.inputDb = brutal::inputNormToDb(0.8f); r.gainDb = brutal::gainNormToDb(0.794418f); r.slay = 1.f; r.bodyDb = brutal::bodyNormToDb(0.65f);
    brutal::SlayTables sl; double bFc = 305.0, dFc = 164.0; float sA = 1.07318f, sN = 2.87328f, dG = -4.2f, pOff = 0.4f; float atkMs = 0.10f, rel = 298.f, hold = 13.8f; int bpos = 1;
    for (int a = 3; a < argc; ++a) { std::string s = argv[a]; auto e = s.find('='); int idx = atoi(s.substr(0, e).c_str()); float v = (float) atof(s.substr(e + 1).c_str());
        switch (idx) { case 0: r.slay = v; break; case 1: r.inputDb = brutal::inputNormToDb(v); break; case 2: r.gainDb = brutal::gainNormToDb(v); break;
            case 3: r.bodyDb = brutal::bodyNormToDb(v); break; case 4: r.mix = v; break; case 6: r.on = v > 0.5f; break; case 7: r.lofi = v > 0.5f; break; case 20: rel = v; break; case 22: hold = v; break; case 23: atkMs = v; break; case 30: pOff = v; break; default: if (idx >= 100 && idx < 121) sl.lk[idx - 100] = v; else if (idx >= 130 && idx < 151) sl.pdb[idx - 130] = v; break; case 31: dG = v; break; case 32: dFc = v; break; case 33: sA = v; break; case 34: sN = v; break; case 35: bFc = v; break; case 21: bpos = (int) v; break; } }
    brutal::Engine en; en.slayT = sl; en.attackMs = atkMs; en.pOffsetDb = pOff; en.detGainDb = dG; en.detFc = dFc; en.satA = sA; en.satN = sN; en.bodyFc = bFc; en.relDbPerSec = rel; en.holdMs = hold; en.bodyPos = bpos; en.prepare(sr); en.setParams(r); en.process(L.data(), R.data(), (int) L.size());
    writeWav(argv[2], L, R, sr); return 0;
}
