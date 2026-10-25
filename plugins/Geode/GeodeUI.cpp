// Geode — custom vector UI (DPF/NanoVG). Original design. License: MIT.
#include "DistrhoUI.hpp"
#include "GeodePlugin.hpp"
#include "UserPresets.hpp"
#include "ResizeHandle.hpp"
#include "Help.hpp"
#include "IrMatch.hpp"
#include <chrono>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#if defined(_WIN32)
extern "C" __declspec(dllimport) void* __stdcall ShellExecuteA(void*, const char*, const char*, const char*, const char*, int);
#endif

START_NAMESPACE_DISTRHO

static void openURL(const char* url) {
#if defined(_WIN32)
    ShellExecuteA(nullptr, "open", url, nullptr, nullptr, 1);
#elif defined(__APPLE__)
    const std::string c = std::string("open '") + url + "' &"; if (std::system(c.c_str())) {}
#else
    const std::string c = std::string("xdg-open '") + url + "' >/dev/null 2>&1 &"; if (std::system(c.c_str())) {}
#endif
}
static bool gTipsOn = true;
static double nowSec() { return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count(); }

using namespace geode;
using DGL_NAMESPACE::ResizeHandle;

static constexpr float W = 1100.f, H = 800.f;

#define C_BG        Color(0x10, 0x0f, 0x13)
#define C_PANEL     Color(0x19, 0x17, 0x1e)
#define C_PANEL_HI  Color(0x25, 0x22, 0x2c)
#define C_LINE      Color(0x31, 0x2d, 0x3a)
#define C_TEXT      Color(0xec, 0xe8, 0xf0)
#define C_DIM       Color(0x92, 0x8c, 0x9c)
#define C_FAINT     Color(0x52, 0x4c, 0x5c)
#define C_ACC       Color(0xb9, 0x95, 0xec)       // geode: amethyst
#define C_DECAY     Color(0xec, 0xc2, 0x6e)       // decay time: citrine
#define C_ANA       Color(0x6c, 0x5e, 0x88)
#define C_DISPLAY   Color(0x0b, 0x0a, 0x0e)
#define C_DEL       Color(0xe0, 0x60, 0x58)

static Color withAlpha(const Color& c, float a) { Color r(c); r.alpha = a; return r; }

// ---------------------------------------------------------------- layout
static constexpr float DX = 16, DY = 64, DW = 1068, DH = 330;           // reverb display
static constexpr float PY = 404, PH = 382;                               // bottom panels
static constexpr float AX = 16, AW = 640, BX = 664, BW = 420;            // main controls, band editor
static constexpr float SEG_H = 24;
static constexpr float FMIN = 20.f, FMAX = 20000.f;
static constexpr float PX0 = DX + 46, PX1 = DX + DW - 56, PTOP = DY + 42, PBOT = DY + DH - 22;
static constexpr float YC = (PTOP + PBOT) / 2, HALF = (PBOT - PTOP) / 2;
static constexpr float L_RANGE = 2.5f, DB_RANGE = 24.f;
static constexpr int kBands = kDecayBands + kPostBands;           // band ids: 0-5 decay EQ, 6-11 post EQ

enum Role { RUsed, ROn, RShape, RFreq, RGain, RQ, RSlope, RPlace, kNumRoles };
enum WType { Knob, Toggle, Segment, DelButton, Button };
// role >= 0: a control of the selected band; role < 0: a global param
struct Ctl { WType type; int role; uint32_t param; float x, y, r; const char* label; };

static std::vector<Ctl> buildControls() {
    std::vector<Ctl> c;
    c.push_back({ Segment, -1, kAlgorithm, AX + 170, PY + 28, 276, "" });
    c.push_back({ Button, -1, kFreeze,     AX + AW - 82, PY + 28, 120, "FREEZE" });
    c.push_back({ Knob, -1, kSpace,      AX + 72,  PY + 118, 40, "SPACE" });
    c.push_back({ Knob, -1, kDecayRate,  AX + 186, PY + 118, 30, "DECAY RATE" });
    c.push_back({ Knob, -1, kBrightness, AX + 286, PY + 118, 30, "BRIGHTNESS" });
    c.push_back({ Knob, -1, kCharacter,  AX + 386, PY + 118, 30, "CHARACTER" });
    c.push_back({ Knob, -1, kDistance,   AX + 486, PY + 118, 30, "DISTANCE" });
    c.push_back({ Knob, -1, kWidth,      AX + 586, PY + 118, 30, "STEREO WIDTH" });
    c.push_back({ Knob, -1, kPredelay,   AX + 60,  PY + 248, 24, "PREDELAY" });
    c.push_back({ Toggle, -1, kPredelaySync, AX + 122, PY + 248, 0, "SYNC" });
    c.push_back({ Knob, -1, kThickness,  AX + 262, PY + 248, 24, "THICKNESS" });
    c.push_back({ Knob, -1, kDucking,    AX + 350, PY + 248, 24, "DUCKING" });
    c.push_back({ Knob, -1, kMix,        AX + 446, PY + 248, 28, "MIX" });
    c.push_back({ Toggle, -1, kMixLock,  AX + 503, PY + 248, 0, "LOCK" });
    c.push_back({ Knob, -1, kOutput,     AX + 600, PY + 248, 24, "OUTPUT" });
    c.push_back({ Toggle, -1, kAutoGate, AX + 40,  PY + 340, 0, "AUTO GATE" });
    c.push_back({ Knob, -1, kGateThreshold, AX + 210, PY + 336, 16, "THRESHOLD" });
    c.push_back({ Knob, -1, kGateHold,   AX + 290, PY + 336, 16, "HOLD" });
    c.push_back({ Toggle, ROn, 0, BX + 30, PY + 28, 0, "" });
    c.push_back({ DelButton, -1, 0, BX + BW - 26, PY + 28, 0, "" });
    c.push_back({ Segment, RShape, 0, BX + BW / 2, PY + 70, 392, "" });
    c.push_back({ Segment, RSlope, 0, BX + BW / 2, PY + 108, 300, "" });
    c.push_back({ Segment, RPlace, 0, BX + BW / 2, PY + 156, 360, "" });
    c.push_back({ Knob, RFreq, 0, BX + 80,  PY + 250, 28, "FREQUENCY" });
    c.push_back({ Knob, RGain, 0, BX + 210, PY + 250, 28, "GAIN" });
    c.push_back({ Knob, RQ,    0, BX + 340, PY + 250, 28, "Q" });
    return c;
}
static const std::vector<Ctl>& controls() { static std::vector<Ctl> c = buildControls(); return c; }

static constexpr float PS_X = 300.f, PS_Y = 20.f, PS_W = 250.f, PS_H = 28.f, PS_ARROW = 30.f;
static constexpr float A_X = 562.f, AB_W = 30.f, COPY_X = 626.f, COPY_W = 44.f;
static constexpr float UNDO_X = 684.f, REDO_X = 738.f, UR_W = 50.f, LEARN_X = 798.f, LEARN_W = 66.f, MIDI_X = 870.f, MIDI_W = 52.f, HELP_X = 928.f, HELP_W = 48.f;
static constexpr float PM_COLW = 150.f, PM_ROW = 23.f, PM_HEAD = 22.f, PM_PAD = 8.f;
static constexpr int ANA_N = 4096;
// display pills
static constexpr float LP_X = DX + 12, LP_Y = DY + 10, LP_H = 22, LP_W0 = 124, LP_W1 = 84, AN_W = 92;
static constexpr float AN_X = DX + DW - 12 - AN_W, PI_W = 66, PI_X = AN_X - 6 - PI_W, IR_W = 104, IR_X = PI_X - 6 - IR_W;

static bool isPost(int id) { return id >= kDecayBands; }
static uint32_t bandParam(int id, int role) {
    if (id < 0 || id >= kBands) return kParamCount;
    if (!isPost(id)) { static const int m[kNumRoles] = { kDUsed, kDOn, kDShape, kDFreq, kDAmount, kDQ, -1, -1 }; return m[role] < 0 ? kParamCount : dp(id, m[role]); }
    if (role == RPlace) return placeParam(id - kDecayBands);
    static const int m[kNumRoles] = { kPUsed, kPOn, kPShape, kPFreq, kPGain, kPQ, kPSlope, -1 };
    return pp(id - kDecayBands, m[role]);
}
static cpx postRespAt(const float* v, float hz) {
    cpx h(1.f, 0.f);
    for (int b = 0; b < kPostBands; ++b) {
        if (v[pp(b, kPUsed)] < 0.5f || v[pp(b, kPOn)] < 0.5f) continue;
        h *= postResponse((int)(v[pp(b, kPShape)] + .5f), (int)(v[pp(b, kPSlope)] + .5f), v[pp(b, kPFreq)], v[pp(b, kPGain)], v[pp(b, kPQ)], hz);
    }
    return h;
}

class GeodeUI : public UI {
public:
    GeodeUI() : UI((uint)W, (uint)H), resizeHandle(this) {
        loadSharedResources();
        Engine* def = new Engine();
        for (uint32_t i = 0; i < kParamCount; ++i) { val[i] = def->p[i]; defv[i] = def->p[i]; }
        delete def;
        for (int& m : paramCC) m = -1;
        const double sf = getScaleFactor();
        if (sf != 1.0) setSize((uint)(W * sf), (uint)(H * sf));
        setGeometryConstraints((uint)(W * 0.6 * sf), (uint)(H * 0.6 * sf), true);
        if (isResizable()) resizeHandle.hide();
        userPresets = listUserPresets();
        std::copy(val, val + kParamCount, before);
        fft.init(ANA_N); spec.assign(ANA_N / 2, -120.f); buf.resize(ANA_N);
    }

protected:
    float uiScale() const { return (float)getWidth() / W; }
    static bool isOut(uint32_t p) { return (paramDef(p).hints & kHintOutput) != 0; }
    static bool isView(uint32_t p) { return (paramDef(p).hints & kHintNoAuto) != 0; }

    // ---- values -------------------------------------------------------------
    void setValue(uint32_t p, float v) {
        const ParamDef& d = paramDef(p);
        v = clampf(v, d.min, d.max);
        if (d.hints & kHintInteger) v = std::round(v);
        if (v == val[p]) return;
        val[p] = v; setParameterValue(p, v); repaint();
    }
    void commit(uint32_t p, float v) { editParameter(p, true); setValue(p, v); editParameter(p, false); }
    void parameterChanged(uint32_t i, float v) override {
        if (i >= kParamCount) return;
        if (i == kOutLearn) {
            if (v != lastLearnEcho && v > 0) {
                const int code = (int)v % 1000000 - 1, prm = code / 128, cc = code % 128;
                for (int& m : paramCC) if (m == cc) m = -1;
                if (prm >= 0 && prm < (int)kParamCount) paramCC[prm] = cc;
                learnWaiting = -1;
            }
            lastLearnEcho = v; repaint(); return;
        }
        if (isOut(i)) { val[i] = v; return; }
        if (val[i] != v) { val[i] = v; if (!gesture) before[i] = v; repaint(); }
    }
    void stateChanged(const char* key, const char* value) override {
        if (std::strcmp(key, "midimap") != 0) return;
        for (int& m : paramCC) m = -1;
        const char* q = value;
        while (q && *q) {
            int cc = -1, prm = -1;
            if (std::sscanf(q, "%d:%d", &cc, &prm) == 2 && prm >= 0 && prm < (int)kParamCount) paramCC[prm] = cc;
            const char* s = std::strchr(q, ';'); if (!s) break; q = s + 1;
        }
        repaint();
    }
    float bv(int id, int role) const { const uint32_t q = bandParam(id, role); return q < kParamCount ? val[q] : 0.f; }
    bool used(int id) const { return bv(id, RUsed) > 0.5f; }
    bool bandOn(int id) const { return bv(id, ROn) > 0.5f; }
    int shapeOf(int id) const { return (int)(bv(id, RShape) + .5f); }
    bool isCut(int id) const { return isPost(id) && (shapeOf(id) == kPLowCut || shapeOf(id) == kPHighCut); }
    int primary() const { return (focus >= 0 && used(focus)) ? focus : -1; }

