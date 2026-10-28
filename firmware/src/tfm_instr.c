/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 CDM Gaming (TILT-FM) */
/* TFM: INSTRUMENTS, TILT's lineup on Felucca's engines (names, groups and macro ideas from TILT's instruments*.ts,
 * CDM Gaming's own). An instrument is one engine, a list of its sounds and eight MACROS; nothing new is stored: a
 * sound is a factory preset of that engine with a few values changed (tfm_snd_t), and a macro sets engine and track
 * values directly (no state of its own: the knob's place is read back from its first target). A track knows its
 * instrument by its preset byte: TFM_PRE(i, s) = 0x20 + 16 i + s, past every engine's factory presets (< 16) and
 * below project.c's PROJ_DEF_KEEP / SOUND (0xFE, 0xFF); a project or preset of before, or a raw engine sound picked
 * in EDIT, has none (tfm_ins_of: TI_NONE) and plays as ever. The project format is unchanged.
 * MACRO RULES (keep them when tuning; MACROS-TO-TUNE.md lists every mapping):
 *   - every macro changes the sound a lot, in one musical direction, across its whole turn: no dead zone at either
 *     end (ranges tuned per instrument, not the parameter's full range)
 *   - pitch, frequency and time move on exponential curves: CUT (F_CUTOFF), the envelope times (F_TIME), the LFO
 *     rate (F_LFOHZ) are already exponential in their 0..127, so a straight line in those units is the curve
 *   - noon (64) sounds good: the instrument's sounds sit near the middle of their macros
 *   - no sudden jumps: a stepped value (a waveform, a mode) only on a macro labelled as a switch (TM_STEP)
 *   - level steady: a macro that drives or opens the filter takes the track LEVEL down as it goes (a second target
 *     running the other way)
 *   - the extremes may be wild
 *   - one macro may move several values together when that is the musical thing to turn */

typedef struct { uint8_t id; int8_t v; } tfm_ov_t;      /* a value changed from the factory preset (P_* id, value) */
typedef struct {
    const char *name;                                   /* (12 at most: the PRESETS list, the header) */
    uint8_t pre;                                        /* the engine's factory preset it starts from */
    tfm_ov_t ov[6];                                     /* the changes (id 0xFF: none; P_LEVEL is id 0, so 0xFF ends) */
} tfm_snd_t;
#define TM_STEP 1u                                      /* a stepped macro: a switch, labelled so */
typedef struct { uint8_t id; int8_t lo, hi; } tfm_tgt_t;
typedef struct {
    const char *name;                                   /* the card's label (5 at most) */
    uint8_t flags;
    tfm_tgt_t t[3];                                     /* (id 0xFF: none) */
} tfm_macro_t;
enum { IG_DRUMS, IG_BASS, IG_HARMONY, IG_LEAD, IG_N };  /* TILT's groups (fx: none here) */
typedef struct {
    const char *name;                                   /* "THUMP LAB" */
    uint8_t group, eng, tilt, wild;                     /* TILT group; the engine; TILT's KIND_CODE; a wildcard */
    uint8_t nsnd;
    tfm_snd_t snd[12];
    tfm_macro_t mac[8];                                 /* KNOB 1..4 page 1, then page 2 */
} tfm_instr_t;
#define X 0xFFu
#define NOV {{X, 0}}
#define NT {X, 0, 0}
enum { TI_THUMP, TI_SQUELCH, TI_WOBBLER, TI_CHORDETTE, TI_RAZOR, TI_HAZE, TI_PIXEL, TI_TINE, TI_GLINT, TI_PLUCK,
       TI_VOX, TI_WHEEL, TI_N, TI_NONE = 0xFF };
