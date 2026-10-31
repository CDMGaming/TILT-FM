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
    uint8_t hasmp;                                      /* 1: one of the five named presets: its eight macros' places (percent) */
    uint8_t mp[8];
} tfm_snd_t;
#define TCV_LIN 0u
#define TCV_EXP 1u                                      /* a share of the turn squared: the low end gets more room */
#define TCV_GATE 2u                                     /* moves only over 70 % of the turn */
typedef struct { uint8_t id; int8_t lo, hi; uint8_t cv; } tfm_tgt_t;
#define TFM_MSW 0xF0u                                   /* (round 8: nsw of MORPH, macro 1: its targets are TFM_MORPH's) */
typedef struct {
    const char *name;                                   /* the dial's label (7 at most) */
    uint8_t nsw;                                        /* a SWITCH (stepped, named): its values sw[], the first target's id */
    int8_t sw[10];
    const char *const *swn;                             /* their names (0: as the parameter shows the value) */
    tfm_tgt_t t[4];                                     /* (id 0xFF: none) */
} tfm_macro_t;
enum { IG_DRUMS, IG_BASS, IG_HARMONY, IG_LEAD, IG_N };
typedef struct {
    const char *name, *desc;                            /* "THUMP LAB", "drum kit . bumpers pop on hits" */
    uint8_t group, eng, tilt, wild;                     /* TILT group; the engine; TILT's KIND_CODE; a wildcard */
    uint8_t nsnd, types;                                /* sounds (5 named + the older ones); SETUP's types (tfm_dice.c TY_INS) */
    uint16_t col[5];                                    /* BG SURF ACC MUTE INK of its face */
    tfm_ov_t vo[16];                                    /* the VOICING: fixed values that make it this instrument */
    tfm_macro_t mac[8];                                 /* KNOB 1..4 page 1, then page 2 */
    tfm_snd_t snd[15];
} tfm_instr_t;
#define X 0xFFu
#define NOV {{X, 0}}
#define NT {X, 0, 0, 0}
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