    // ---- undo / redo / A-B (view settings are not part of a setting) -------------
    typedef std::vector<float> Snap;
    Snap snapshot() const { Snap s(val, val + kParamCount); s.push_back((float)abSlot); return s; }
    bool differs(const float* a, const float* b) const {
        for (uint32_t i = 0; i < kParamCount; ++i) if (!isOut(i) && !isView(i) && a[i] != b[i]) return true;
        return false;
    }
    void beginGesture() { gesture = true; std::copy(val, val + kParamCount, before); }
    void endGesture() {
        gesture = false;
        if (differs(before, val)) { Snap u(before, before + kParamCount); u.push_back((float)abSlot); undoStack.push_back(u); if (undoStack.size() > 200) undoStack.erase(undoStack.begin()); redoStack.clear(); }
        std::copy(val, val + kParamCount, before); repaint();
    }
    void applyAll(const float* s) { for (uint32_t i = 0; i < kParamCount; ++i) if (!isOut(i) && !isView(i) && val[i] != s[i]) commit(i, s[i]); }
    void restoreSnap(const Snap& s) {
        if (s.size() > kParamCount && (int)s[kParamCount] != abSlot) { abStore[abSlot] = snapshot(); abSlot = (int)s[kParamCount]; }
        applyAll(s.data()); std::copy(val, val + kParamCount, before); cleanSelection(); }
    void undo() { if (undoStack.empty()) return; redoStack.push_back(snapshot()); restoreSnap(undoStack.back()); undoStack.pop_back(); flash("Undo"); }
    void redo() { if (redoStack.empty()) return; undoStack.push_back(snapshot()); restoreSnap(redoStack.back()); redoStack.pop_back(); flash("Redo"); }
    void switchAB(int to) {
        if (to == abSlot) return;
        abStore[abSlot] = snapshot(); if (abStore[to].empty()) abStore[to] = snapshot();
        beginGesture(); applyAll(abStore[to].data()); endGesture(); cleanSelection();
        abSlot = to; flash(to == 0 ? "Setting A" : "Setting B");
    }
    void copyAB() { abStore[1 - abSlot] = snapshot(); flash(abSlot == 0 ? "Copied A to B" : "Copied B to A"); }
    void cleanSelection() { if (focus >= 0 && !used(focus)) focus = -1; }

    // ---- bands ------------------------------------------------------------------
    int newBand(bool post, float hz, float y) {
        const int first = post ? kDecayBands : 0, last = post ? kBands : kDecayBands;
        int id = -1; for (int i = first; i < last; ++i) if (!used(i)) { id = i; break; }
        if (id < 0) { flash(post ? "All 6 post EQ bands are in use" : "All 6 decay EQ bands are in use"); return -1; }
        commit(bandParam(id, RShape), 0); commit(bandParam(id, RFreq), clampf(hz, FMIN, FMAX));
        commit(bandParam(id, RGain), post ? clampf(dbOfY(y), -30.f, 30.f) : clampf(lOfY(y), -2.f, 2.f));
        commit(bandParam(id, RQ), 1.f); if (post) commit(bandParam(id, RSlope), 2);
        commit(bandParam(id, ROn), 1); commit(bandParam(id, RUsed), 1);
        return id;
    }
    void deleteFocused() {
        const int id = primary(); if (id < 0) return;
        beginGesture(); commit(bandParam(id, RUsed), 0); commit(bandParam(id, RGain), 0); endGesture();
        focus = -1; repaint();
    }

    // ---- formatting ---------------------------------------------------------
    static void fmtFreq(float v, char* b, size_t n) { if (v >= 1000) snprintf(b, n, v >= 10000 ? "%.1f kHz" : "%.2f kHz", v / 1000); else snprintf(b, n, v < 100 ? "%.1f Hz" : "%.0f Hz", v); }
    void fmtVal(uint32_t p, float v, char* b, size_t n) const {
        int ne = 0;
        if (const char* const* l = enumLabels(p, ne)) { snprintf(b, n, "%s", l[std::max(0, std::min(ne - 1, (int)std::round(v)))]); return; }
        if (paramDef(p).hints & kHintBoolean) { snprintf(b, n, "%s", v > 0.5f ? "On" : "Off"); return; }
        switch (p) {
        case kSpace: snprintf(b, n, "%s", kSpaces[spaceIndex(v)].name); return;
        case kOutput: snprintf(b, n, "%+.1f dB", v); return;
        case kPredelay: snprintf(b, n, v < 10 ? "%.1f ms" : "%.0f ms", v); return;
        case kGateThreshold: snprintf(b, n, "%.0f dB", v); return;
        case kGateHold: if (v >= 1000) snprintf(b, n, "%.2f s", v / 1000); else snprintf(b, n, "%.0f ms", v); return;
        default: break;
        }
        const int df = decayFieldOf(p), pf = postFieldOf(p);
        if (df == kDFreq || pf == kPFreq) { fmtFreq(v, b, n); return; }
        if (df == kDAmount) { snprintf(b, n, "%.0f%% decay", 100.f * std::exp2(v)); return; }
        if (pf == kPGain) { snprintf(b, n, "%+.1f dB", v); return; }
        if (df == kDQ || pf == kPQ) { snprintf(b, n, v < 10 ? "%.2f" : "%.1f", v); return; }
        snprintf(b, n, "%.0f%%", v);
    }
    void fmt(uint32_t p, char* b, size_t n) const { fmtVal(p, val[p], b, n); }
    bool parse(uint32_t p, const std::string& in, float& out) const {
        std::string s; for (char ch : in) if (!std::isspace((unsigned char)ch)) s += (char)std::tolower((unsigned char)ch);
        if (s.empty()) return false;
        if (p == kSpace) for (int i = 0; i < kNumSpaces; ++i) {
            std::string nm; for (const char* c = kSpaces[i].name; *c; ++c) if (*c != ' ') nm += (char)std::tolower((unsigned char)*c);
            if (nm.compare(0, s.size(), s) == 0) { out = (float)i / (kNumSpaces - 1); return true; }
        }
        char* end = nullptr; double x = std::strtod(s.c_str(), &end);
        if (end == s.c_str()) return false;
        const std::string suf(end);
        if (suf == "k" || suf == "khz") x *= 1000;
        if (p == kSpace) x = x > 1 ? x / 100.0 : x;
        if (decayFieldOf(p) == kDAmount) x = std::log2(std::max(1.0, x) / 100.0);
        if ((p == kPredelay || p == kGateHold) && suf == "s") x *= 1000;
        out = (float)x; return true;
    }
    uint32_t effParam(const Ctl& c) const {
        if (c.role >= 0) return primary() >= 0 ? bandParam(primary(), c.role) : kParamCount;
        if (c.param == kPredelay && val[kPredelaySync] > 0.5f) return kPredelayNote;
        return c.param;
    }
    bool visible(const Ctl& c) const {
        if (c.type == DelButton) return primary() >= 0;
        if (c.role < 0) return true;
        if (primary() < 0) return false;
        if (c.role == RSlope || c.role == RPlace) return isPost(primary());
        return true;
    }
    bool dimmed(const Ctl& c) const {
        if (c.role < 0 && (c.param == kGateThreshold || c.param == kGateHold)) return val[kAutoGate] < 0.5f;
        if (c.role < 0 || primary() < 0) return false;
        const int id = primary();
        if (c.role == RSlope) return !isCut(id);
        if (c.role == RPlace) return !bandOn(id);
        if (c.role == RGain) return isCut(id) || (!isPost(id) && shapeOf(id) == kDNotch);
        if (c.role != ROn && !bandOn(id)) return true;
        return false;
    }

    // ---- display mapping ------------------------------------------------------------
    static float xOfHz(float hz) { return PX0 + (PX1 - PX0) * std::log(clampf(hz, 1.f, 1e5f) / FMIN) / std::log(FMAX / FMIN); }
    static float hzOfX(float x) { return FMIN * std::pow(FMAX / FMIN, (x - PX0) / (PX1 - PX0)); }
    static float yOfL(float l) { return YC - l / L_RANGE * HALF; }
    static float lOfY(float y) { return (YC - y) / HALF * L_RANGE; }
    static float yOfDb(float db) { return YC - db / DB_RANGE * HALF; }
    static float dbOfY(float y) { return (YC - y) / HALF * DB_RANGE; }
    void dotPos(int id, float& x, float& y) const {
        x = xOfHz(bv(id, RFreq));
        if (!isPost(id)) { y = shapeOf(id) == kDNotch ? yOfL(-2.2f) : yOfL(clampf(bv(id, RGain), -2.4f, 2.4f)); return; }
        const float db = isCut(id) ? 20.f * std::log10(bv(id, RQ) / 0.70710678f) : bv(id, RGain);
        y = yOfDb(clampf(db, -DB_RANGE, DB_RANGE));
    }

    // ---- drawing ------------------------------------------------------------
    void onNanoDisplay() override {
        const float s = uiScale();
        save(); scale(s, s);
        beginPath(); rect(0, 0, W, H); fillColor(C_BG); fill();
        fontFace(NANOVG_DEJAVU_SANS_TTF);
        drawLogo();
        fontSize(22); textLetterSpacing(9); fillColor(C_TEXT); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); text(58, 34, "GEODE", nullptr);
        fontSize(10); textLetterSpacing(2.5f); fillColor(C_DIM); text(188, 30, "REVERB", nullptr);
        fontSize(7.5f); textLetterSpacing(1.8f); fillColor(C_FAINT); text(188, 42, "EPSILON AUDIO  2.0", nullptr);
        drawPresetBar();
        for (int k = 0; k < 2; ++k) {
            const float x = A_X + k * AB_W; const bool on = abSlot == k, hot = hoverHeader == 6 + k;
            beginPath(); roundedRect(x, PS_Y, AB_W - 0.5f, PS_H, 5); fillColor(on ? withAlpha(C_ACC, 0.85f) : (hot ? C_PANEL_HI : C_PANEL)); fill(); strokeColor(on ? C_ACC : C_LINE); strokeWidth(1); stroke();
            fontSize(11); textLetterSpacing(0); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(on ? C_BG : C_DIM); text(x + AB_W / 2, PS_Y + PS_H / 2 + 0.5f, k ? "B" : "A", nullptr);
        }
        drawHeaderButton(COPY_X, COPY_W, "COPY", false, hoverHeader == 8, false);
        drawHeaderButton(UNDO_X, UR_W, "UNDO", false, hoverHeader == 3, undoStack.empty());
        drawHeaderButton(REDO_X, UR_W, "REDO", false, hoverHeader == 4, redoStack.empty());
        drawHeaderButton(LEARN_X, LEARN_W, learnMode ? "LEARNING" : "LEARN", learnMode, hoverHeader == 1, false);
        drawHeaderButton(MIDI_X, MIDI_W, "MIDI", val[kMidiOn] > 0.5f, hoverHeader == 2, false);
        drawHeaderButton(HELP_X, HELP_W, "HELP", helpOpen, hoverHeader == 5, false);
        drawMeters();