#define TFM_PRE(i, s) (TFM_PRE0 + 16u * (i) + (s))
#define E_ANALOG 0u
#define E_LOFI 3u
#define E_FORMANT 5u
#define E_TRIO 6u
#define E_WHEEL 7u
#define E_PHYS 9u
#define E_DRUM 10u
#define E_FM6 12u
static const tfm_instr_t TFM_INS[TI_N] = {
    /* THUMP LAB: the DRUM engine's virtual-analog kits (TILT: Thump Lab, kind "analog", the drums group) */
    {"THUMP LAB", IG_DRUMS, E_DRUM, 4, 0, 10, {
        {"DEEP CLASSIC", 1, NOV},
        {"LONG BOOM", 1, {{P_E3, 100}, {P_E1, 40}, {P_E5, 110}, {X, 0}}},
        {"CLUB STD", 0, NOV},
        {"WAREHOUSE", 0, {{P_E7, 60}, {P_E4, 100}, {P_REV, 30}, {X, 0}}},
        {"TIGHT TECHNO", 2, {{P_E3, 30}, {P_E4, 90}, {X, 0}}},
        {"TINY BOX", 2, {{P_E1, 90}, {P_E2, 100}, {P_E3, 40}, {X, 0}}},
        {"HARD FLOOR", 0, {{P_E7, 100}, {P_E5, 127}, {P_E4, 110}, {P_LEVEL, 96}, {X, 0}}},
        {"BOSSA BOX", 3, NOV},
        {"LOUNGE", 3, {{P_E1, 50}, {P_E3, 90}, {P_REV, 40}, {X, 0}}},
        {"SLOW JAM", 1, {{P_E3, 80}, {P_E4, 40}, {P_E2, 40}, {P_REV, 24}, {X, 0}}},
    }, {
        {"TUNE", 0, {{P_E1, 16, 112}, NT, NT}},
        {"DECAY", 0, {{P_E3, 8, 120}, NT, NT}},
        {"SNAP", 0, {{P_E4, 8, 120}, NT, NT}},
        {"TONE", 0, {{P_E2, 10, 120}, NT, NT}},
        {"DRIVE", 0, {{P_E7, 0, 110}, {P_LEVEL, 108, 90}, NT}},
        {"ACCNT", 0, {{P_E5, 30, 127}, NT, NT}},
        {"ROOM", 0, {{P_REV, 0, 90}, NT, NT}},
        {"LEVEL", 0, {{P_LEVEL, 70, 120}, NT, NT}},
    }},
    /* SQUELCH: ANALOG as an acid bass (TILT: cutoff reso envmod decay bite gnaw accent glide) */
    {"SQUELCH", IG_BASS, E_ANALOG, 1, 0, 10, {
        {"CLASSIC", 4, NOV},
        {"RUBBER", 4, {{P_E4, 40}, {P_E5, 45}, {P_ED_FLT, 20}, {P_DEC, 70}, {X, 0}}},
        {"SCREAMER", 4, {{P_E4, 70}, {P_E5, 110}, {P_ED_FLT, 50}, {P_E6, 90}, {P_LEVEL, 92}, {X, 0}}},
        {"SQUARE PLUCK", 2, {{P_E5, 50}, {P_ED_FLT, 35}, {P_DEC, 40}, {X, 0}}},
        {"DEEP SUB", 7, NOV},
        {"GNARL", 4, {{P_E0, 1}, {P_E5, 95}, {P_E6, 120}, {P_LEVEL, 90}, {X, 0}}},
        {"LIQUID", 4, {{P_E4, 60}, {P_E5, 70}, {P_DEC, 95}, {P_GLIDE, 40}, {X, 0}}},
        {"RAVE LINE", 6, NOV},
        {"NIGHT BUS", 7, {{P_E4, 35}, {P_E5, 60}, {P_E6, 30}, {X, 0}}},
        {"LASER TOOTH", 4, {{P_E4, 90}, {P_E5, 122}, {P_ED_FLT, 63}, {P_DEC, 25}, {X, 0}}},
    }, {
        {"CUT", 0, {{P_E4, 18, 112}, NT, NT}},
        {"RESO", 0, {{P_E5, 10, 118}, NT, NT}},
        {"ENV", 0, {{P_ED_FLT, 0, 60}, NT, NT}},
        {"DECAY", 0, {{P_DEC, 20, 100}, NT, NT}},
        {"BITE", 0, {{P_E6, 0, 120}, {P_LEVEL, 108, 88}, NT}},
        {"GNAW", 0, {{P_DIST, 0, 100}, {P_LEVEL, 108, 92}, NT}},
        {"MIX", 0, {{P_E2, 0, 127}, NT, NT}},
        {"GLIDE", 0, {{P_GLIDE, 0, 80}, NT, NT}},
    }},
    /* WOBBLER: TRIO with its LFO on the filter, synced (TILT: wobble wobRate growl crush cutoff sub glide level) */
    {"WOBBLER", IG_BASS, E_TRIO, 5, 0, 8, {
        {"CLASSIC WOB", 0, {{P_LD_FLT, 40}, {P_LSYNC, 6}, {P_E5, 50}, {P_E6, 70}, {X, 0}}},
        {"SLOW GROWL", 0, {{P_LD_FLT, 50}, {P_LSYNC, 5}, {P_E6, 95}, {X, 0}}},
        {"YOI", 0, {{P_LD_FLT, 55}, {P_LSYNC, 8}, {P_E6, 100}, {P_E5, 35}, {X, 0}}},
        {"TRIPLET WUB", 0, {{P_LD_FLT, 45}, {P_LSYNC, 7}, {P_E5, 60}, {X, 0}}},
        {"CRUSHED WOB", 0, {{P_LD_FLT, 40}, {P_LSYNC, 6}, {P_ITYPE, 5}, {P_IA, 70}, {X, 0}}},
        {"REESE", 0, {{P_E3, 30}, {P_LD_FLT, 15}, {P_LSYNC, 3}, {P_E5, 40}, {X, 0}}},
        {"SUB WOB", 0, {{P_E2, -12}, {P_LD_FLT, 35}, {P_LSYNC, 6}, {P_E5, 30}, {X, 0}}},
        {"FAT STAB", 0, NOV},
    }, {
        {"WOB", 0, {{P_LD_FLT, 0, 63}, NT, NT}},
        {"RATE", TM_STEP, {{P_LSYNC, 3, 10}, NT, NT}},
        {"GROWL", 0, {{P_E6, 10, 120}, NT, NT}},
        {"CRUSH", 0, {{P_IMIX, 0, 110}, NT, NT}},
        {"CUT", 0, {{P_E5, 15, 110}, NT, NT}},
        {"SPRD", 0, {{P_E3, 0, 40}, NT, NT}},
        {"GLIDE", 0, {{P_GLIDE, 0, 80}, NT, NT}},
        {"LEVEL", 0, {{P_LEVEL, 70, 120}, NT, NT}},
    }},
    /* CHORDETTE: ANALOG with the chord keys (TILT: warmth strum voicing chord attack release cutoff level) */
    {"CHORDETTE", IG_HARMONY, E_ANALOG, 2, 0, 9, {
        {"WARM STAB", 1, {{P_CHRD, 2}, {P_ATK, 4}, {P_REL, 40}, {X, 0}}},
        {"HOUSE ORGAN", 1, {{P_CHRD, 7}, {P_E0, 1}, {P_ATK, 0}, {P_DEC, 40}, {P_SUS, 60}, {X, 0}}},
        {"PWM CHORDS", 3, {{P_CHRD, 1}, {X, 0}}},
        {"STRING PAD", 11, {{P_CHRD, 2}, {X, 0}}},
        {"DUB CHORD", 1, {{P_CHRD, 7}, {P_DLY, 60}, {P_DEC, 35}, {P_SUS, 20}, {X, 0}}},
        {"BRIGHT 7TH", 0, {{P_CHRD, 2}, {P_VOICE, 0}, {P_ATK, 2}, {P_REL, 50}, {X, 0}}},
        {"SOFT TRIAD", 1, {{P_CHRD, 1}, {P_VOIC, 1}, {X, 0}}},
        {"POWER", 0, {{P_CHRD, 9}, {P_VOICE, 0}, {P_E6, 60}, {X, 0}}},
        {"SUS GLOW", 3, {{P_CHRD, 8}, {P_ATK, 30}, {X, 0}}},
    }, {
        {"WARM", 0, {{P_E4, 30, 110}, NT, NT}},
        {"ATK", 0, {{P_ATK, 0, 90}, NT, NT}},
        {"VOICE", TM_STEP, {{P_VOIC, 0, 4}, NT, NT}},
        {"CHORD", TM_STEP, {{P_CHRD, 1, 9}, NT, NT}},
        {"REL", 0, {{P_REL, 10, 110}, NT, NT}},
        {"RES", 0, {{P_E5, 0, 90}, NT, NT}},
        {"SPACE", 0, {{P_REV, 0, 90}, {P_CHOR, 0, 60}, NT}},
        {"LEVEL", 0, {{P_LEVEL, 70, 120}, NT, NT}},
    }},
    /* RAZOR: ANALOG as a lead, sync and unison (TILT: scream sweep cutoff glide unison spread attack level) */
    {"RAZOR", IG_LEAD, E_ANALOG, 3, 0, 9, {
        {"SAW LEAD", 0, NOV},
        {"SYNC SCREAM", 0, {{P_E0, 5}, {P_E5, 70}, {P_ED_PIT, 30}, {X, 0}}},
        {"SUPER UNISON", 0, {{P_VOICE, 3}, {P_E1, 40}, {P_SPRD, 90}, {X, 0}}},
        {"BRASS LEAD", 9, NOV},
        {"SQUARE LEAD", 0, {{P_E0, 1}, {P_E4, 80}, {X, 0}}},
        {"GLIDE LEAD", 0, {{P_GLIDE, 50}, {P_VOICE, 2}, {X, 0}}},
        {"HOOVER", 0, {{P_E0, 4}, {P_VOICE, 3}, {P_E1, 60}, {P_ED_PIT, -20}, {X, 0}}},
        {"RAVE STAB", 6, NOV},
        {"WIND LEAD", 10, NOV},
    }, {
        {"SCRM", 0, {{P_E5, 10, 110}, {P_E6, 0, 70}, {P_LEVEL, 106, 92}}},
        {"SWEEP", 0, {{P_ED_FLT, -20, 60}, NT, NT}},
        {"CUT", 0, {{P_E4, 30, 120}, NT, NT}},
        {"GLIDE", 0, {{P_GLIDE, 0, 80}, NT, NT}},
        {"DTUN", 0, {{P_E1, 0, 80}, NT, NT}},
        {"SPRD", 0, {{P_SPRD, 0, 127}, NT, NT}},
        {"ATK", 0, {{P_ATK, 0, 80}, NT, NT}},
        {"LEVEL", 0, {{P_LEVEL, 70, 120}, NT, NT}},
    }},
    /* HAZE: TRIO as a pad, slow, crushed and wet (TILT: drift dust space low hi higher attack level) */
    {"HAZE", IG_HARMONY, E_TRIO, 6, 0, 8, {
        {"WARM HAZE", 4, {{P_ATK, 70}, {P_REL, 90}, {P_REV, 70}, {X, 0}}},
        {"DUSTY", 4, {{P_ATK, 60}, {P_ITYPE, 5}, {P_IA, 80}, {P_IMIX, 70}, {P_REV, 60}, {X, 0}}},
        {"GLASS AIR", 3, {{P_ATK, 80}, {P_REL, 100}, {P_REV, 90}, {X, 0}}},
        {"DARK DRIFT", 0, {{P_E5, 30}, {P_ATK, 90}, {P_LD_PIT, 4}, {P_REV, 80}, {X, 0}}},
        {"CHOIR FOG", 4, {{P_ATK, 100}, {P_CHOR, 80}, {P_REV, 90}, {X, 0}}},
        {"TAPE PAD", 4, {{P_LD_PIT, 6}, {P_LRATE, 30}, {P_ATK, 70}, {P_REV, 60}, {X, 0}}},
        {"BRIGHT SWELL", 1, {{P_ATK, 90}, {P_REL, 100}, {P_E5, 90}, {X, 0}}},
        {"LOW CLOUD", 0, {{P_E1, -12}, {P_E5, 40}, {P_ATK, 80}, {P_REV, 70}, {X, 0}}},
    }, {
        {"DRIFT", 0, {{P_LD_PIT, 0, 14}, NT, NT}},
        {"DUST", 0, {{P_IMIX, 0, 100}, NT, NT}},
        {"SPACE", 0, {{P_REV, 10, 127}, NT, NT}},
        {"LOW", 0, {{P_E5, 10, 70}, NT, NT}},
        {"HI", 0, {{P_E5, 60, 127}, NT, NT}},
        {"SHINE", 0, {{P_CHOR, 0, 110}, NT, NT}},
        {"ATK", 0, {{P_ATK, 10, 120}, NT, NT}},
        {"LEVEL", 0, {{P_LEVEL, 70, 120}, NT, NT}},
    }},
    /* PIXEL: LOFI, the chiptune (TILT: jump jumpRate run glitch wave decay attack level) */
    {"PIXEL", IG_LEAD, E_LOFI, 7, 0, 9, {
        {"PULSE LEAD", 0, NOV},
        {"COIN ARP", 2, NOV},
        {"WAVE LEAD", 3, NOV},
        {"STEP LEAD", 4, NOV},
        {"GAME BASS", 1, NOV},
        {"1BIT BUZZ", 0, {{P_E0, 3}, {X, 0}}},
        {"VIBRATO", 0, {{P_E5, 70}, {X, 0}}},
        {"LASER", 0, {{P_E4, 90}, {P_DEC, 30}, {X, 0}}},
        {"MAJ ARP", 0, {{P_E6, 2}, {X, 0}}},
    }, {
        {"JUMP", TM_STEP, {{P_E6, 0, 3}, NT, NT}},
        {"SWEEP", 0, {{P_E4, 0, 110}, NT, NT}},
        {"VIB", 0, {{P_E5, 0, 110}, NT, NT}},
        {"CRUSH", 0, {{P_E3, 0, 120}, NT, NT}},
        {"WAVE", TM_STEP, {{P_E1, 0, 4}, NT, NT}},
        {"DECAY", 0, {{P_DEC, 15, 100}, NT, NT}},
        {"TONE", 0, {{P_E7, 20, 127}, NT, NT}},
        {"LEVEL", 0, {{P_LEVEL, 70, 120}, NT, NT}},
    }},
    /* TINE: FM6 as an electric piano (TILT: Tine) */
    {"TINE", IG_HARMONY, E_FM6, 11, 0, 8, {
        {"CLASSIC EP", 0, NOV},
        {"BRIGHT EP", 0, {{P_E2, 20}, {X, 0}}},
        {"MELLOW EP", 0, {{P_E2, -24}, {P_REV, 40}, {X, 0}}},
        {"TREMOLO EP", 0, {{P_LD_AMP, 50}, {P_LRATE, 70}, {X, 0}}},
        {"CHORUS EP", 0, {{P_CHOR, 80}, {X, 0}}},
        {"BELL EP", 0, {{P_E3, 3}, {P_E2, 10}, {X, 0}}},
        {"DRIVEN EP", 0, {{P_ITYPE, 1}, {P_IA, 70}, {X, 0}}},
        {"FAR EP", 0, {{P_DLY, 50}, {P_REV, 80}, {X, 0}}},
    }, {
        {"BARK", 0, {{P_E2, -40, 40}, NT, NT}},
        {"TINE", 0, {{P_E4, -40, 40}, NT, NT}},
        {"TREM", 0, {{P_LD_AMP, 0, 90}, NT, NT}},
        {"CHOR", 0, {{P_CHOR, 0, 110}, NT, NT}},
        {"DECAY", 0, {{P_DEC, 30, 110}, NT, NT}},
        {"REL", 0, {{P_REL, 10, 100}, NT, NT}},
        {"SPACE", 0, {{P_REV, 0, 100}, NT, NT}},
        {"LEVEL", 0, {{P_LEVEL, 70, 120}, NT, NT}},
    }},
    /* GLINT: FM6 bells and leads, DX7 heritage (TILT: Glint) */
    {"GLINT", IG_HARMONY, E_FM6, 8, 0, 8, {
        {"GLASS BELL", 1, NOV},
        {"MARIMBA", 5, NOV},
        {"FM BRASS", 3, NOV},
        {"FM PLUCK", 7, NOV},
        {"FM PAD", 4, NOV},
        {"TUBULAR", 1, {{P_REV, 70}, {P_DEC, 110}, {X, 0}}},
        {"METAL LEAD", 3, {{P_E2, 30}, {P_E1, 4}, {X, 0}}},
        {"KALIMBA FM", 5, {{P_E3, 4}, {X, 0}}},
    }, {
        {"METAL", 0, {{P_E2, -40, 50}, NT, NT}},
        {"RATIO", TM_STEP, {{P_E3, -8, 8}, NT, NT}},
        {"BITE", 0, {{P_E1, -5, 7}, NT, NT}},
        {"BLOOM", 0, {{P_E4, -40, 50}, NT, NT}},
        {"DECAY", 0, {{P_DEC, 30, 120}, NT, NT}},
        {"ATK", 0, {{P_ATK, 0, 80}, NT, NT}},
        {"SPACE", 0, {{P_REV, 0, 110}, NT, NT}},
        {"LEVEL", 0, {{P_LEVEL, 70, 120}, NT, NT}},
    }},
    /* PLUCK: PHYS, strings and bells (TILT: Pluck) */
    {"PLUCK", IG_HARMONY, E_PHYS, 12, 0, 9, {
        {"NYLON", 2, NOV},
        {"HARP", 8, NOV},
        {"KALIMBA", 4, NOV},
        {"MARIMBA", 1, NOV},
        {"BELL TREE", 0, NOV},
        {"BOWED METAL", 3, NOV},
        {"DRONE STRING", 7, NOV},
        {"KOTO", 2, {{P_E2, 110}, {P_E4, 60}, {X, 0}}},
        {"MUTED", 2, {{P_E3, 30}, {P_E2, 50}, {X, 0}}},
    }, {
        {"BRITE", 0, {{P_E2, 20, 127}, NT, NT}},
        {"DAMP", 0, {{P_E3, 20, 127}, NT, NT}},
        {"POS", 0, {{P_E4, 0, 110}, NT, NT}},
        {"BODY", 0, {{P_E1, 10, 120}, NT, NT}},
        {"BOW", 0, {{P_E6, 0, 110}, NT, NT}},
        {"ACCNT", 0, {{P_E5, 30, 127}, NT, NT}},
        {"SPACE", 0, {{P_REV, 0, 100}, NT, NT}},
        {"LEVEL", 0, {{P_LEVEL, 70, 120}, NT, NT}},
    }},
    /* VOX: VOICE, the formant singer (TILT: Vox; a wildcard) */
    {"VOX", IG_HARMONY, E_FORMANT, 16, 1, 6, {
        {"CHOIR AAH", 0, NOV},
        {"VOX LEAD", 1, NOV},
        {"WHISPER", 3, NOV},
        {"WOW BASS", 2, NOV},
        {"TALKBOX", 1, {{P_E2, 60}, {P_E6, 100}, {X, 0}}},
        {"ROBOT", 1, {{P_E4, 120}, {P_E5, 0}, {X, 0}}},
    }, {
        {"VOWEL", 0, {{P_E0, 0, 127}, NT, NT}},
        {"TALK", 0, {{P_E2, 0, 110}, NT, NT}},
        {"BUZZ", 0, {{P_E4, 10, 127}, NT, NT}},
        {"BRTH", 0, {{P_E5, 0, 110}, NT, NT}},
        {"SHIFT", TM_STEP, {{P_E3, -12, 12}, NT, NT}},
        {"Q", 0, {{P_E6, 10, 127}, NT, NT}},
        {"SPACE", 0, {{P_REV, 0, 100}, NT, NT}},
        {"LEVEL", 0, {{P_LEVEL, 70, 120}, NT, NT}},
    }},
    /* WHEELHOUSE: WHEEL, the drawbar organ (TILT: Wheelhouse, kind "organ"; a wildcard) */
    {"WHEELHOUSE", IG_HARMONY, E_WHEEL, 13, 1, 6, {
        {"FULL ORGAN", 0, NOV},
        {"JAZZ PERC", 1, NOV},
        {"GOSPEL", 2, NOV},
        {"SOFT FLUTE", 3, NOV},
        {"ROCK DRIVE", 4, NOV},
        {"CHAPEL", 2, {{P_E0, 12}, {P_REV, 80}, {X, 0}}},
    }, {
        {"REG", TM_STEP, {{P_E0, 0, 15}, NT, NT}},
        {"SUB", 0, {{P_E1, -8, 8}, NT, NT}},
        {"BODY", 0, {{P_E2, -8, 8}, NT, NT}},
        {"TOP", 0, {{P_E3, -8, 8}, NT, NT}},
        {"CLICK", 0, {{P_E5, 0, 127}, NT, NT}},
        {"DRIVE", 0, {{P_E6, 0, 120}, {P_LEVEL, 108, 92}, NT}},
        {"ROTOR", TM_STEP, {{P_E7, 0, 2}, NT, NT}},
        {"LEVEL", 0, {{P_LEVEL, 70, 120}, NT, NT}},
    }},
};
#undef X
#undef NOV
#undef NT

