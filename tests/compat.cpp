// 1.x compatibility: random 1.x settings (2.0 parameters at their defaults), including decay and post EQ bands, with
// changes mid-stream, through the frozen 1.0 engine and the 2.0 engine at 44.1-96 kHz: the outputs must match.
#include "../plugins/Geode/ParamDefs.hpp"
#include "legacy/GeodeEngine1.hpp"
#include <cstdio>
#include <random>
#include <vector>
using namespace geode;
int main() {
    std::mt19937 g(11); std::uniform_real_distribution<float> u(0, 1);
    int bad = 0; float worstAll = -1000;
    for (int sr : { 44100, 48000, 96000 }) for (int trial = 0; trial < 12; ++trial) {
        Engine e; geode1::Engine o;
        for (uint32_t i = 0; i <= kOutOutR; ++i) {
            const ParamDef& d = paramDef(i); if (d.hints & kHintOutput) continue;
            float lo = d.min, hi = d.max;
            if (i == kDecayRate) { lo = 50; hi = 200; }                  // the 1.x range
            float v = (d.hints & kHintLog) ? lo * std::pow(hi / lo, u(g)) : lo + (hi - lo) * u(g);
            if (d.hints & kHintInteger) v = std::round(v);
            if ((decayFieldOf(i) == kDUsed || postFieldOf(i) == kPUsed) && u(g) < 0.4f) v = 0;
            e.p[i] = v; o.p[i] = v;
        }
        e.setSampleRate((float)sr); o.setSampleRate((float)sr); e.reset(); o.reset();
        const int N = sr * 2; std::vector<float> l(N), r(N), L1(N), R1(N), L2(N), R2(N);
        for (int i = 0; i < N; ++i) { const float env = (i / (sr / 3)) % 2 ? 0.0f : 0.5f; l[i] = env * (2 * u(g) - 1); r[i] = 0.6f * l[i] + 0.2f * env * (2 * u(g) - 1); }
        for (int i = 0; i < N; i += 300) {
            const int n = std::min(300, N - i);
            if (i >= N / 2 && i < N / 2 + 300) { const float s = u(g), d = 50 + 150 * u(g); e.p[kSpace] = o.p[geode1::kSpace] = s; e.p[kDecayRate] = o.p[geode1::kDecayRate] = d; e.p[pp(0, kPUsed)] = o.p[geode1::pp(0, geode1::kPUsed)] = 1; }
            o.process(l.data() + i, r.data() + i, L1.data() + i, R1.data() + i, n);
            e.process(l.data() + i, r.data() + i, L2.data() + i, R2.data() + i, n);
        }
        float worst = 0; for (int i = 0; i < N; ++i) worst = std::max(worst, std::max(std::fabs(L1[i] - L2[i]), std::fabs(R1[i] - R2[i])));
        const float rel = 20 * std::log10(worst / 0.5f + 1e-12f);
        worstAll = std::max(worstAll, rel);
        if (rel > -120) { ++bad; printf("FAIL sr %d trial %d space %.2f: difference %.1f dB below the input\n", sr, trial, e.p[kSpace], -rel); }
    }
    printf("1.x compatibility: largest difference %.1f dB below the input level, %d failures\n", -worstAll, bad);
    return bad;
}
