// Geode CPU benchmark: g++ -O3 -std=c++11 bench.cpp -o bench && ./bench
#include "../plugins/Geode/GeodeEngine.hpp"
#include <chrono>
#include <cstdio>
using namespace geode;
int main() {
    for (int cfg = 0; cfg < 3; ++cfg) {
        Engine e; e.setSampleRate(48000); e.p[kMix] = 50;
        if (cfg >= 1) for (int b = 0; b < kDecayBands; ++b) { e.p[dp(b, kDUsed)] = 1; e.p[dp(b, kDFreq)] = 200.f * (b + 1); e.p[dp(b, kDAmount)] = 0.3f; e.p[pp(b, kPUsed)] = 1; e.p[pp(b, kPFreq)] = 300.f * (b + 1); e.p[pp(b, kPGain)] = 2; }
        e.reset();
        const int n = 512, secs = 20; std::vector<float> l(n), r(n), ol(n), orr(n);
        for (int i = 0; i < n; ++i) { l[i] = std::sin(i * 0.03f) * 0.3f; r[i] = std::cos(i * 0.017f) * 0.3f; }
        const auto t0 = std::chrono::steady_clock::now();
        for (int b = 0; b < 48000 * secs / n; ++b) {
            if (cfg == 2) e.p[kSpace] = 0.5f + 0.5f * std::sin(b * 0.01f);        // knob moving all the time
            e.process(l.data(), r.data(), ol.data(), orr.data(), n);
        }
        const double el = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
        printf("%s: %.2f%% of one core\n", cfg == 0 ? "default            " : (cfg == 1 ? "all 12 EQ bands    " : "SPACE being turned "), 100 * el / secs);
    }
}