/* TILT's KIND_CODE -> the TFM instrument that plays it (the closest of its role when TFM has none) */
static const uint8_t TILT_TO_TFM[22] = {
    TI_THUMP,                                           /* drums (Kitbox) */
    TI_SQUELCH, TI_CHORDETTE, TI_RAZOR,                 /* acid, poly, lead */
    TI_THUMP,                                           /* analog (Thump Lab) */
    TI_WOBBLER, TI_HAZE, TI_PIXEL, TI_GLINT,            /* wobble, pad, chip, fm */
    TI_HAZE,                                            /* wave (Morph, a pad) */
    TI_VOX,                                             /* sampler (Grab, fx) */
    TI_TINE, TI_PLUCK, TI_WHEEL,                        /* epiano, pluck, organ */
    TI_RAZOR,                                           /* anthem (a lead) */
    TI_WOBBLER,                                         /* patch (Patchwork, a bass) */
    TI_VOX,                                             /* vox */
    TI_THUMP,                                           /* boom (fx: booms, hits) */
    TI_TINE,                                            /* piano (Parlor) */
    TI_THUMP, TI_THUMP,                                 /* garage, chop (drums) */
    TI_PLUCK,                                           /* guitar (Axe) */
};
static uint32_t tfm_kind(uint32_t i) { return i < TI_N ? TFM_INS[i].tilt : 0u; }
static uint32_t tfm_nsounds(uint32_t i) { return i < TI_N ? TFM_INS[i].nsnd : 1u; }
/* the instrument a track plays (its preset byte, TFM_PRE), TI_NONE none */
static uint32_t tfm_ins_of(const track_t *t)
{
    uint32_t p = t->preset, i;
    if (p < TFM_PRE0)
        return TI_NONE;
    i = (p - TFM_PRE0) / 16u;
    return i < TI_N && TFM_INS[i].eng == t->eng_req && (p - TFM_PRE0) % 16u < TFM_INS[i].nsnd ? i : TI_NONE;
}
static uint32_t tfm_snd_of(const track_t *t) { return (t->preset - TFM_PRE0) % 16u; }

