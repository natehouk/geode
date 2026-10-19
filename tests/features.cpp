// Geode feature tests: g++ -O2 -std=c++11 features.cpp -o features && ./features
#include "../plugins/Geode/GeodeEngine.hpp"
#include "../plugins/Geode/Presets.hpp"
#include <cstdio>
#include <random>
using namespace geode;

static int fails = 0;
#define check(cond, ...) do { const bool ok_ = (cond); printf("%s  ", ok_ ? "PASS" : "FAIL"); printf(__VA_ARGS__); printf("\n"); if (!ok_) ++fails; } while (0)

static const float FS = 48000.f;
struct Out { std::vector<float> l, r; };
static Out run(Engine& e, const std::vector<float>& inL, const std::vector<float>& inR) {
    Out o; o.l.resize(inL.size()); o.r.resize(inL.size());
    for (size_t i = 0; i < inL.size(); i += 256) { const uint32_t n = (uint32_t)std::min<size_t>(256, inL.size() - i); e.process(&inL[i], &inR[i], &o.l[i], &o.r[i], n); }
    return o;
}
static Engine* make(std::initializer_list<std::pair<uint32_t, float>> ps) {
    Engine* e = new Engine(); e->setSampleRate(FS); e->p[kMix] = 100;
    for (auto& kv : ps) e->p[kv.first] = kv.second;
    e->reset(); return e;
}
static double energy(const std::vector<float>& a, size_t from, size_t to) { double s = 0; for (size_t i = from; i < to && i < a.size(); ++i) s += (double)a[i] * a[i]; return s; }
static double db(double e) { return 10 * std::log10(e + 1e-30); }
// RT60 from the Schroeder backward integral of an impulse response, fitted between -5 and -35 dB
static float rt60(const std::vector<float>& h) {
    std::vector<double> sch(h.size() + 1, 0.0);
    for (size_t i = h.size(); i-- > 0;) sch[i] = sch[i + 1] + (double)h[i] * h[i];
    const double tot = sch[0]; size_t a = 0, b = 0;
    for (size_t i = 0; i < h.size(); ++i) { const double d = 10 * std::log10(sch[i] / tot + 1e-30); if (!a && d < -5) a = i; if (d < -35) { b = i; break; } }
    if (!a || !b) return -1;
    return (float)((b - a) / FS * 60.0 / 30.0);
}
// a simple band-pass (RBJ, constant peak gain) for per-band decay checks
static std::vector<float> bandpass(const std::vector<float>& x, float f, float q) {
    const double w = 2 * 3.14159265358979 * f / FS, al = std::sin(w) / (2 * q), a0 = 1 + al;
    const double b0 = al / a0, b2 = -al / a0, a1 = -2 * std::cos(w) / a0, a2 = (1 - al) / a0;
    std::vector<float> y(x.size()); double x1 = 0, x2 = 0, y1 = 0, y2 = 0;
    for (size_t i = 0; i < x.size(); ++i) { const double v = b0 * x[i] + b2 * x2 - a1 * y1 - a2 * y2; x2 = x1; x1 = x[i]; y2 = y1; y1 = v; y[i] = (float)v; }
    return y;
}
static std::vector<float> band3(const std::vector<float>& x, float f, float q) { return bandpass(bandpass(bandpass(x, f, q), f, q), f, q); }
static std::vector<float> impulseResponse(Engine& e, float secs) {
    std::vector<float> in((size_t)(secs * FS), 0.f); in[0] = 1;
    Out o = run(e, in, in);
    std::vector<float> m(in.size()); for (size_t i = 0; i < in.size(); ++i) m[i] = 0.5f * (o.l[i] + o.r[i]);
    return m;
}