static const char *const SWN_2_1[] = {"1/1", "1/2", "1/4", "1/8", "8T", "1/16", "16T", "1/32"};
static const char *const SWN_2_6[] = {"0", "-1 OCT", "-2 OCT"};
static const char *const SWN_3_0[] = {"AUTO", "AUTO 7", "MAJ", "MIN", "7", "MAJ7", "MIN7", "SUS4", "POWER"};   /* (round 3: AUTO follows the scale) */
static const char *const SWN_3_1[] = {"CLOSE", "OPEN", "DROP2", "SPREAD", "STACK"};
static const char *const SWN_6_0[] = {"OFF", "OCT", "5TH"};   /* (round 3: no major arp: it is out of key in minor modes) */
static const char *const SWN_6_1[] = {"PULSE", "TRI", "SAW", "NOISE", "WAVE"};
static const char *const SWN_11_2[] = {"OFF", "SLOW", "FAST"};
static const tfm_instr_t TFM_INS[TI_N] = {
    /* THUMP LAB */
    {"THUMP LAB", "drum kit \xB7 bumpers pop on hits", IG_DRUMS, E_DRUM, 4, 0, 15, 1, {RGB(21, 10, 8), RGB(31, 14, 11), RGB(255, 122, 61), RGB(181, 148, 138), RGB(248, 236, 230)}, {{P_E0, 0}, {P_E6, 0}, {P_LN0, 110}, {P_LN1, 100}, {P_LN2, 92}, {P_LN3, 110}, {P_LN4, 110}, {P_LN5, 80}, {P_LN6, 80}, {P_LN7, 80}, {P_ITYPE, 1}, {P_IMIX, 20}, {P_REV, 10}, {P_E2, 26}, {X, 0}}, {
        {"MORPH", TFM_MSW, {0}, 0, {NT, NT, NT, NT}},   /* (round 8: TFM_MORPH below) */
        {"PUNCH", 0, {0}, 0, {{P_LN0, 96, 127, 0}, {P_E7, 0, 80, 0}, {P_E5, 40, 120, 0}, {P_LEVEL, 108, 102, 0}}},
        {"SMACK", 0, {0}, 0, {{P_E4, 10, 120, 0}, {P_LN1, 80, 115, 0}, {P_LN2, 80, 115, 0}, {P_LEVEL, 106, 100, 0}}},
        {"TIME", 0, {0}, 0, {{P_E3, 8, 120, TCV_EXP}, {P_LEVEL, 104, 94, 0}, NT, NT}},
        {"ROOM", 0, {0}, 0, {{P_REV, 0, 90, 0}, {P_DLY, 0, 25, TCV_GATE}, NT, NT}},
        {"TUNE", 0, {0}, 0, {{P_E1, 16, 112, 0}, NT, NT, NT}},
        {"DIRT", 0, {0}, 0, {{P_IA, 0, 110, 0}, {P_IMIX, 20, 100, 0}, {P_LEVEL, 106, 104, 0}, NT}},
        {"HATS", 0, {0}, 0, {{P_LN3, 80, 127, 0}, {P_LN4, 80, 127, 0}, NT, NT}},
    }, {
        {"WAREHOUSE", 1, {{P_E0, 0}, {P_DLY, 8}, {X, 0}}, 1, {0, 60, 80, 70, 50, 40, 35, 95}},
        {"BOOM BAP", 1, {{P_E0, 4}, {P_DLY, 24}, {X, 0}}, 1, {0, 55, 75, 75, 35, 35, 25, 100}},
        {"DISCO", 1, {{P_E0, 3}, {P_DLY, 36}, {X, 0}}, 1, {0, 40, 85, 25, 70, 65, 5, 95}},
        {"TRASH CAN", 1, {{P_E0, 1}, {P_DLY, 31}, {X, 0}}, 1, {0, 75, 80, 40, 55, 50, 80, 70}},
        {"ARCADE", 1, {{P_E0, 2}, {P_DLY, 16}, {X, 0}}, 1, {0, 60, 90, 30, 20, 85, 50, 80}},
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
    }},
    /* SQUELCH */
    {"SQUELCH", "acid bass \xB7 spins with the cutoff", IG_BASS, E_ANALOG, 1, 0, 15, 2, {RGB(14, 18, 6), RGB(30, 42, 10), RGB(200, 242, 90), RGB(155, 168, 137), RGB(242, 246, 232)}, {{P_VOICE, 2}, {P_FTYPE, 0}, {P_E0, 0}, {P_ED_FLT, 30}, {P_DEC, 40}, {P_ITYPE, 4}, {P_IMIX, 0}, {P_M1SRC, 3}, {P_M1DST, 2}, {P_M1AMT, 20}, {X, 0}}, {
        {"MORPH", TFM_MSW, {0}, 0, {NT, NT, NT, NT}},   /* (round 8: TFM_MORPH below) */
        {"CUTOFF", 0, {0}, 0, {{P_E4, 18, 112, TCV_EXP}, NT, NT, NT}},
        {"RESO", 0, {0}, 0, {{P_E5, 10, 118, 0}, {P_LEVEL, 108, 102, 0}, NT, NT}},
        {"ENV MOD", 0, {0}, 0, {{P_ED_FLT, 0, 60, 0}, {P_DEC, 30, 55, 0}, {P_LEVEL, 108, 102, 0}, NT}},
        {"DECAY", 0, {0}, 0, {{P_DEC, 20, 100, 0}, {P_REL, 10, 50, 0}, NT, NT}},
        {"BITE", 0, {0}, 0, {{P_E6, 0, 120, 0}, {P_LEVEL, 108, 99, 0}, NT, NT}},
        {"WAVE", 0, {0}, 0, {{P_E2, 0, 127, 0}, NT, NT, NT}},
        {"SLIDE", 0, {0}, 0, {{P_GLIDE, 0, 80, TCV_EXP}, NT, NT, NT}},
    }, {
        {"RUBBER", 4, {{P_IMIX, 20}, {P_LEVEL, 106}, {X, 0}}, 1, {0, 45, 40, 40, 45, 35, 30, 40}},
        {"ACID RAIN", 4, {{P_IMIX, 30}, {P_LEVEL, 107}, {X, 0}}, 1, {0, 70, 62, 62, 55, 40, 25, 50}},
        {"LOW RIDER", 4, {{P_E0, 1}, {P_IMIX, 24}, {P_LEVEL, 106}, {X, 0}}, 1, {0, 30, 50, 30, 60, 55, 70, 30}},
        {"ZAPPER", 4, {{P_IMIX, 39}, {P_LEVEL, 107}, {X, 0}}, 1, {0, 60, 80, 85, 20, 30, 40, 20}},
        {"GNARLY", 4, {{P_IMIX, 80}, {P_LEVEL, 108}, {X, 0}}, 1, {0, 55, 88, 55, 40, 85, 25, 45}},
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
    }},
    /* WOBBLER */
    {"WOBBLER", "wobble bass \xB7 the magnet shakes with the LFO", IG_BASS, E_TRIO, 5, 0, 13, 2, {RGB(10, 11, 22), RGB(22, 26, 48), RGB(140, 123, 255), RGB(142, 138, 176), RGB(238, 235, 255)}, {{P_VOICE, 1}, {P_LSYNC, 6}, {P_LTRIG, 0}, {P_LWAVE, 0}, {P_LD_FLT, 40}, {P_ITYPE, 3}, {P_IMIX, 0}, {P_E3, 6}, {X, 0}}, {
        {"MORPH", TFM_MSW, {0}, 0, {NT, NT, NT, NT}},   /* (round 8: TFM_MORPH below) */
        {"WOB", 0, {0}, 0, {{P_LD_FLT, 0, 63, 0}, {P_E6, 10, 60, 0}, NT, NT}},
        {"RATE", 8, {3, 4, 5, 6, 7, 8, 9, 10}, SWN_2_1, {{P_LSYNC, 0, 0, 0}, NT, NT, NT}},
        {"GROWL", 0, {0}, 0, {{P_IMIX, 0, 100, 0}, {P_IA, 20, 90, 0}, NT, NT}},
        {"CUT", 0, {0}, 0, {{P_E5, 15, 110, TCV_EXP}, NT, NT, NT}},
        {"DIRT", 0, {0}, 0, {{P_DIST, 0, 90, 0}, {P_LEVEL, 106, 98, 0}, NT, NT}},
        {"DROP", 3, {0, -12, -24}, SWN_2_6, {{P_TRANS, 0, 0, 0}, NT, NT, NT}},
        {"SLIDE", 0, {0}, 0, {{P_GLIDE, 0, 80, 0}, NT, NT, NT}},
    }, {
        {"HALF STEP", 0, {{P_E3, 12}, {P_SPRD, 30}, {X, 0}}, 1, {0, 60, 40, 30, 55, 20, 0, 10}},
        {"YOY", 0, {{P_E3, 16}, {P_SPRD, 39}, {X, 0}}, 1, {0, 75, 20, 70, 50, 30, 0, 20}},
        {"TRASH FLOOR", 0, {{P_E3, 24}, {P_SPRD, 60}, {X, 0}}, 1, {0, 85, 60, 90, 40, 80, 40, 10}},
        {"LASER WOB", 0, {{P_E3, 8}, {P_SPRD, 20}, {X, 0}}, 1, {0, 55, 90, 40, 70, 30, 0, 50}},
        {"SUBWAY", 0, {{P_E3, 10}, {P_SPRD, 24}, {X, 0}}, 1, {0, 50, 30, 20, 35, 40, 100, 30}},
        {"CLASSIC WOB", 0, {{P_LD_FLT, 40}, {P_LSYNC, 6}, {P_E5, 50}, {P_E6, 70}, {X, 0}}},
        {"SLOW GROWL", 0, {{P_LD_FLT, 50}, {P_LSYNC, 5}, {P_E6, 95}, {X, 0}}},
        {"YOI", 0, {{P_LD_FLT, 55}, {P_LSYNC, 8}, {P_E6, 100}, {P_E5, 35}, {X, 0}}},
        {"TRIPLET WUB", 0, {{P_LD_FLT, 45}, {P_LSYNC, 7}, {P_E5, 60}, {X, 0}}},
        {"CRUSHED WOB", 0, {{P_LD_FLT, 40}, {P_LSYNC, 6}, {P_ITYPE, 5}, {P_IA, 70}, {X, 0}}},
        {"REESE", 0, {{P_E3, 30}, {P_LD_FLT, 15}, {P_LSYNC, 3}, {P_E5, 40}, {X, 0}}},
        {"SUB WOB", 0, {{P_E2, -12}, {P_LD_FLT, 35}, {P_LSYNC, 6}, {P_E5, 30}, {X, 0}}},
        {"FAT STAB", 0, NOV},
    }},
    /* CHORDETTE */
    {"CHORDETTE", "one key, whole chords \xB7 targets drop per note", IG_HARMONY, E_ANALOG, 2, 0, 14, 4, {RGB(18, 10, 20), RGB(36, 20, 42), RGB(255, 143, 208), RGB(184, 152, 172), RGB(255, 238, 247)}, {{P_VOICE, 0}, {P_CHRD, 1}, {P_FTYPE, 0}, {P_E0, 0}, {P_E2, 64}, {P_CHOR, 20}, {P_ITYPE, 5}, {P_IMIX, 0}, {P_LSYNC, 5}, {P_LD_AMP, 0}, {X, 0}}, {
        {"MORPH", TFM_MSW, {0}, 0, {NT, NT, NT, NT}},   /* (round 8: TFM_MORPH below) */
        {"CHORD", 9, {1, 2, 3, 4, 5, 6, 7, 8, 9}, SWN_3_0, {{P_CHRD, 0, 0, 0}, NT, NT, NT}},
        {"VOICE", 5, {0, 1, 2, 3, 4}, SWN_3_1, {{P_VOIC, 0, 0, 0}, NT, NT, NT}},
        {"WARM", 0, {0}, 0, {{P_E4, 30, 110, 0}, {P_E5, 40, 10, 0}, {P_LEVEL, 110, 100, 0}, NT}},
        {"SWELL", 0, {0}, 0, {{P_ATK, 0, 90, TCV_EXP}, {P_REL, 10, 110, 0}, NT, NT}},
        {"SHIMMER", 0, {0}, 0, {{P_CHOR, 0, 60, 0}, {P_E1, 0, 20, 0}, NT, NT}},
        {"SPACE", 0, {0}, 0, {{P_REV, 0, 90, 0}, {P_DLY, 0, 40, TCV_GATE}, NT, NT}},
        {"PUMP", 0, {0}, 0, {{P_LD_AMP, 0, 80, 0}, NT, NT, NT}},
    }, {
        {"HOUSE STAB", 1, {{P_IMIX, 7}, {P_IA, 18}, {X, 0}}, 1, {0, 0, 0, 60, 10, 20, 30, 40}},
        {"SUNDAY", 1, {{P_IMIX, 14}, {P_IA, 26}, {X, 0}}, 1, {0, 10, 40, 55, 50, 40, 50, 10}},
        {"VELVET", 1, {{P_IMIX, 7}, {P_IA, 18}, {X, 0}}, 1, {0, 30, 0, 35, 70, 60, 60, 20}},
        {"PUMPER", 1, {{P_IMIX, 0}, {P_IA, 10}, {X, 0}}, 1, {0, 0, 0, 70, 20, 30, 20, 90}},
        {"MIXTAPE", 1, {{P_IMIX, 56}, {P_IA, 74}, {X, 0}}, 1, {0, 20, 20, 45, 40, 70, 30, 30}},
        {"WARM STAB", 1, {{P_CHRD, 2}, {P_ATK, 4}, {P_REL, 40}, {X, 0}}},
        {"HOUSE ORGAN", 1, {{P_CHRD, 1}, {P_E0, 1}, {P_ATK, 0}, {P_DEC, 40}, {P_SUS, 60}, {X, 0}}},
        {"PWM CHORDS", 3, {{P_CHRD, 1}, {X, 0}}},
        {"STRING PAD", 11, {{P_CHRD, 2}, {X, 0}}},
        {"DUB CHORD", 1, {{P_CHRD, 2}, {P_DLY, 60}, {P_DEC, 35}, {P_SUS, 20}, {X, 0}}},
        {"BRIGHT 7TH", 0, {{P_CHRD, 2}, {P_VOICE, 0}, {P_ATK, 2}, {P_REL, 50}, {X, 0}}},
        {"SOFT TRIAD", 1, {{P_CHRD, 1}, {P_VOIC, 1}, {X, 0}}},
        {"POWER", 0, {{P_CHRD, 9}, {P_VOICE, 0}, {P_E6, 60}, {X, 0}}},
        {"SUS GLOW", 3, {{P_CHRD, 8}, {P_ATK, 30}, {X, 0}}},
    }},
    /* RAZOR */
    {"RAZOR", "unison lead \xB7 the ball rides the ramp", IG_LEAD, E_ANALOG, 3, 0, 14, 8, {RGB(11, 16, 20), RGB(20, 32, 40), RGB(79, 212, 255), RGB(135, 162, 176), RGB(232, 248, 255)}, {{P_VOICE, 3}, {P_E0, 0}, {P_FTYPE, 0}, {P_LRATE, 20}, {P_LD_PIT, 0}, {P_LFADE, 60}, {P_DLY, 0}, {P_ITYPE, 2}, {P_IMIX, 0}, {X, 0}}, {
        {"MORPH", TFM_MSW, {0}, 0, {NT, NT, NT, NT}},   /* (round 8: TFM_MORPH below) */
        {"SCREAM", 0, {0}, 0, {{P_E5, 10, 110, 0}, {P_E6, 0, 70, 0}, {P_LEVEL, 108, 92, 0}, NT}},
        {"SWEEP", 0, {0}, 0, {{P_ED_FLT, -20, 60, 0}, {P_DEC, 20, 70, 0}, NT, NT}},
        {"CUT", 0, {0}, 0, {{P_E4, 30, 120, TCV_EXP}, {P_LEVEL, 122, 100, 0}, NT, NT}},
        {"GLIDE", 0, {0}, 0, {{P_GLIDE, 0, 80, 0}, NT, NT, NT}},
        {"WIDTH", 0, {0}, 0, {{P_DETUNE, 0, 80, 0}, {P_SPRD, 0, 127, 0}, NT, NT}},
        {"ATTACK", 0, {0}, 0, {{P_ATK, 0, 80, 0}, NT, NT, NT}},
        {"VIB", 0, {0}, 0, {{P_LD_PIT, 0, 2, 0}, NT, NT, NT}},
    }, {
        {"HERO", 0, {{P_DLY, 27}, {X, 0}}, 1, {0, 53, 47, 76, 56, 28, 24, 40}},
        {"TRANCE GATE", 0, {{P_E0, 1}, {P_DLY, 54}, {X, 0}}, 1, {0, 35, 70, 55, 10, 80, 10, 10}},
        {"ARENA", 0, {{P_DLY, 45}, {X, 0}}, 1, {0, 60, 30, 50, 40, 90, 40, 50}},
        {"SINE RIDER", 0, {{P_E0, 3}, {P_DLY, 35}, {X, 0}}, 1, {0, 10, 20, 40, 70, 20, 60, 70}},
        {"HOT WIRE", 0, {{P_DLY, 18}, {X, 0}}, 1, {0, 90, 75, 70, 20, 50, 10, 20}},
        {"SAW LEAD", 0, NOV},
        {"SYNC SCREAM", 0, {{P_E0, 5}, {P_E5, 70}, {P_ED_PIT, 30}, {X, 0}}},
        {"SUPER UNISON", 0, {{P_VOICE, 3}, {P_E1, 40}, {P_SPRD, 90}, {X, 0}}},
        {"BRASS LEAD", 9, NOV},
        {"SQUARE LEAD", 0, {{P_E0, 1}, {P_E4, 80}, {X, 0}}},
        {"GLIDE LEAD", 0, {{P_GLIDE, 50}, {P_VOICE, 2}, {X, 0}}},
        {"HOOVER", 0, {{P_E0, 4}, {P_VOICE, 3}, {P_E1, 60}, {P_ED_PIT, -20}, {X, 0}}},
        {"RAVE STAB", 6, NOV},
        {"WIND LEAD", 10, NOV},
    }},
    /* HAZE */
    {"HAZE", "slow pad \xB7 the lights glow and drift", IG_HARMONY, E_TRIO, 6, 0, 13, 16, {RGB(14, 12, 20), RGB(28, 24, 40), RGB(185, 166, 255), RGB(154, 146, 180), RGB(242, 238, 255)}, {{P_VOICE, 0}, {P_ATK, 60}, {P_REL, 70}, {P_LRATE, 15}, {P_LTRIG, 1}, {P_LD_PIT, 0}, {P_REV, 60}, {P_ITYPE, 5}, {P_IMIX, 0}, {X, 0}}, {
        {"MORPH", TFM_MSW, {0}, 0, {NT, NT, NT, NT}},   /* (round 8: TFM_MORPH below) */
        {"DRIFT", 0, {0}, 0, {{P_LD_PIT, 0, 2, TCV_GATE}, {P_E3, 5, 30, 0}, NT, NT}},
        {"DUST", 0, {0}, 0, {{P_IMIX, 0, 100, 0}, NT, NT, NT}},
        {"SPACE", 0, {0}, 0, {{P_REV, 10, 127, 0}, {P_LEVEL, 108, 99, 0}, NT, NT}},
        {"BLOOM", 0, {0}, 0, {{P_ATK, 10, 120, TCV_EXP}, {P_REL, 30, 120, 0}, NT, NT}},
        {"TONE", 0, {0}, 0, {{P_E5, 10, 28, TCV_EXP}, NT, NT, NT}},
        {"SHINE", 0, {0}, 0, {{P_CHOR, 0, 110, 0}, NT, NT, NT}},
        {"MOVE", 0, {0}, 0, {{P_LD_FLT, 0, 60, 0}, NT, NT, NT}},
    }, {
        {"FOG", 4, {{P_SUS, 93}, {P_REL, 55}, {X, 0}}, 1, {0, 20, 30, 60, 40, 30, 40, 30}},
        {"MORNING", 4, {{P_SUS, 86}, {P_REL, 52}, {X, 0}}, 1, {0, 10, 0, 50, 35, 70, 50, 20}},
        {"VHS", 4, {{P_SUS, 80}, {P_REL, 49}, {X, 0}}, 1, {0, 50, 55, 40, 30, 45, 60, 40}},
        {"AURORA", 4, {{P_SUS, 100}, {P_REL, 58}, {X, 0}}, 1, {0, 30, 10, 75, 60, 60, 80, 70}},
        {"DEEP END", 4, {{P_SUS, 120}, {P_REL, 67}, {X, 0}}, 1, {0, 40, 20, 90, 80, 20, 30, 20}},
        {"WARM HAZE", 4, {{P_ATK, 70}, {P_REL, 90}, {P_REV, 70}, {X, 0}}},
        {"DUSTY", 4, {{P_ATK, 60}, {P_ITYPE, 5}, {P_IA, 80}, {P_IMIX, 70}, {P_REV, 60}, {X, 0}}},
        {"GLASS AIR", 3, {{P_ATK, 80}, {P_REL, 100}, {P_REV, 90}, {X, 0}}},
        {"DARK DRIFT", 0, {{P_E5, 30}, {P_ATK, 90}, {P_REV, 80}, {X, 0}}},
        {"CHOIR FOG", 4, {{P_ATK, 100}, {P_CHOR, 80}, {P_REV, 90}, {X, 0}}},
        {"TAPE PAD", 4, {{P_LRATE, 30}, {P_ATK, 70}, {P_REV, 60}, {X, 0}}},
        {"BRIGHT SWELL", 1, {{P_ATK, 90}, {P_REL, 100}, {P_E5, 90}, {X, 0}}},
        {"LOW CLOUD", 0, {{P_E1, -12}, {P_E5, 40}, {P_ATK, 80}, {P_REV, 70}, {X, 0}}},
    }},
    /* PIXEL */
    {"PIXEL", "chip lead \xB7 the sprite jumps on notes", IG_LEAD, E_LOFI, 7, 0, 14, 8, {RGB(8, 16, 12), RGB(16, 32, 24), RGB(92, 255, 143), RGB(134, 168, 146), RGB(234, 255, 240)}, {{P_VOICE, 1}, {P_E1, 0}, {P_DEC, 30}, {P_ED_PIT, 0}, {X, 0}}, {
        {"MORPH", TFM_MSW, {0}, 0, {NT, NT, NT, NT}},   /* (round 8: TFM_MORPH below) */
        {"JUMP", 3, {0, 1, 2}, SWN_6_0, {{P_E6, 0, 0, 0}, NT, NT, NT}},
        {"WAVE", 5, {0, 1, 2, 3, 4}, SWN_6_1, {{P_E1, 0, 0, 0}, NT, NT, NT}},
        {"SWEEP", 0, {0}, 0, {{P_E4, 0, 25, 0}, {P_LEVEL, 104, 110, 0}, NT, NT}},
        {"CRUSH", 0, {0}, 0, {{P_E3, 0, 120, 0}, NT, NT, NT}},
        {"VIB", 0, {0}, 0, {{P_E5, 0, 110, 0}, NT, NT, NT}},
        {"DECAY", 0, {0}, 0, {{P_DEC, 15, 100, 0}, NT, NT, NT}},
        {"DUTY", 0, {0}, 0, {{P_E2, 0, 120, 0}, {P_LEVEL, 110, 104, 0}, NT, NT}},
    }, {
        {"1UP", 0, {{P_E7, 94}, {X, 0}}, 1, {0, 0, 0, 0, 30, 20, 25, 70}},
        {"BOSS LEVEL", 0, {{P_E1, 2}, {P_E7, 84}, {X, 0}}, 1, {0, 30, 0, 0, 50, 30, 0, 20}},
        {"WARP ZONE", 0, {{P_E7, 105}, {X, 0}}, 1, {0, 0, 40, 90, 20, 40, 30, 10}},
        {"HIGH SCORE", 0, {{P_E7, 73}, {X, 0}}, 1, {0, 60, 70, 0, 10, 80, 90, 20}},
        {"GLITCH", 0, {{P_E7, 73}, {X, 0}}, 1, {0, 90, 80, 0, 90, 70, 20, 30}},
        {"PULSE LEAD", 0, NOV},
        {"COIN ARP", 2, NOV},
        {"WAVE LEAD", 3, NOV},
        {"STEP LEAD", 4, NOV},
        {"GAME BASS", 1, NOV},
        {"1BIT BUZZ", 0, {{P_E0, 3}, {X, 0}}},
        {"VIBRATO", 0, {{P_E5, 70}, {X, 0}}},
        {"LASER", 0, {{P_E4, 90}, {P_DEC, 30}, {X, 0}}},
        {"MAJ ARP", 0, {{P_E6, 1}, {X, 0}}},
    }},
    /* TINE */
    {"TINE", "electric piano \xB7 notes light the rollovers", IG_HARMONY, E_FM6, 11, 0, 13, 4, {RGB(20, 16, 10), RGB(38, 30, 18), RGB(255, 197, 107), RGB(180, 164, 134), RGB(255, 246, 230)}, {{P_LRATE, 60}, {P_LD_AMP, 0}, {P_CHOR, 20}, {P_ITYPE, 1}, {P_IMIX, 0}, {X, 0}}, {
        {"MORPH", TFM_MSW, {0}, 0, {NT, NT, NT, NT}},   /* (round 8: TFM_MORPH below) */
        {"BARK", 0, {0}, 0, {{P_E2, -40, 40, 0}, NT, NT, NT}},
        {"TINE", 0, {0}, 0, {{P_E4, -40, 40, 0}, NT, NT, NT}},
        {"TREM", 0, {0}, 0, {{P_LD_AMP, 0, 90, 0}, NT, NT, NT}},
        {"CHORUS", 0, {0}, 0, {{P_CHOR, 0, 110, 0}, NT, NT, NT}},
        {"DECAY", 0, {0}, 0, {{P_DEC, 30, 110, 0}, NT, NT, NT}},
        {"WARMTH", 0, {0}, 0, {{P_IMIX, 0, 80, 0}, {P_LEVEL, 106, 110, 0}, NT, NT}},
        {"SPACE", 0, {0}, 0, {{P_REV, 0, 100, 0}, NT, NT, NT}},
    }, {
        {"SUITCASE", 0, {{P_REL, 55}, {X, 0}}, 1, {0, 50, 45, 20, 30, 50, 30, 20}},
        {"LOUNGE", 0, {{P_REL, 64}, {X, 0}}, 1, {0, 30, 40, 59, 0, 33, 0, 0}},
        {"BARK BARK", 0, {{P_REL, 41}, {X, 0}}, 1, {0, 85, 55, 15, 25, 40, 55, 10}},
        {"DREAMY", 0, {{P_REL, 77}, {X, 0}}, 1, {0, 25, 60, 55, 92, 65, 0, 70}},
        {"CHURCH BASEMENT", 0, {{P_REL, 100}, {X, 0}}, 1, {0, 15, 10, 100, 0, 100, 30, 95}},
        {"CLASSIC EP", 0, NOV},
        {"BRIGHT EP", 0, {{P_E2, 20}, {X, 0}}},
        {"MELLOW EP", 0, {{P_E2, -24}, {P_REV, 40}, {X, 0}}},
        {"TREMOLO EP", 0, {{P_LD_AMP, 50}, {P_LRATE, 70}, {X, 0}}},
        {"CHORUS EP", 0, {{P_CHOR, 80}, {X, 0}}},
        {"BELL EP", 0, {{P_E3, 3}, {P_E2, 10}, {X, 0}}},
        {"DRIVEN EP", 0, {{P_ITYPE, 1}, {P_IA, 70}, {X, 0}}},
        {"FAR EP", 0, {{P_DLY, 50}, {P_REV, 80}, {X, 0}}},
    }},
    /* GLINT */
    {"GLINT", "FM bells \xB7 stars sparkle on notes", IG_HARMONY, E_FM6, 8, 0, 13, 40, {RGB(10, 18, 20), RGB(20, 36, 40), RGB(127, 240, 230), RGB(134, 170, 166), RGB(234, 255, 252)}, {{P_E7, 1}, {P_E3, 5}, {P_REV, 40}, {P_ITYPE, 8}, {P_IMIX, 0}, {X, 0}}, {
        {"MORPH", TFM_MSW, {0}, 0, {NT, NT, NT, NT}},   /* (round 8: TFM_MORPH below) */
        {"METAL", 0, {0}, 0, {{P_E2, -40, 50, 0}, {P_LEVEL, 106, 100, 0}, NT, NT}},
        {"RATIO", 5, {-4, -2, 0, 2, 4}, 0, {{P_E3, 0, 0, 0}, NT, NT, NT}},
        {"BLOOM", 0, {0}, 0, {{P_E4, -40, 50, 0}, NT, NT, NT}},
        {"DECAY", 0, {0}, 0, {{P_DEC, 30, 120, 0}, NT, NT, NT}},
        {"BITE", 0, {0}, 0, {{P_E1, -5, 7, 0}, NT, NT, NT}},
        {"ATTACK", 0, {0}, 0, {{P_ATK, 0, 80, 0}, NT, NT, NT}},
        {"SPACE", 0, {0}, 0, {{P_REV, 0, 110, 0}, NT, NT, NT}},
    }, {
        {"MUSIC BOX", 1, {{P_IMIX, 9}, {P_DLY, 4}, {X, 0}}, 1, {0, 50, 30, 50, 15, 30, 0, 25}},         /* (round 4: RATIO at its in-tune place, 30 %) */
        {"TEMPLE", 1, {{P_IMIX, 35}, {P_DLY, 16}, {X, 0}}, 1, {0, 55, 30, 60, 100, 20, 40, 100}},
        {"ICE", 1, {{P_IMIX, 81}, {P_DLY, 36}, {X, 0}}, 1, {0, 55, 30, 90, 40, 10, 60, 70}},
        {"CHIMES", 1, {{P_IMIX, 0}, {P_DLY, 0}, {X, 0}}, 1, {0, 70, 30, 30, 90, 60, 0, 20}},
        {"ROBOT BELLS", 1, {{P_IMIX, 0}, {P_DLY, 0}, {X, 0}}, 1, {0, 95, 30, 95, 30, 95, 10, 10}},
        {"GLASS BELL", 1, NOV},
        {"MARIMBA", 5, NOV},
        {"FM BRASS", 3, NOV},
        {"FM PLUCK", 7, NOV},
        {"FM PAD", 4, NOV},
        {"TUBULAR", 1, {{P_REV, 70}, {P_DEC, 110}, {X, 0}}},
        {"METAL LEAD", 3, {{P_E2, 30}, {P_E1, 4}, {X, 0}}},
        {"KALIMBA FM", 5, {{P_E3, 4}, {X, 0}}},
    }},
    /* PLUCK */
    {"PLUCK", "plucked strings \xB7 flippers kick on notes", IG_HARMONY, E_PHYS, 12, 0, 14, 12, {RGB(18, 14, 8), RGB(36, 28, 16), RGB(232, 176, 122), RGB(168, 152, 128), RGB(255, 244, 230)}, {{P_E0, 1}, {P_REV, 20}, {P_ITYPE, 1}, {P_IMIX, 0}, {X, 0}}, {
        {"MORPH", TFM_MSW, {0}, 0, {NT, NT, NT, NT}},   /* (round 8: TFM_MORPH below) */
        {"BRIGHT", 0, {0}, 0, {{P_E2, 20, 127, 0}, {P_E4, 40, 20, 0}, {P_LEVEL, 106, 100, 0}, NT}},
        {"DAMP", 0, {0}, 0, {{P_E3, 20, 127, 0}, NT, NT, NT}},
        {"BODY", 0, {0}, 0, {{P_E1, 10, 120, 0}, NT, NT, NT}},
        {"PICK", 0, {0}, 0, {{P_E4, 0, 110, 0}, NT, NT, NT}},
        {"BOW", 0, {0}, 0, {{P_E6, 0, 110, 0}, {P_LEVEL, 106, 90, 0}, NT, NT}},
        {"ACCENT", 0, {0}, 0, {{P_E5, 30, 127, 0}, NT, NT, NT}},
        {"SPACE", 0, {0}, 0, {{P_REV, 0, 100, 0}, {P_LEVEL, 108, 99, 0}, NT, NT}},
    }, {
        {"KOTO", 2, {{P_DLY, 24}, {X, 0}}, 1, {0, 60, 40, 40, 30, 0, 60, 30}},
        {"NYLON", 2, {{P_DLY, 8}, {X, 0}}, 1, {0, 25, 25, 55, 40, 0, 40, 25}},                 /* (round 4: DAMP where the string is in tune) */
        {"MUTED", 2, {{P_DLY, 4}, {X, 0}}, 1, {0, 30, 10, 40, 50, 0, 70, 15}},
        {"HARP", 2, {{P_DLY, 40}, {X, 0}}, 1, {0, 35, 95, 20, 10, 0, 20, 85}},
        {"BANJO-ISH", 2, {{P_DLY, 16}, {X, 0}}, 1, {0, 90, 20, 30, 15, 8, 90, 10}},
        {"NYLON", 2, NOV},
        {"HARP", 8, NOV},
        {"KALIMBA", 4, NOV},
        {"MARIMBA", 1, NOV},
        {"BELL TREE", 0, NOV},
        {"BOWED METAL", 3, NOV},
        {"DRONE STRING", 7, NOV},
        {"KOTO", 2, {{P_E2, 110}, {P_E4, 60}, {X, 0}}},
        {"MUTED", 2, {{P_E3, 30}, {P_E2, 50}, {X, 0}}},
    }},
    /* VOX */
    {"VOX", "talking synth \xB7 the mouth opens with the sound", IG_HARMONY, E_FORMANT, 16, 1, 11, 48, {RGB(20, 10, 14), RGB(40, 20, 28), RGB(255, 111, 111), RGB(184, 144, 152), RGB(255, 238, 240)}, {{P_LRATE, 15}, {P_CHOR, 10}, {P_ITYPE, 7}, {P_IMIX, 0}, {X, 0}}, {
        {"MORPH", TFM_MSW, {0}, 0, {NT, NT, NT, NT}},   /* (round 8: TFM_MORPH below) */
        {"VOWEL", 0, {0}, 0, {{P_E0, 0, 127, 0}, NT, NT, NT}},
        {"TALK", 0, {0}, 0, {{P_E2, 0, 110, 0}, NT, NT, NT}},
        {"BUZZ", 0, {0}, 0, {{P_E4, 10, 127, 0}, {P_LEVEL, 108, 100, 0}, NT, NT}},
        {"BREATH", 0, {0}, 0, {{P_E5, 0, 110, 0}, NT, NT, NT}},
        {"Q", 0, {0}, 0, {{P_E6, 10, 127, 0}, NT, NT, NT}},
        {"CHOIR", 0, {0}, 0, {{P_CHOR, 0, 90, 0}, {P_SPRD, 0, 120, 0}, {P_IMIX, 0, 40, 0}, NT}},
        {"SPACE", 0, {0}, 0, {{P_REV, 0, 100, 0}, {P_LEVEL, 106, 100, 0}, NT, NT}},
    }, {
        {"ROBOT", 0, {{P_E3, 0}, {X, 0}}, 1, {0, 20, 60, 90, 10, 40, 10, 20}},
        {"CHOIR BOY", 0, {{P_E3, 0}, {X, 0}}, 1, {0, 40, 10, 50, 20, 50, 85, 60}},
        {"WHISPER", 0, {{P_E3, 0}, {X, 0}}, 1, {0, 45, 20, 0, 95, 31, 62, 18}},
        {"YAMMER", 0, {{P_E3, 5}, {X, 0}}, 1, {0, 50, 90, 60, 30, 60, 20, 30}},
        {"MONK", 0, {{P_E3, -7}, {X, 0}}, 1, {0, 30, 5, 55, 10, 70, 70, 80}},
        {"CHOIR AAH", 0, NOV},
        {"VOX LEAD", 1, NOV},
        {"WHISPER", 3, NOV},
        {"WOW BASS", 2, NOV},
        {"TALKBOX", 1, {{P_E2, 60}, {P_E6, 100}, {X, 0}}},
        {"ROBOT", 1, {{P_E4, 120}, {P_E5, 0}, {X, 0}}},
    }},
    /* WHEELHOUSE */
    {"WHEELHOUSE", "tonewheel organ \xB7 the disc spins with the rotor", IG_HARMONY, E_WHEEL, 13, 1, 11, 20, {RGB(18, 12, 6), RGB(36, 24, 12), RGB(255, 170, 60), RGB(176, 156, 128), RGB(255, 242, 224)}, {{P_E7, 1}, {P_ITYPE, 1}, {P_IMIX, 0}, {X, 0}}, {
        {"MORPH", TFM_MSW, {0}, 0, {NT, NT, NT, NT}},   /* (round 8: TFM_MORPH below) */
        {"STOPS", 10, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9}, 0, {{P_E0, 0, 0, 0}, NT, NT, NT}},
        {"DRIVE", 0, {0}, 0, {{P_E6, 0, 120, 0}, {P_LEVEL, 104, 100, 0}, NT, NT}},
        {"ROTOR", 3, {0, 1, 2}, SWN_11_2, {{P_E7, 0, 0, 0}, NT, NT, NT}},
        {"SUB", 0, {0}, 0, {{P_E1, -8, 8, 0}, NT, NT, NT}},
        {"BODY", 0, {0}, 0, {{P_E2, -8, 8, 0}, {P_LEVEL, 120, 102, 0}, NT, NT}},
        {"TOP", 0, {0}, 0, {{P_E3, -8, 8, 0}, NT, NT, NT}},
        {"SPACE", 0, {0}, 0, {{P_REV, 0, 90, 0}, NT, NT, NT}},
    }, {
        {"SMOKY", 0, {{P_E5, 25}, {X, 0}}, 1, {0, 20, 25, 50, 70, 60, 30, 40}},
        {"SUNDAY SERVICE", 0, {{P_E5, 50}, {X, 0}}, 1, {0, 60, 0, 50, 55, 55, 82, 91}},
        {"ROCK OUT", 0, {{P_E5, 76}, {X, 0}}, 1, {0, 70, 85, 100, 60, 70, 80, 20}},
        {"BALLPARK", 0, {{P_E5, 101}, {X, 0}}, 1, {0, 90, 40, 100, 50, 30, 95, 30}},
        {"FLUTES", 0, {{P_E5, 38}, {X, 0}}, 1, {0, 0, 0, 50, 23, 19, 61, 71}},
        {"FULL ORGAN", 0, NOV},
        {"JAZZ PERC", 1, NOV},
        {"GOSPEL", 2, NOV},
        {"SOFT FLUTE", 3, NOV},
        {"ROCK DRIVE", 4, NOV},
        {"CHAPEL", 2, {{P_E0, 12}, {P_REV, 80}, {X, 0}}},
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

/* round 6: LEVELS. Each instrument's loudness matched (tests/ui_test_r6.h: BS.1770 K-weighted, a part in its role, the
 * default LEVEL): TFM_TRIM in half dB, [0] the instrument (every sound), [1..5] its five named presets' own on top; the
 * mix takes it with LEVEL (fx.c trk_lvl). TFM_KT: the key tracking of level, [0] the top of the home range (a note),
 * [1] half dB less per octave above it (notes there no louder: bright FM, pulse, formant sounds). The tables and the
 * measurements: docs/TFM-INSTRUMENTS.md */
static const int8_t TFM_TRIM[TI_N][6] = {
    {1, 0, -3, 0, 10, 1},                /* THUMP LAB */
    {4, 5, -2, -3, 6, 0},                /* SQUELCH */
    {8, 0, 0, 5, -2, -3},                /* WOBBLER */
    {-14, 5, 0, -4, 9, -2},              /* CHORDETTE */
    {-30, -2, 0, 1, 16, -6},             /* RAZOR */
    {-33, -3, 0, 2, 4, -5},              /* HAZE */
    {-14, -5, 1, 9, -3, 0},              /* PIXEL */
    {-20, 0, 3, 0, -3, 2},               /* TINE */
    {-2, 1, -2, 2, 0, -1},               /* GLINT */
    {23, -6, 0, 2, -33, 3},              /* PLUCK */
    {-28, -5, 0, 9, 2, -2},              /* VOX */
    {-28, -1, 0, 2, -7, 6},              /* WHEELHOUSE */
};
static const uint8_t TFM_KT[TI_N][2] = {
    {0, 0}, {48, 5}, {48, 4}, {60, 3}, {72, 1}, {60, 2}, {72, 3}, {60, 0}, {72, 1}, {60, 4}, {60, 6}, {60, 3},
};
static int32_t tfm_hdb_q12(int32_t x)                   /* x half dB -> Q12 (+19.5 dB at most) */
{
    x = clamp(x, -112, 39);
    return x > 15 ? (int32_t)LEVEL_Q12[88 + x] << 2 : LEVEL_Q12[112 + x];   /* (LEVEL_Q12: 112 is 0 dB, half dB a step; x 4: +12 dB) */
}
static int32_t tfm_lvl_trim(const track_t *t)
{
    uint32_t i = tfm_ins_of(t), s;
    if (i == TI_NONE)
        return 4096;
    s = tfm_snd_of(t);
    return tfm_hdb_q12(TFM_TRIM[i][0] + (s < 5u ? TFM_TRIM[i][1u + s] : 0));
}
static int32_t tfm_key_gain(const track_t *t, uint32_t note)
{
    uint32_t i = tfm_ins_of(t);
    int32_t st;
    if (i == TI_NONE || !TFM_KT[i][1] || note <= TFM_KT[i][0])
        return 32767;
    st = (int32_t)((note - TFM_KT[i][0]) * TFM_KT[i][1] / 12u);
    return st ? (int32_t)LEVEL_Q12[112 - clamp(st, 0, 100)] << 3 : 32767;
}
static int32_t (*lvl_trim_fn)(const track_t *t) = tfm_lvl_trim;
static int32_t (*key_gain_fn)(const track_t *t, uint32_t note) = tfm_key_gain;

/* track t plays instrument i's sound s: its engine (as EDIT's engine keys load it), the factory preset (as PRESETS
 * does: one undo, the steps kept), the changes, the instrument's mark */
static void tfm_macro_put(track_t *t, uint32_t m, int32_t pos, int rec);
static uint8_t tfm_mok[NTRK];                           /* bit m: macro m's place is stored (the places: below) */
static uint8_t tfm_mb[NTRK];                            /* (round 8: MORPH's base is taken: tfm_morph_put) */
static int8_t tfm_swhalf[NTRK][8];                      /* a switch steps once per two detents: the detents so far */
static int tfm_mslot(const track_t *t) { return t >= trk && t < trk + NTRK ? (int)(t - trk) : -1; }
static void tfm_load(track_t *t, uint32_t i, uint32_t s)
{
    const tfm_instr_t *in = &TFM_INS[i % TI_N];
    const tfm_snd_t *sn = &in->snd[s % in->nsnd];
    uint32_t k;
    if (t->eng_req != in->eng)
        set_engine_of(t, in->eng);
    apply_preset_to(t, sn->pre);
    for (k = 0; k < 16u && in->vo[k].id != 0xFFu; k++) {   /* the VOICING: what makes it this instrument */
        const param_desc_t *d = param_desc_of(in->eng, in->vo[k].id);
        t->p[in->vo[k].id] = (int16_t)clamp(in->vo[k].v, d->min, d->max);
    }
    for (k = 0; k < 6u && sn->ov[k].id != 0xFFu; k++) {    /* the sound's changes */
        const param_desc_t *d = param_desc_of(in->eng, sn->ov[k].id);
        t->p[sn->ov[k].id] = (int16_t)clamp(sn->ov[k].v, d->min, d->max);
    }
    t->preset = (uint8_t)TFM_PRE(i % TI_N, s % in->nsnd);
    if (tfm_mslot(t) >= 0) {                            /* (the places are the preset's: set below, or derived once from it) */
        tfm_mok[tfm_mslot(t)] = 0;
        tfm_mb[tfm_mslot(t)] = 0;                       /* (round 8: MORPH starts from this sound) */
        memset(tfm_swhalf[tfm_mslot(t)], 0, 8);
    }
    if (sn->hasmp) {                                    /* a named preset: its eight macros' places (not recorded) */
        for (k = 0; k < 8u; k++)
            tfm_macro_put(t, k, (int32_t)sn->mp[k] * 127 / 100, 0);
        for (k = 0; k < 6u && sn->ov[k].id != 0xFFu; k++) {   /* (round 8: then its own changes again: what the macro that */
            const param_desc_t *d = param_desc_of(in->eng, sn->ov[k].id);   /* MORPH replaced set, kept as it was) */
            t->p[sn->ov[k].id] = (int16_t)clamp(sn->ov[k].v, d->min, d->max);
        }
    }
}
/* a sound's name (the PRESETS list, NEW SONG, the bottom line): "SQUELCH RUBBER" */
static void tfm_snd_name(uint32_t i, uint32_t s, char *b, uint32_t n)
{
    str_cpy(b, TFM_INS[i % TI_N].name, n);
    str_cpy(b + str_len(b), " ", n - str_len(b));
    str_cpy(b + str_len(b), TFM_INS[i % TI_N].snd[s % TFM_INS[i % TI_N].nsnd].name, n - str_len(b));
}

/* ------------------------------------------------------------ the macros --- */
/* a macro's place is 0..127: each target is a line from its lo to its hi (EXP: the square of the place, GATE: moving
 * only over 70 %); a SWITCH has nsw values, the place picks one. A macro writes its targets' absolute values: a value
 * changed in DEEP is overwritten by the macro's next move. */
static uint32_t tfm_isqrt(uint32_t n)
{
    uint32_t r = 0;
    while ((r + 1u) * (r + 1u) <= n)
        r++;
    return r;
}
/* round 8: MORPH (macro 1 of every instrument, JAM's knob): one turn from the sound as it is (0) to a contrasting voicing
 * (127), many values together. It bends from where the sound is: the values it moves are taken when it leaves 0 (the base)
 * and put back exactly at 0; ABS goes from the base to v, REL adds v to the base (the level), SW is v from the middle up.
 * Another macro moving one of them while MORPH is out moves the base (MORPH bends from there). RAM only: after a load
 * the sound as it was saved is the base (MORPH reads 0) */
enum { TMM_ABS, TMM_REL, TMM_SW };
typedef struct { uint8_t id; int8_t v; uint8_t md; } tfm_mt_t;
#define MT_END {0xFFu, 0, 0}
static const tfm_mt_t TFM_MORPH[TI_N][8] = {
    /* THUMP LAB: tight and dry -> big, crushed, a room */
    {{P_E3, 110, TMM_ABS}, {P_REV, 90, TMM_ABS}, {P_IA, 110, TMM_ABS}, {P_IMIX, 100, TMM_ABS}, {P_E7, 90, TMM_ABS},
     {P_DLY, 25, TMM_ABS}, {P_LN3, 127, TMM_ABS}, {P_LEVEL, -9, TMM_REL}},
    /* SQUELCH: round and mellow -> screaming acid */
    {{P_E4, 100, TMM_ABS}, {P_E5, 118, TMM_ABS}, {P_ED_FLT, 60, TMM_ABS}, {P_E6, 100, TMM_ABS}, {P_DEC, 55, TMM_ABS},
     {P_IMIX, 60, TMM_ABS}, {P_LEVEL, -3, TMM_REL}, MT_END},
    /* WOBBLER: a soft sway -> a heavy, growling wobble */
    {{P_LD_FLT, 63, TMM_ABS}, {P_E6, 60, TMM_ABS}, {P_E5, 100, TMM_ABS}, {P_IMIX, 90, TMM_ABS}, {P_IA, 80, TMM_ABS},
     {P_DIST, 60, TMM_ABS}, {P_LEVEL, 0, TMM_REL}, MT_END},
    /* CHORDETTE: warm and still -> bright, pumping, wide */
    {{P_E4, 110, TMM_ABS}, {P_E5, 10, TMM_ABS}, {P_CHOR, 60, TMM_ABS}, {P_LD_AMP, 70, TMM_ABS}, {P_REV, 70, TMM_ABS},
     {P_DLY, 30, TMM_ABS}, {P_LEVEL, 0, TMM_REL}, MT_END},
    /* RAZOR: thin and dark -> screaming, wide, echoing */
    {{P_E5, 110, TMM_ABS}, {P_E6, 70, TMM_ABS}, {P_E4, 120, TMM_ABS}, {P_ED_FLT, 60, TMM_ABS}, {P_DETUNE, 80, TMM_ABS},
     {P_SPRD, 127, TMM_ABS}, {P_DLY, 60, TMM_ABS}, {P_LEVEL, 0, TMM_REL}},
    /* HAZE: dark and still -> bright and moving */
    {{P_E5, 22, TMM_ABS}, {P_LD_FLT, 60, TMM_ABS}, {P_CHOR, 110, TMM_ABS}, {P_REV, 127, TMM_ABS}, {P_IMIX, 60, TMM_ABS},
     {P_LEVEL, 12, TMM_REL}, MT_END, MT_END},
    /* PIXEL: a clean chip -> crushed, bright, ringing (no pitch sweep: MORPH stays in key) */
    {{P_E3, 70, TMM_ABS}, {P_E2, 120, TMM_ABS}, {P_DEC, 80, TMM_ABS}, {P_E7, 127, TMM_ABS}, {P_REV, 40, TMM_ABS},
     {P_LEVEL, -2, TMM_REL}, MT_END, MT_END},
    /* TINE: soft and dry -> barking, trembling, wet */
    {{P_E2, 40, TMM_ABS}, {P_E4, 40, TMM_ABS}, {P_LD_AMP, 90, TMM_ABS}, {P_CHOR, 110, TMM_ABS}, {P_IMIX, 80, TMM_ABS},
     {P_REV, 60, TMM_ABS}, {P_LEVEL, 4, TMM_REL}, MT_END},
    /* GLINT: pure -> metallic, blooming, shimmering */
    {{P_E2, 50, TMM_ABS}, {P_E4, 50, TMM_ABS}, {P_DEC, 120, TMM_ABS}, {P_REV, 110, TMM_ABS}, {P_IMIX, 90, TMM_ABS},
     {P_DLY, 40, TMM_ABS}, {P_LEVEL, 0, TMM_REL}, MT_END},
    /* PLUCK: muted and short -> bright, ringing, wide (no bow: it pulls the pitch) */
    {{P_E2, 80, TMM_ABS}, {P_E3, 50, TMM_ABS}, {P_E5, 127, TMM_ABS}, {P_REV, 80, TMM_ABS}, {P_DLY, 30, TMM_ABS},
     {P_LEVEL, -6, TMM_REL}, MT_END, MT_END},
    /* VOX: a closed mumble -> a buzzing choir */
    {{P_E4, 127, TMM_ABS}, {P_E6, 127, TMM_ABS}, {P_E2, 110, TMM_ABS}, {P_CHOR, 90, TMM_ABS}, {P_SPRD, 120, TMM_ABS},
     {P_REV, 80, TMM_ABS}, {P_LEVEL, -7, TMM_REL}, MT_END},
    /* WHEELHOUSE: clean -> driven, the rotor fast, clicking */
    {{P_E6, 120, TMM_ABS}, {P_E7, 2, TMM_SW}, {P_E5, 127, TMM_ABS}, {P_E3, 8, TMM_ABS}, {P_REV, 60, TMM_ABS},
     {P_LEVEL, 0, TMM_REL}, MT_END, MT_END},
};
static int16_t tfm_mbase[NTRK][8];                      /* MORPH's base, per track (tfm_mb: taken) */
static int32_t tfm_morph_val(const track_t *t, uint32_t ins, uint32_t k, int32_t base, int32_t pos)
{
    const tfm_mt_t *g = &TFM_MORPH[ins % TI_N][k];
    const param_desc_t *d = param_desc_of(TFM_INS[ins % TI_N].eng, g->id);
    int32_t v = g->md == TMM_SW ? (pos >= 64 ? g->v : base) :
                g->md == TMM_REL ? base + g->v * pos / 127 : base + (g->v - base) * pos / 127;
    (void)t;
    return clamp(v, d->min, d->max);
}
static int tfm_mslot(const track_t *t);
static void tfm_mstore(const track_t *t, uint32_t m, int32_t pos);
static void tfm_morph_put(track_t *t, int32_t pos, int rec)
{
    uint32_t ins = tfm_ins_of(t), k;
    int s = tfm_mslot(t);
    if (s < 0 || ins == TI_NONE)
        return;
    if (pos && !tfm_mb[s]) {                            /* leaving 0: the sound as it is, the base */
        for (k = 0; k < 8u && TFM_MORPH[ins][k].id != 0xFFu; k++)
            tfm_mbase[s][k] = t->p[TFM_MORPH[ins][k].id];
        tfm_mb[s] = 1;
    }
    if (tfm_mb[s])
        for (k = 0; k < 8u && TFM_MORPH[ins][k].id != 0xFFu; k++) {
            uint32_t id = TFM_MORPH[ins][k].id;
            t->p[id] = (int16_t)tfm_morph_val(t, ins, k, tfm_mbase[s][k], pos);
            if (rec)
                motion_capture(t, id, t->p[id]);
        }
    if (!pos)
        tfm_mb[s] = 0;                                  /* (at 0: the base put back exactly; the next move takes it again) */
    tfm_mstore(t, 0, pos);
}
/* another macro wrote value id: while MORPH is out that is its new base, MORPH bends from there */
static uint8_t tfm_mpos[NTRK][8];
static void tfm_morph_follow(track_t *t, uint32_t id)
{
    uint32_t ins = tfm_ins_of(t), k;
    int s = tfm_mslot(t);
    if (s < 0 || ins == TI_NONE || !tfm_mb[s])
        return;
    for (k = 0; k < 8u && TFM_MORPH[ins][k].id != 0xFFu; k++)
        if (TFM_MORPH[ins][k].id == id) {
            tfm_mbase[s][k] = t->p[id];
            t->p[id] = (int16_t)tfm_morph_val(t, ins, k, tfm_mbase[s][k], tfm_mpos[s][0]);
        }
}
static int32_t tfm_tgt_val(const tfm_tgt_t *g, int32_t pos)
{
    int32_t f = pos, span = g->hi - g->lo;
    if (g->cv == TCV_EXP)
        f = pos * pos / 127;
    else if (g->cv == TCV_GATE)
        f = pos <= 89 ? 0 : (pos - 89) * 127 / 38;
    return g->lo + (span * f + (span >= 0 ? 63 : -63)) / 127;
}
static int32_t tfm_tgt_pos(const tfm_tgt_t *g, int32_t v)
{
    int32_t span = g->hi - g->lo, f;
    if (!span)
        return 0;
    f = clamp((v - g->lo) * 127 / span, 0, 127);
    if (g->cv == TCV_EXP)
        return (int32_t)tfm_isqrt((uint32_t)f * 127u);
    if (g->cv == TCV_GATE)
        return f ? 89 + f * 38 / 127 : 0;
    return f;
}
static int32_t tfm_sw_pos(const tfm_macro_t *mc, uint32_t idx)   /* the place of a switch's value idx */
{
    return mc->nsw > 1u ? (int32_t)((idx * 127u + (mc->nsw - 1u) / 2u) / (mc->nsw - 1u)) : 0;
}
static uint32_t tfm_sw_idx(const tfm_macro_t *mc, int32_t v)     /* the value nearest v */
{
    uint32_t k, best = 0;
    int32_t bd = 1000;
    for (k = 0; k < mc->nsw; k++) {
        int32_t d = v - mc->sw[k];
        d = d < 0 ? -d : d;
        if (d < bd) {
            bd = d;
            best = k;
        }
    }
    return best;
}
/* each track's eight macro places (0..127), kept: a turn moves the stored place, never one read back from a target (that
 * rounds, and a knob turn stalled). tfm_mval is the first target's value the place wrote: while the parameter still
 * has it the place is good; anything else that moved it (DEEP, a load, undo, a MIDI CC) makes the place derived once
 * from the parameter and kept again. RAM only: no format change. */
static int16_t tfm_mval[NTRK][8];
static void tfm_mstore(const track_t *t, uint32_t m, int32_t pos)
{
    int s = tfm_mslot(t);
    uint32_t i = tfm_ins_of(t);
    if (s < 0 || i == TI_NONE)
        return;
    tfm_mpos[s][m & 7u] = (uint8_t)clamp(pos, 0, 127);
    tfm_mval[s][m & 7u] = TFM_INS[i].mac[m & 7u].nsw == TFM_MSW ? 0 : (int16_t)motion_base_value(t, TFM_INS[i].mac[m & 7u].t[0].id);
    tfm_mok[s] |= (uint8_t)(1u << (m & 7u));
}
/* macro m of the track's instrument: its place 0..127 (stored; else read back from its first target, once) */
static int32_t tfm_macro_get(const track_t *t, uint32_t m)
{
    uint32_t i = tfm_ins_of(t);
    const tfm_macro_t *mc;
    int s = tfm_mslot(t);
    int32_t pos;
    if (i == TI_NONE)
        return 0;
    mc = &TFM_INS[i].mac[m & 7u];
    if (mc->nsw == TFM_MSW)                             /* (MORPH: its place while its base is held, else 0) */
        return s >= 0 && tfm_mb[s] && ((tfm_mok[s] >> (m & 7u)) & 1u) ? tfm_mpos[s][m & 7u] : 0;
    if (s >= 0 && ((tfm_mok[s] >> (m & 7u)) & 1u) && tfm_mval[s][m & 7u] == (int16_t)motion_base_value(t, mc->t[0].id))
        return tfm_mpos[s][m & 7u];
    if (mc->nsw)                                        /* (MORPH returned above) */
        pos = tfm_sw_pos(mc, tfm_sw_idx(mc, motion_base_value(t, mc->t[0].id)));
    else
        pos = tfm_tgt_pos(&mc->t[0], motion_base_value(t, mc->t[0].id));
    tfm_mstore(t, m, pos);
    return pos;
}
static void tfm_macro_put(track_t *t, uint32_t m, int32_t pos, int rec)
{
    uint32_t i = tfm_ins_of(t), k;
    const tfm_macro_t *mc;
    if (i == TI_NONE)
        return;
    mc = &TFM_INS[i].mac[m & 7u];
    pos = clamp(pos, 0, 127);
    if (mc->nsw == TFM_MSW) {
        tfm_morph_put(t, pos, rec);
        return;
    }
    for (k = 0; k < 4u; k++) {
        const tfm_tgt_t *g = &mc->t[k];
        const param_desc_t *d;
        int32_t v;
        if (g->id == 0xFFu)
            break;
        d = param_desc_of(TFM_INS[i].eng, g->id);
        if (mc->nsw) {
            if (k)
                break;
            v = mc->sw[(uint32_t)pos * mc->nsw / 128u];
        } else {
            v = tfm_tgt_val(g, pos);
        }
        t->p[g->id] = (int16_t)clamp(v, d->min, d->max);
        tfm_morph_follow(t, g->id);                     /* (round 8: MORPH out: its base moves, it bends from there) */
        if (rec)
            motion_capture(t, g->id, t->p[g->id]);      /* (REC: recorded as automation, as a knob turn is) */
    }
    tfm_mstore(t, m, pos);
}
static void tfm_macro_set(track_t *t, uint32_t m, int32_t pos) { tfm_macro_put(t, m, pos, 1); }
/* a macro's value as the card shows it: a switch its name (or the parameter's), else 0..100 */
static void tfm_macro_text(const track_t *t, uint32_t m, char *val, const char **unit)
{
    uint32_t i = tfm_ins_of(t);
    const tfm_macro_t *mc;
    *unit = "";
    val[0] = 0;
    if (i == TI_NONE)
        return;
    mc = &TFM_INS[i].mac[m & 7u];
    if (mc->nsw && mc->nsw != TFM_MSW) {
        uint32_t k = tfm_sw_idx(mc, motion_base_value(t, mc->t[0].id));
        if (mc->swn)
            str_cpy(val, mc->swn[k], 8);
        else
            param_format(param_desc_of(TFM_INS[i].eng, mc->t[0].id), motion_base_value(t, mc->t[0].id), val, unit);
    } else {
        fmt_int(val, tfm_macro_get(t, m) * 100 / 127);
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

/* ------------------------------------------------------------ the UI --- */
/* PRESETS (ui.c preset_all_*): every instrument's sounds in order, as (engine, preset byte) */
static uint32_t tfm_list_n(void)
{
    uint32_t i, n = 0;
    for (i = 0; i < TI_N; i++)
        n += TFM_INS[i].nsnd;
    return n;
}
static uint32_t tfm_list_at(uint32_t n, uint32_t *k)
{
    uint32_t i;
    for (i = 0; i < TI_N && n >= TFM_INS[i].nsnd; i++)
        n -= TFM_INS[i].nsnd;
    i %= TI_N;
    *k = TFM_PRE(i, n);
    return TFM_INS[i].eng;
}
static uint32_t tfm_list_pos(const track_t *t)
{
    uint32_t i, n = 0, ins = tfm_ins_of(t);
    if (ins == TI_NONE)
        return 0;
    for (i = 0; i < ins; i++)
        n += TFM_INS[i].nsnd;
    return n + tfm_snd_of(t);
}
static void tfm_load_byte(track_t *t, uint32_t k) { tfm_load(t, (k - TFM_PRE0) / 16u, (k - TFM_PRE0) % 16u); }
static uint32_t tfm_base_pre(const track_t *t)
{
    uint32_t i = tfm_ins_of(t);
    return i == TI_NONE ? t->preset : TFM_INS[i].snd[tfm_snd_of(t)].pre;
}
/* the list's row: a four-letter tag for the instrument, the sound's name */
static const char *const TFM_TAG[TI_N] = {"THMP", "SQLC", "WOBL", "CHRD", "RAZR", "HAZE", "PIXL", "TINE",
                                         "GLNT", "PLCK", "VOX", "WHEL"};   /* (4: the list's tag column) */
static const char *tfm_ins_tag(const track_t *t) { return TFM_TAG[tfm_ins_of(t) % TI_N]; }
static void tfm_tag_name(uint32_t k, char *tag, char *nm)
{
    uint32_t i = (k - TFM_PRE0) / 16u % TI_N, s = (k - TFM_PRE0) % 16u;
    str_cpy(tag, TFM_TAG[i], 6);
    str_cpy(nm, TFM_INS[i].snd[s % TFM_INS[i].nsnd].name, 13);
}
static const char *tfm_ins_name(const track_t *t) { return TFM_INS[tfm_ins_of(t) % TI_N].name; }
/* JAM's knob: MORPH, macro 1 of every instrument (round 8) */
static uint32_t tfm_jam_m(const track_t *t) { (void)t; return 0u; }
static void tfm_snd_label(const track_t *t, char *b)
{
    const tfm_instr_t *in = &TFM_INS[tfm_ins_of(t) % TI_N];
    str_cpy(b, in->snd[tfm_snd_of(t) % in->nsnd].name, 16);
}
/* KNOB m: a turn moves its place; a switch one value a detent (never stuck between two values); _k: turned on KNOB k
 * (its acceleration's) */
static void tfm_macro_turn_k(track_t *t, uint32_t m, uint32_t k, int32_t steps)
{
    uint32_t i = tfm_ins_of(t);
    const tfm_macro_t *mc;
    int32_t pos;
    if (i == TI_NONE)
        return;
    mc = &TFM_INS[i].mac[m & 7u];
    pos = tfm_macro_get(t, m);
    if (mc->nsw && mc->nsw != TFM_MSW) {                /* a switch: one value per two detents (a detent alone only counts) */
        int s = tfm_mslot(t);
        int32_t d = steps > 0 ? 1 : -1, idx, h;
        h = s >= 0 ? tfm_swhalf[s][m & 7u] : 0;
        h = (h && (h > 0) != (d > 0)) ? d : h + d;      /* (turned back: counting again from this detent) */
        if (h > -2 && h < 2) {
            if (s >= 0)
                tfm_swhalf[s][m & 7u] = (int8_t)h;
            return;
        }
        if (s >= 0)
            tfm_swhalf[s][m & 7u] = 0;
        idx = clamp((int32_t)tfm_sw_idx(mc, motion_base_value(t, mc->t[0].id)) + d, 0, (int32_t)mc->nsw - 1);
        pos = tfm_sw_pos(mc, (uint32_t)idx);
    } else {
        pos += accel(EN_K1 + (k & 3u), steps, 127);
    }
    tfm_macro_set(t, m, pos);
}
static void tfm_macro_turn(track_t *t, uint32_t m, int32_t steps) { tfm_macro_turn_k(t, m, m, steps); }
/* MACROS / MACROS 2 (Felucca's page, DEEP and CLASSIC): the four cards, each its label and its place */
static void tfm_macro_cards(uint32_t page)
{
    uint32_t c, i = tfm_ins_of(TSEL);
    for (c = 0; c < 4u; c++) {
        const tfm_macro_t *mc;
        char val[12], lb[6];
        const char *unit = "";
        int32_t pos;
        if (i == TI_NONE) {
            draw_column(c, "", "", "", T_THEME, -1, ICON_NONE);
            continue;
        }
        mc = &TFM_INS[i].mac[page * 4u + c];
        pos = tfm_macro_get(TSEL, page * 4u + c);
        tfm_macro_text(TSEL, page * 4u + c, val, &unit);
        str_cpy(lb, mc->name, 6);                       /* (Felucca's card: 5 letters; the face shows the whole name) */
        draw_column(c, lb, val, unit, VAL(c), pos * 1000 / 127, ICON_AUTO);
    }
}
/* the page: the instrument and its sound, big, and which half of the macros this is (round 4: the pinball toys are gone) */
static void graph_macro(void)
{
    uint32_t i = tfm_ins_of(TSEL);
    int toy_on = 0;
    char b[24];
    if (i == TI_NONE)
        return;
    GFX_HOOK_ALIGN(0, 0, 240, 0, AL_H, "macro page instrument centred");
    cv_text_in(0, toy_on ? 2 : 30, 240, &AF_M, TFM_INS[i].name, T_THEME, T_BG);
    tfm_snd_label(TSEL, b);
    GFX_HOOK_ALIGN(0, 0, 240, 0, AL_H, "macro page sound centred");
    cv_text_in(0, toy_on ? 24 : 54, 240, &AF_S, b, T_TEXT, T_BG);
    GFX_HOOK_ALIGN(0, 0, 240, 0, AL_H, "macro page hint centred");
    cv_text_in(0, toy_on ? 90 : 84, 240, &AF_S, str_eq(cur_page()->title, "MACROS 2") ? "MACROS 5-8  [EDIT] ENGINE" : "MACROS 1-4  [EDIT] 5-8",
               T_DIM, T_BG);
}