/* track t plays instrument i's sound s: its engine (as EDIT's engine keys load it), the factory preset (as PRESETS
 * does: one undo, the steps kept), the changes, the instrument's mark */
static void tfm_load(track_t *t, uint32_t i, uint32_t s)
{
    const tfm_instr_t *in = &TFM_INS[i % TI_N];
    const tfm_snd_t *sn = &in->snd[s % in->nsnd];
    uint32_t k;
    if (t->eng_req != in->eng)
        set_engine_of(t, in->eng);
    apply_preset_to(t, sn->pre);
    for (k = 0; k < 6u && sn->ov[k].id != 0xFFu; k++) {
        const param_desc_t *d = param_desc_of(in->eng, sn->ov[k].id);
        t->p[sn->ov[k].id] = (int16_t)clamp(sn->ov[k].v, d->min, d->max);
    }
    t->preset = (uint8_t)TFM_PRE(i % TI_N, s % in->nsnd);
}
/* a sound's name (the PRESETS list, NEW SONG, the bottom line): "SQUELCH RUBBER" */
static void tfm_snd_name(uint32_t i, uint32_t s, char *b, uint32_t n)
{
    str_cpy(b, TFM_INS[i % TI_N].name, n);
    str_cpy(b + str_len(b), " ", n - str_len(b));
    str_cpy(b + str_len(b), TFM_INS[i % TI_N].snd[s % TFM_INS[i % TI_N].nsnd].name, n - str_len(b));
}

