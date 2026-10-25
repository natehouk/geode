// Geode preset check: plays a short noise burst then silence through every preset.
// g++ -O2 -std=c++11 presets.cpp -o presets && ./presets
#include "../plugins/Geode/GeodeEngine.hpp"
#include "../plugins/Geode/Presets.hpp"
#include <cstdio>
#include <random>
using namespace geode;
int main() {
    std::mt19937 rng(3); std::normal_distribution<float> nd(0.f, 0.2f);
    const int fs = 48000, len = fs * 8;
    std::vector<float> in(len, 0.f); for (int i = 0; i < fs / 2; ++i) in[i] = nd(rng);
    int bad = 0;
    for (uint32_t p = 0; p < kNumPresets; ++p) {
        Engine e; e.setSampleRate((float)fs); e.setTempo(120); std::copy(presets()[p].v, presets()[p].v + kParamCount, e.p); e.p[kMix] = 100; e.reset();
        std::vector<float> l(len), r(len); e.process(in.data(), in.data(), l.data(), r.data(), len);
        double eb = 0, et = 0; float peak = 0; bool fin = true;
        const int from = e.p[kDucking] > 0 ? fs / 2 : 0;          // ducked presets: measure just after the input stops
        for (int i = from; i < from + fs / 2; ++i) eb += l[i] * l[i] + r[i] * r[i];
        for (int i = len - fs; i < len; ++i) et += l[i] * l[i] + r[i] * r[i];
        for (int i = 0; i < len; ++i) { peak = std::max(peak, std::max(std::fabs(l[i]), std::fabs(r[i]))); if (!std::isfinite(l[i])) fin = false; }
        const double lvl = 10 * std::log10(eb / (fs / 2) / (0.04 * 2) + 1e-30), tail = 10 * std::log10(et / (eb + 1e-30) + 1e-30);
        const bool ok = fin && peak < 4 && lvl > -18 && lvl < 2;
        if (!ok) ++bad;
        printf("%-20s %-20s wet %+5.1f dB  peak %.2f  last second %6.1f dB %s\n", presets()[p].name, presets()[p].category, lvl, peak, tail, ok ? "" : "<-- CHECK");
    }
    printf("%u presets, %d flagged\n", kNumPresets, bad); return bad;
}