        drawDisplay();
        panel(AX, PY, AW, PH); panel(BX, PY, BW, PH);
        beginPath(); moveTo(AX + 14, PY + 196); lineTo(AX + AW - 14, PY + 196); moveTo(AX + 14, PY + 312); lineTo(AX + AW - 14, PY + 312); strokeColor(C_LINE); strokeWidth(1); stroke();
        fontSize(7.5f); textLetterSpacing(1.8f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(C_FAINT); text(AX + 16, PY + 10, "ALGORITHM", nullptr);
        if (primary() >= 0 && isPost(primary())) { fontSize(7.5f); textLetterSpacing(1.8f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(C_FAINT); text(BX + BW / 2, PY + 136, "CHANNEL", nullptr); }
        if (val[kFreeze] > 0.5f) { fontSize(9); textLetterSpacing(1.8f); textAlign(ALIGN_RIGHT | ALIGN_MIDDLE); fillColor(C_ACC); text(PX1, PTOP + 18, "FROZEN", nullptr); }
        drawBandPanel();
        const std::vector<Ctl>& cs = controls();
        for (int i = 0; i < (int)cs.size(); ++i) if (visible(cs[i])) drawCtl(cs[i], i == active || i == hover);
        if (editing >= 0) drawEditBox();
        drawStatus();
        if (menuOpen) drawPresetMenu();
        if (saving) drawSaveBox();
        if (helpOpen) drawHelp();
        else if (tipShown && gTipsOn) drawTooltip();
        restore();
        if (irBusy) irShown = true;
    }
    void drawLogo() {
        // a cracked-open geode: a stone shell with crystals pointing inwards
        const float cx = 34, cy = 34, r = 16;
        beginPath(); circle(cx, cy, r); fillColor(Color(0x4a, 0x44, 0x50)); fill();
        beginPath(); circle(cx, cy, r - 3.5f); fillColor(Color(0x2a, 0x1c, 0x3c)); fill();
        for (int k = 0; k < 9; ++k) {
            const float a = k * 2.f * kPi / 9.f + 0.2f, a1 = a - 0.28f, a2 = a + 0.28f;
            beginPath(); moveTo(cx + std::cos(a1) * (r - 3.5f), cy + std::sin(a1) * (r - 3.5f)); lineTo(cx + std::cos(a) * (r - 10.5f), cy + std::sin(a) * (r - 10.5f));
            lineTo(cx + std::cos(a2) * (r - 3.5f), cy + std::sin(a2) * (r - 3.5f)); closePath();
            fillColor(withAlpha(C_ACC, 0.55f + 0.05f * (k % 3))); fill();
        }
    }
    void panel(float x, float y, float w, float h) { beginPath(); roundedRect(x, y, w, h, 6); fillColor(C_PANEL); fill(); strokeColor(C_LINE); strokeWidth(1); stroke(); }
    void drawHeaderButton(float x, float w, const char* label, bool on, bool hot, bool disabled) {
        beginPath(); roundedRect(x, PS_Y, w, PS_H, 5); fillColor(on ? withAlpha(C_ACC, 0.2f) : C_PANEL); fill();
        strokeColor(on ? C_ACC : (hot && !disabled ? C_DIM : C_LINE)); strokeWidth(1); stroke();
        fontSize(9.5f); textLetterSpacing(2.f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(disabled ? C_FAINT : (on ? C_TEXT : C_DIM));
        text(x + w / 2, PS_Y + PS_H / 2 + 0.5f, label, nullptr);
    }
    // small input/output meters at the right of the header
    static constexpr float MTX = 990, MTW = 94;
    bool metersHit(float x, float y) const { return inRect(x, y, MTX, PS_Y, MTW, PS_H); }
    void drawMeters() {
        auto bar = [&](float y, float db, const char* lbl) {
            fontSize(7); textLetterSpacing(1.2f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(C_FAINT); text(MTX, y + 2.5f, lbl, nullptr);
            const float x0 = MTX + 26, w = MTW - 26;
            beginPath(); rect(x0, y, w, 5); fillColor(C_PANEL_HI); fill();
            const float n = clampf((db + 60.f) / 60.f, 0.f, 1.f);
            if (n > 0) { beginPath(); rect(x0, y, w * n, 5); fillColor(db > -0.1f ? C_DEL : C_ACC); fill(); }
        };
        bar(PS_Y + 6, std::max(val[kOutInL], val[kOutInR]), "IN");
        bar(PS_Y + 17, std::max(val[kOutOutL], val[kOutOutR]), "OUT");
    }

    void drawDisplay() {
        beginPath(); roundedRect(DX, DY, DW, DH, 6); fillColor(C_DISPLAY); fill(); strokeColor(C_LINE); strokeWidth(1); stroke();
        const float grid[] = { 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000 };
        const char* gl[] = { "20", "50", "100", "200", "500", "1k", "2k", "5k", "10k", "20k" };
        beginPath();
        for (float g : grid) { const float x = xOfHz(g); moveTo(x, PTOP); lineTo(x, PBOT); }
        for (int k = -2; k <= 2; ++k) { const float y = yOfL((float)k); moveTo(PX0, y); lineTo(PX1, y); }
        strokeColor(Color(255, 255, 255, 8 / 255.f)); strokeWidth(1); stroke();
        beginPath(); moveTo(PX0, YC); lineTo(PX1, YC); strokeColor(Color(255, 255, 255, 20 / 255.f)); stroke();
        fontSize(8.5f); textLetterSpacing(0.8f); fillColor(C_FAINT); textAlign(ALIGN_CENTER | ALIGN_BOTTOM);
        if (val[kPiano] > 0.5f) {
            // piano scale: a keyboard along the bottom, C notes labelled (A4 = 440 Hz)
            const float ky = PBOT - 9;
            for (int note = 12; note <= 135; ++note) {           // C0 .. D#10
                const float f = 440.f * std::exp2((note - 69) / 12.f); if (f < FMIN || f > FMAX) continue;
                const int pc = note % 12; const bool black = pc == 1 || pc == 3 || pc == 6 || pc == 8 || pc == 10;
                const float x0 = xOfHz(f * std::exp2(-1.f / 24.f)), x1 = xOfHz(f * std::exp2(1.f / 24.f));
                beginPath(); rect(x0 + 0.3f, ky, std::max(0.6f, x1 - x0 - 0.6f), 9); fillColor(black ? Color(0x1c, 0x19, 0x22) : Color(0x4a, 0x45, 0x55)); fill();
                if (pc == 0) { char nb[8]; snprintf(nb, sizeof nb, "C%d", note / 12 - 1); text(xOfHz(f), DY + DH - 5, nb, nullptr); }
            }
        } else for (int i = 0; i < 10; ++i) text(xOfHz(grid[i]), DY + DH - 5, gl[i], nullptr);
        // scales: post EQ dB (left), decay time (right)
        const bool post = layer == 1;
        const float T0 = decayTime(val);
        fontSize(8.5f); textAlign(ALIGN_RIGHT | ALIGN_MIDDLE); fillColor(withAlpha(C_ACC, post ? 0.9f : 0.45f));
        for (int db = -24; db <= 24; db += 12) { char b[8]; snprintf(b, sizeof b, db > 0 ? "+%d" : "%d", db); text(PX0 - 6, yOfDb((float)db), b, nullptr); }
        textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(withAlpha(C_DECAY, post ? 0.45f : 0.9f));
        for (int k = -2; k <= 2; ++k) { char b[12]; const float t = T0 * std::exp2((float)k); snprintf(b, sizeof b, t < 10 ? "%.2f s" : "%.1f s", t); text(PX1 + 6, yOfL((float)k), b, nullptr); }

        // analyser: the reverb after the post EQ (own dB scale: 0 dBFS at the top, -90 at the bottom, 3 dB/oct tilt around 1 kHz)
        if (val[kAnalyzer] > 0.5f && anaValid) {
            const float sr = sampleRateUI;
            beginPath(); moveTo(PX0, PBOT);
            const int cols = 300;
            for (int i = 0; i <= cols; ++i) {
                const float f0 = hzOfX(PX0 + (PX1 - PX0) * (float)i / cols), f1 = hzOfX(PX0 + (PX1 - PX0) * (float)(i + 1) / cols);
                int k0 = (int)(f0 / sr * ANA_N), k1 = std::max(k0 + 1, (int)(f1 / sr * ANA_N));
                k0 = std::max(1, std::min(k0, ANA_N / 2 - 1)); k1 = std::max(k0 + 1, std::min(k1, ANA_N / 2));
                float m = -120; for (int k = k0; k < k1; ++k) m = std::max(m, spec[k]);
                const float tilt = 3.f * std::log2(std::max(f0, 1.f) / 1000.f);
                lineTo(PX0 + (PX1 - PX0) * (float)i / cols, clampf(PTOP + (PBOT - PTOP) * (-(m + tilt) / 90.f), PTOP, PBOT));
            }
            lineTo(PX1, PBOT); closePath();
            fillPaint(linearGradient(0, PTOP, 0, PBOT, withAlpha(C_ANA, 0.5f), withAlpha(C_ANA, 0.08f))); fill();
        }

        const int N = 360;
        scissor(DX + 1, PTOP - 6, DW - 2, PBOT - PTOP + 12);
        // the selected post band's own curve, filled
        const int pf = primary();
        if (pf >= 0 && isPost(pf)) {
            const int sh = shapeOf(pf), sl = (int)(bv(pf, RSlope) + .5f);
            beginPath();
            for (int i = 0; i <= N; ++i) {
                const float x = PX0 + (PX1 - PX0) * i / N;
                const float db = 20 * std::log10(std::max(std::abs(postResponse(sh, sl, bv(pf, RFreq), bv(pf, RGain), bv(pf, RQ), hzOfX(x))), 1e-5f));
                const float y = clampf(yOfDb(db), PTOP - 40, PBOT + 40); if (!i) moveTo(x, y); else lineTo(x, y);
            }
            lineTo(PX1, YC); lineTo(PX0, YC); closePath(); fillColor(withAlpha(C_ACC, bandOn(pf) ? 0.16f : 0.06f)); fill();
        }
        // post EQ curve
        beginPath();
        for (int i = 0; i <= N; ++i) {
            const float x = PX0 + (PX1 - PX0) * i / N;
            const float y = clampf(yOfDb(20 * std::log10(std::max(std::abs(postRespAt(val, hzOfX(x))), 1e-5f))), PTOP - 40, PBOT + 40);
            if (!i) moveTo(x, y); else lineTo(x, y);
        }
        strokeColor(withAlpha(C_ACC, post ? 1.f : 0.5f)); strokeWidth(post ? 2.2f : 1.5f); stroke();
        // decay time curve (what the network really does at each frequency)
        DecayCurve dc; dc.build(val, sampleRateUI);
        beginPath();
        for (int i = 0; i <= N; ++i) {
            const float x = PX0 + (PX1 - PX0) * i / N;
            const float y = clampf(yOfL(std::log2(dc.timeAt(hzOfX(x)) / T0)), PTOP - 40, PBOT + 40);
            if (!i) moveTo(x, y); else lineTo(x, y);
        }
        strokeColor(withAlpha(C_DECAY, post ? 0.5f : 1.f)); strokeWidth(post ? 1.5f : 2.2f); stroke();
        resetScissor();
        // dots: the inactive layer first, small
        for (int pass = 0; pass < 2; ++pass)
            for (int id = 0; id < kBands; ++id) {
                if (!used(id) || (isPost(id) == post) != (pass == 1)) continue;
                float x, y; dotPos(id, x, y);
                const bool act = pass == 1, isSel = id == focus, on = bandOn(id), hot = hoverDot == id;
                const Color col = isPost(id) ? C_ACC : C_DECAY;
                const float r = act ? 10.f : 6.f;
                if (act && (isSel || hot)) { beginPath(); circle(x, y, 16); fillColor(withAlpha(col, 0.2f)); fill(); }
                beginPath(); circle(x, y, r); fillColor(on ? withAlpha(col, act ? 1.f : 0.5f) : C_PANEL_HI); fill();
                if (isSel) { strokeColor(C_TEXT); strokeWidth(2); stroke(); } else if (!on) { strokeColor(col); strokeWidth(1.2f); stroke(); }
                if (act) { char t[4]; snprintf(t, sizeof t, "%d", id % kDecayBands + 1);
                    fontSize(9.5f); textLetterSpacing(0); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(on ? C_BG : col); text(x, y + 0.5f, t, nullptr); }
            }
        // readout for the dot under the mouse / being dragged
        const int rb = dragDot >= 0 ? dragDot : hoverDot;
        if (rb >= 0 && used(rb)) {
            float x, y; dotPos(rb, x, y);
            char f[24], g[24], q[24]; fmt(bandParam(rb, RFreq), f, sizeof f); fmt(bandParam(rb, RGain), g, sizeof g); fmt(bandParam(rb, RQ), q, sizeof q);
            int ne; const char* shape = enumLabels(bandParam(rb, RShape), ne)[shapeOf(rb)];
            char line[112];
            if (isCut(rb) || (!isPost(rb) && shapeOf(rb) == kDNotch)) snprintf(line, sizeof line, "%s %d  %s   %s   Q %s", isPost(rb) ? "POST" : "DECAY", rb % kDecayBands + 1, shape, f, q);
            else snprintf(line, sizeof line, "%s %d  %s   %s   %s   Q %s", isPost(rb) ? "POST" : "DECAY", rb % kDecayBands + 1, shape, f, g, q);
            fontSize(10); textLetterSpacing(0.4f);
            Rectangle<float> tb; textBounds(0, 0, line, nullptr, tb);
            const float w = tb.getWidth() + 16, bx = clampf(x - w / 2, DX + 6, DX + DW - w - 6), by = y > PTOP + 40 ? y - 42 : y + 20;
            beginPath(); roundedRect(bx, by, w, 20, 4); fillColor(Color(0x1e, 0x1b, 0x24, 0.95f)); fill(); strokeColor(withAlpha(isPost(rb) ? C_ACC : C_DECAY, 0.8f)); strokeWidth(1); stroke();
            textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(C_TEXT); text(bx + 8, by + 10.5f, line, nullptr);
        }
        // pills: which EQ the mouse edits, analyser
        pill(LP_X, LP_W0, "DECAY RATE EQ", layer == 0, hoverPill == 1, C_DECAY);
        pill(LP_X + LP_W0 + 6, LP_W1, "POST EQ", layer == 1, hoverPill == 2, C_ACC);
        pill(AN_X, AN_W, "ANALYZER", val[kAnalyzer] > 0.5f, hoverPill == 3, C_ANA);
        pill(PI_X, PI_W, "PIANO", val[kPiano] > 0.5f, hoverPill == 4, C_ANA);
        pill(IR_X, IR_W, irBusy ? "ANALYSING..." : "IMPORT IR", irBusy, hoverPill == 5, C_DECAY);
        // the space, and its decay time
        char b[80]; snprintf(b, sizeof b, "%s   ·   %.2f s at 1 kHz", kSpaces[spaceIndex(val[kSpace])].name, dc.timeAt(1000.f));
        fontSize(10.5f); textLetterSpacing(0.8f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(C_DIM); text(DX + DW / 2, LP_Y + LP_H / 2 + 0.5f, b, nullptr);
    }
    void pill(float x, float w, const char* label, bool on, bool hot, const Color& col) {
        beginPath(); roundedRect(x, LP_Y, w, LP_H, LP_H / 2); fillColor(on ? withAlpha(col, 0.22f) : (hot ? C_PANEL_HI : C_PANEL)); fill();
        strokeColor(on ? col : C_LINE); strokeWidth(1); stroke();
        fontSize(8.5f); textLetterSpacing(1.6f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(on ? C_TEXT : C_DIM); text(x + w / 2, LP_Y + LP_H / 2 + 0.5f, label, nullptr);
    }
    int pillHit(float x, float y) const {
        if (y < LP_Y || y > LP_Y + LP_H) return 0;
        if (x >= LP_X && x < LP_X + LP_W0) return 1;
        if (x >= LP_X + LP_W0 + 6 && x < LP_X + LP_W0 + 6 + LP_W1) return 2;
        if (x >= AN_X && x < AN_X + AN_W) return 3;
        if (x >= PI_X && x < PI_X + PI_W) return 4;
        if (x >= IR_X && x < IR_X + IR_W) return 5;
        return 0;
    }
    void drawBandPanel() {
        if (primary() >= 0) return;
        fontSize(12); textLetterSpacing(0.5f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(C_DIM);
        text(BX + BW / 2, PY + PH / 2 - 22, layer == 0 ? "DECAY RATE EQ" : "POST EQ", nullptr);
        fontSize(10.5f); fillColor(C_FAINT);
        text(BX + BW / 2, PY + PH / 2 + 2, "Double-click the display to add a band,", nullptr);
        text(BX + BW / 2, PY + PH / 2 + 20, "or click a dot to edit it.  Up to 6 bands each.", nullptr);
    }

    void drawKnobBody(float x, float y, float r, float n, bool bipolar, bool hot, Color col, bool dim) {
        const float a0 = 0.75f * 3.14159265f, a1 = 2.25f * 3.14159265f, ang = a0 + (a1 - a0) * n;
        beginPath(); arc(x, y, r, a0, a1, NanoVG::CW); strokeColor(C_LINE); strokeWidth(r > 34 ? 6.f : (r > 24 ? 5.f : 3.5f)); lineCap(ROUND); stroke();
        const float from = bipolar ? (a0 + a1) / 2 : a0;
        if (std::fabs(ang - from) > 0.001f) { beginPath(); arc(x, y, r, std::min(from, ang), std::max(from, ang), NanoVG::CW); strokeColor(dim ? C_FAINT : col); stroke(); }
        const float cr = r - (r > 34 ? 11 : (r > 24 ? 9 : 7));
        beginPath(); circle(x, y, cr);
        fillPaint(radialGradient(x - cr * 0.3f, y - cr * 0.4f, 0, cr * 1.4f, hot ? Color(0x3c, 0x37, 0x46) : C_PANEL_HI, Color(0x14, 0x12, 0x18)));
        fill(); strokeColor(Color(255, 255, 255, 18 / 255.f)); strokeWidth(1); stroke();
        beginPath(); moveTo(x + std::cos(ang) * cr * 0.35f, y + std::sin(ang) * cr * 0.35f); lineTo(x + std::cos(ang) * cr * 0.9f, y + std::sin(ang) * cr * 0.9f);
        strokeColor(dim ? C_DIM : C_TEXT); strokeWidth(r > 24 ? 2.5f : 2.f); lineCap(ROUND); stroke();
    }
    const char* const* segNames(const Ctl& c, int& n) const {
        static const char* const dshape[] = { "BELL", "LO SHELF", "HI SHELF", "NOTCH" };
        static const char* const pshape[] = { "BELL", "LO SHELF", "HI SHELF", "LO CUT", "HI CUT" };
        static const char* const slope[] = { "6", "12", "24", "48", "96" };
        if (c.role == RShape) { const bool post = primary() >= 0 ? isPost(primary()) : layer == 1; n = post ? 5 : 4; return post ? pshape : dshape; }
        static const char* const place[] = { "STEREO", "MID", "SIDE", "LEFT", "RIGHT" };
        static const char* const alg[] = { "MODERN", "VINTAGE", "PLATE" };
        if (c.role == RSlope) { n = 5; return slope; }
        if (c.role == RPlace) { n = kNumPlaces; return place; }
        if (c.param == kAlgorithm && c.role < 0) { n = kNumAlgorithms; return alg; }
        n = 0; return nullptr;
    }
    void drawCtl(const Ctl& c, bool hot) {
        const uint32_t p = effParam(c); if (p >= kParamCount && c.type != DelButton) return;
        const bool dim = dimmed(c); char buf2[48];
        const Color col = c.role >= 0 && primary() >= 0 ? (isPost(primary()) ? C_ACC : C_DECAY) : C_ACC;
        switch (c.type) {
        case Knob: {
            const bool bip = c.role == RGain || p == kOutput;
            drawKnobBody(c.x, c.y, c.r, toNorm(p, val[p]), bip, hot, col, dim);
            const char* label = c.label;
            if (c.role == RGain) label = primary() >= 0 && !isPost(primary()) ? "DECAY" : "GAIN";
            if (c.param == kPredelay && p == kPredelayNote) label = "PREDELAY (SYNC)";
            const bool big = c.r >= 28;
            fontSize(big ? 8.5f : 8.f); textLetterSpacing(1.6f); textAlign(ALIGN_CENTER | ALIGN_TOP);
            fillColor(dim ? C_FAINT : C_DIM); text(c.x, c.y + c.r + 6, label, nullptr);
            fmt(p, buf2, sizeof buf2); fontSize(big ? 11.f : 10.f); textLetterSpacing(0.4f); fillColor(dim ? C_FAINT : (hot ? col : C_TEXT)); text(c.x, c.y + c.r + 19, buf2, nullptr);
            break; }
        case Toggle: {
            const bool on = val[p] > 0.5f;
            const float w = 34, h = 18, x = c.x - w / 2, y = c.y - h / 2;
            beginPath(); roundedRect(x, y, w, h, h / 2); fillColor(on ? withAlpha(col, 0.3f) : C_PANEL_HI); fill(); strokeColor(on ? col : C_LINE); strokeWidth(1); stroke();
            beginPath(); circle(on ? x + w - h / 2 : x + h / 2, c.y, 6); fillColor(on ? col : C_DIM); fill();
            char t[32];
            if (c.role == ROn) snprintf(t, sizeof t, "%s BAND %d", isPost(primary()) ? "POST" : "DECAY", primary() % kDecayBands + 1);
            else snprintf(t, sizeof t, "%s", c.label);
            fontSize(c.role == ROn ? 11.f : 9.f); textLetterSpacing(2.f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(on ? C_TEXT : C_DIM); text(c.x + w / 2 + 10, c.y + 0.5f, t, nullptr);
            break; }
        case Segment: {
            int n = 0; const char* const* names = segNames(c, n);
            const int cur = (int)(val[p] + .5f); const float sw = c.r / n, x0 = c.x - c.r / 2;
            for (int i = 0; i < n; ++i) {
                const float x = x0 + sw * i;
                beginPath(); roundedRect(x + 1.5f, c.y - SEG_H / 2, sw - 3, SEG_H, 5); fillColor(i == cur ? (dim ? C_FAINT : col) : C_PANEL_HI); fill();
                fontSize(9); textLetterSpacing(1.4f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(i == cur ? C_BG : (dim ? C_FAINT : C_DIM)); text(x + sw / 2, c.y + 0.5f, names[i], nullptr);
            }
            if (c.role == RSlope) { fontSize(7.5f); textLetterSpacing(1.6f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(C_FAINT); text(c.x + c.r / 2 + 8, c.y, "DB/OCT", nullptr); }
            break; }
        case Button: {
            const bool on = val[p] > 0.5f;
            beginPath(); roundedRect(c.x - c.r / 2, c.y - 13, c.r, 26, 13); fillColor(on ? withAlpha(C_ACC, 0.85f) : (hot ? C_PANEL_HI : C_PANEL)); fill(); strokeColor(on || hot ? C_ACC : C_LINE); strokeWidth(1); stroke();
            // a small snowflake
            const float sx = c.x - c.r / 2 + 20, sy = c.y;
            beginPath(); for (int k = 0; k < 3; ++k) { const float a = k * kPi / 3; moveTo(sx - 6 * std::cos(a), sy - 6 * std::sin(a)); lineTo(sx + 6 * std::cos(a), sy + 6 * std::sin(a)); }
            strokeColor(on ? C_BG : C_DIM); strokeWidth(1.4f); stroke();
            fontSize(9.5f); textLetterSpacing(2.2f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(on ? C_BG : C_DIM); text(c.x + 8, c.y + 0.5f, c.label, nullptr);
            break; }
        case DelButton: {
            beginPath(); roundedRect(c.x - 14, c.y - 12, 28, 24, 5); fillColor(hot ? withAlpha(C_DEL, 0.3f) : C_PANEL_HI); fill(); strokeColor(hot ? C_DEL : C_LINE); strokeWidth(1); stroke();
            beginPath(); moveTo(c.x - 5, c.y - 5); lineTo(c.x + 5, c.y + 5); moveTo(c.x + 5, c.y - 5); lineTo(c.x - 5, c.y + 5); strokeColor(hot ? C_TEXT : C_DIM); strokeWidth(1.6f); stroke();
            break; }
        }
        if (learnMode && c.type != DelButton) {
            const int cc = paramCC[p]; const bool waiting = learnWaiting == (int)p;
            if (cc >= 0 || waiting) {
                const float by = c.type == Knob ? c.y - c.r - 12 : c.y - SEG_H / 2 - 8;
                snprintf(buf2, sizeof buf2, waiting ? "?" : "CC%d", cc);
                fontSize(8.5f); textLetterSpacing(0.5f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE);
                const float bw = waiting ? 16 : 34;
                beginPath(); roundedRect(c.x - bw / 2, by - 7, bw, 14, 3); fillColor(waiting ? C_ACC : C_PANEL_HI); fill(); strokeColor(C_ACC); strokeWidth(1); stroke();
                fillColor(waiting ? C_BG : C_ACC); text(c.x, by + 0.5f, buf2, nullptr);
            }
        }
    }
    void drawStatus() {
        std::string msg;
        if (statusFlash > 0) msg = statusText;
        else if (learnMode) msg = learnWaiting >= 0 ? std::string("Move a MIDI controller to assign: ") + paramDef(learnWaiting).name : "MIDI Learn: click a control, then move a controller.  Right-click a control to unassign.";
        if (!msg.empty()) { fontSize(10); textLetterSpacing(0.8f); textAlign(ALIGN_RIGHT | ALIGN_TOP); fillColor(C_ACC); text(PX1, PTOP + 4, msg.c_str(), nullptr); }
        if (learnMode && learnWaiting < 0) { textAlign(ALIGN_RIGHT | ALIGN_BOTTOM); fillColor(hoverClear ? C_TEXT : C_DIM); text(PX1, PBOT - 4, "CLEAR ALL ASSIGNMENTS", nullptr); }
        if (statusFlash > 0) { --statusFlash; repaint(); }
    }
    void flash(const std::string& s) { statusText = s; statusFlash = 60; repaint(); }
    void drawEditBox() {
        const Ctl& c = controls()[editing];
        const float w = 100, h = 22, x = c.x - w / 2, y = c.y + c.r + 4;
        beginPath(); roundedRect(x, y, w, h, 4); fillColor(C_DISPLAY); fill(); strokeColor(C_ACC); strokeWidth(1); stroke();
        fontSize(12); textLetterSpacing(0.5f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(C_TEXT); text(c.x, y + h / 2 + 0.5f, (editBuf + "|").c_str(), nullptr);
    }

    // ---- presets ------------------------------------------------------------------
    bool matchesValues(const float* v) const {
        const bool lock = val[kMixLock] > 0.5f;
        for (uint32_t i = 0; i < kParamCount; ++i) {
            if (isOut(i) || isView(i) || (lock && i == kMix)) continue;
            if (std::fabs(val[i] - v[i]) > 1e-4f * std::max(1.f, std::fabs(v[i]))) return false;
        }
        return true;
    }
    void applyValues(const float* v) {
        float w[kParamCount]; std::copy(v, v + kParamCount, w);
        if (val[kMixLock] > 0.5f) w[kMix] = val[kMix];
        beginGesture(); applyAll(w); endGesture(); focus = -1;
    }
    void applyFactory(int idx) { idx = (idx % (int)kNumPresets + (int)kNumPresets) % (int)kNumPresets; applyValues(presets()[idx].v); curFactory = idx; curUser.clear(); repaint(); }
    void applyUser(const std::string& name) { float v[kParamCount]; std::copy(defv, defv + kParamCount, v); if (!loadUserPreset(name, v)) return; applyValues(v); std::copy(v, v + kParamCount, userVals); curUser = name; curFactory = -1; repaint(); }
    void programLoaded(uint32_t index) override { curFactory = (int)index; curUser.clear(); focus = -1; repaint(); }
    void step(int d) {
        const int nf = (int)kNumPresets, nu = (int)userPresets.size(), total = nf + nu;
        int cur = curFactory; if (!curUser.empty()) for (int i = 0; i < nu; ++i) if (userPresets[i] == curUser) cur = nf + i;
        if (cur < 0) cur = 0;
        const int next = ((cur + d) % total + total) % total;
        if (next < nf) applyFactory(next); else applyUser(userPresets[next - nf]);
    }
    void drawPresetBar() {
        const float x = PS_X, y = PS_Y, w = PS_W, h = PS_H, a = PS_ARROW;
        beginPath(); roundedRect(x, y, w, h, 5); fillColor(C_PANEL); fill(); strokeColor(menuOpen ? C_ACC : C_LINE); strokeWidth(1); stroke();
        beginPath(); moveTo(x + a, y + 5); lineTo(x + a, y + h - 5); moveTo(x + w - a, y + 5); lineTo(x + w - a, y + h - 5); strokeColor(C_LINE); stroke();
        for (int side = 0; side < 2; ++side) {
            const bool hot = presetHover == (side == 0 ? 0 : 2);
            const float cx = side == 0 ? x + a / 2 : x + w - a / 2, cy = y + h / 2, d = side == 0 ? -1.f : 1.f;
            beginPath(); moveTo(cx - 3 * d, cy - 5); lineTo(cx + 3 * d, cy); lineTo(cx - 3 * d, cy + 5); closePath(); fillColor(hot ? C_ACC : C_DIM); fill();
        }
        std::string name; bool modified = false;
        if (!curUser.empty()) { name = curUser; modified = !matchesValues(userVals); }
        else if (curFactory >= 0) { name = presets()[curFactory].name; modified = !matchesValues(presets()[curFactory].v); }
        else name = "Custom";
        if (modified) name += " *";
        fontSize(12); textLetterSpacing(1.f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(presetHover == 1 || menuOpen ? C_TEXT : Color(0xd6, 0xd2, 0xdc));
        text(x + w / 2, y + h / 2 + 0.5f, name.c_str(), nullptr);
    }
    // preset browser: categories, user presets and favourites in columns; typing searches names and categories.
    // kind: 0 factory, 1 user, 2 save button, 3 search field, 4 favourite (idx into favs)
    struct MRect { float x, y, w, h; int kind; int idx; };
    static std::string lower(const std::string& s) { std::string r(s); for (char& c : r) c = (char)std::tolower((unsigned char)c); return r; }
    std::string favKey(int kind, int idx) const { return kind == 0 ? std::string("f:") + presets()[idx].name : (kind == 1 ? "u:" + userPresets[idx] : (idx < (int)favs.size() ? favs[idx] : std::string())); }
    bool isFav(const std::string& k) const { return std::find(favs.begin(), favs.end(), k) != favs.end(); }
    void toggleFav(const std::string& k) {
        if (k.empty()) return;
        auto it = std::find(favs.begin(), favs.end(), k); if (it != favs.end()) favs.erase(it); else favs.push_back(k);
        saveFavourites(favs); repaint();
    }
    static constexpr int kMenuCols = kNumCategories + 2, kMenuRows = 16;
    void menuLayout(float& mx, float& my, float& mw, float& mh, std::vector<MRect>& items) const {
        mw = PM_COLW * kMenuCols + 2 * PM_PAD; mx = std::min(W - mw - 8, std::max(8.f, PS_X + PS_W / 2 - mw / 2)); my = PS_Y + PS_H + 4;
        items.clear();
        const float top = my + PM_PAD + PM_ROW + 6 + PM_HEAD;
        int maxRows = 1;
        items.push_back({ mx + PM_PAD + PM_COLW, my + PM_PAD, 300, PM_ROW, 3, 0 });
        items.push_back({ mx + mw - PM_PAD - 150, my + PM_PAD, 150, PM_ROW, 2, 0 });
        if (!query.empty()) {
            const std::string q = lower(query); int k = 0;
            auto place = [&](int kind, int idx) { const int col = k / kMenuRows, row = k % kMenuRows; if (col >= kMenuCols) return; items.push_back({ mx + PM_PAD + col * PM_COLW, top + row * PM_ROW, PM_COLW - 4, PM_ROW, kind, idx }); maxRows = std::max(maxRows, row + 1); ++k; };
            for (uint32_t i = 0; i < kNumPresets; ++i) if (lower(presets()[i].name).find(q) != std::string::npos || lower(presets()[i].category).find(q) != std::string::npos) place(0, (int)i);
            for (int u = 0; u < (int)userPresets.size(); ++u) if (lower(userPresets[u]).find(q) != std::string::npos || std::string("user").find(q) != std::string::npos) place(1, u);
        } else {
            int rows[kNumCategories] = {};
            for (uint32_t i = 0; i < kNumPresets; ++i) {
                const char* cat = presets()[i].category;
                if (std::strcmp(cat, "Init") == 0) { items.push_back({ mx + PM_PAD, my + PM_PAD, PM_COLW - 4, PM_ROW, 0, (int)i }); continue; }
                for (int c = 0; c < kNumCategories; ++c)
                    if (std::strcmp(cat, kCategories[c]) == 0) { items.push_back({ mx + PM_PAD + c * PM_COLW, top + rows[c] * PM_ROW, PM_COLW - 4, PM_ROW, 0, (int)i }); maxRows = std::max(maxRows, ++rows[c]); }
            }
            for (int u = 0; u < (int)userPresets.size() && u < kMenuRows; ++u) items.push_back({ mx + PM_PAD + kNumCategories * PM_COLW, top + u * PM_ROW, PM_COLW - 4, PM_ROW, 1, u });
            maxRows = std::max(maxRows, std::min(kMenuRows, (int)userPresets.size()));
            for (int f = 0; f < (int)favs.size() && f < kMenuRows; ++f) items.push_back({ mx + PM_PAD + (kNumCategories + 1) * PM_COLW, top + f * PM_ROW, PM_COLW - 4, PM_ROW, 4, f });
            maxRows = std::max(maxRows, std::min(kMenuRows, (int)favs.size()));
        }
        mh = PM_PAD + PM_ROW + 6 + PM_HEAD + maxRows * PM_ROW + PM_PAD;
    }
    std::string itemLabel(const MRect& r) const {
        if (r.kind == 0) return presets()[r.idx].name;
        if (r.kind == 1) return userPresets[r.idx];
        if (r.kind == 4 && r.idx < (int)favs.size()) return favs[r.idx].substr(2);
        return "";
    }
    void drawPresetMenu() {
        float mx, my, mw, mh; std::vector<MRect> items; menuLayout(mx, my, mw, mh, items);
        beginPath(); rect(mx - 12, my - 4, mw + 24, mh + 20); fillPaint(boxGradient(mx, my + 2, mw, mh, 6, 14, Color(0, 0, 0, 0.6f), Color(0, 0, 0, 0.f))); fill();
        beginPath(); roundedRect(mx, my, mw, mh, 6); fillColor(C_PANEL_HI); fill(); strokeColor(C_LINE); strokeWidth(1); stroke();
        beginPath(); moveTo(mx + PM_PAD, my + PM_PAD + PM_ROW + 3); lineTo(mx + mw - PM_PAD, my + PM_PAD + PM_ROW + 3); strokeColor(C_LINE); stroke();
        fontSize(8.5f); textLetterSpacing(1.8f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE);
        const float hy = my + PM_PAD + PM_ROW + 6 + PM_HEAD / 2;
        if (query.empty()) {
            for (int c = 0; c < kMenuCols; ++c) {
                std::string up = c < kNumCategories ? kCategories[c] : (c == kNumCategories ? "User" : "Favourites"); for (char& ch : up) ch = (char)std::toupper((unsigned char)ch);
                fillColor(c < kNumCategories ? C_ACC : C_DECAY); text(mx + PM_PAD + c * PM_COLW + 12, hy, up.c_str(), nullptr);
            }
            fontSize(10); textLetterSpacing(0.4f); fillColor(C_FAINT);
            if (userPresets.empty()) text(mx + PM_PAD + kNumCategories * PM_COLW + 12, hy + PM_HEAD / 2 + PM_ROW / 2, "(none saved yet)", nullptr);
            if (favs.empty()) text(mx + PM_PAD + (kNumCategories + 1) * PM_COLW + 12, hy + PM_HEAD / 2 + PM_ROW / 2, "(click a star)", nullptr);
        } else {
            int found = 0; for (const MRect& r : items) if (r.kind == 0 || r.kind == 1) ++found;
            char b[64]; snprintf(b, sizeof b, found ? "%d MATCHING PRESET%s" : "NO PRESET MATCHES", found, found == 1 ? "" : "S"); fillColor(C_ACC); text(mx + PM_PAD + 12, hy, b, nullptr);
        }
        for (int i = 0; i < (int)items.size(); ++i) {
            const MRect& r = items[i]; const bool hot = i == menuHover;
            if (r.kind == 2) {
                beginPath(); roundedRect(r.x, r.y + 1, r.w, r.h - 2, 4); fillColor(hot ? withAlpha(C_ACC, 0.25f) : C_PANEL); fill(); strokeColor(C_ACC); strokeWidth(1); stroke();
                fontSize(9.5f); textLetterSpacing(1.6f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(C_TEXT); text(r.x + r.w / 2, r.y + r.h / 2 + 0.5f, "SAVE CURRENT AS...", nullptr);
                continue;
            }
            if (r.kind == 3) {             // search field
                beginPath(); roundedRect(r.x, r.y + 1, r.w, r.h - 2, 4); fillColor(C_DISPLAY); fill(); strokeColor(query.empty() ? C_LINE : C_ACC); strokeWidth(1); stroke();
                beginPath(); circle(r.x + 12, r.y + r.h / 2, 4); moveTo(r.x + 15, r.y + r.h / 2 + 3); lineTo(r.x + 18, r.y + r.h / 2 + 6); strokeColor(C_DIM); strokeWidth(1.3f); stroke();
                fontSize(11); textLetterSpacing(0.3f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(query.empty() ? C_FAINT : C_TEXT);
                text(r.x + 26, r.y + r.h / 2 + 0.5f, query.empty() ? "Type to search names and categories" : (query + "|").c_str(), nullptr);
                continue;
            }
            const std::string key = favKey(r.kind, r.idx);
            const bool isCur = r.kind == 0 ? (curUser.empty() && r.idx == curFactory) : (r.kind == 1 ? curUser == userPresets[r.idx] : false);
            const bool del = r.kind == 1 && pendingDelete == r.idx;
            if (hot || del) { beginPath(); roundedRect(r.x, r.y, r.w, r.h, 4); fillColor(del ? Color(0xd0, 0x50, 0x40, 0.35f) : withAlpha(C_ACC, 0.18f)); fill(); }
            if (isCur) { beginPath(); circle(r.x + 6, r.y + r.h / 2, 2.5f); fillColor(C_ACC); fill(); }
            fontSize(11); textLetterSpacing(0.3f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(hot || isCur || del ? C_TEXT : C_DIM);
            const std::string label = del ? "Delete? click again" : itemLabel(r);
            text(r.x + 12, r.y + r.h / 2 + 0.5f, label.c_str(), nullptr);
            // the favourite star
            const bool fav = isFav(key);
            if (fav || hot) drawStar(r.x + r.w - 10, r.y + r.h / 2, 5.f, fav);
        }
    }
    void drawStar(float cx, float cy, float rr, bool filled) {
        beginPath();
        for (int k = 0; k < 10; ++k) { const float a = -kPi / 2 + k * kPi / 5, q = k % 2 ? rr * 0.45f : rr; if (!k) moveTo(cx + q * std::cos(a), cy + q * std::sin(a)); else lineTo(cx + q * std::cos(a), cy + q * std::sin(a)); }
        closePath();
        if (filled) { fillColor(C_DECAY); fill(); } else { strokeColor(C_DIM); strokeWidth(1); stroke(); }
    }
    int menuHit(float x, float y) const { float mx, my, mw, mh; std::vector<MRect> it; menuLayout(mx, my, mw, mh, it);
        for (int i = 0; i < (int)it.size(); ++i) if (x >= it[i].x && x < it[i].x + it[i].w && y >= it[i].y && y < it[i].y + it[i].h) return i;
        return -1; }
    MRect menuItem(int i) const { float mx, my, mw, mh; std::vector<MRect> it; menuLayout(mx, my, mw, mh, it); return it[i]; }
    void openMenu() { userPresets = listUserPresets(); favs = loadFavourites(); query.clear(); menuOpen = true; menuHover = -1; pendingDelete = -1; repaint(); }
    void applyFavourite(int idx) {
        if (idx >= (int)favs.size()) return;
        const std::string& k = favs[idx];
        if (k.compare(0, 2, "f:") == 0) { for (uint32_t i = 0; i < kNumPresets; ++i) if (k.substr(2) == presets()[i].name) { applyFactory((int)i); return; } }
        else applyUser(k.substr(2));
    }
    void drawSaveBox() {
        const float w = 360, h = 96, x = W / 2 - w / 2, y = 130;
        beginPath(); rect(0, 0, W, H); fillColor(Color(0, 0, 0, 0.45f)); fill();
        beginPath(); roundedRect(x, y, w, h, 8); fillColor(C_PANEL_HI); fill(); strokeColor(C_ACC); strokeWidth(1); stroke();
        fontSize(9.5f); textLetterSpacing(2.f); textAlign(ALIGN_LEFT | ALIGN_TOP); fillColor(C_DIM); text(x + 16, y + 14, "SAVE PRESET AS", nullptr);
        beginPath(); roundedRect(x + 16, y + 36, w - 32, 26, 4); fillColor(C_DISPLAY); fill(); strokeColor(C_LINE); stroke();
        fontSize(13); textLetterSpacing(0.4f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(C_TEXT); text(x + 26, y + 49.5f, (saveName + "|").c_str(), nullptr);
        fontSize(8.5f); textLetterSpacing(1.2f); fillColor(C_FAINT); textAlign(ALIGN_RIGHT | ALIGN_TOP);
        text(x + w - 16, y + 72, saveError ? "COULD NOT SAVE - CHECK THE NAME" : "ENTER TO SAVE   ·   ESC TO CANCEL", nullptr);
    }

    // ---- impulse response import ---------------------------------------------------------------
    void uiFileBrowserSelected(const char* filename) override {
        if (!filename || !*filename) return;
        irPath = filename; irBusy = true; irShown = false; statusText = "Analysing the impulse response..."; statusFlash = 200; repaint();
    }
    void runIrImport() {
        WavData w; std::string err;
        if (!readWav(irPath.c_str(), w, err)) { irBusy = false; flashLong(std::string("Could not import: ") + err); return; }
        float p[kParamCount]; std::copy(val, val + kParamCount, p); IrReport rep;
        if (!matchIr(w, p, rep)) { irBusy = false; flashLong(std::string("Could not import: ") + rep.message); return; }
        beginGesture(); applyAll(p); endGesture(); focus = -1; layer = 0; irBusy = false;
        curFactory = -1; curUser.clear();
        flashLong(rep.message);
    }
    void flashLong(const std::string& s) { statusText = s; statusFlash = 240; repaint(); }

    // ---- analyser + tips (idle) ---------------------------------------------------------------
    void uiIdle() override {
        const double t = nowSec();
        if (irBusy && !irPath.empty() && irShown) { runIrImport(); irPath.clear(); }
        if (t - lastAna > 0.033) {
            lastAna = t;
            if (GeodePlugin* const dsp = (GeodePlugin*)getPluginInstancePointer()) {
                const Engine& e = dsp->engine;
                sampleRateUI = e.sampleRate();
                const uint32_t w = e.anaWrite.load(std::memory_order_acquire);
                if (val[kAnalyzer] > 0.5f && w != lastWrite) {
                    lastWrite = w;
                    for (int i = 0; i < ANA_N; ++i) {
                        const float hann = 0.5f - 0.5f * std::cos(2 * kPi * i / (ANA_N - 1));
                        buf[i] = cpx(e.ana[(w - ANA_N + i) & (Engine::kAnaSize - 1)] * hann, 0.f);
                    }
                    fft.run(buf.data(), false);
                    const float norm = 2.f / (ANA_N * 0.5f);
                    for (int k = 1; k < ANA_N / 2; ++k) {
                        const float db = 20.f * std::log10(std::max(std::abs(buf[k]) * norm, 1e-7f));
                        spec[k] = db > spec[k] ? db : std::max(db, spec[k] - 1.8f);
                    }
                    anaValid = true;
                } else if (anaValid) for (float& s : spec) s = std::max(-120.f, s - 1.8f);
            }
            repaint();
        }
        if (gTipsOn && !tipShown && tipKey >= 0 && tipKey != tipSuppressed && active < 0 && dragDot < 0 && !menuOpen && !saving && !helpOpen && editing < 0
            && t - tipSince > 0.6) { tipShown = true; repaint(); }
    }
    bool tipContent(std::string& title, std::string& value, std::string& body) const {
        if (tipKey < 0) return false;
        if (tipKey < 1000) {
            const Ctl& c = controls()[tipKey];
            if (c.type == DelButton) { title = targetTitle(kTipDelete); value.clear(); body = targetTip(kTipDelete); return true; }
            const uint32_t p = effParam(c); if (p >= kParamCount) return false;
            char b[40]; fmt(p, b, sizeof b);
            title = paramDef(p).name; value = b; body = paramTip(p);
            if (c.type == Knob) body += "\n\nDrag the middle up/down or grab the rim and turn. Double-click to type a value, Cmd/Ctrl-click to reset.";
            if (paramCC[p] >= 0) { char cc[32]; snprintf(cc, sizeof cc, "   ·   CC%d", paramCC[p]); value += cc; }
            return true;
        }
        int t = -1;
        if (tipKey >= 5000) t = kTipDisplay;
        else if (tipKey >= 4000) { const int k = tipKey - 4000; t = k == 1 ? kTipLayerDecay : (k == 2 ? kTipLayerPost : (k == 3 ? kTipAnalyzer : (k == 4 ? kTipPiano : kTipImport))); }
        else if (tipKey >= 3000) t = kTipDot;
        else if (tipKey >= 2000) t = kTipPrev + (tipKey - 2000);
        else if (tipKey == 1100) t = kTipMeters;
        else { const int hh = tipKey - 1000;
            t = hh == 1 ? kTipLearn : hh == 2 ? kTipMidi : hh == 3 ? kTipUndo : hh == 4 ? kTipRedo : hh == 5 ? kTipHelp : hh == 6 ? kTipA : hh == 7 ? kTipB : kTipCopy; }
        title = targetTitle(t); value.clear(); body = targetTip(t);
        if (t == kTipDot) { const int id = tipKey - 3000; char b[32]; snprintf(b, sizeof b, "%s band %d", isPost(id) ? "Post EQ" : "Decay EQ", id % kDecayBands + 1); title = b; }
        if (t == kTipUndo || t == kTipRedo) { const size_t n = t == kTipUndo ? undoStack.size() : redoStack.size(); char b[32]; snprintf(b, sizeof b, "%d step%s", (int)n, n == 1 ? "" : "s"); value = b; }
        return true;
    }
    void drawTooltip() {
        std::string title, value, body;
        if (!tipContent(title, value, body)) return;
        const float w = 320, pad = 12;
        fontSize(11); textLetterSpacing(0.2f); textLineHeight(1.3f);
        float bb[4]; textBoxBounds(0, 0, w - 2 * pad, body.c_str(), nullptr, bb);
        const float h = pad + 16 + 8 + (bb[3] - bb[1]) + pad;
        float x = tipX + 16, y = tipY + 20;
        if (x + w > W - 8) x = tipX - w - 12;
        if (y + h > H - 8) y = tipY - h - 12;
        x = clampf(x, 8, W - w - 8); y = clampf(y, 8, H - h - 8);
        beginPath(); rect(x - 10, y - 4, w + 20, h + 18); fillPaint(boxGradient(x, y + 3, w, h, 6, 14, Color(0, 0, 0, 0.55f), Color(0, 0, 0, 0.f))); fill();
        beginPath(); roundedRect(x, y, w, h, 6); fillColor(Color(0x1e, 0x1b, 0x24)); fill(); strokeColor(C_LINE); strokeWidth(1); stroke();
        fontSize(10); textLetterSpacing(1.6f); textAlign(ALIGN_LEFT | ALIGN_TOP); fillColor(C_TEXT);
        std::string up = title; for (char& ch : up) ch = (char)std::toupper((unsigned char)ch);
        text(x + pad, y + pad + 1, up.c_str(), nullptr);
        if (!value.empty()) { fontSize(10.5f); textLetterSpacing(0.4f); textAlign(ALIGN_RIGHT | ALIGN_TOP); fillColor(C_ACC); text(x + w - pad, y + pad + 1, value.c_str(), nullptr); }
        fontSize(11); textLetterSpacing(0.2f); textLineHeight(1.3f); textAlign(ALIGN_LEFT | ALIGN_TOP); fillColor(C_DIM);
        textBox(x + pad, y + pad + 24, w - 2 * pad, body.c_str(), nullptr);
        textLineHeight(1.f);
    }

    // ---- help window ---------------------------------------------------------------
    static constexpr float HX = 170, HY = 80, HW = 760, HH = 540, TAB_W = 180, TAB_H = 34;
    int helpHit(float x, float y) const {
        if (x < HX || x > HX + HW || y < HY || y > HY + HH) return -2;
        for (int i = 0; i < kNumHelpPages; ++i) if (x >= HX + 12 && x < HX + 12 + TAB_W && y >= HY + 56 + i * TAB_H && y < HY + 56 + (i + 1) * TAB_H) return i;
        const float by = HY + HH - 44;
        if (y >= by && y < by + 28) {
            if (x >= HX + 12 && x < HX + 12 + TAB_W) return 100;
            if (x >= HX + HW - 330 && x < HX + HW - 140) return 101;
            if (x >= HX + HW - 124 && x < HX + HW - 16) return 102;
        }
        if (x >= HX + HW - 40 && y < HY + 40) return 102;
        return -1;
    }
    void helpClick(float x, float y) {
        const int h = helpHit(x, y);
        if (h >= 0 && h < kNumHelpPages) helpPage = h;
        else if (h == 100) gTipsOn = !gTipsOn;
        else if (h == 101) openURL(kManualURL);
        else if (h == 102 || h == -2) helpOpen = false;
        repaint();
    }
    void drawHelp() {
        beginPath(); rect(0, 0, W, H); fillColor(Color(0, 0, 0, 0.55f)); fill();
        beginPath(); roundedRect(HX, HY, HW, HH, 10); fillColor(C_PANEL); fill(); strokeColor(C_LINE); strokeWidth(1); stroke();
        fontSize(13); textLetterSpacing(4.f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(C_TEXT); text(HX + 22, HY + 28, "GEODE HELP", nullptr);
        const float cx = HX + HW - 24, cy = HY + 26;
        beginPath(); moveTo(cx - 6, cy - 6); lineTo(cx + 6, cy + 6); moveTo(cx + 6, cy - 6); lineTo(cx - 6, cy + 6); strokeColor(helpHover == 102 ? C_TEXT : C_DIM); strokeWidth(1.6f); stroke();
        for (int i = 0; i < kNumHelpPages; ++i) {
            const float ty = HY + 56 + i * TAB_H;
            if (i == helpPage || i == helpHover) { beginPath(); roundedRect(HX + 12, ty + 2, TAB_W, TAB_H - 4, 5); fillColor(i == helpPage ? withAlpha(C_ACC, 0.18f) : C_PANEL_HI); fill(); }
            if (i == helpPage) { beginPath(); roundedRect(HX + 12, ty + 8, 3, TAB_H - 16, 1.5f); fillColor(C_ACC); fill(); }
            fontSize(11.5f); textLetterSpacing(0.6f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(i == helpPage ? C_TEXT : C_DIM);
            text(HX + 28, ty + TAB_H / 2 + 0.5f, kHelpPages[i].title, nullptr);
        }
        beginPath(); moveTo(HX + TAB_W + 26, HY + 56); lineTo(HX + TAB_W + 26, HY + HH - 56); strokeColor(C_LINE); strokeWidth(1); stroke();
        const float bx = HX + TAB_W + 46, bw = HW - TAB_W - 70;
        fontSize(15); textLetterSpacing(0.8f); textAlign(ALIGN_LEFT | ALIGN_TOP); fillColor(C_ACC); text(bx, HY + 60, kHelpPages[helpPage].title, nullptr);
        fontSize(12); textLetterSpacing(0.2f); textLineHeight(1.38f); fillColor(Color(0xcc, 0xc6, 0xd2));
        textBox(bx, HY + 92, bw, kHelpPages[helpPage].body, nullptr);
        textLineHeight(1.f);
        const float by = HY + HH - 44;
        beginPath(); moveTo(HX + 12, by - 10); lineTo(HX + HW - 12, by - 10); strokeColor(C_LINE); stroke();
        const float tw = 34, th = 18, tx = HX + 16, tyy = by + 14 - th / 2;
        beginPath(); roundedRect(tx, tyy, tw, th, th / 2); fillColor(gTipsOn ? withAlpha(C_ACC, 0.25f) : C_PANEL_HI); fill(); strokeColor(gTipsOn ? C_ACC : C_LINE); stroke();
        beginPath(); circle(gTipsOn ? tx + tw - th / 2 : tx + th / 2, by + 14, 6); fillColor(gTipsOn ? C_ACC : C_DIM); fill();
        fontSize(10); textLetterSpacing(1.6f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(helpHover == 100 ? C_TEXT : C_DIM); text(tx + tw + 10, by + 14.5f, "HOVER TIPS", nullptr);
        auto btn = [&](float x0, float w0, const char* label, bool hot, bool primaryBtn) {
            beginPath(); roundedRect(x0, by, w0, 28, 5); fillColor(primaryBtn ? (hot ? withAlpha(C_ACC, 0.35f) : withAlpha(C_ACC, 0.18f)) : (hot ? C_PANEL_HI : C_PANEL)); fill();
            strokeColor(primaryBtn ? C_ACC : C_LINE); strokeWidth(1); stroke();
            fontSize(9.5f); textLetterSpacing(1.8f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(C_TEXT); text(x0 + w0 / 2, by + 14.5f, label, nullptr);
        };
        btn(HX + HW - 330, 190, "FULL MANUAL (WEB)", helpHover == 101, false);
        btn(HX + HW - 124, 108, "CLOSE", helpHover == 102, true);
    }

    // ---- interaction --------------------------------------------------------------
    int hit(float x, float y, int* seg = nullptr) const {
        const std::vector<Ctl>& cs = controls();
        for (int i = 0; i < (int)cs.size(); ++i) {
            const Ctl& c = cs[i];
            if (!visible(c)) continue;
            switch (c.type) {
            case Knob: { const float rr = c.r + 6; if ((x - c.x) * (x - c.x) + (y - c.y) * (y - c.y) <= rr * rr) return i; break; }
            case Toggle: if (x >= c.x - 20 && x <= c.x + (c.role == ROn ? 150 : (c.param == kAutoGate ? 110 : 70)) && std::fabs(y - c.y) <= 12) return i; break;
            case DelButton: if (std::fabs(x - c.x) <= 14 && std::fabs(y - c.y) <= 12) return i; break;
            case Button: if (std::fabs(x - c.x) <= c.r / 2 && std::fabs(y - c.y) <= 13) return i; break;
            case Segment: { int n = 0; segNames(c, n); const float x0 = c.x - c.r / 2;
                if (x >= x0 && x < x0 + c.r && std::fabs(y - c.y) <= SEG_H / 2 + 1) { if (seg) *seg = (int)((x - x0) / (c.r / n)); return i; } break; }
            }
        }
        return -1;
    }
    static bool inRect(float x, float y, float rx, float ry, float rw, float rh) { return x >= rx && x < rx + rw && y >= ry && y < ry + rh; }
    int headerHit(float x, float y) const {
        if (y < PS_Y || y > PS_Y + PS_H) return 0;
        if (x >= LEARN_X && x <= LEARN_X + LEARN_W) return 1;
        if (x >= MIDI_X && x <= MIDI_X + MIDI_W) return 2;
        if (x >= UNDO_X && x <= UNDO_X + UR_W) return 3;
        if (x >= REDO_X && x <= REDO_X + UR_W) return 4;
        if (x >= HELP_X && x <= HELP_X + HELP_W) return 5;
        if (x >= A_X && x < A_X + AB_W) return 6;
        if (x >= A_X + AB_W && x < A_X + 2 * AB_W) return 7;
        if (x >= COPY_X && x <= COPY_X + COPY_W) return 8;
        return 0;
    }
    int presetBarHit(float x, float y) const {
        if (y < PS_Y || y > PS_Y + PS_H || x < PS_X || x > PS_X + PS_W) return -1;
        return x < PS_X + PS_ARROW ? 0 : (x > PS_X + PS_W - PS_ARROW ? 2 : 1);
    }
    bool clearHit(float x, float y) const { return learnMode && learnWaiting < 0 && inRect(x, y, PX1 - 190, PBOT - 20, 190, 18); }
    bool inDisplay(float x, float y) const { return inRect(x, y, PX0 - 10, PTOP - 8, PX1 - PX0 + 20, PBOT - PTOP + 16); }
    int dotHit(float x, float y) const {
        int best = -1; float bd = 15 * 15;
        for (int pass = 0; pass < 2; ++pass) {       // the active layer wins
            for (int id = 0; id < kBands; ++id) {
                if (!used(id) || (isPost(id) == (layer == 1)) != (pass == 0)) continue;
                float dx, dy; dotPos(id, dx, dy); const float d = (x - dx) * (x - dx) + (y - dy) * (y - dy);
                if (d < bd) { bd = d; best = id; }
            }
            if (best >= 0) return best;
        }
        return -1;
    }
    bool onCurve(float x, float y) const {
        const float hz = hzOfX(x);
        if (layer == 1) return std::fabs(yOfDb(20 * std::log10(std::max(std::abs(postRespAt(val, hz)), 1e-5f))) - y) < 7;
        DecayCurve dc; dc.build(val, sampleRateUI);
        return std::fabs(yOfL(std::log2(dc.timeAt(hz) / decayTime(val))) - y) < 7;
    }

    void setLearn(bool on) { learnMode = on; learnWaiting = -1; if (!on) setState("cmd", "cancel"); repaint(); }
    void endEdit(bool apply) {
        if (editing < 0) return;
        const uint32_t p = effParam(controls()[editing]); float v;
        if (apply && p < kParamCount && parse(p, editBuf, v)) { beginGesture(); commit(p, v); endGesture(); }
        editing = -1; editBuf.clear(); repaint();
    }
    void editBand(int id, bool begin) { if (id < 0) return; for (int r = 0; r < kNumRoles; ++r) { const uint32_t p = bandParam(id, r); if (p < kParamCount) editParameter(p, begin); } }
    static float angleNorm(const Ctl& c, float x, float y) {
        float a = std::atan2(y - c.y, x - c.x);
        const float a0 = 0.75f * 3.14159265f;
        float rel = a - a0; while (rel < 0) rel += 2 * 3.14159265f;
        if (rel > 1.5f * 3.14159265f) rel = rel > 1.75f * 3.14159265f ? 0.f : 1.5f * 3.14159265f;
        return rel / (1.5f * 3.14159265f);
    }
    float defaultFor(uint32_t p) const {
        if (decayFieldOf(p) == kDFreq || postFieldOf(p) == kPFreq) return val[p];
        if (postFieldOf(p) == kPQ && isCut(primary())) return 0.70710678f;
        if (decayFieldOf(p) == kDQ || postFieldOf(p) == kPQ) return 1.f;
        return defv[p];
    }

    bool onKeyboard(const KeyboardEvent& ev) override {
        if (!ev.press) return false;
        std::string* b2 = saving ? &saveName : (editing >= 0 ? &editBuf : nullptr);
        if (!b2 && menuOpen) {                               // typing in the preset browser searches
            if (ev.key == kKeyEscape) { if (!query.empty()) query.clear(); else menuOpen = false; }
            else if (ev.key == kKeyBackspace || ev.key == kKeyDelete) { if (!query.empty()) query.pop_back(); }
            else if (ev.key >= 32 && ev.key < 127 && query.size() < 32) query += (char)ev.key;
            else return false;
            menuHover = -1; pendingDelete = -1; repaint(); return true;
        }
        if (!b2) {
            const bool cmd = (ev.mod & (kModifierControl | kModifierSuper)) != 0;
            if (cmd && (ev.key == 'z' || ev.key == 'Z')) { if (ev.mod & kModifierShift) redo(); else undo(); return true; }
            if (cmd && (ev.key == 'y' || ev.key == 'Y')) { redo(); return true; }
            if ((ev.key == kKeyDelete || ev.key == kKeyBackspace) && primary() >= 0 && !helpOpen) { deleteFocused(); return true; }
            if (ev.key == kKeyEscape) { if (helpOpen) helpOpen = false; else if (menuOpen) menuOpen = false; else if (learnMode) setLearn(false); repaint(); return true; }
            return false;
        }
        if (ev.key == kKeyEnter) {
            if (saving) {
                if (saveUserPreset(saveName, val)) { userPresets = listUserPresets(); curUser = sanitizeName(saveName); curFactory = -1; std::copy(val, val + kParamCount, userVals); saving = false; saveError = false; }
                else saveError = true;
            } else endEdit(true);
            repaint(); return true;
        }
        if (ev.key == kKeyEscape) { if (saving) saving = false; else endEdit(false); repaint(); return true; }
        if (ev.key == kKeyBackspace || ev.key == kKeyDelete) { if (!b2->empty()) b2->pop_back(); repaint(); return true; }
        if (ev.key >= 32 && ev.key < 127 && b2->size() < (saving ? 40u : 16u)) { *b2 += (char)ev.key; repaint(); return true; }
        return true;
    }

    bool onMouse(const MouseEvent& ev) override {
        const float s = uiScale(), x = ev.pos.getX() / s, y = ev.pos.getY() / s;
        if (saving) return true;
        if (ev.press) { statusFlash = 0; tipShown = false; tipSuppressed = tipKey; }
        if (helpOpen) { if (ev.press && ev.button == 1) helpClick(x, y); return true; }
        if (ev.button == 2 && ev.press) {                  // right-click (DPF numbers buttons 1 left, 2 right, 3 middle)
            if (menuOpen) { const int m = menuHit(x, y); if (m >= 0 && menuItem(m).kind == 1) { pendingDelete = menuItem(m).idx; repaint(); } return true; }
            if (learnMode) { const int i = hit(x, y); if (i >= 0) { const uint32_t p = effParam(controls()[i]); if (p < kParamCount) { char b[24]; snprintf(b, sizeof b, "unmap:%u", p); setState("cmd", b); paramCC[p] = -1; repaint(); } } return true; }
            const int d = inDisplay(x, y) ? dotHit(x, y) : -1;
            if (d >= 0) { focus = d; deleteFocused(); return true; }
            return false;
        }
        if (ev.button != 1) return false;
        if (!ev.press) {
            if (active >= 0) { const uint32_t p = effParam(controls()[active]); if (p < kParamCount) editParameter(p, false); active = -1; endGesture(); return true; }
            if (dragDot >= 0) { editBand(dragDot, false); dragDot = -1; endGesture(); return true; }
            return false;
        }
        if (editing >= 0) endEdit(true);
        if (menuOpen) {
            const int m = menuHit(x, y);
            if (m >= 0) {
                const MRect r = menuItem(m);
                if (r.kind == 3) return true;                       // the search field: just type
                if ((r.kind == 0 || r.kind == 1 || r.kind == 4) && x >= r.x + r.w - 20) { toggleFav(favKey(r.kind, r.idx)); return true; }
                if (r.kind == 2) { menuOpen = false; saving = true; saveError = false; saveName = curUser; }
                else if (r.kind == 1 && pendingDelete == r.idx) { deleteUserPreset(userPresets[r.idx]); if (curUser == userPresets[r.idx]) curUser.clear(); userPresets = listUserPresets(); pendingDelete = -1; repaint(); return true; }
                else { menuOpen = false; if (r.kind == 0) applyFactory(r.idx); else if (r.kind == 1) applyUser(userPresets[r.idx]); else applyFavourite(r.idx); }
            } else menuOpen = false;
            pendingDelete = -1; menuHover = -1; repaint(); return true;
        }
        const int hh = headerHit(x, y);
        if (hh == 1) { setLearn(!learnMode); return true; }
        if (hh == 2) { beginGesture(); commit(kMidiOn, val[kMidiOn] > 0.5f ? 0.f : 1.f); endGesture(); return true; }
        if (hh == 3) { undo(); return true; }
        if (hh == 4) { redo(); return true; }
        if (hh == 5) { helpOpen = true; repaint(); return true; }
        if (hh == 6 || hh == 7) { switchAB(hh - 6); return true; }
        if (hh == 8) { copyAB(); return true; }
        const int pb = presetBarHit(x, y);
        if (pb == 0) { step(-1); return true; }
        if (pb == 2) { step(1); return true; }
        if (pb == 1) { openMenu(); return true; }
        if (clearHit(x, y)) { setState("cmd", "clear"); for (int& m : paramCC) m = -1; repaint(); return true; }
        const int ph = pillHit(x, y);
        if (ph == 1 || ph == 2) { layer = ph - 1; if (focus >= 0 && isPost(focus) != (layer == 1)) focus = -1; repaint(); return true; }
        if (ph == 3) { commit(kAnalyzer, val[kAnalyzer] > 0.5f ? 0.f : 1.f); return true; }
        if (ph == 4) { commit(kPiano, val[kPiano] > 0.5f ? 0.f : 1.f); return true; }
        if (ph == 5) { FileBrowserOptions o; o.title = "Import an impulse response (WAV)"; if (!openFileBrowser(o)) flash("Could not open a file browser here"); return true; }

        int seg = 0; const int i = hit(x, y, &seg);
        if (i >= 0) {
            const Ctl& c = controls()[i]; const uint32_t p = effParam(c);
            if (c.type == DelButton) { deleteFocused(); return true; }
            if (p >= kParamCount) return true;
            if (learnMode) { learnWaiting = (int)p; char b[24]; snprintf(b, sizeof b, "learn:%u", p); setState("cmd", b); repaint(); return true; }
            if (isView(p)) { if (c.type == Toggle) commit(p, val[p] > 0.5f ? 0.f : 1.f); return true; }
            if (c.type == Toggle || c.type == Button) { beginGesture(); commit(p, val[p] > 0.5f ? 0.f : 1.f); endGesture(); return true; }
            if (c.type == Segment) { if (dimmed(c) && c.role == RSlope) return true; beginGesture(); commit(p, (float)seg); endGesture(); return true; }
            if (ev.mod & (kModifierControl | kModifierSuper)) { beginGesture(); commit(p, defaultFor(p)); endGesture(); return true; }
            if (i == lastClick && ev.time - lastClickTime < 350) {
                lastClick = -1; editing = i;
                char b[40]; fmt(p, b, sizeof b); editBuf = b;
                if (p != kSpace) { const size_t sp = editBuf.find(' '); if (sp != std::string::npos) editBuf.resize(sp); }
                if (!editBuf.empty() && editBuf.back() == '%') editBuf.pop_back();
                if ((decayFieldOf(p) == kDFreq || postFieldOf(p) == kPFreq) && val[p] >= 1000) { char kb[16]; snprintf(kb, sizeof kb, "%.3gk", val[p] / 1000); editBuf = kb; }
                repaint(); return true;
            }
            lastClick = i; lastClickTime = ev.time;
            beginGesture();
            active = i; dragY = y; dragNorm = toNorm(p, val[p]);
            circular = std::sqrt((x - c.x) * (x - c.x) + (y - c.y) * (y - c.y)) > c.r * 0.62f && c.r > 15;
            editParameter(p, true);
            if (circular) setValue(p, fromNorm(p, angleNorm(c, x, y)));
            repaint(); return true;
        }
        if (!learnMode && inDisplay(x, y)) {
            const int d = dotHit(x, y);
            if (d >= 0) {
                layer = isPost(d) ? 1 : 0;
                if (ev.mod & kModifierAlt) { beginGesture(); commit(bandParam(d, ROn), bandOn(d) ? 0.f : 1.f); endGesture(); return true; }
                focus = d; beginGesture(); dragDot = d; dragX0 = x; dragY0 = y;
                startF = bv(d, RFreq); startG = bv(d, RGain); startQ = bv(d, RQ);
                editBand(d, true); repaint(); return true;
            }
            const bool dbl = ev.time - lastDispClick < 350 && std::fabs(x - lastDispX) < 6 && std::fabs(y - lastDispY) < 6;
            lastDispClick = ev.time; lastDispX = x; lastDispY = y;
            if (dbl || onCurve(x, y)) {      // add a band to the active EQ and start dragging it
                beginGesture();
                const int b = newBand(layer == 1, hzOfX(x), y);
                if (b < 0) { endGesture(); return true; }
                focus = b; dragDot = b; dragX0 = x; dragY0 = y;
                startF = bv(b, RFreq); startG = bv(b, RGain); startQ = bv(b, RQ);
                editBand(b, true); repaint(); return true;
            }
            focus = -1; repaint(); return true;
        }
        return false;
    }

    bool onMotion(const MotionEvent& ev) override {
        const float s = uiScale(), x = ev.pos.getX() / s, y = ev.pos.getY() / s;
        if (saving) return true;
        if (helpOpen) { const int h = helpHit(x, y); if (h != helpHover) { helpHover = h; repaint(); } return true; }
        if (active >= 0) {
            const Ctl& c = controls()[active]; const uint32_t p = effParam(c);
            if (p >= kParamCount) return true;
            if (circular) { setValue(p, fromNorm(p, angleNorm(c, x, y))); return true; }
            float sens = (ev.mod & kModifierShift) ? 1500.f : 240.f;
            if (paramDef(p).hints & kHintInteger) sens = 120.f;
            dragNorm = clampf(dragNorm + (dragY - y) / sens, 0.f, 1.f); dragY = y;
            setValue(p, fromNorm(p, dragNorm));
            return true;
        }
        if (dragDot >= 0) {
            const float fine = (ev.mod & kModifierShift) ? 0.15f : 1.f;
            const float ratio = hzOfX(dragX0 + (x - dragX0) * fine) / hzOfX(dragX0);
            setValue(bandParam(dragDot, RFreq), clampf(startF * ratio, FMIN, FMAX));
            const float yy = dragY0 + (y - dragY0) * fine;
            if (isPost(dragDot)) {
                const float dDb = dbOfY(yy) - dbOfY(dragY0);
                if (isCut(dragDot)) setValue(bandParam(dragDot, RQ), clampf(startQ * std::pow(10.f, dDb / 20.f), 0.1f, 40.f));
                else setValue(bandParam(dragDot, RGain), clampf(startG + dDb, -30.f, 30.f));
            } else if (shapeOf(dragDot) != kDNotch) setValue(bandParam(dragDot, RGain), clampf(startG + lOfY(yy) - lOfY(dragY0), -2.f, 2.f));
            return true;
        }
        if (menuOpen) { const int m = menuHit(x, y); if (m != menuHover) { menuHover = m; repaint(); } return true; }
        const int hh = headerHit(x, y); if (hh != hoverHeader) { hoverHeader = hh; repaint(); }
        const int pb = presetBarHit(x, y); if (pb != presetHover) { presetHover = pb; repaint(); }
        const bool hc = clearHit(x, y); if (hc != hoverClear) { hoverClear = hc; repaint(); }
        const int h = hit(x, y); if (h != hover) { hover = h; repaint(); }
        const int ph = pillHit(x, y); if (ph != hoverPill) { hoverPill = ph; repaint(); }
        const int hd = h < 0 && ph == 0 && inDisplay(x, y) ? dotHit(x, y) : -1; if (hd != hoverDot) { hoverDot = hd; repaint(); }
        int key = -1;
        if (h >= 0) key = h; else if (hh > 0) key = 1000 + hh; else if (metersHit(x, y)) key = 1100; else if (pb >= 0) key = 2000 + pb;
        else if (ph > 0) key = 4000 + ph; else if (hd >= 0) key = 3000 + hd; else if (inDisplay(x, y)) key = 5000;
        tipX = x; tipY = y;
        if (key != tipKey) { const bool was = tipShown; tipKey = key; tipSince = nowSec(); tipShown = false; if (key != tipSuppressed) tipSuppressed = -2; if (was) repaint(); }
        return false;
    }

    bool onScroll(const ScrollEvent& ev) override {
        const float s = uiScale(), sx = ev.pos.getX() / s, sy = ev.pos.getY() / s;
        if (menuOpen || saving || helpOpen) return true;
        if (presetBarHit(sx, sy) >= 0) { step(ev.delta.getY() > 0 ? -1 : 1); return true; }
        const float dir = ev.delta.getY() > 0 ? 1.f : -1.f;
        if (inDisplay(sx, sy)) {          // mouse wheel over a dot: Q
            const int d = dotHit(sx, sy); if (d < 0) return false;
            focus = d; layer = isPost(d) ? 1 : 0;
            const uint32_t q = bandParam(d, RQ); const float k = std::pow(2.f, dir * ((ev.mod & kModifierShift) ? 0.05f : 0.2f));
            beginGesture(); commit(q, clampf(val[q] * k, paramDef(q).min, paramDef(q).max)); endGesture();
            return true;
        }
        const int i = hit(sx, sy);
        if (i < 0 || controls()[i].type != Knob) return false;
        const uint32_t p = effParam(controls()[i]); if (p >= kParamCount) return false;
        const float st = (paramDef(p).hints & kHintInteger) ? 1.f / (paramDef(p).max - paramDef(p).min) : ((ev.mod & kModifierShift) ? 0.004f : 0.02f);
        beginGesture(); commit(p, fromNorm(p, toNorm(p, val[p]) + dir * st)); endGesture();
        return true;
    }

private:
    float val[kParamCount], defv[kParamCount], userVals[kParamCount] = {}, before[kParamCount];
    int paramCC[kParamCount];
    int focus = -1, layer = 0;
    int active = -1, hover = -1, lastClick = -1, editing = -1, hoverDot = -1, dragDot = -1, hoverPill = 0;
    uint lastClickTime = 0, lastDispClick = 0; float lastDispX = 0, lastDispY = 0;
    float dragY = 0, dragNorm = 0, dragX0 = 0, dragY0 = 0, startF = 0, startG = 0, startQ = 0; bool circular = false, gesture = false;
    std::vector<Snap> undoStack, redoStack;
    Snap abStore[2]; int abSlot = 0;
    Fft fft; std::vector<cpx> buf; std::vector<float> spec; bool anaValid = false; uint32_t lastWrite = 0; double lastAna = 0; float sampleRateUI = 48000.f;
    std::string editBuf, statusText; int statusFlash = 0;
    int curFactory = 0, presetHover = -1, menuHover = -1, pendingDelete = -1, hoverHeader = 0;
    std::string curUser; std::vector<std::string> userPresets;
    bool menuOpen = false, saving = false, saveError = false; std::string saveName;
    bool learnMode = false, hoverClear = false; int learnWaiting = -1; float lastLearnEcho = 0;
    int tipKey = -1, tipSuppressed = -2; double tipSince = 0; bool tipShown = false; float tipX = 0, tipY = 0;
    bool helpOpen = false; int helpPage = 0, helpHover = -1;
    std::string irPath; bool irBusy = false, irShown = false;
    std::string query; std::vector<std::string> favs;
    ResizeHandle resizeHandle;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GeodeUI)
};

UI* createUI() { return new GeodeUI(); }

END_NAMESPACE_DISTRHO