/* ------------------------------------------------------------ the macros --- */
/* macro m of the track's instrument: its place 0..127, read back from its first target */
static int32_t tfm_macro_get(const track_t *t, uint32_t m)
{
    uint32_t i = tfm_ins_of(t);
    const tfm_tgt_t *g;
    int32_t v, span;
    if (i == TI_NONE)
        return 0;
    g = &TFM_INS[i].mac[m & 7u].t[0];
    span = g->hi - g->lo;
    v = motion_base_value(t, g->id);
    return span ? clamp((v - g->lo) * 127 / span, 0, 127) : 0;
}
static void tfm_macro_set(track_t *t, uint32_t m, int32_t pos)
{
    uint32_t i = tfm_ins_of(t), k;
    if (i == TI_NONE)
        return;
    pos = clamp(pos, 0, 127);
    for (k = 0; k < 3u; k++) {
        const tfm_tgt_t *g = &TFM_INS[i].mac[m & 7u].t[k];
        const param_desc_t *d;
        int32_t v;
        if (g->id == 0xFFu)
            break;
        d = param_desc_of(TFM_INS[i].eng, g->id);
        v = g->lo + ((g->hi - g->lo) * pos + (g->hi >= g->lo ? 63 : -63)) / 127;   /* (rounded) */
        t->p[g->id] = (int16_t)clamp(v, d->min, d->max);
        motion_capture(t, g->id, t->p[g->id]);          /* (REC: recorded as automation, as a knob turn is) */
    }
}