int main() {
    std::mt19937 rng(7); std::normal_distribution<float> nd(0.f, 0.1f);
    std::vector<float> noise(96000); for (float& v : noise) v = nd(rng);

    // 1. mix 0: the dry signal passes untouched
    { Engine* e = make({ { kMix, 0 } }); Out o = run(*e, noise, noise);
      double err = 0; for (size_t i = 4800; i < noise.size(); ++i) err = std::max(err, (double)std::fabs(o.l[i] - noise[i]));
      check(err < 1e-5, "mix 0%%: dry passes untouched (max error %.1e)", err); delete e; }

    // 2. mix 100: nothing dry, the reverb starts after the first reflection
    { Engine* e = make({}); std::vector<float> in(4800, 0.f); in[0] = 1; Out o = run(*e, in, in);
      float early = 0; for (int i = 0; i < 20; ++i) early = std::max(early, std::fabs(o.l[i]));
      float later = 0; for (int i = 20; i < 4800; ++i) later = std::max(later, std::fabs(o.l[i]));
      check(early < 1e-6f && later > 0.01f, "mix 100%%: no dry signal (first 20 samples %.1e), reverb follows (peak %.2f)", early, later); delete e; }

    // 3. the decay time follows the space, and matches the decay time the display shows
    { bool ok = true; char msg[512] = ""; size_t m = 0;
      for (float s : { 0.15f, 0.5f, 0.85f }) {
          Engine* e = make({ { kSpace, s } });
          const std::vector<float> h = bandpass(impulseResponse(*e, 12.f), 1000.f, 1.f);
          DecayCurve dc; dc.build(e->p, FS); const float want = dc.timeAt(1000.f), got = rt60(h);
          if (!(got > want * 0.8f && got < want * 1.2f)) ok = false;
          m += snprintf(msg + m, sizeof msg - m, "%s %.2f s (display %.2f)  ", kSpaces[spaceIndex(s)].name, got, want); delete e;
      }
      check(ok, "RT60 at 1 kHz: %s", msg); }

    // 4. decay rate scales the decay time
    { Engine* a = make({ { kSpace, 0.5f }, { kDecayRate, 50 } }), * b = make({ { kSpace, 0.5f }, { kDecayRate, 200 } });
      const float ta = rt60(bandpass(impulseResponse(*a, 12.f), 1000.f, 1.f)), tb = rt60(bandpass(impulseResponse(*b, 12.f), 1000.f, 1.f));
      check(tb / ta > 3.2f && tb / ta < 4.8f, "decay rate 50%% -> 200%%: %.2f s -> %.2f s (x%.2f, expect x4)", ta, tb, tb / ta); delete a; delete b; }

    // 5. decay rate EQ: a high shelf makes the highs die away sooner, a low bell makes the lows ring longer; the level barely changes
    { Engine* f = make({ { kSpace, 0.5f } });
      Engine* e = make({ { kSpace, 0.5f }, { dp(0, kDUsed), 1 }, { dp(0, kDShape), kDHighShelf }, { dp(0, kDFreq), 3000 }, { dp(0, kDAmount), -1.5f }, { dp(0, kDQ), 0.7f },
                         { dp(1, kDUsed), 1 }, { dp(1, kDShape), kDBell }, { dp(1, kDFreq), 200 }, { dp(1, kDAmount), 1.f }, { dp(1, kDQ), 1.f } });
      const std::vector<float> hf = impulseResponse(*f, 12.f), he = impulseResponse(*e, 12.f);
      const float h0 = rt60(band3(hf, 8000.f, 2.f)), h1 = rt60(band3(he, 8000.f, 2.f)), l0 = rt60(band3(hf, 200.f, 2.f)), l1 = rt60(band3(he, 200.f, 2.f));
      check(h1 < h0 * 0.5f && l1 > l0 * 1.6f, "decay EQ: 8 kHz %.2f -> %.2f s (high shelf -1.5 oct), 200 Hz %.2f -> %.2f s (bell +1 oct)", h0, h1, l0, l1);
      delete f; delete e; }

    // 6. a decay notch shortens the decay in its band only
    { Engine* f = make({ { kSpace, 0.6f } }), * e = make({ { kSpace, 0.6f }, { dp(0, kDUsed), 1 }, { dp(0, kDShape), kDNotch }, { dp(0, kDFreq), 2000 }, { dp(0, kDQ), 4.f } });
      const std::vector<float> hf = impulseResponse(*f, 12.f), he = impulseResponse(*e, 12.f);
      const float n0 = rt60(band3(hf, 2000.f, 8.f)), n1 = rt60(band3(he, 2000.f, 8.f)), o0 = rt60(band3(hf, 500.f, 2.f)), o1 = rt60(band3(he, 500.f, 2.f));
      DecayCurve dc; dc.build(e->p, FS); const float shown = dc.timeAt(2000.f) / decayTime(e->p);
      check(n1 < n0 * 0.4f && shown < 0.25f && std::fabs(o1 / o0 - 1) < 0.1f, "decay notch at 2 kHz: %.2f -> %.2f s measured through a band-pass (display: x%.2f there); 500 Hz %.2f -> %.2f s",
            n0, n1, shown, o0, o1); delete f; delete e; }

    // 7. brightness: darker reverb has less top end
    { Engine* d = make({ { kBrightness, 0 } }), * b = make({ { kBrightness, 100 } });
      Out od = run(*d, noise, noise), ob = run(*b, noise, noise);
      const double rd = db(energy(bandpass(od.l, 8000.f, 1.f), 48000, 96000)) - db(energy(bandpass(od.l, 500.f, 1.f), 48000, 96000));
      const double rb = db(energy(bandpass(ob.l, 8000.f, 1.f), 48000, 96000)) - db(energy(bandpass(ob.l, 500.f, 1.f), 48000, 96000));
      check(rb - rd > 8, "brightness 0%% -> 100%%: 8 kHz relative to 500 Hz %.1f -> %.1f dB", rd, rb); delete d; delete b; }

    // 8. predelay, in ms and synced to tempo
    { Engine* e = make({ { kPredelay, 100 } }); std::vector<float> in(24000, 0.f); in[0] = 1; Out o = run(*e, in, in);
      size_t first = 0; for (size_t i = 0; i < in.size(); ++i) if (std::fabs(o.l[i]) > 1e-4f) { first = i; break; }
      Engine* s = make({ { kPredelaySync, 1 }, { kPredelayNote, 4 } }); s->setTempo(120); s->reset();
      Out os = run(*s, in, in); size_t fs2 = 0; for (size_t i = 0; i < in.size(); ++i) if (std::fabs(os.l[i]) > 1e-4f) { fs2 = i; break; }
      check(std::fabs(first / 48.f - 100.5f) < 1.5f && std::fabs(fs2 / 48.f - 125.5f) < 1.5f, "predelay 100 ms: reverb starts at %.1f ms; 1/16 at 120 BPM: %.1f ms", first / 48.f, fs2 / 48.f);
      delete e; delete s; }

    // 9. stereo width: 0% is mono, 100% is decorrelated, 120% is wider still
    { auto corr = [&](float w) { Engine* e = make({ { kWidth, w } }); std::vector<float> z(noise.size(), 0.f); Out o = run(*e, noise, z);
          double lr = 0, ll = 0, rr = 0; for (size_t i = 48000; i < noise.size(); ++i) { lr += o.l[i] * o.r[i]; ll += o.l[i] * o.l[i]; rr += o.r[i] * o.r[i]; }
          delete e; return lr / std::sqrt(ll * rr + 1e-30); };
      const double c0 = corr(0), c1 = corr(100), c2 = corr(120);
      check(c0 > 0.999 && std::fabs(c1) < 0.5 && c2 < c1, "width: correlation 0%% %.3f, 100%% %.2f, 120%% %.2f", c0, c1, c2); }

    // 10. post EQ: a 48 dB/oct low cut at 500 Hz removes the lows of the reverb only
    { Engine* f = make({ { kMix, 50 } }), * e = make({ { kMix, 50 }, { pp(0, kPUsed), 1 }, { pp(0, kPShape), kPLowCut }, { pp(0, kPFreq), 500 }, { pp(0, kPSlope), 3 } });
      Out of = run(*f, noise, noise), oe = run(*e, noise, noise);
      // the dry part is untouched: compare output minus the dry share
      std::vector<float> wf(noise.size()), we(noise.size()); const float dry = std::cos(0.5f * kPi * 0.5f);
      for (size_t i = 0; i < noise.size(); ++i) { wf[i] = of.l[i] - dry * noise[i]; we[i] = oe.l[i] - dry * noise[i]; }
      auto low = [&](const std::vector<float>& v) { return db(energy(bandpass(bandpass(bandpass(v, 100.f, 3.f), 100.f, 3.f), 100.f, 3.f), 48000, 96000)); };
      const double lo0 = low(wf), lo1 = low(we);
      const double hi0 = db(energy(bandpass(wf, 4000.f, 1.f), 48000, 96000)), hi1 = db(energy(bandpass(we, 4000.f, 1.f), 48000, 96000));
      check(lo0 - lo1 > 40 && std::fabs(hi0 - hi1) < 1, "post EQ low cut: reverb at 100 Hz %.1f dB down, at 4 kHz %.2f dB", lo0 - lo1, hi0 - hi1); delete f; delete e; }

    // 11. the level stays about the same across spaces (wet, noise in)
    { double lo = 1e9, hi = -1e9;
      for (int k = 0; k < kNumSpaces; ++k) { Engine* e = make({ { kSpace, (float)k / (kNumSpaces - 1) } }); Out o = run(*e, noise, noise);
          const double l = db(energy(o.l, 48000, 96000)) - db(energy(noise, 48000, 96000)); lo = std::min(lo, l); hi = std::max(hi, l); delete e; }
      check(hi - lo < 4 && hi < 0 && lo > -10, "wet level over all %d spaces: %.1f to %.1f dB", kNumSpaces, lo, hi); }

    // 12. stability: extreme decay boosts stay bounded and still decay (capped at %g s)
    { Engine* e = make({ { kSpace, 1 }, { kDecayRate, 200 } });
      for (int b = 0; b < kDecayBands; ++b) { e->p[dp(b, kDUsed)] = 1; e->p[dp(b, kDShape)] = b % 3; e->p[dp(b, kDFreq)] = 300.f * (b + 1); e->p[dp(b, kDAmount)] = 2; e->p[dp(b, kDQ)] = 0.3f; }
      e->reset();
      std::vector<float> in((size_t)(FS * 70), 0.f); for (int i = 0; i < 48000; ++i) in[i] = noise[i];
      Out o = run(*e, in, in); float peak = 0; bool fin = true;
      for (float v : o.l) { peak = std::max(peak, std::fabs(v)); if (!std::isfinite(v)) fin = false; }
      const double tail = db(energy(o.l, in.size() - 48000, in.size())), body = db(energy(o.l, 48000, 96000));
      check(fin && peak < 8 && body - tail > 60, "all decay bands at +2 oct on the longest space: peak %.2f, after 70 s %.0f dB down", peak, body - tail); delete e; }

    // 13. character changes the sound (more reflections and modulation), space morphs without clicks
    { Engine* a = make({ { kCharacter, 0 } }), * b = make({ { kCharacter, 100 } });
      Out oa = run(*a, noise, noise), ob = run(*b, noise, noise); double d = 0;
      for (size_t i = 48000; i < noise.size(); ++i) d += (oa.l[i] - ob.l[i]) * (oa.l[i] - ob.l[i]);
      Engine* m = make({}); std::vector<float> sine(FS * 4); for (size_t i = 0; i < sine.size(); ++i) sine[i] = 0.3f * std::sin(2 * kPi * 440 * i / FS);
      Out om; om.l.resize(sine.size()); om.r.resize(sine.size()); float jump = 0;
      for (size_t i = 0; i < sine.size(); i += 256) { m->p[kSpace] = 0.5f + 0.5f * std::sin((float)i / FS * 3.f); m->process(&sine[i], &sine[i], &om.l[i], &om.r[i], 256); }
      for (size_t i = 48001; i < sine.size(); ++i) jump = std::max(jump, std::fabs(om.l[i] - om.l[i - 1]));
      check(db(d) - db(energy(oa.l, 48000, 96000)) > -3 && jump < 0.12f, "character 0%% vs 100%%: difference %.1f dB; sweeping SPACE on a sine: largest step %.3f",
            db(d) - db(energy(oa.l, 48000, 96000)), jump); delete a; delete b; delete m; }

    // 14. every preset is finite
    { bool ok = true; for (uint32_t i = 0; i < kNumPresets; ++i) { Engine e; std::copy(presets()[i].v, presets()[i].v + kParamCount, e.p); e.reset(); Out o = run(e, noise, noise);
          for (float v : o.l) if (!std::isfinite(v) || std::fabs(v) > 8) ok = false; }
      check(ok, "all %u presets finite and bounded", kNumPresets); }

    printf("\n%s (%d failure%s)\n", fails ? "FAILED" : "ALL PASSED", fails, fails == 1 ? "" : "s");
    return fails;
}
