// Factory presets (each lists only what it changes from the engine defaults). License: MIT.
// Engine defaults: Space 0.5 (between Chamber and Small Hall), decay 100%, brightness 50%, character 25%, distance 40%,
// width 100%, mix 30%, no predelay, no EQ bands; 2.0: Modern algorithm, no thickness, ducking, gate or freeze.
#pragma once
#include "GeodeEngine.hpp"
#include <algorithm>

namespace geode {

struct PV { uint32_t id; float v; };
struct PresetDef { const char* name; const char* category; const PV* pv; };
struct Preset { const char* name; const char* category; float v[kParamCount]; };

static const char* const kCategories[] = { "Rooms", "Halls & Churches", "Vocals", "Drums & Instruments", "Creative" };
static constexpr int kNumCategories = 5;

#define END { kParamCount, 0 }
#define SP(k) { kSpace, (k) / 13.f }
// decay EQ band: shape, frequency, decay change in octaves, Q
#define DB(b, sh, f, L, q) { dp(b, kDUsed), 1 }, { dp(b, kDShape), (float)(sh) }, { dp(b, kDFreq), f }, { dp(b, kDAmount), L }, { dp(b, kDQ), q }
// post EQ bands
#define PCUT(b, sh, f, sl) { pp(b, kPUsed), 1 }, { pp(b, kPShape), (float)(sh) }, { pp(b, kPFreq), f }, { pp(b, kPSlope), (float)(sl) }
#define PEQ(b, sh, f, g, q) { pp(b, kPUsed), 1 }, { pp(b, kPShape), (float)(sh) }, { pp(b, kPFreq), f }, { pp(b, kPGain), g }, { pp(b, kPQ), q }
namespace pv {
static const PV init[] = { END };
// ---------------------------------------------------------------- Rooms
static const PV ambience[]   = { SP(0), {kDistance,25}, {kMix,22}, {kCharacter,15}, PCUT(0, kPLowCut, 150, 2), END };
static const PV smallRoom[]  = { SP(1), {kDistance,35}, {kMix,25}, END };
static const PV booth[]      = { SP(2), {kBrightness,35}, {kDistance,20}, {kMix,18}, END };
static const PV studio[]     = { SP(3), {kMix,25}, PCUT(0, kPLowCut, 120, 2), END };
static const PV liveRoom[]   = { SP(5), {kCharacter,45}, {kBrightness,60}, {kMix,28}, END };
static const PV woodRoom[]   = { SP(4), {kBrightness,40}, DB(0, kDLowShelf, 300, 0.4f, 0.7f), PEQ(1, kPBell, 400, 1.5f, 0.8f), END };
static const PV chamber[]    = { SP(6), {kCharacter,30}, {kBrightness,55}, {kMix,28}, END };
// ---------------------------------------------------------------- Halls & Churches
static const PV smallHall[]  = { SP(7), {kMix,30}, {kPredelay,12}, END };
static const PV concert[]    = { SP(8), {kMix,30}, {kPredelay,20}, {kDistance,55}, DB(0, kDLowShelf, 250, 0.3f, 0.7f), END };
static const PV darkHall[]   = { SP(9), {kBrightness,25}, {kMix,30}, {kPredelay,25}, END };
static const PV brightHall[] = { SP(8), {kBrightness,78}, {kMix,28}, PCUT(0, kPLowCut, 200, 2), END };
static const PV church[]     = { SP(10), {kDistance,60}, {kMix,32}, {kPredelay,30}, END };
static const PV cathedral[]  = { SP(12), {kDistance,70}, {kMix,35}, {kPredelay,40}, {kWidth,110}, END };
static const PV vast[]       = { SP(13), {kDecayRate,160}, {kDistance,80}, {kMix,40}, {kPredelay,50}, {kWidth,120}, END };
// ---------------------------------------------------------------- Vocals
static const PV vocalRoom[]  = { SP(3), {kMix,18}, {kPredelay,15}, PCUT(0, kPLowCut, 220, 2), PEQ(1, kPBell, 3000, -2, 1.f), END };
static const PV vocalPlate[] = { SP(6), {kBrightness,72}, {kCharacter,10}, {kDistance,25}, {kMix,22}, {kPredelay,25}, PCUT(0, kPLowCut, 300, 3), END };
static const PV vocalHall[]  = { SP(8), {kMix,22}, {kPredelaySync,1}, {kPredelayNote,4}, PCUT(0, kPLowCut, 250, 2), DB(1, kDHighShelf, 6000, -0.6f, 0.7f), END };
static const PV balladTail[] = { SP(9), {kDecayRate,140}, {kBrightness,60}, {kMix,28}, {kPredelay,60}, PCUT(0, kPLowCut, 300, 3), END };
static const PV deEssTail[]  = { SP(7), {kMix,24}, {kPredelay,20}, DB(0, kDNotch, 6500, 0, 1.5f), PCUT(1, kPLowCut, 250, 2), END };
static const PV narration[]  = { SP(2), {kDistance,15}, {kMix,12}, {kBrightness,40}, PCUT(0, kPLowCut, 180, 2), END };
// ---------------------------------------------------------------- Drums & Instruments
static const PV drumRoom[]   = { SP(4), {kCharacter,50}, {kMix,30}, PCUT(0, kPLowCut, 100, 2), END };
static const PV snareHall[]  = { SP(8), {kDecayRate,80}, {kMix,25}, PCUT(0, kPLowCut, 180, 3), DB(1, kDLowShelf, 200, -1.f, 0.7f), END };
static const PV tightKit[]   = { SP(1), {kDecayRate,70}, {kCharacter,40}, {kMix,20}, END };
static const PV pianoHall[]  = { SP(8), {kMix,26}, {kPredelay,15}, {kDistance,50}, DB(0, kDHighShelf, 5000, -0.4f, 0.7f), END };
static const PV guitarRoom[] = { SP(5), {kMix,24}, {kBrightness,45}, PCUT(0, kPLowCut, 150, 2), END };
static const PV stringsHall[]= { SP(10), {kDecayRate,120}, {kDistance,65}, {kMix,30}, {kPredelay,30}, END };
static const PV padWash[]    = { SP(12), {kDecayRate,180}, {kCharacter,60}, {kMix,45}, {kWidth,120}, END };
// ---------------------------------------------------------------- Creative
static const PV chorusVerb[] = { SP(8), {kCharacter,100}, {kMix,35}, {kWidth,120}, END };
static const PV echoRoom[]   = { SP(6), {kCharacter,70}, {kMix,30}, END };
static const PV infinite[]   = { SP(13), {kDecayRate,200}, {kCharacter,55}, {kMix,50}, DB(0, kDLowShelf, 200, 1.0f, 0.7f), DB(1, kDHighShelf, 4000, 0.6f, 0.7f), END };
static const PV telephone[]  = { SP(5), {kMix,35}, PCUT(0, kPLowCut, 500, 3), PCUT(1, kPHighCut, 3000, 3), END };
static const PV darkTail[]   = { SP(11), {kBrightness,10}, {kMix,35}, PCUT(0, kPHighCut, 2500, 2), END };
static const PV syncedSlap[] = { SP(3), {kPredelaySync,1}, {kPredelayNote,7}, {kDecayRate,60}, {kMix,30}, {kCharacter,60}, END };
static const PV monoWash[]   = { SP(10), {kWidth,0}, {kMix,35}, {kDistance,80}, END };
// ---------------------------------------------------------------- 2.0
static const PV vintageChamber[] = { {kAlgorithm, kVintage}, SP(6), {kCharacter,40}, {kMix,28}, END };
static const PV digital80s[]  = { {kAlgorithm, kVintage}, SP(8), {kDecayRate,120}, {kCharacter,45}, {kMix,30}, {kPredelay,20}, PCUT(0, kPLowCut, 150, 2), END };
static const PV endlessHall[] = { SP(13), {kDecayRate,400}, {kDistance,75}, {kMix,40}, {kPredelay,40}, {kWidth,115}, DB(0, kDHighShelf, 5000, -0.5f, 0.7f), END };
static const PV plateVocal[]  = { {kAlgorithm, kPlate}, SP(5), {kBrightness,65}, {kCharacter,15}, {kMix,22}, {kPredelay,20}, PCUT(0, kPLowCut, 250, 3), END };
static const PV duckedVocal[] = { SP(8), {kDucking,60}, {kMix,30}, {kPredelay,15}, PCUT(0, kPLowCut, 250, 2), END };
static const PV drumPlate[]   = { {kAlgorithm, kPlate}, SP(3), {kDecayRate,80}, {kBrightness,60}, {kMix,25}, PCUT(0, kPLowCut, 180, 2), END };
static const PV gatedSnare[]  = { SP(7), {kAutoGate,1}, {kGateThreshold,-35}, {kGateHold,180}, {kThickness,40}, {kCharacter,40}, {kMix,35}, PCUT(0, kPLowCut, 200, 2), END };
static const PV thickRoom[]   = { SP(4), {kThickness,70}, {kCharacter,45}, {kMix,30}, PCUT(0, kPLowCut, 100, 2), END };
static const PV lofiVerb[]    = { {kAlgorithm, kVintage}, SP(10), {kThickness,60}, {kMix,35}, PCUT(0, kPHighCut, 4000, 2), PCUT(1, kPLowCut, 200, 2), END };
static const PV duckedWash[]  = { SP(13), {kDecayRate,250}, {kDucking,80}, {kCharacter,55}, {kMix,50}, {kWidth,120}, END };
static const PV shortBurst[]  = { SP(10), {kDecayRate,25}, {kCharacter,60}, {kMix,35}, END };
} // namespace pv
#undef END
#undef SP
#undef DB
#undef PCUT
#undef PEQ

static const PresetDef kPresetDefs[] = {
    { "Init", "Init", pv::init },
    { "Ambience", "Rooms", pv::ambience }, { "Small Room", "Rooms", pv::smallRoom }, { "Vocal Booth", "Rooms", pv::booth },
    { "Studio Room", "Rooms", pv::studio }, { "Live Room", "Rooms", pv::liveRoom }, { "Wood Room", "Rooms", pv::woodRoom }, { "Chamber", "Rooms", pv::chamber },
    { "Small Hall", "Halls & Churches", pv::smallHall }, { "Concert Hall", "Halls & Churches", pv::concert }, { "Dark Hall", "Halls & Churches", pv::darkHall },
    { "Bright Hall", "Halls & Churches", pv::brightHall }, { "Church", "Halls & Churches", pv::church }, { "Cathedral", "Halls & Churches", pv::cathedral },
    { "Vast Space", "Halls & Churches", pv::vast },
    { "Vocal Room", "Vocals", pv::vocalRoom }, { "Bright Vocal", "Vocals", pv::vocalPlate }, { "Vocal Hall (1/16)", "Vocals", pv::vocalHall },
    { "Ballad Tail", "Vocals", pv::balladTail }, { "Soft Esses", "Vocals", pv::deEssTail }, { "Narration", "Vocals", pv::narration },
    { "Drum Room", "Drums & Instruments", pv::drumRoom }, { "Snare Hall", "Drums & Instruments", pv::snareHall }, { "Tight Kit", "Drums & Instruments", pv::tightKit },
    { "Piano Hall", "Drums & Instruments", pv::pianoHall }, { "Guitar Room", "Drums & Instruments", pv::guitarRoom }, { "Strings Hall", "Drums & Instruments", pv::stringsHall },
    { "Pad Wash", "Drums & Instruments", pv::padWash },
    { "Chorus Verb", "Creative", pv::chorusVerb }, { "Echo Room", "Creative", pv::echoRoom }, { "Near Infinite", "Creative", pv::infinite },
    { "Telephone Room", "Creative", pv::telephone }, { "Dark Tail", "Creative", pv::darkTail }, { "Synced Slap (1/8)", "Creative", pv::syncedSlap },
    { "Mono Wash", "Creative", pv::monoWash },
    // 2.0 (appended, so 1.x program numbers stay the same)
    { "Vintage Chamber", "Rooms", pv::vintageChamber }, { "80s Digital Hall", "Halls & Churches", pv::digital80s }, { "Endless Hall", "Halls & Churches", pv::endlessHall },
    { "Vocal Plate", "Vocals", pv::plateVocal }, { "Ducked Vocal Hall", "Vocals", pv::duckedVocal },
    { "Drum Plate", "Drums & Instruments", pv::drumPlate }, { "Gated Snare", "Drums & Instruments", pv::gatedSnare }, { "Thick Drum Room", "Drums & Instruments", pv::thickRoom },
    { "Lo-Fi Verb", "Creative", pv::lofiVerb }, { "Ducked Wash", "Creative", pv::duckedWash }, { "Short Burst", "Creative", pv::shortBurst },
};
static constexpr uint32_t kNumPresets = sizeof(kPresetDefs) / sizeof(kPresetDefs[0]);

inline const Preset* presets() {
    static Preset table[kNumPresets];
    static bool done = false;
    if (done) return table;
    done = true;
    Engine* e = new Engine();
    for (uint32_t i = 0; i < kNumPresets; ++i) {
        table[i].name = kPresetDefs[i].name; table[i].category = kPresetDefs[i].category;
        std::copy(e->p, e->p + kParamCount, table[i].v);
        for (const PV* x = kPresetDefs[i].pv; x->id != kParamCount; ++x) table[i].v[x->id] = x->v;
    }
    delete e;
    return table;
}

} // namespace geode