/* ------------------------------------------------------------ the rolls --- */
enum { LU_BAND, LU_WILD, LU_BAND1, LU_N };              /* NEW SONG's LINEUP */
static const char *const LU_NAME[LU_N] = {"BAND", "WILD", "BAND + 1"};
/* a random instrument of group g (wild: any, the wildcards included; else not a wildcard) */
static uint32_t tfm_pick_ins(tfm_rng_t *r, uint32_t g, int wild)
{
    uint32_t i, n = 0, k;
    for (i = 0; i < TI_N; i++)
        n += wild || (TFM_INS[i].group == g && !TFM_INS[i].wild);
    k = tfm_pick(r, n ? n : 1u);
    for (i = 0; i < TI_N; i++)
        if ((wild || (TFM_INS[i].group == g && !TFM_INS[i].wild)) && !k--)
            return i;
    return TI_SQUELCH;
}
/* BAND, track i: T1 a bass, T2 harmony, T3 a lead (or harmony, 1 in 3), T4 drums */
static void tfm_band_track(tfm_rng_t *r, tfm_setup_t *s, uint32_t i)
{
    static const uint8_t G[NTRK] = {IG_BASS, IG_HARMONY, IG_LEAD, IG_DRUMS};
    uint32_t g = G[i % NTRK];
    if (i == 2u && tfm_pick(r, 3) == 0u)
        g = IG_HARMONY;
    s->ins[i] = (uint8_t)tfm_pick_ins(r, g, 0);
    s->pre[i] = (uint8_t)tfm_pick(r, TFM_INS[s->ins[i]].nsnd);
}
/* the lineup's track i rolled: BAND by role, WILD any instrument; the same instrument twice never the same sound */
static void tfm_roll_track(tfm_rng_t *r, tfm_setup_t *s, uint32_t i, uint32_t mode, uint32_t wild_track)
{
    uint32_t k, tries;
    for (tries = 0; tries < 8u; tries++) {
        if (mode == LU_WILD || (mode == LU_BAND1 && i == wild_track)) {
            s->ins[i] = (uint8_t)tfm_pick_ins(r, 0, 1);
            s->pre[i] = (uint8_t)tfm_pick(r, TFM_INS[s->ins[i]].nsnd);
        } else {
            tfm_band_track(r, s, i);
        }
        for (k = 0; k < NTRK && (k == i || s->ins[k] != s->ins[i] || s->pre[k] != s->pre[i]); k++)
            ;
        if (k == NTRK)
            break;
    }
}
