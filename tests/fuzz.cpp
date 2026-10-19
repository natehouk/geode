// Geode fuzz test: random parameters and automation at 44.1-192 kHz; output must stay finite and bounded.
// g++ -O2 -std=c++11 fuzz.cpp -o fuzz && ./fuzz
#include "../plugins/Geode/GeodeEngine.hpp"
#include "../plugins/Geode/ParamDefs.hpp"
#include <cstdio>
#include <random>
using namespace geode;
int main() {
    std::mt19937 rng(1234); std::uniform_real_distribution<float> u(0.f, 1.f);
    const float rates[] = { 44100, 48000, 88200, 96000, 192000 };
    int bad = 0; float worst = 0;
    for (int trial = 0; trial < 60; ++trial) {
        Engine e; const float fs = rates[trial % 5]; e.setSampleRate(fs); e.setTempo(60 + 140 * u(rng));
        auto randomize = [&]() { for (uint32_t i = 0; i < kParamCount; ++i) if (!(paramDef(i).hints & kHintOutput)) e.p[i] = fromNorm(i, u(rng)); };
        randomize(); e.reset();
        std::vector<float> l(512), r(512), ol(512), orr(512);
        const int blocks = (int)(fs * 3 / 512);
        for (int b = 0; b < blocks; ++b) {
            const int kind = (int)(u(rng) * 4);
            for (int i = 0; i < 512; ++i) {
                float x = 0;
                if (kind == 0) x = u(rng) * 2 - 1; else if (kind == 1) x = std::sin(i * 0.05f * (1 + trial)); else if (kind == 2) x = (i == 0) ? 1.f : 0.f;
                l[i] = x; r[i] = kind == 1 ? -x : x * 0.7f;
            }
            if (u(rng) < 0.3f) { const uint32_t p = (uint32_t)(u(rng) * kParamCount); if (!(paramDef(p).hints & kHintOutput)) e.p[p] = fromNorm(p, u(rng)); }
            if (u(rng) < 0.02f) randomize();
            e.process(l.data(), r.data(), ol.data(), orr.data(), 512);
            for (int i = 0; i < 512; ++i) { if (!std::isfinite(ol[i]) || !std::isfinite(orr[i])) ++bad; worst = std::max(worst, std::max(std::fabs(ol[i]), std::fabs(orr[i]))); }
        }
    }
    printf("60 trials at 44.1-192 kHz with random automation: %d non-finite samples, peak %.2f\n", bad, worst);
    const bool ok = bad == 0 && worst <= 64.f;
    printf("%s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
