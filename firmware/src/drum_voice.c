/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* Drum voices: lean fixed-point drums for a kit of 8 lanes. Felucca's own design, voiced by ear.
 *
 * Every tuned part is a damped sine (a phase accumulator, the SINE table, a decaying envelope): exact
 * pitch, always stable, no high-Q resonator. Noise is xorshift through dsp.c's trapezoidal SVF or a
 * one-pole. Only 32-bit products in the sample loops. Envelopes:
 *   fast (tau under ~40 ms)  Q16 per sample, e = (e * k) >> 15 with k Q15 (the floor form: always reaches 0)
 *   slow (tau above)         Q30 per CTL block times a Q16 factor, ramped linearly inside the block
 *
 * Lanes and their variants (dv_type: what a lane plays; ROUND: KICK's other kick; CONGA CLAVE CYM: the GM notes
 * of congas, claves and cymbals on KIT STD, eng_drum.c DRUM_GM):
 *   KICK    PUNCH: a sine swept in two stages (an attack of +2 octaves, then TONE's 0..+3 octaves after a
 *           hold), held flat for 12 ms, phase-locked 2nd and 3rd harmonics, a DC-free click (two cycles of
 *           a high sine under a raised-cosine window) and a soft-clip DRIVE; ROUND: no attack stage and
 *           no hold, a shallow slow sweep (up to +1 octave), strong 2nd and 3rd harmonics, more drive, a
 *           softer, lower click, a longer body.
 *           The sweep through 100..400 Hz, the harmonics and the drive keep the kick audible on a small
 *           speaker (energy above 150 Hz >= -12 dB at the defaults) without touching the fundamental
 *   SNARE   two sines (f, 1.6 f) with a small downward glide under a short shell envelope, plus noise
 *           through a band-pass (TONE: its centre) under a spike and a body envelope and a little
 *           high-passed noise on the spike; soft-clipped. Extra: SNAPPY (the noise level)
 *   CLAP    noise through a band-pass (TUNE: its centre), re-struck 3 times (4 when SPREAD is high) at
 *           SPREAD's interval, then a long low tail; TONE adds high-passed noise. Extra: SPREAD
 *   HAT CL, HAT OP   classic analog hats: the metal source (six square waves at 205, 304, 370, 523, 540 and
 *           800 Hz: a dense, clangy cluster) through a band-pass at 7.1 kHz (Q 0.8), then a bilinear one-pole
 *           high-pass per hat (CL 12 kHz, OP 8.8 kHz), plus a sizzle: the raw cluster high-passed at 13 kHz
 *           (strong on the closed hat, a trace on the open one). TONE moves both high-passes +-12 semitones,
 *           TUNE the cluster and every filter. Envelope: a 0.3 ms rise, then CL tau 6..42 ms (16 at the design),
 *           OP 25..380 ms (98). Accent up to +6 dB. Extra: SIZZLE (0: none, 64: as designed, 127: double).
 *           A closed hat chokes the open one (dv_choke: a 1.5 ms release)
 *   TOM     a sine whose pitch drops as it rings down (it follows the square of the amplitude), a little
 *           low-passed noise for the stick (TONE). Extra: BEND. CONGA: higher, shorter, a slap of high noise
 *           instead of the stick (TONE), a little soft clip
 *   RIM     two short sines (f, 3.25 f) into a soft clip (extra: DRIVE, the saturation; TONE: the crack's
 *           share). CLAVE: one sine, higher (TONE: a little of the upper sine)
 *   BELL    two square waves (f, 1.48 f) through a band-pass under a short strike and a tail envelope
 *           (extra: STRIKE; TONE: the squares themselves mixed in). CYM: the metal source through two
 *           band-passes (TONE: high / low balance; TUNE moves both) under a strike (STRIKE) and a ring
 * The model kits (KIT 80 10 66 55 77): each lane a kit's own voice, played through the runs above with that kit's
 * numbers (DV_KIT: pitches, sweeps, decays, the noise's colour, the metal clusters, the clap's bursts, the accent):
 *   80  a deep sine kick with a small drop and a long decay, a snare of two shells and bright noise, a 1 kHz clap of
 *       three bursts and a tail, the six-square hats, a low-noise tom, a rim's crack, a two-square bell
 *   10  a small kit: a kick swept down from 90 Hz, one shell over white noise, three slow claps and a long tail,
 *       hats and a cymbal (on BELL) of four squares; TOM and RIM stand-ins built from its body and shell
 *   66  an early rhythm box: a kick that swells in, a bright noisy snare with a short shell, hats of noise in a
 *       narrow band near 8 kHz, a low conga on TOM, a high rim, a bell of two sines; a CLAP stand-in from its
 *       snare's noise
 *   55  a kick dropping from 100 Hz, a 245 Hz snare under band noise, hats of a high cluster and noise, a tom with
 *       its stick, a click rim, a bell of a wide metal cluster; a CLAP stand-in from its snare's noise
 *   77  a long kick that swells in, a 226 Hz snare under wide noise, six close claps and a tail, pulse-cluster
 *       hats, a bent tom, claves on RIM, the cymbal on BELL
 * 1.5.6 (Discussion #216): three more kits put together from the five kits' pieces (the same runs and numbers, data
 * only; nothing new in the sample loops):
 *   DEEP   77's swelling kick tuned down 2 semitones, 80's snare, 77's claps, 80's hats, 77's bent tom, 80's rim,
 *          10's long cymbal on BELL
 *   TIGHT  short and dry: 55's kick, 66's bright snare, 80's clap, 55's hats, 55's tom, 66's high rim, 80's bell
 *   PERC   a percussion kit: 10's kick, 77's snare and claps, 66's hats, 66's conga on TOM, 77's claves on RIM, 66's
 *          two-sine bell
 * Parameters per lane (dv_param_t): TUNE (1/16 semitone from the designed pitch, held in the voice's
 * range), DECAY, TONE, extra, LEVEL, accent, 0..127 each; the designed sound is at DV_DEF's values.
 * Accent adds up to 4 dB, a little more on the transient (click, spike, the claps). At LEVEL 100 the
 * voices peak near -7 dBFS (the snare's, the clap's and the closed hat's spikes a little higher);
 * louder settings go into the output's soft knee.
 *
 * Use: dv_setup when a parameter changes (dv_param_t compared as two words), dv_trigger on a hit,
 * dv_run per block (n <= CTL) into y: the voice at its level, Q15, linear up to -6 dBFS and never at full
 * scale (a soft knee above).
 * Voices whose type uses the metal source take its block (dv_metal_run) in metal. A voice that has rung out
 * (-84 dB) stops computing (live 0) and writes zeros. */

enum { DV_KICK, DV_SNARE, DV_CLAP, DV_HATC, DV_HATO, DV_TOM, DV_RIM, DV_BELL, DV_NLANE };
enum {
    DVT_PUNCH, DVT_ROUND, DVT_SNARE, DVT_CLAP, DVT_HATC, DVT_HATO, DVT_TOM, DVT_CONGA, DVT_RIM, DVT_CLAVE,
    DVT_BELL, DVT_CYM, DVT_COUNT
};
/* a lane of a model kit as a type: the kit (1..DV_NKIT) in the high 4 bits, the lane in the low 3 (Felucca's own
 * types: 0 above). Its run (the DVT_* it plays through) is the kit's (DV_KIT) */
#define DV_NKIT 23                                       /* (1.5.6, #216: DEEP TIGHT PERC after 77; TILT-FM round 13: 15 more,
                                                          * one per drum preset of THUMP LAB, KITBOX and GARAGE) */
/* (round 13: 23 kits do not fit 4 bits: a kit's lanes are 16 + 8 x (kit - 1) + lane, Felucca's own types stay under 16;
 * the type is runtime state, never stored) */
#define DV_KITOF(t) ((uint32_t)(t) >= 16u ? (((uint32_t)(t) - 16u) >> 3) + 1u : 0u)
#define DV_KTYPE(kit, lane) ((uint8_t)(16u + ((uint32_t)(kit) - 1u) * 8u + (uint32_t)(lane)))
_Static_assert(16u + DV_NKIT * 8u <= 256u, "the kits' types in a byte");

typedef struct {
    uint8_t type;                                        /* DVT_*, or a model kit's (DV_KTYPE) */
    uint8_t decay, tone, extra, level, accent;           /* 0..127 */
    int16_t tune;                                        /* 1/16 semitone from the designed pitch */
} dv_param_t;

typedef struct {                                         /* coefficients (dv_setup); meaning per type, see the run */
    uint32_t inc[3];                                     /* phase increments, cycles Q32 */
    uint32_t span[2];                                    /* pitch sweeps (increment above inc[0]) */
    uint32_t k[3];                                       /* fast decays per sample, Q15 */
    uint32_t kb[2];                                      /* slow decays per block, Q16 */
    int32_t g[6];                                        /* gains, Q15 unless noted */
    tsvf_t f[2];                                         /* band-passes */
    int32_t hp;                                          /* one-pole coefficient, Q15 */
    int32_t out;                                         /* output gain: type, LEVEL, accent (Q12) */
    uint16_t hold, click;                                /* kick: hold (blocks), click length (samples); clap:
                                                          * tooth spacing (samples), teeth */
    uint8_t type, mrow;                                  /* mrow: the metal source's ratios (DV_METAL_R) */
    int16_t metal;                                       /* the metal source's pitch (p16) for this lane */
} dv_coef_t;

typedef struct {
    uint8_t type, trig, choke, live;
    uint16_t n, cnt;                                     /* counters: samples, teeth / hold blocks */
    uint32_t ph[3];
    uint32_t e[3];                                       /* fast envelopes, Q16 */
    int32_t q[2];                                        /* slow envelopes, Q30 */
    int32_t s[5];                                        /* filter states */
    int32_t rng;
} dv_voice_t;

typedef struct {                                         /* the metal source: 6 square waves */
    uint32_t ph[6], inc[6];
} dv_metal_t;

/* designed sound per type: pitch (p16 = 16 x MIDI note) and its range, the main decay's tau at DECAY 0
 * (0.1 ms) and its span (octaves x 16 over DECAY 0..127), the designed DECAY TONE extra, the output gain
 * (Q12, evens the voices out by RMS) */
static const struct {
    int16_t p16, lo, hi;
    uint16_t tau0;
    uint8_t oct16, def[3];
    uint16_t gain;
} DV_DEF[DVT_COUNT] = {
    {502, 403, 664, 100, 75, {64, 64, 32}, 3225},        /* PUNCH  50 Hz (35..90), tau 10..250 ms */
    {533, 403, 664, 200, 69, {64, 64, 56}, 3222},        /* ROUND  56 Hz, tau 20..400 ms */
    {856, 744, 958, 300, 37, {64, 64, 80}, 4080},        /* SNARE  180 Hz (120..260), noise tau 30..150 ms */
    {1358, 1233, 1523, 200, 53, {64, 64, 64}, 8000},     /* CLAP   band 1.1 kHz (0.7..2), tail tau 20..200 ms */
    {893, 701, 1085, 60, 45, {64, 64, 64}, 9015},       /* HAT CL metal 205 Hz (103..410), tau 6..42 ms */
    {893, 701, 1085, 250, 63, {64, 64, 64}, 14040},      /* HAT OP tau 25..380 ms */
    {720, 502, 1190, 600, 53, {64, 64, 40}, 2384},       /* TOM    110 Hz (50..400 with the variants' range),
                                                          * tau 60..600 ms */
    {998, 806, 1190, 300, 53, {64, 64, 40}, 2638},       /* CONGA  300 Hz (150..600), tau 30..300 ms */
    {1139, 947, 1331, 20, 32, {64, 64, 64}, 5215},       /* RIM    500 Hz (250..1000), tau 2..8 ms */
    {1585, 1382, 1715, 30, 32, {64, 64, 64}, 5608},      /* CLAVE  2.5 kHz (1.2..4), tau 3..12 ms */
    {1161, 969, 1353, 300, 59, {64, 64, 64}, 2785},      /* BELL   540 Hz (270..1080), tail tau 30..400 ms */
    {1078, 886, 1270, 3000, 53, {64, 64, 64}, 7583},     /* CYM    metal 400 Hz, ring tau 0.3..3 s */
};
#define DV_SEED 0x5EED1234
#define DV_P16(hz_p16) pitch_inc(clamp((hz_p16), 0, 2047))
/* the metal source's ratios (Q12). Hats: the six-square cluster of the classic analog hats (205.3, 304.4,
 * 369.6, 522.7, 540, 800 Hz over 205.3); the cymbal: inharmonic, picked by ear */
static const uint16_t DV_METAL_R[10][6] = {
    {4096, 6073, 7374, 10428, 10774, 15961},
    {4096, 5800, 6560, 7530, 8810, 10650},
    /* the model kits' (a ratio 0: that square stands still, a constant the band-passes take away) */
    {4096, 5989, 10562, 14799, 0, 0},                    /* 10: four squares, 318 465 820 1149 Hz */
    {4096, 4203, 4368, 4836, 6059, 7486},                /* 55 hats: a high cluster, 8.0 .. 14.7 kHz */
    {4096, 12254, 17607, 21692, 25765, 43372},           /* 55 bell: 720 Hz .. 7.6 kHz, wide apart */
    {4096, 4695, 5646, 10855, 13031, 15117},             /* 77: two triads of pulses, 3.7 .. 13.5 kHz */
    {0, 0, 0, 0, 0, 0},                                  /* 66: none (its metal is noise through a narrow band) */
    {4096, 5360, 7300, 8900, 12500, 14100},  /* round 13: TIN ROOF: a tinny, ringing cluster, 5.4 .. 14 kHz over the base */
    {4096, 6144, 0, 0, 0, 0},  /* round 13: ARCADE: two squares a fifth apart (thin clicks) */
    {4096, 6330, 9810, 13200, 0, 0},  /* round 13: TIN SHED: cans and lids, inharmonic */
};

/* The model kits (KIT 80 10 66 55 77 in eng_drum.c): every lane a kit's own voice, built on the runs above
 * with its own numbers. Per lane: the run, the designed pitch (p16; TUNE +-1 octave), the main decay's tau at
 * DECAY 0 (0.1 ms) and its span (octaves x 16 over DECAY 0..127), the output gain (Q12), then the run's own
 * numbers, v[] (p16 pitches and offsets, gains Q15, taus in 0.1 ms unless noted):
 *   KICK   v0 the sweep (p16, x TONE / 64, TONE above 64 adds up to 4 semitones), v1 its tau, v2 an attack
 *          sweep above it (tau 1.5 ms), v3 v4 the 2nd and 3rd harmonics, v5 the click's pitch (0: none), v6 its
 *          level, v7 the drive (Q12, x (SNAP + 64) / 128, at least x1), v8 a rise (tau; 0: none), v9 a hold (blocks)
 *   SNARE  v0 the 2nd shell sine's ratio - 1 (x 256), v1 the glide (p16), v2 its tau, v3 the shell's tau
 *          (x 1/64 of the noise's), v4 the 2nd sine's level, v5 the shell's level, v6 the noise band (p16, TONE
 *          +-8 semitones), v7 its damping (Q12), v8 the spike's tau, v9 the spike, v10 the noise (x SNAP / 64),
 *          v11 high-passed noise on the spike, v12 its corner (p16)
 *   CLAP   v1 the band's damping (its centre: the pitch, TONE +-8 semitones), v2 the bright noise's corner, v3 its
 *          level (x TONE / 64), v4 the tail, v5 the teeth, v6 a tooth's tau, v7 the teeth's spacing (samples,
 *          x 0.5 .. 1.5 with SNAP), v8 how many
 *   HATS   v0 the band (p16, TONE +-8 semitones), v1 its damping, v2 the high-pass (p16, TONE +-12 semitones), v3
 *          the sizzle's corner,
 *          v4 the sizzle (Q12, x SNAP / 64; SNAP above 64 adds up to x1), v5 the metal source's level, v6 white
 *          noise's, v7 the rise, v8 the source's ratios (DV_METAL_R)
 *   CYM    v0 v1 the high band and its damping, v2 v3 the low band, v4 the high band's share (x TONE / 64), v5 the
 *          strike (x SNAP / 64), v6 its tau (ms), v7 v8 the metal source's and noise's levels, v9 the ratios
 *   TOM    v0 the bend (p16, x SNAP / 64; SNAP above 64 adds up to 4 semitones), v1 the stick's tau, v2 its
 *          low-pass (p16), v3 the stick (low noise), v4 the slap (high noise), both x TONE / 64, v5 a soft clip
 *          (Q12, 0: none; TONE above 64 drives it up to x1 more)
 *   RIM    v0 the 2nd sine (p16 from the 1st), v1 v2 their levels (the 2nd x TONE / 64), v3 the drive (Q12, as
 *          the kick's)
 *   BELL   v0 the 2nd square (p16 from the 1st), v1 the band (p16 from the 1st, TONE +-8 semitones), v2 its
 *          damping, v3 the strike (Q14, x SNAP / 64), v4 its tau, v5 the squares themselves (x TONE / 64) */
typedef struct {
    uint8_t run, oct16;
    int16_t p16;
    uint16_t tau0, gain;
    int16_t v[13];
} dv_kit_t;
/* the accent per kit: + (x / 16384) per accent step (75: +4 dB at full, as Felucca's own) */
static const uint8_t DV_KIT_ACC[DV_NKIT] = {120, 45, 100, 40, 40, 80, 60, 70,
                                            /* round 13 */ 100, 90, 110, 80, 50, 90, 70, 60, 100, 60, 60, 70, 60, 70, 70};
static const dv_kit_t DV_KIT[DV_NKIT][DV_NLANE] = {
    {   /* 80 */
        /* deep sine, a small drop, long decay */
        {DVT_PUNCH, 64, 499, 198, 3124, {26, 150, 0, 300, 0, 0, 0, 4096, 12, 0, 0, 0, 0}},
        /* shells 173 / 336 Hz, noise 2.7 .. 7 kHz, tau 30 ms */
        {DVT_SNARE, 40, 845, 125, 5690, {241, 0, 100, 53, 32767, 11000, 1678, 6827, 30, 0, 28000, 0, 1828}},
        /* band 1 kHz, 3 teeth 10 ms, tail 90 ms */
        {DVT_CLAP, 48, 1331, 316, 13485, {0, 4096, 1678, 0, 2000, 30000, 35, 441, 3, 0, 0, 0, 0}},
        /* six squares, band 7.1 kHz, tau 16 ms */
        {DVT_HATC, 45, 893, 60, 9017, {1874, 5120, 2020, 2042, 7209, 32767, 0, 3, 0, 0, 0, 0, 0}},
        /* tau 75 ms */
        {DVT_HATO, 63, 893, 253, 13955, {1874, 5120, 1934, 2042, 1147, 32767, 0, 3, 0, 0, 0, 0, 0}},
        /* mid tom 139 Hz, tau 58 ms, a little bend */
        {DVT_TOM, 53, 785, 182, 4055, {8, 100, 1302, 1200, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
        /* 455 + 1667 Hz into a hard clip */
        {DVT_RIM, 32, 1113, 12, 4599, {360, 12000, 30000, 32000, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
        /* squares 540 / 800 Hz, band 860 Hz */
        {DVT_BELL, 50, 1161, 252, 2697, {109, 129, 3413, 30000, 45, 0, 0, 0, 0, 0, 0, 0, 0}},
    },
    {   /* 10 */
        /* 62 Hz swept from 90, tau 190 ms */
        {DVT_PUNCH, 60, 561, 464, 2239, {103, 180, 0, 0, 0, 1233, 2600, 4096, 0, 0, 0, 0, 0}},
        /* one shell 185 Hz swept from 280, noise tau 100 ms */
        {DVT_SNARE, 40, 864, 418, 3710, {0, 115, 160, 77, 0, 9000, 1828, 8192, 30, 0, 32767, 0, 1828}},
        /* 3 claps 28 ms apart, a 0.7 s tail */
        {DVT_CLAP, 30, 1678, 3636, 2932, {0, 9102, 1715, 8000, 17000, 30000, 100, 1235, 3, 0, 0, 0, 0}},
        /* four squares, band 7.2 kHz, tau 80 ms */
        {DVT_HATC, 45, 1014, 300, 6315, {1900, 3277, 1444, 2042, 0, 19661, 0, 3, 2, 0, 0, 0, 0}},
        /* tau 0.7 s */
        {DVT_HATO, 63, 1014, 1516, 5167, {1900, 3277, 1444, 2042, 0, 19661, 0, 3, 2, 0, 0, 0, 0}},
        /* (stand-in) a body of the kit: 120 Hz bent down from 160 */
        {DVT_TOM, 53, 744, 377, 2858, {80, 30, 1444, 1500, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
        /* (stand-in) a short shell click, 740 Hz */
        {DVT_RIM, 32, 1248, 25, 5868, {254, 22000, 9000, 8192, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
        /* the cymbal: band 4.2 kHz, 60 ms + 0.9 s */
        {DVT_CYM, 53, 1014, 3773, 10841, {1766, 3413, 1729, 3413, 16384, 20000, 60, 19661, 0, 2, 0, 0, 0}},
    },
    {   /* 66 */
        /* 64 Hz, a slow rise, tau 58 ms */
        {DVT_PUNCH, 64, 570, 119, 4353, {0, 100, 0, 0, 0, 0, 0, 4096, 40, 0, 0, 0, 0}},
        /* shells 280 / 397 Hz, bright noise, tau 14 ms */
        {DVT_SNARE, 40, 979, 54, 10633, {107, 0, 100, 120, 6000, 1200, 1907, 8192, 30, 16000, 32767, 32767, 1828}},
        /* (stand-in) the snare noise struck 3 times */
        {DVT_CLAP, 48, 1777, 210, 7505, {0, 6827, 1870, 0, 6000, 30000, 50, 400, 3, 0, 0, 0, 0}},
        /* noise through a narrow band at 8 kHz, tau 52 ms */
        {DVT_HATC, 45, 1907, 225, 4594, {1907, 1024, 1870, 2042, 0, 0, 7000, 3, 6, 0, 0, 0, 0}},
        /* tau 78 ms */
        {DVT_HATO, 63, 1907, 197, 6084, {1907, 1024, 1870, 2042, 0, 0, 7000, 3, 6, 0, 0, 0, 0}},
        /* a low conga 157 Hz, tau 30 ms */
        {DVT_CONGA, 53, 819, 94, 5803, {0, 50, 1523, 0, 1500, 0, 0, 0, 0, 0, 0, 0, 0}},
        /* 1.5 kHz, tau 2.7 ms */
        {DVT_RIM, 32, 1441, 13, 8636, {304, 26000, 1000, 4096, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
        /* sines 830 / 613 Hz, tau 14 ms */
        {DVT_RIM, 40, 1280, 60, 3629, {-84, 30000, 7500, 4096, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
    },
    {   /* 55 */
        /* 55 Hz dropping from 100, tau 45 ms */
        {DVT_PUNCH, 60, 531, 103, 4388, {163, 120, 0, 1600, 0, 1444, 1500, 6000, 0, 0, 0, 0, 0}},
        /* 245 Hz, noise 4.2 .. 7.6 kHz, tau 22 ms */
        {DVT_SNARE, 40, 942, 92, 5087, {0, 26, 180, 64, 0, 9000, 1809, 4096, 100, 32767, 32000, 0, 1828}},
        /* (stand-in) the snare noise struck 3 times */
        {DVT_CLAP, 48, 1809, 175, 6050, {0, 5120, 1870, 0, 6000, 30000, 50, 360, 3, 0, 0, 0, 0}},
        /* a cluster 8 .. 14.7 kHz + noise, tau 25 ms */
        {DVT_HATC, 45, 1909, 94, 2473, {1996, 5120, 1940, 2042, 4000, 11469, 18022, 3, 3, 0, 0, 0, 0}},
        /* tau 125 ms */
        {DVT_HATO, 63, 1909, 354, 1662, {1996, 5120, 1940, 2042, 4000, 11469, 18022, 3, 3, 0, 0, 0, 0}},
        /* a low tom 150 Hz, tau 57 ms */
        {DVT_TOM, 53, 824, 179, 4091, {21, 600, 1523, 9000, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
        /* 500 + 1467 Hz, tau 2 ms */
        {DVT_RIM, 32, 1139, 10, 5884, {435, 22000, 22000, 24576, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
        /* a wide cluster 0.7 .. 7.6 kHz, tau 34 ms */
        {DVT_CYM, 50, 1240, 114, 13203, {1701, 4096, 1550, 4096, 22000, 20000, 30, 19661, 8192, 4, 0, 0, 0}},
    },
    {   /* 77 */
        /* 66 Hz, a swell, tau 140 ms */
        {DVT_PUNCH, 60, 579, 337, 2590, {26, 450, 0, 1500, 520, 1478, 1800, 4096, 30, 0, 0, 0, 0}},
        /* 226 Hz, noise 1.2 .. 8 kHz, 2.5 + 60 ms */
        {DVT_SNARE, 40, 919, 251, 4125, {0, 0, 100, 37, 0, 20000, 1585, 10240, 25, 32767, 14000, 0, 1828}},
        /* 8 claps 7 ms apart, a tail */
        {DVT_CLAP, 48, 1393, 281, 8368, {0, 6302, 1585, 1500, 8000, 30000, 52, 322, 6, 0, 0, 0, 0}},
        /* six pulses 3.7 .. 13.5 kHz, tau 80 ms */
        {DVT_HATC, 45, 1690, 300, 3359, {1828, 8192, 1596, 2042, 0, 29491, 0, 3, 5, 0, 0, 0, 0}},
        /* tau 0.47 s */
        {DVT_HATO, 63, 1690, 1188, 2748, {1828, 8192, 1596, 2042, 0, 29491, 0, 3, 5, 0, 0, 0, 0}},
        /* tom 140 Hz bent 7.5 %, tau 120 ms */
        {DVT_TOM, 53, 797, 377, 2858, {20, 180, 1523, 2500, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
        /* claves 2350 / 3180 Hz, tau 30 ms */
        {DVT_CLAVE, 32, 1568, 149, 2131, {84, 28000, 10000, 6348, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
        /* the cymbal: 34 ms + 0.3 s */
        {DVT_CYM, 53, 1690, 1100, 7582, {1969, 5120, 1748, 5120, 24000, 16000, 34, 24576, 6554, 5, 0, 0, 0}},
    },
    {   /* DEEP (1.5.6): the pieces of 77 80 77 80 80 77 80 10 */
        /* 77's kick 2 semitones down (66 -> 59 Hz), the swell, tau 140 ms */
        {DVT_PUNCH, 60, 547, 337, 2590, {26, 450, 0, 1500, 520, 1478, 1800, 4096, 30, 0, 0, 0, 0}},
        /* 80's snare: shells 173 / 336 Hz, noise 2.7 .. 7 kHz */
        {DVT_SNARE, 40, 845, 125, 5690, {241, 0, 100, 53, 32767, 11000, 1678, 6827, 30, 0, 28000, 0, 1828}},
        /* 77's claps: 8 claps 7 ms apart, a tail */
        {DVT_CLAP, 48, 1393, 281, 8368, {0, 6302, 1585, 1500, 8000, 30000, 52, 322, 6, 0, 0, 0, 0}},
        /* 80's hats: six squares, band 7.1 kHz */
        {DVT_HATC, 45, 893, 60, 9017, {1874, 5120, 2020, 2042, 7209, 32767, 0, 3, 0, 0, 0, 0, 0}},
        {DVT_HATO, 63, 893, 253, 13955, {1874, 5120, 1934, 2042, 1147, 32767, 0, 3, 0, 0, 0, 0, 0}},
        /* 77's tom 140 Hz bent 7.5 % */
        {DVT_TOM, 53, 797, 377, 2858, {20, 180, 1523, 2500, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
        /* 80's rim: 455 + 1667 Hz into a hard clip */
        {DVT_RIM, 32, 1113, 12, 4599, {360, 12000, 30000, 32000, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
        /* 10's cymbal: band 4.2 kHz, 60 ms + 0.9 s */
        {DVT_CYM, 53, 1014, 3773, 10841, {1766, 3413, 1729, 3413, 16384, 20000, 60, 19661, 0, 2, 0, 0, 0}},
    },
    {   /* TIGHT (1.5.6): the pieces of 55 66 80 55 55 55 66 80 */
        /* 55's kick: 55 Hz dropping from 100, tau 45 ms */
        {DVT_PUNCH, 60, 531, 103, 4388, {163, 120, 0, 1600, 0, 1444, 1500, 6000, 0, 0, 0, 0, 0}},
        /* 66's snare: shells 280 / 397 Hz, bright noise, tau 14 ms */
        {DVT_SNARE, 40, 979, 54, 10633, {107, 0, 100, 120, 6000, 1200, 1907, 8192, 30, 16000, 32767, 32767, 1828}},
        /* 80's clap: band 1 kHz, 3 teeth 10 ms, tail 90 ms */
        {DVT_CLAP, 48, 1331, 316, 13485, {0, 4096, 1678, 0, 2000, 30000, 35, 441, 3, 0, 0, 0, 0}},
        /* 55's hats: a cluster 8 .. 14.7 kHz + noise */
        {DVT_HATC, 45, 1909, 94, 2473, {1996, 5120, 1940, 2042, 4000, 11469, 18022, 3, 3, 0, 0, 0, 0}},
        {DVT_HATO, 63, 1909, 354, 1662, {1996, 5120, 1940, 2042, 4000, 11469, 18022, 3, 3, 0, 0, 0, 0}},
        /* 55's low tom 150 Hz */
        {DVT_TOM, 53, 824, 179, 4091, {21, 600, 1523, 9000, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
        /* 66's rim: 1.5 kHz, tau 2.7 ms */
        {DVT_RIM, 32, 1441, 13, 8636, {304, 26000, 1000, 4096, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
        /* 80's bell: squares 540 / 800 Hz, band 860 Hz */
        {DVT_BELL, 50, 1161, 252, 2697, {109, 129, 3413, 30000, 45, 0, 0, 0, 0, 0, 0, 0, 0}},
    },
    {   /* PERC (1.5.6): the pieces of 10 77 77 66 66 66 77 66 */
        /* 10's kick: 62 Hz swept from 90, tau 190 ms */
        {DVT_PUNCH, 60, 561, 464, 2239, {103, 180, 0, 0, 0, 1233, 2600, 4096, 0, 0, 0, 0, 0}},
        /* 77's snare: 226 Hz, noise 1.2 .. 8 kHz */
        {DVT_SNARE, 40, 919, 251, 4125, {0, 0, 100, 37, 0, 20000, 1585, 10240, 25, 32767, 14000, 0, 1828}},
        /* 77's claps */
        {DVT_CLAP, 48, 1393, 281, 8368, {0, 6302, 1585, 1500, 8000, 30000, 52, 322, 6, 0, 0, 0, 0}},
        /* 66's hats: noise through a narrow band at 8 kHz */
        {DVT_HATC, 45, 1907, 225, 4594, {1907, 1024, 1870, 2042, 0, 0, 7000, 3, 6, 0, 0, 0, 0}},
        {DVT_HATO, 63, 1907, 197, 6084, {1907, 1024, 1870, 2042, 0, 0, 7000, 3, 6, 0, 0, 0, 0}},
        /* 66's low conga 157 Hz on TOM */
        {DVT_CONGA, 53, 819, 94, 5803, {0, 50, 1523, 0, 1500, 0, 0, 0, 0, 0, 0, 0, 0}},
        /* 77's claves on RIM: 2350 / 3180 Hz */
        {DVT_CLAVE, 32, 1568, 149, 2131, {84, 28000, 10000, 6348, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
        /* 66's bell: sines 830 / 613 Hz */
        {DVT_RIM, 40, 1280, 60, 3629, {-84, 30000, 7500, 4096, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
    },
    {   /* THUMP LAB WAREHOUSE (round 13) */
        /* 52 Hz, +13 st over 22 ms, tau 105 ms */
        {DVT_PUNCH, 60, 512, 283, 24000, {208, 220, 160, 3000, 1500, 1636, 9500, 6200, 0, 6, 0, 0, 0}},
        /* shell 190 Hz x1.8, noise 5.2 kHz, tau 40 ms */
        {DVT_SNARE, 40, 871, 167, 4287, {192, 32, 150, 45, 21000, 13000, 1788, 4600, 40, 30000, 32000, 14000, 1850}},
        /* 4 bursts 7.5 ms apart, band 1.5 kHz, tail tau 55 ms */
        {DVT_CLAP, 48, 1444, 193, 4092, {0, 4800, 1686, 7000, 6000, 30000, 35, 331, 4, 0, 0, 0, 0}},
        /* row 0 from 205 Hz, band 7.5 kHz, tau 20 ms */
        {DVT_HATC, 45, 892, 75, 1458, {1890, 5120, 2020, 2042, 7000, 32767, 0, 3, 0, 0, 0, 0, 0}},
        /* row 0 from 205 Hz, band 7.5 kHz, tau 120 ms */
        {DVT_HATO, 63, 892, 303, 2138, {1890, 5120, 1955, 2042, 1200, 32767, 0, 3, 0, 0, 0, 0, 0}},
        /* 118 Hz bent 5 st, tau 120 ms */
        {DVT_TOM, 53, 739, 377, 24000, {80, 80, 1425, 3000, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
        /* 470 Hz + +22 st, tau 2.2 ms */
        {DVT_RIM, 32, 1122, 11, 23589, {352, 20000, 24000, 22000, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
        /* row 1 from 400 Hz, bands 6.2 / 3.2 kHz, ring tau 700 ms */
        {DVT_CYM, 53, 1078, 2201, 22347, {1837, 5120, 1654, 5851, 22000, 18000, 36, 20000, 6000, 1, 0, 0, 0}},
    },
    {   /* THUMP LAB BOOM BAP (round 13) */
        /* 50 Hz, +5 st over 120 ms, tau 280 ms */
        {DVT_PUNCH, 60, 502, 756, 23185, {80, 1200, 0, 3600, 1000, 0, 0, 4300, 0, 0, 0, 0, 0}},
        /* shell 330 Hz x1.6, noise 3.0 kHz, tau 55 ms */
        {DVT_SNARE, 40, 1024, 230, 24000, {141, 8, 80, 70, 21000, 22000, 1636, 6000, 20, 22000, 12000, 3000, 1777}},
        /* 3 bursts 11 ms apart, band 1.1 kHz, tail tau 45 ms */
        {DVT_CLAP, 48, 1345, 158, 18122, {0, 4096, 1636, 2500, 4000, 26000, 50, 485, 3, 0, 0, 0, 0}},
        /* row 0 from 205 Hz, band 6.0 kHz, tau 16 ms */
        {DVT_HATC, 45, 892, 60, 24000, {1828, 4500, 1940, 2020, 2000, 14000, 12000, 3, 0, 0, 0, 0, 0}},
        /* row 0 from 205 Hz, band 5.5 kHz, tau 110 ms */
        {DVT_HATO, 63, 892, 278, 24000, {1804, 4500, 1907, 2020, 800, 14000, 12000, 3, 0, 0, 0, 0, 0}},
        /* 82 Hz bent 6 st, tau 180 ms */
        {DVT_TOM, 53, 639, 566, 24000, {96, 100, 1302, 3500, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
        /* 2600 Hz + +5 st, tau 22 ms */
        {DVT_CLAVE, 32, 1596, 109, 24000, {80, 26000, 5000, 4096, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
        /* row 1 from 400 Hz, bands 5.2 / 2.6 kHz, ring tau 450 ms */
        {DVT_CYM, 53, 1078, 1415, 24000, {1788, 5120, 1596, 5851, 15000, 18000, 30, 10000, 14000, 1, 0, 0, 0}},
    },
    {   /* THUMP LAB DISCO (round 13) */
        /* 58 Hz, +9 st over 28 ms, tau 120 ms */
        {DVT_PUNCH, 60, 543, 324, 24000, {144, 280, 96, 3800, 1400, 1550, 6000, 4700, 0, 3, 0, 0, 0}},
        /* shell 215 Hz x1.6, noise 6.8 kHz, tau 40 ms */
        {DVT_SNARE, 40, 906, 167, 7933, {154, 24, 100, 40, 16000, 11000, 1862, 4200, 30, 28000, 34000, 12000, 1870}},
        /* 3 bursts 6 ms apart, band 1.9 kHz, tail tau 32 ms */
        {DVT_CLAP, 48, 1509, 112, 3851, {0, 5000, 1715, 9000, 3500, 32000, 25, 265, 3, 0, 0, 0, 0}},
        /* row 0 from 205 Hz, band 8.2 kHz, tau 11 ms */
        {DVT_HATC, 45, 892, 41, 1304, {1914, 5120, 2008, 2052, 3000, 30000, 6000, 2, 0, 0, 0, 0, 0}},
        /* hiss: noise through a wide band, tau 220 ms */
        {DVT_HATO, 63, 892, 556, 475, {1955, 2600, 1924, 2020, 800, 9000, 30000, 10, 0, 0, 0, 0, 0}},
        /* 205 Hz bent 3 st, tau 110 ms */
        {DVT_TOM, 53, 892, 346, 24000, {48, 70, 1494, 4500, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
        /* 620 Hz + +20 st, tau 2.5 ms */
        {DVT_RIM, 32, 1199, 12, 23026, {320, 16000, 22000, 14000, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
        /* squares 560 / 830 Hz, tau 90 ms */
        {DVT_BELL, 50, 1171, 302, 10568, {109, 129, 3413, 30000, 45, 3000, 0, 0, 0, 0, 0, 0, 0}},
    },
    {   /* THUMP LAB TRASH CAN (round 13) */
        /* 70 Hz, +11 st over 10 ms, tau 65 ms */
        {DVT_PUNCH, 60, 595, 175, 20617, {176, 100, 128, 5000, 3500, 1596, 12000, 20000, 0, 2, 0, 0, 0}},
        /* shell 250 Hz x1.3, noise 3.5 kHz, tau 45 ms */
        {DVT_SNARE, 40, 947, 188, 24000, {77, 48, 120, 60, 22000, 9000, 1678, 8000, 50, 30000, 34000, 22000, 1748}},
        /* 6 bursts 5 ms apart, band 0.9 kHz, tail tau 85 ms */
        {DVT_CLAP, 48, 1302, 298, 24000, {0, 7000, 1585, 12000, 7000, 32000, 40, 220, 6, 0, 0, 0, 0}},
        /* row 3 from 205 Hz, band 8.0 kHz, tau 20 ms */
        {DVT_HATC, 45, 892, 75, 24000, {1907, 3000, 1924, 1969, 5000, 24000, 18000, 3, 3, 0, 0, 0, 0}},
        /* row 3 from 205 Hz, band 8.0 kHz, tau 150 ms */
        {DVT_HATO, 63, 892, 379, 24000, {1907, 3000, 1924, 1969, 5000, 24000, 18000, 3, 3, 0, 0, 0, 0}},
        /* 92 Hz bent 4 st, tau 90 ms */
        {DVT_TOM, 53, 671, 283, 24000, {64, 140, 1550, 6000, 2000, 14000, 0, 0, 0, 0, 0, 0, 0}},
        /* 700 Hz + +20 st, tau 3 ms */
        {DVT_RIM, 32, 1233, 15, 24000, {320, 26000, 26000, 32000, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
        /* row 3 from 400 Hz, bands 4.3 / 2.1 kHz, ring tau 380 ms */
        {DVT_CYM, 53, 1078, 1195, 22580, {1735, 3000, 1537, 3500, 20000, 28000, 28, 12000, 26000, 3, 0, 0, 0}},
    },
    {   /* THUMP LAB ARCADE (round 13) */
        /* 80 Hz, +24 st over 35 ms, tau 60 ms */
        {DVT_PUNCH, 60, 632, 162, 24000, {384, 350, 0, 4000, 3000, 0, 0, 22000, 0, 0, 0, 0, 0}},
        /* shell 300 Hz x1, noise 6.5 kHz, tau 50 ms */
        {DVT_SNARE, 40, 998, 209, 24000, {0, 192, 250, 20, 0, 8000, 1850, 8192, 40, 20000, 30000, 0, 1940}},
        /* 2 bursts 10 ms apart, band 2.2 kHz, tail tau 22 ms */
        {DVT_CLAP, 48, 1550, 77, 21464, {0, 8000, 1777, 1500, 1500, 30000, 25, 441, 2, 0, 0, 0, 0}},
        /* two squares a fifth apart (5.2 kHz), tau 6 ms */
        {DVT_HATC, 45, 1788, 22, 24000, {1837, 2000, 1940, 2042, 500, 32767, 0, 2, 8, 0, 0, 0, 0}},
        /* two squares, tau 55 ms */
        {DVT_HATO, 63, 1788, 139, 24000, {1837, 2000, 1940, 2042, 500, 32767, 0, 2, 8, 0, 0, 0, 0}},
        /* 330 Hz bent 14 st, tau 70 ms */
        {DVT_TOM, 53, 1024, 220, 24000, {224, 30, 1636, 2000, 0, 16000, 0, 0, 0, 0, 0, 0, 0}},
        /* 1760 Hz + +12 st, tau 14 ms */
        {DVT_CLAVE, 32, 1488, 70, 23211, {192, 28000, 9000, 4096, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
        /* the noise channel: noise through two bands, ring tau 480 ms */
        {DVT_CYM, 53, 1078, 943, 24000, {1870, 6000, 1636, 6000, 20000, 20000, 20, 0, 32000, 6, 0, 0, 0}},
    },
    {   /* KITBOX DENTED (round 13) */
        /* 68 Hz, +6 st over 10 ms, tau 16 ms */
        {DVT_PUNCH, 60, 587, 43, 24000, {96, 100, 0, 2800, 0, 1494, 4000, 14000, 0, 0, 0, 0, 0}},
        /* shell 185 Hz x1, noise 4.8 kHz, tau 17 ms */
        {DVT_SNARE, 40, 864, 71, 24000, {0, 96, 120, 50, 0, 10000, 1766, 8192, 30, 28000, 30000, 0, 1870}},
        /* 3 bursts 7 ms apart, band 1.3 kHz, tail tau 14 ms */
        {DVT_CLAP, 48, 1404, 49, 24000, {0, 4800, 1678, 4000, 3000, 28000, 30, 309, 3, 0, 0, 0, 0}},
        /* four squares, band 7.2 kHz, tau 11 ms */
        {DVT_HATC, 45, 989, 41, 24000, {1878, 3277, 2020, 2042, 0, 19661, 0, 3, 2, 0, 0, 0, 0}},
        /* four squares, tau 22 ms */
        {DVT_HATO, 63, 989, 56, 24000, {1878, 3277, 2020, 2042, 0, 19661, 0, 3, 2, 0, 0, 0, 0}},
        /* 150 Hz bent 4 st, tau 19 ms */
        {DVT_TOM, 53, 806, 60, 24000, {64, 60, 1425, 2500, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
        /* 2400 Hz + +5 st, tau 9 ms */
        {DVT_CLAVE, 32, 1574, 45, 24000, {80, 28000, 10000, 4096, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
        /* row 2 from 290 Hz, bands 4.2 / 2.4 kHz, ring tau 450 ms */
        {DVT_CYM, 53, 989, 1415, 14879, {1729, 3413, 1574, 3413, 16000, 20000, 60, 19661, 0, 2, 0, 0, 0}},
    },
    {   /* KITBOX TIN ROOF (round 13) */
        /* 105 Hz, +9 st over 6 ms, tau 16 ms */
        {DVT_PUNCH, 60, 707, 43, 24000, {144, 60, 0, 1800, 0, 1654, 5000, 12000, 0, 0, 0, 0, 0}},
        /* shell 300 Hz x1.5, noise 6.5 kHz, tau 9 ms */
        {DVT_SNARE, 40, 998, 38, 24000, {128, 48, 80, 40, 8000, 10000, 1850, 2200, 20, 26000, 24000, 6000, 1907}},
        /* 2 bursts 6 ms apart, band 2.1 kHz, tail tau 9 ms */
        {DVT_CLAP, 48, 1537, 32, 24000, {0, 5000, 1777, 3000, 2500, 26000, 25, 265, 2, 0, 0, 0, 0}},
        /* tinny cluster, narrow band 9 kHz, tau 10 ms */
        {DVT_HATC, 45, 1150, 37, 4738, {1940, 1200, 1907, 2020, 5000, 32767, 0, 2, 7, 0, 0, 0, 0}},
        /* tinny cluster, tau 16 ms */
        {DVT_HATO, 63, 1150, 40, 7128, {1940, 1200, 1907, 2020, 5000, 32767, 0, 2, 7, 0, 0, 0, 0}},
        /* 230 Hz bent 3 st, tau 12 ms */
        {DVT_TOM, 53, 924, 38, 24000, {48, 50, 1523, 3000, 1000, 8000, 0, 0, 0, 0, 0, 0, 0}},
        /* 1250 Hz + +20 st, tau 3 ms */
        {DVT_RIM, 32, 1393, 15, 24000, {320, 22000, 22000, 14000, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
        /* row 7 from 640 Hz, bands 8.2 / 5.2 kHz, ring tau 700 ms */
        {DVT_CYM, 53, 1208, 2201, 15235, {1914, 2000, 1788, 3000, 25000, 26000, 25, 24000, 2000, 7, 0, 0, 0}},
    },
    {   /* KITBOX BUCKET (round 13) */
        /* 52 Hz, +4 st over 14 ms, tau 21 ms */
        {DVT_PUNCH, 60, 512, 57, 24000, {64, 140, 0, 2000, 0, 0, 0, 9000, 0, 0, 0, 0, 0}},
        /* shell 150 Hz x1.5, noise 1.8 kHz, tau 19 ms */
        {DVT_SNARE, 40, 806, 79, 24000, {128, 64, 140, 60, 12000, 16000, 1494, 8000, 40, 20000, 18000, 0, 1777}},
        /* 3 bursts 9 ms apart, band 0.7 kHz, tail tau 15 ms */
        {DVT_CLAP, 48, 1233, 53, 24000, {0, 5000, 1523, 1500, 3000, 20000, 40, 397, 3, 0, 0, 0, 0}},
        /* soft noise, band 4 kHz, tau 11 ms */
        {DVT_HATC, 45, 892, 41, 24000, {1715, 5120, 1850, 1907, 500, 0, 12000, 5, 6, 0, 0, 0, 0}},
        /* soft noise, tau 22 ms */
        {DVT_HATO, 63, 892, 56, 24000, {1715, 5120, 1850, 1907, 500, 0, 12000, 5, 6, 0, 0, 0, 0}},
        /* 85 Hz bent 4 st, tau 19 ms */
        {DVT_TOM, 53, 649, 60, 22868, {64, 120, 1233, 3500, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
        /* 580 Hz + +14 st, tau 3.5 ms */
        {DVT_RIM, 32, 1181, 17, 10982, {224, 18000, 6000, 6000, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
        /* row 2 from 290 Hz, bands 3.6 / 1.9 kHz, ring tau 240 ms */
        {DVT_CYM, 53, 989, 755, 24000, {1686, 4000, 1509, 4000, 6000, 14000, 40, 12000, 8000, 2, 0, 0, 0}},
    },
    {   /* KITBOX STATIC (round 13) */
        /* 62 Hz, +3 st over 6 ms, tau 11 ms */
        {DVT_PUNCH, 60, 561, 30, 24000, {48, 60, 0, 1500, 0, 1574, 14000, 16000, 0, 0, 0, 0, 0}},
        /* shell 240 Hz x1, noise 5.5 kHz, tau 7 ms */
        {DVT_SNARE, 40, 936, 29, 24000, {0, 0, 50, 20, 0, 4000, 1804, 9000, 30, 28000, 34000, 20000, 1777}},
        /* 3 bursts 6 ms apart, band 2.4 kHz, tail tau 7 ms */
        {DVT_CLAP, 48, 1574, 25, 19917, {0, 7000, 1617, 3000, 4000, 30000, 25, 265, 3, 0, 0, 0, 0}},
        /* white noise, band 8.5 kHz, tau 8 ms */
        {DVT_HATC, 45, 892, 30, 1855, {1924, 2500, 1940, 2042, 3000, 0, 30000, 2, 6, 0, 0, 0, 0}},
        /* white noise, tau 11 ms */
        {DVT_HATO, 63, 892, 28, 1893, {1924, 2500, 1940, 2042, 3000, 0, 30000, 2, 6, 0, 0, 0, 0}},
        /* 140 Hz bent 2 st, tau 11 ms */
        {DVT_TOM, 53, 787, 35, 23428, {32, 180, 1494, 9000, 3000, 8000, 0, 0, 0, 0, 0, 0, 0}},
        /* 1800 Hz + +14 st, tau 3 ms */
        {DVT_RIM, 32, 1494, 15, 15949, {224, 22000, 22000, 20000, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
        /* hiss: noise through two bands, ring tau 800 ms */
        {DVT_CYM, 53, 1078, 1886, 5071, {1890, 6000, 1715, 6000, 22000, 22000, 25, 0, 32000, 6, 0, 0, 0}},
    },
    {   /* KITBOX PARKING LOT (round 13) */
        /* 60 Hz, +8 st over 14 ms, tau 18 ms */
        {DVT_PUNCH, 60, 552, 49, 24000, {128, 140, 96, 2400, 800, 1617, 8000, 13000, 0, 3, 0, 0, 0}},
        /* shell 210 Hz x1.6, noise 5.0 kHz, tau 17 ms */
        {DVT_SNARE, 40, 899, 71, 24000, {154, 32, 100, 45, 14000, 12000, 1777, 5120, 25, 32000, 30000, 12000, 1828}},
        /* 2 bursts 8 ms apart, band 1.5 kHz, tail tau 16 ms */
        {DVT_CLAP, 48, 1444, 56, 24000, {0, 4800, 1678, 2500, 3500, 30000, 30, 353, 2, 0, 0, 0, 0}},
        /* row 0 from 205 Hz, band 7.8 kHz, tau 10 ms */
        {DVT_HATC, 45, 892, 37, 24000, {1900, 5120, 2031, 2052, 5000, 30000, 0, 3, 0, 0, 0, 0, 0}},
        /* row 0 from 205 Hz, band 7.8 kHz, tau 21 ms */
        {DVT_HATO, 63, 892, 53, 24000, {1900, 5120, 2031, 2052, 5000, 30000, 0, 3, 0, 0, 0, 0, 0}},
        /* 135 Hz bent 4 st, tau 17 ms */
        {DVT_TOM, 53, 777, 53, 24000, {64, 80, 1425, 2800, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
        /* 740 Hz + +22 st, tau 2.4 ms */
        {DVT_RIM, 32, 1248, 12, 21863, {352, 24000, 20000, 20000, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
        /* row 0 from 420 Hz, bands 6.8 / 3.6 kHz, ring tau 400 ms */
        {DVT_CYM, 53, 1091, 1258, 24000, {1862, 5120, 1686, 5851, 20000, 22000, 30, 18000, 4000, 0, 0, 0, 0}},
    },
    {   /* GARAGE STUDIO (round 13) */
        /* 62 Hz, +5 st over 30 ms, tau 85 ms */
        {DVT_PUNCH, 60, 561, 229, 24000, {80, 300, 48, 2600, 400, 1523, 3500, 4096, 0, 2, 0, 0, 0}},
        /* shell 195 Hz x1.6, noise 4.3 kHz, tau 40 ms */
        {DVT_SNARE, 40, 879, 167, 3533, {154, 24, 140, 80, 19000, 20000, 1735, 5600, 30, 22000, 22000, 4000, 1850}},
        /* 3 bursts 11 ms apart, band 1.1 kHz, tail tau 70 ms */
        {DVT_CLAP, 48, 1358, 245, 6727, {0, 4096, 1654, 3000, 7000, 28000, 40, 485, 3, 0, 0, 0, 0}},
        /* row 0 from 205 Hz, band 7.0 kHz, tau 30 ms */
        {DVT_HATC, 45, 892, 112, 12566, {1870, 5120, 1983, 2031, 4000, 24000, 4000, 5, 0, 0, 0, 0, 0}},
        /* row 0 from 205 Hz, band 7.0 kHz, tau 160 ms */
        {DVT_HATO, 63, 892, 404, 15801, {1870, 5120, 1955, 2031, 1500, 24000, 4000, 5, 0, 0, 0, 0, 0}},
        /* 108 Hz bent 3.5 st, tau 190 ms */
        {DVT_TOM, 53, 715, 597, 6789, {56, 140, 1462, 4000, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
        /* 620 Hz + +21 st, tau 2.2 ms */
        {DVT_RIM, 32, 1199, 11, 3757, {336, 16000, 18000, 10000, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
        /* row 1 from 420 Hz, bands 5.4 / 2.8 kHz, ring tau 900 ms */
        {DVT_CYM, 53, 1091, 2830, 10648, {1799, 5120, 1617, 5851, 18000, 20000, 45, 18000, 3000, 1, 0, 0, 0}},
    },
    {   /* GARAGE BASEMENT (round 13) */
        /* 80 Hz, +5 st over 18 ms, tau 95 ms */
        {DVT_PUNCH, 60, 632, 256, 24000, {80, 180, 0, 3500, 800, 1425, 4500, 4096, 0, 0, 0, 0, 0}},
        /* shell 290 Hz x1.5, noise 3.0 kHz, tau 55 ms */
        {DVT_SNARE, 40, 989, 230, 5797, {128, 48, 100, 55, 18000, 20000, 1636, 6000, 30, 26000, 24000, 5000, 1804}},
        /* 2 bursts 12 ms apart, band 0.8 kHz, tail tau 40 ms */
        {DVT_CLAP, 48, 1270, 140, 4671, {0, 4096, 1585, 2500, 8000, 26000, 45, 529, 2, 0, 0, 0, 0}},
        /* a shaker: noise with a slow attack, tau 45 ms */
        {DVT_HATC, 45, 892, 168, 3183, {1850, 4000, 1828, 1907, 300, 0, 22000, 40, 6, 0, 0, 0, 0}},
        /* a long shaker, tau 120 ms */
        {DVT_HATO, 63, 892, 303, 2455, {1850, 4000, 1828, 1907, 300, 0, 22000, 70, 6, 0, 0, 0, 0}},
        /* 210 Hz bent 3 st, tau 75 ms */
        {DVT_CONGA, 53, 899, 236, 4650, {48, 60, 1444, 0, 3500, 6144, 0, 0, 0, 0, 0, 0, 0}},
        /* 1850 Hz + +8 st, tau 14 ms */
        {DVT_CLAVE, 32, 1502, 70, 2711, {128, 26000, 8000, 4096, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
        /* row 9 from 500 Hz, bands 8.8 / 5.2 kHz, ring tau 450 ms */
        {DVT_CYM, 53, 1139, 1415, 4417, {1934, 3000, 1788, 4000, 24000, 22000, 18, 22000, 10000, 9, 0, 0, 0}},
    },
    {   /* GARAGE PORCH (round 13) */
        /* 78 Hz, +3 st over 25 ms, tau 120 ms */
        {DVT_PUNCH, 60, 625, 324, 24000, {48, 250, 0, 3200, 1000, 1462, 3000, 5000, 0, 0, 0, 0, 0}},
        /* shell 330 Hz x1.5, noise 3.2 kHz, tau 45 ms */
        {DVT_SNARE, 40, 1024, 188, 14442, {128, 32, 100, 80, 8000, 24000, 1654, 5120, 20, 14000, 5000, 0, 1870}},
        /* 3 bursts 9 ms apart, band 1.3 kHz, tail tau 45 ms */
        {DVT_CLAP, 48, 1404, 158, 7674, {0, 4096, 1636, 4000, 5000, 28000, 35, 397, 3, 0, 0, 0, 0}},
        /* a shaker: noise, tau 35 ms */
        {DVT_HATC, 45, 892, 131, 2574, {1870, 3500, 1850, 1940, 300, 0, 24000, 25, 6, 0, 0, 0, 0}},
        /* a long shaker, tau 130 ms */
        {DVT_HATO, 63, 892, 329, 2036, {1870, 3500, 1850, 1940, 300, 0, 24000, 40, 6, 0, 0, 0, 0}},
        /* 300 Hz bent 4 st, tau 110 ms */
        {DVT_CONGA, 53, 998, 346, 6216, {64, 80, 1494, 0, 5000, 6144, 0, 0, 0, 0, 0, 0, 0}},
        /* 2300 Hz + +5 st, tau 20 ms */
        {DVT_CLAVE, 32, 1562, 99, 2507, {80, 28000, 9000, 4096, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
        /* row 1 from 420 Hz, bands 6.6 / 3.4 kHz, ring tau 600 ms */
        {DVT_CYM, 53, 1091, 1886, 14628, {1854, 5120, 1670, 5851, 16000, 18000, 40, 14000, 3000, 1, 0, 0, 0}},
    },
    {   /* GARAGE CANTINA (round 13) */
        /* 72 Hz, +4 st over 25 ms, tau 120 ms */
        {DVT_PUNCH, 60, 603, 324, 24000, {64, 250, 0, 3000, 0, 1444, 3000, 4096, 0, 0, 0, 0, 0}},
        /* shell 360 Hz x1.5, noise 4.5 kHz, tau 55 ms */
        {DVT_SNARE, 40, 1048, 230, 14723, {128, 16, 80, 50, 14000, 22000, 1748, 4000, 20, 28000, 22000, 9000, 1828}},
        /* 4 bursts 8 ms apart, band 1.4 kHz, tail tau 38 ms */
        {DVT_CLAP, 48, 1425, 133, 4326, {0, 4500, 1701, 1500, 4500, 30000, 30, 353, 4, 0, 0, 0, 0}},
        /* a guiro scrape: noise rising over 22 ms */
        {DVT_HATC, 45, 892, 150, 11264, {1777, 2500, 1748, 1870, 200, 3000, 26000, 220, 0, 0, 0, 0, 0}},
        /* a long scrape, rising over 30 ms */
        {DVT_HATO, 63, 892, 278, 7000, {1777, 2500, 1748, 1870, 200, 3000, 26000, 300, 0, 0, 0, 0, 0}},
        /* 420 Hz bent 3 st, tau 95 ms */
        {DVT_TOM, 53, 1091, 299, 6159, {48, 40, 1636, 4000, 3000, 8000, 0, 0, 0, 0, 0, 0, 0}},
        /* 2600 Hz + +5 st, tau 22 ms */
        {DVT_CLAVE, 32, 1596, 109, 2570, {80, 26000, 9000, 4096, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
        /* squares 520 / 770 Hz, tau 160 ms */
        {DVT_BELL, 50, 1150, 537, 1011, {109, 129, 3413, 32000, 50, 4000, 0, 0, 0, 0, 0, 0, 0}},
    },
    {   /* GARAGE TIN SHED (round 13) */
        /* 95 Hz, +7 st over 12 ms, tau 90 ms */
        {DVT_PUNCH, 60, 679, 243, 24000, {112, 120, 80, 4000, 2500, 1523, 7000, 6500, 0, 0, 0, 0, 0}},
        /* shell 420 Hz x1.4, noise 3.2 kHz, tau 75 ms */
        {DVT_SNARE, 40, 1091, 313, 3212, {115, 64, 120, 70, 20000, 18000, 1654, 2800, 30, 28000, 24000, 8000, 1777}},
        /* 3 bursts 14 ms apart, band 1.8 kHz, tail tau 55 ms */
        {DVT_CLAP, 48, 1494, 193, 4226, {0, 6000, 1678, 7000, 6000, 28000, 30, 617, 3, 0, 0, 0, 0}},
        /* a lid: inharmonic cluster, tau 13 ms */
        {DVT_HATC, 45, 1233, 49, 23106, {1841, 2000, 1924, 1996, 2500, 32767, 3000, 2, 9, 0, 0, 0, 0}},
        /* a pipe: the same cluster ringing, tau 170 ms */
        {DVT_HATO, 63, 1233, 430, 23106, {1841, 2000, 1924, 1996, 2500, 32767, 3000, 2, 9, 0, 0, 0, 0}},
        /* 270 Hz bent 5 st, tau 100 ms */
        {DVT_TOM, 53, 969, 314, 2878, {80, 70, 1574, 4000, 1500, 12000, 0, 0, 0, 0, 0, 0, 0}},
        /* 2100 Hz + +12 st, tau 40 ms */
        {DVT_CLAVE, 32, 1537, 199, 14952, {192, 24000, 16000, 4096, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
        /* squares 1180 / 2429 Hz, tau 250 ms */
        {DVT_BELL, 50, 1377, 839, 989, {200, 100, 2500, 24000, 30, 1500, 0, 0, 0, 0, 0, 0, 0}},
    }
};

/* ------------------------------------------------------- setup helpers (not in the sample loops) --- */
/* 2^(x / 4096), 0 <= x < 16 * 4096, Q16 (a cubic for the fraction: within 0.01 %) */
static uint32_t dv_exp2(uint32_t x)
{
    uint32_t f = x & 4095u, m;
    m = 65536u + ((f * (45594u + ((f * (14851u + ((f * 5092u) >> 12))) >> 12))) >> 12);
    return m << (x >> 12);
}
/* tau (us) -> per-sample decay, Q15: e^(-1 / (tau fs)), to the second order */
static uint32_t dv_k(uint32_t us)
{
    uint32_t x = 743039u / (us | 1u);
    return x >= 32768u ? 0u : 32768u - x + ((x * x) >> 16);
}
/* tau (us) -> per-block decay, Q16: e^(-CTL / (tau fs)), to the fourth order */
static uint32_t dv_kb(uint32_t us)
{
    uint32_t x = 47554467u / (us | 1u), x2, x3, x4, k;
    if (x > 46000u)
        x = 46000u;
    x2 = (x * x) >> 16;
    x3 = (x2 * x) >> 16;
    x4 = (x3 * x) >> 16;
    k = 65536u - x + (x2 >> 1) - x3 / 6u + x4 / 24u;
    return k > 65535u ? 65535u : k;
}
/* a main tau at DECAY d (us), from its tau at DECAY 0 (0.1 ms) and its span (octaves x 16) */
static uint32_t dv_tau_of(uint32_t tau0, uint32_t oct16, uint32_t d)
{
    uint32_t m = dv_exp2(d * oct16 * 4096u / (127u * 16u));   /* Q16, below 2^21 */
    return (tau0 * ((m * 100u) >> 12)) >> 4;
}
/* the main tau of a type at DECAY d (us) */
static uint32_t dv_tau(uint32_t type, uint32_t d) { return dv_tau_of(DV_DEF[type].tau0, DV_DEF[type].oct16, d); }
/* p16 -> the SVF's cutoff index (0..127 << 8: 30 Hz .. 16 kHz) */
static int32_t dv_cut(int32_t p16) { return clamp(((p16 - 360) * 4785) >> 8, 0, 127 << 8); }
/* p16 -> one-pole coefficient, Q15: w / (1 + w), w = 2 pi f / fs */
static int32_t dv_pole(int32_t p16)
{
    int32_t w = (int32_t)(((DV_P16(p16) >> 17) * 205887u) >> 15);
    return (w << 15) / (32768 + w);
}
/* p16 -> a bilinear one-pole high-pass (s / (s + w), not prewarped): y = g (x - x1) + p y1, g and p in Q14 */
static void dv_hp1(int32_t *g, int32_t *p, int32_t p16)
{
    uint32_t inc = p16 > 1900 ? DV_P16(clamp(p16, 0, 2239) - 192) << 1 : DV_P16(p16);   /* (above the table: an octave
                                                          * down, doubled) */
    int32_t u = (int32_t)(((inc >> 17) * 205887u) >> 15);   /* w / 2fs = pi f / fs, Q16 */
    *g = (65536 << 14) / (65536 + u);
    *p = (65536 - u) * 16384 / (65536 + u);
}
/* 0..127 -> a / b .. 1: a gain in Q15 from v (lin) */
static int32_t dv_lin(uint32_t v, int32_t lo, int32_t hi) { return lo + (int32_t)(((hi - lo) * (int32_t)v) / 127); }

static void dv_setup_own(dv_coef_t *c, const dv_param_t *p)
{
    uint32_t t = p->type < DVT_COUNT ? p->type : 0u, d = p->decay, tn = p->tone, x = p->extra;
    int32_t p16 = clamp(DV_DEF[t].p16 + p->tune, DV_DEF[t].lo, DV_DEF[t].hi), acc = p->accent;
    uint32_t tau = dv_tau(t, d), i;
    for (i = 0; i < sizeof *c / 4u; i++)
        ((uint32_t *)c)[i] = 0;
    c->type = (uint8_t)t;
    c->mrow = t == DVT_CYM;
    c->inc[0] = DV_P16(p16);
    c->metal = (int16_t)p16;
    /* output: the type's gain x LEVEL (127 = 1) x accent (+0..4 dB) */
    c->out = (int32_t)(DV_DEF[t].gain * (uint32_t)p->level / 127u);
    c->out += (c->out * acc * 75) >> 14;                 /* 1 + 0.585 a */
    switch (t) {
    case DVT_PUNCH:
    case DVT_ROUND: {
        int32_t punch = t == DVT_PUNCH, b = punch ? (int32_t)tn * 576 / 127 : (int32_t)tn * 192 / 127;   /* stage 2:
                                                          * +3 / +1 octave at TONE 127 */
        int32_t a = punch ? 384 : 0, g;                  /* stage 1: +2 octaves (p16) */
        c->span[1] = DV_P16(p16 + b) - c->inc[0];
        c->span[0] = DV_P16(p16 + a + b) - DV_P16(p16 + b);
        c->k[0] = dv_k(1500);                            /* attack sweep tau 1.5 ms */
        c->k[1] = dv_k(punch ? 8000u + tn * 32000u / 127u : 30000u + tn * 30000u / 127u);
        c->kb[0] = dv_kb(tau);
        c->hold = punch ? 17 : 0;                        /* blocks: 12 ms */
        c->g[0] = punch ? 2300 : 6500;                   /* 2nd harmonic -23 / -14 dB */
        c->g[1] = punch ? 1300 : 9800;                   /* 3rd -28 / -10 dB (ROUND: above 150 Hz) */
        c->inc[1] = DV_P16(punch ? 1636 : 1523);         /* click: 3 / 2 kHz, two cycles */
        c->inc[2] = c->inc[1] >> 1;
        c->click = (uint16_t)(0xFFFFFFFFu / c->inc[2]);
        c->g[2] = (punch ? 9000 : 5000) + acc * 24;      /* click level */
        g = 4096 + (int32_t)x * 3 * 4096 / 127;          /* DRIVE: x1 .. x4 into the soft clip, Q12 */
        c->g[3] = g;
        c->g[4] = (int32_t)((19661u << 15) / (uint32_t)softclip((19661 * g) >> 12));   /* level kept */
        break;
    }
    case DVT_SNARE:
        c->click = 154;                                  /* the 2nd shell sine: x1.6 */
        c->span[0] = DV_P16(p16 + 11) - c->inc[0];       /* glide: +4 % */
        c->k[0] = dv_k(20000);
        c->k[1] = dv_k(tau * 2u / 5u);                   /* shell: 0.4 x the noise body */
        c->k[2] = dv_k(3000);                            /* noise spike */
        c->kb[0] = dv_kb(tau);
        tsvf_coef_k(&c->f[0], dv_cut(1715 + (int32_t)tn * 160 / 127 - 80), 5120);   /* 2.5 .. 7 kHz, Q 0.8 */
        c->hp = dv_pole(1777);                           /* bright noise above 5 kHz */
        c->g[0] = 19000;                                 /* 2nd shell sine */
        c->g[1] = dv_lin(tn, 8192, 16384) * 3 / 2;       /* shell level */
        c->g[2] = (int32_t)x * 34000 / 127;              /* SNAPPY: noise */
        c->g[3] = 26000 + acc * 50;                      /* spike */
        c->g[4] = 8000;                                  /* bright noise */
        break;
    case DVT_CLAP:
        tsvf_coef_k(&c->f[0], dv_cut(p16), 4096);        /* Q 1 */
        c->hp = dv_pole(1680 + p16 - DV_DEF[t].p16);     /* brightness: noise above 3.5 kHz (with TUNE) */
        c->k[0] = dv_k(5000);                            /* tooth tau */
        c->kb[0] = dv_kb(tau);
        c->hold = (uint16_t)(265u + x * 353u / 127u);    /* SPREAD: 6 .. 14 ms */
        c->click = x > 89u ? 4 : 3;
        c->g[0] = dv_lin(tn, 0, 9000);                   /* TONE: the bright copy */
        c->g[1] = 11000;                                 /* tail */
        c->g[2] = 30000 + acc * 20;                      /* teeth */
        break;
    case DVT_HATC:
    case DVT_HATO: {
        int32_t dp = p16 - DV_DEF[t].p16, cl = t == DVT_HATC, tp;
        tsvf_coef_k(&c->f[0], dv_cut(1874 + dp), 5120);  /* 7.1 kHz with TUNE, Q 0.8 */
        tp = (int32_t)tn * 384 / 127 - 192 + dp;          /* TONE: +-12 semitones on both high-passes */
        dv_hp1(&c->g[2], &c->g[3], (cl ? 2020 : 1934) + tp);   /* 12 / 8.8 kHz */
        dv_hp1(&c->g[4], &c->g[5], 2042 + tp);           /* the sizzle: 13 kHz */
        c->g[1] = (cl ? 7209 : 1147) * (int32_t)x / 64;   /* SIZZLE: the raw cluster, 0 .. 2 x the design (CL 1.76,
                                                          * OP 0.28 x the band, Q12) */
        c->kb[0] = dv_kb(tau);
        c->k[1] = dv_k(300);                             /* the rise: 0.3 ms */
        c->kb[1] = dv_kb(1500);                          /* the choke */
        c->out += (c->out * acc * 34) >> 14;             /* accent: +6 dB in all */
        break;
    }
    case DVT_TOM:
    case DVT_CONGA:
        c->kb[0] = dv_kb(tau);
        c->span[0] = DV_P16(p16 + (int32_t)x * 96 / 127) - c->inc[0];   /* BEND: up to +6 semitones */
        c->k[0] = dv_k(10000);                           /* stick */
        if (t == DVT_TOM) {                              /* the stick: low-passed noise, 300 Hz .. 2.7 kHz */
            c->hp = dv_pole(1302 + (int32_t)tn * 384 / 127 - 192);
            c->g[0] = dv_lin(tn, 1600, 6000);
        } else {                                         /* CONGA: a slap, noise above 2 kHz */
            c->hp = dv_pole(1523);
            c->g[2] = dv_lin(tn, 0, 6000);
        }
        c->g[1] = t == DVT_CONGA ? 6144 : 0;             /* soft clip x1.5 (Q12) */
        break;
    case DVT_RIM:
    case DVT_CLAVE:
        c->k[0] = dv_k(tau);
        c->inc[1] = DV_P16(p16 + 326);                   /* x3.25 */
        c->g[0] = t == DVT_RIM ? dv_lin(tn, 24000, 10000) : dv_lin(tn, 30000, 24000);   /* body */
        c->g[1] = t == DVT_RIM ? dv_lin(tn, 8000, 24000) : dv_lin(tn, 0, 9000);         /* crack */
        c->g[2] = 8192 + (int32_t)x * (t == DVT_RIM ? 24576 : 32768) / 127;   /* DRIVE x2..8 / x2..10, Q12 */
        c->g[2] += (c->g[2] * acc) >> 9;
        c->g[3] = 32767 * 4096 / c->g[2] + 9000;         /* (louder as it clips: a little) */
        break;
    case DVT_BELL:
        c->inc[1] = DV_P16(p16 + 109);                   /* x1.48 */
        tsvf_coef_k(&c->f[0], dv_cut(p16 + 130 + (int32_t)tn * 192 / 127 - 96), 3413);   /* 1.6 f, Q 1.2 */
        c->k[0] = dv_k(4000);
        c->kb[0] = dv_kb(tau);
        c->g[0] = dv_lin(x, 8000, 40000) + acc * 40;     /* STRIKE */
        c->g[1] = dv_lin(tn, 0, 5000);                   /* TONE: the squares themselves (brighter) */
        break;
    case DVT_CYM:
        tsvf_coef_k(&c->f[0], dv_cut(1870 + p16 - DV_DEF[t].p16), 5120);   /* 7 kHz with TUNE, Q 0.8 */
        tsvf_coef_k(&c->f[1], dv_cut(1678 + p16 - DV_DEF[t].p16), 5851);   /* 3.5 kHz, Q 0.7 */
        c->kb[0] = dv_kb(tau);                           /* ring */
        c->kb[1] = dv_kb(150000);                        /* strike / body */
        c->g[0] = dv_lin(tn, 6000, 26000);               /* high band */
        c->g[1] = 32767 - c->g[0];                       /* low band */
        c->g[2] = dv_lin(x, 8000, 32000) + acc * 60;     /* STRIKE: the strike over the ring */
        break;
    }
}

/* x (0..127) of a design value v at 64: v x / 64 */
static int32_t dv_at(int32_t v, uint32_t x) { return v * (int32_t)x / 64; }
/* a drive g (Q12, at least x1) and its level kept (Q15) */
static void dv_drive(int32_t *g, int32_t *keep, int32_t d)
{
    *g = d < 4096 ? 4096 : d;
    *keep = (int32_t)((19661u << 15) / (uint32_t)softclip((19661 * *g) >> 12));
}

/* a model kit's voice: the same runs, the kit's numbers (DV_KIT). TUNE TONE DECAY extra (SNAP) and the accent move
 * them as they move Felucca's own (64: the kit's design) */
static void dv_setup_kit(dv_coef_t *c, const dv_param_t *p)
{
    uint32_t kit = DV_KITOF(p->type) - 1u, d = p->decay, tn = p->tone, x = p->extra, i;
    const dv_kit_t *K = &DV_KIT[kit][p->type & 7u];
    const int16_t *v = K->v;
    int32_t p16, dp, acc = p->accent;
    uint32_t tau;
    p16 = clamp(K->p16 + p->tune, K->p16 - 192, K->p16 + 192);
    dp = p16 - K->p16;
    tau = dv_tau_of(K->tau0, K->oct16, d);
    for (i = 0; i < sizeof *c / 4u; i++)
        ((uint32_t *)c)[i] = 0;
    c->type = K->run;                                    /* (the run; the lane's type is the caller's) */
    c->inc[0] = DV_P16(p16);
    c->metal = (int16_t)p16;
    c->out = (int32_t)(K->gain * (uint32_t)p->level / 127u);
    c->out += (c->out * acc * DV_KIT_ACC[kit]) >> 14;
    switch (K->run) {
    case DVT_PUNCH: {
        int32_t b = dv_at(v[0], tn) + (tn > 64u ? (int32_t)tn - 64 : 0);   /* (TONE above 64: +4 semitones more) */
        c->span[1] = DV_P16(p16 + b) - c->inc[0];
        c->span[0] = DV_P16(p16 + v[2] + b) - DV_P16(p16 + b);
        c->k[0] = dv_k(1500);
        c->k[1] = dv_k((uint32_t)v[1] * 100u);
        c->k[2] = v[8] ? dv_k((uint32_t)v[8] * 100u) : 0;   /* the rise */
        c->kb[0] = dv_kb(tau);
        c->hold = (uint16_t)v[9];
        c->g[0] = v[3];
        c->g[1] = v[4];
        if (v[5]) {
            c->inc[1] = DV_P16(v[5] + dp);
            c->inc[2] = c->inc[1] >> 1;
            c->click = (uint16_t)(0xFFFFFFFFu / c->inc[2]);
            c->g[2] = v[6] + ((v[6] * acc) >> 8);
        }
        dv_drive(&c->g[3], &c->g[4], v[7] * ((int32_t)x + 64) / 128);
        break;
    }
    case DVT_SNARE:
        c->click = (uint16_t)v[0];
        c->span[0] = DV_P16(p16 + v[1]) - c->inc[0];
        c->k[0] = dv_k((uint32_t)v[2] * 100u);
        c->k[1] = dv_k(tau * (uint32_t)v[3] / 64u);
        c->k[2] = dv_k((uint32_t)v[8] * 100u);
        c->kb[0] = dv_kb(tau);
        tsvf_coef_k(&c->f[0], dv_cut(v[6] + ((int32_t)tn - 64) * 2 + dp), v[7]);
        c->hp = dv_pole(v[12] + dp);
        c->g[0] = v[4];
        c->g[1] = v[5];
        c->g[2] = dv_at(v[10], x);
        c->g[3] = v[9] + ((v[9] * acc) >> 8);
        c->g[4] = v[11];
        break;
    case DVT_CLAP:
        tsvf_coef_k(&c->f[0], dv_cut(p16 + ((int32_t)tn - 64) * 2), v[1]);
        c->hp = dv_pole(v[2] + dp);
        c->k[0] = dv_k((uint32_t)v[6] * 100u);
        c->kb[0] = dv_kb(tau);
        c->hold = (uint16_t)(v[7] * (int32_t)(x + 64u) / 128);
        c->click = (uint16_t)v[8];
        c->g[0] = dv_at(v[3], tn);
        c->g[1] = v[4];
        c->g[2] = v[5] + acc * 20;
        break;
    case DVT_HATC:
    case DVT_HATO: {
        int32_t tp = ((int32_t)tn - 64) * 3 + dp;
        tsvf_coef_k(&c->f[0], dv_cut(v[0] + dp + ((int32_t)tn - 64) * 2), v[1]);
        dv_hp1(&c->g[2], &c->g[3], v[2] + tp);
        dv_hp1(&c->g[4], &c->g[5], v[3] + tp);
        c->g[1] = dv_at(v[4], x) + (x > 64u ? ((int32_t)x - 64) * 64 : 0);   /* (SNAP above 64: up to x1 more) */
        c->kb[0] = dv_kb(tau);
        c->k[1] = dv_k((uint32_t)v[7] * 100u);
        c->kb[1] = dv_kb(1500);
        c->span[0] = (uint32_t)v[5];                     /* the metal source, noise (dv_run) */
        c->span[1] = (uint32_t)v[6];
        c->mrow = (uint8_t)v[8];
        break;
    }
    case DVT_CYM:
        tsvf_coef_k(&c->f[0], dv_cut(v[0] + dp), v[1]);
        tsvf_coef_k(&c->f[1], dv_cut(v[2] + dp), v[3]);
        c->kb[0] = dv_kb(tau);
        c->kb[1] = dv_kb((uint32_t)v[6] * 1000u);
        c->g[0] = clamp(dv_at(v[4], tn), 0, 32767);
        c->g[1] = 32767 - c->g[0];
        c->g[2] = dv_at(v[5], x) + acc * 60;
        c->span[0] = (uint32_t)v[7];
        c->span[1] = (uint32_t)v[8];
        c->mrow = (uint8_t)v[9];
        break;
    case DVT_TOM:
    case DVT_CONGA:
        c->kb[0] = dv_kb(tau);
        c->span[0] = DV_P16(p16 + dv_at(v[0], x) + (x > 64u ? (int32_t)x - 64 : 0)) - c->inc[0];
        c->k[0] = dv_k((uint32_t)v[1] * 100u);
        c->hp = dv_pole(v[2] + ((int32_t)tn - 64) * 3);
        c->g[0] = dv_at(v[3], tn);
        c->g[2] = dv_at(v[4], tn);
        c->g[1] = tn > 64u ? (v[5] > 4096 ? v[5] : 4096) + ((int32_t)tn - 64) * 64 : v[5];   /* (TONE above 64:
                                                          * into the soft clip, up to x1 more) */
        break;
    case DVT_RIM:
    case DVT_CLAVE:
        c->k[0] = dv_k(tau);
        c->inc[1] = DV_P16(p16 + v[0]);
        c->g[0] = v[1];
        c->g[1] = dv_at(v[2], tn);
        dv_drive(&c->g[2], &c->g[3], v[3] * ((int32_t)x + 64) / 128 * (512 + acc) / 512);
        break;
    default:                                             /* DVT_BELL */
        c->inc[1] = DV_P16(p16 + v[0]);
        tsvf_coef_k(&c->f[0], dv_cut(p16 + v[1] + ((int32_t)tn - 64) * 2), v[2]);
        c->k[0] = dv_k((uint32_t)v[4] * 100u);
        c->kb[0] = dv_kb(tau);
        c->g[0] = dv_at(v[3], x) * 2 + acc * 40;
        c->g[1] = dv_at(v[5], tn);
        break;
    }
}

static void dv_setup(dv_coef_t *c, const dv_param_t *p)
{
    if (DV_KITOF(p->type) - 1u < DV_NKIT)
        dv_setup_kit(c, p);
    else
        dv_setup_own(c, p);
}

static void dv_metal_tune(dv_metal_t *b, const dv_coef_t *c)
{
    uint32_t i, inc = DV_P16(c->metal);
    const uint16_t *r = DV_METAL_R[c->mrow];
    for (i = 0; i < 6u; i++)
        b->inc[i] = (inc >> 12) * r[i];
}

static __attribute__((noinline)) void dv_metal_run(dv_metal_t *b, int32_t *y, uint32_t n)
{
    uint32_t i, p0 = b->ph[0], p1 = b->ph[1], p2 = b->ph[2], p3 = b->ph[3], p4 = b->ph[4], p5 = b->ph[5];
    for (i = 0; i < n; i++) {
        uint32_t s;
        p0 += b->inc[0];
        p1 += b->inc[1];
        p2 += b->inc[2];
        p3 += b->inc[3];
        p4 += b->inc[4];
        p5 += b->inc[5];
        s = (p0 >> 31) + (p1 >> 31) + (p2 >> 31) + (p3 >> 31) + (p4 >> 31) + (p5 >> 31);
        y[i] = (int32_t)s * 10922 - 32766;               /* six +-1 squares, Q15 */
    }
    b->ph[0] = p0, b->ph[1] = p1, b->ph[2] = p2, b->ph[3] = p3, b->ph[4] = p4, b->ph[5] = p5;
}

/* ----------------------------------------------------------------- the voices --- */
/* SVF band-pass (dsp.c's tsvf, its band output). No clamps: every input here is within +-32768 and Q at most
 * 1.25, so the states stay below about 2 x 2 x 32768 x 1.25 and a1 s below 2^31 */
static inline int32_t dv_bp(const tsvf_t *c, int32_t in, int32_t *s)
{
    int32_t v3 = in - s[1];
    int32_t v1 = (c->a1 * s[0] + c->a2 * v3) >> 13;
    int32_t v2 = s[1] + ((c->a2 * s[0] + c->a3 * v3) >> 13);
    s[0] = 2 * v1 - s[0];
    s[1] = 2 * v2 - s[1];
    return v1;
}
/* a slow envelope's block: the ramp start (Q20) and step per sample */
static inline void dv_slow(int32_t *q, uint32_t kb, int32_t *a, int32_t *d)
{
    *a = *q >> 10;
    *q = mulq16(*q, kb);
    *d = ((*q >> 10) - *a) >> CTL_LOG2;
}
#define DV_NOISE(v) ((int32_t)noise32(&(v)->rng) >> 16)  /* white, Q15 */
#define DV_QUIET (1 << 16)                               /* Q30: -84 dB */

static __attribute__((noinline)) void dv_kick_run(const dv_coef_t *c, dv_voice_t *v, int32_t *y, uint32_t n)
{
    uint32_t i, ph = v->ph[0], cph = v->ph[1], wph = v->ph[2], p1 = v->e[0], p2 = v->e[1], cn = v->n, r = v->e[2];
    uint32_t k2 = v->cnt ? 32768u : c->k[1], s1 = c->span[0] >> 16, s2 = c->span[1] >> 16;
    int32_t a, d;
    dv_slow(&v->q[0], v->cnt ? 65536u : c->kb[0], &a, &d);   /* (held) */
    if (v->cnt)
        v->cnt--;
    for (i = 0; i < n; i++) {
        int32_t b, x;
        ph += c->inc[0] + s1 * p1 + s2 * p2;
        p1 = (p1 * c->k[0]) >> 15;
        p2 = (p2 * k2) >> 15;
        b = sine_i(ph) + ((sine_i(ph << 1) * c->g[0]) >> 15) + ((sine_i(ph * 3u) * c->g[1]) >> 15);
        a += d;
        x = (b * ((a >> 5) - (int32_t)(r >> 1))) >> 15;   /* (r: a rise, the model kits'; 0 on Felucca's own) */
        r = (r * c->k[2]) >> 15;
        if (cn) {                                        /* the click: a windowed burst, mean 0 */
            int32_t w = (32768 - sine_i(wph + 0x40000000u)) >> 1;
            x += (((sine_i(cph) * w) >> 15) * c->g[2]) >> 15;
            cph += c->inc[1];
            wph += c->inc[2];
            cn--;
        }
        y[i] = (softclip((x * c->g[3]) >> 12) * c->g[4]) >> 15;
    }
    v->ph[0] = ph, v->ph[1] = cph, v->ph[2] = wph, v->e[0] = p1, v->e[1] = p2, v->e[2] = r, v->n = (uint16_t)cn;
    v->live = v->q[0] > DV_QUIET;
}

static __attribute__((noinline)) void dv_snare_run(const dv_coef_t *c, dv_voice_t *v, int32_t *y, uint32_t n)
{
    uint32_t i, p0 = v->ph[0], p1 = v->ph[1], eg = v->e[0], es = v->e[1], ek = v->e[2], gs = c->span[0] >> 16;
    int32_t a, d, lp = v->s[2];
    dv_slow(&v->q[0], c->kb[0], &a, &d);
    for (i = 0; i < n; i++) {
        uint32_t inc = c->inc[0] + gs * eg;
        int32_t sh, nz = DV_NOISE(v), bp, hp, x;
        eg = (eg * c->k[0]) >> 15;
        p0 += inc;
        p1 += inc + (inc >> 8) * c->click;               /* x (1 + click / 256) */
        sh = sine_i(p0) + ((sine_i(p1) * c->g[0]) >> 15);
        sh = (sh * (int32_t)(es >> 1)) >> 15;
        es = (es * c->k[1]) >> 15;
        bp = dv_bp(&c->f[0], nz, v->s);
        lp += ((nz - lp) * c->hp) >> 15;
        hp = nz - lp;
        a += d;
        x = (bp * (((((int32_t)(ek >> 1) * c->g[3]) >> 15) + (a >> 5)) >> 1)) >> 14;
        x += (hp * (((int32_t)(ek >> 1) * c->g[4]) >> 15)) >> 15;
        ek = (ek * c->k[2]) >> 15;
        x = ((sh * c->g[1]) >> 15) + ((x * c->g[2]) >> 15);
        y[i] = softclip((x * 5325) >> 12);               /* x1.3 */
    }
    v->ph[0] = p0, v->ph[1] = p1, v->e[0] = eg, v->e[1] = es, v->e[2] = ek, v->s[2] = lp;
    v->live = v->q[0] > DV_QUIET || es > 4u;
}

static __attribute__((noinline)) void dv_clap_run(const dv_coef_t *c, dv_voice_t *v, int32_t *y, uint32_t n)
{
    uint32_t i, et = v->e[0], cnt = v->n;
    int32_t a, d, lp = v->s[2];
    if (v->cnt == 0 && v->q[1] == 0) {                   /* the last tooth struck: the tail starts */
        v->q[0] = 1 << 30;
        v->q[1] = 1;
    }
    dv_slow(&v->q[0], c->kb[0], &a, &d);
    for (i = 0; i < n; i++) {
        int32_t nz = DV_NOISE(v), x, env;
        if (v->cnt && !--cnt) {                          /* the next tooth */
            et = 65536u;
            cnt = c->hold;
            v->cnt--;
        }
        x = dv_bp(&c->f[0], nz, v->s);
        lp += ((nz - lp) * c->hp) >> 15;
        x += ((nz - lp) * c->g[0]) >> 15;
        a += d;
        env = (((int32_t)(et >> 1) * c->g[2]) >> 15) + (((a >> 5) * c->g[1]) >> 15);
        et = (et * c->k[0]) >> 15;
        y[i] = (x * (env >> 1)) >> 14;
    }
    v->e[0] = et, v->n = (uint16_t)cnt, v->s[2] = lp;
    v->live = v->cnt || v->q[0] > DV_QUIET || et > 4u;
}

/* the hats: band-pass -> high-pass, + the sizzle (the raw cluster high-passed), x (decay - rise) */
static __attribute__((noinline)) void dv_hat_run(const dv_coef_t *c, dv_voice_t *v, const int32_t *m, int32_t *y,
                                                 uint32_t n)
{
    uint32_t i, r = v->e[1];
    int32_t a, d, y1 = v->s[2], x1 = v->s[3], z1 = v->s[4], m1 = (int32_t)v->ph[0];
    dv_slow(&v->q[0], v->choke ? c->kb[1] : c->kb[0], &a, &d);
    for (i = 0; i < n; i++) {
        int32_t b = dv_bp(&c->f[0], m[i], v->s), x;
        y1 = (c->g[2] * (b - x1) + c->g[3] * y1) >> 14;
        z1 = (c->g[4] * (m[i] - m1) + c->g[5] * z1) >> 14;
        x1 = b, m1 = m[i];
        x = y1 + ((z1 * c->g[1]) >> 12);
        a += d;
        y[i] = (x * (((a >> 5) - (int32_t)(r >> 1)) >> 2)) >> 13;   /* (x can pass 2^17: squares flipping
                                                          * together through the sizzle) */
        r = (r * c->k[1]) >> 15;
    }
    v->e[1] = r, v->s[2] = y1, v->s[3] = x1, v->s[4] = z1, v->ph[0] = (uint32_t)m1;
    v->live = v->q[0] > DV_QUIET;
}

static __attribute__((noinline)) void dv_tom_run(const dv_coef_t *c, dv_voice_t *v, int32_t *y, uint32_t n)
{
    uint32_t i, ph = v->ph[0], es = v->e[0], bs = c->span[0] >> 16;
    int32_t a, d, lp = v->s[2];
    dv_slow(&v->q[0], c->kb[0], &a, &d);
    for (i = 0; i < n; i++) {
        int32_t al, x, nz;
        a += d;
        al = a >> 5;
        ph += c->inc[0] + bs * (uint32_t)((al * al) >> 14);   /* the pitch follows amplitude^2 (Q16) */
        x = (sine_i(ph) * al) >> 15;
        nz = DV_NOISE(v);
        lp += ((nz - lp) * c->hp) >> 15;                 /* the stick (low), the slap (high) */
        x += ((((lp * c->g[0]) >> 15) + (((nz - lp) * c->g[2]) >> 15)) * (int32_t)(es >> 1)) >> 15;
        es = (es * c->k[0]) >> 15;
        if (c->g[1])
            x = softclip((x * c->g[1]) >> 12);
        y[i] = x;
    }
    v->ph[0] = ph, v->e[0] = es, v->s[2] = lp;
    v->live = v->q[0] > DV_QUIET;
}

static __attribute__((noinline)) void dv_rim_run(const dv_coef_t *c, dv_voice_t *v, int32_t *y, uint32_t n)
{
    uint32_t i, p0 = v->ph[0], p1 = v->ph[1], e = v->e[0];
    for (i = 0; i < n; i++) {
        int32_t x;
        p0 += c->inc[0];
        p1 += c->inc[1];
        x = ((sine_i(p0) * c->g[0]) >> 15) + ((sine_i(p1) * c->g[1]) >> 15);
        x = (x * (int32_t)(e >> 1)) >> 15;
        e = (e * c->k[0]) >> 15;
        y[i] = (softclip((x * c->g[2]) >> 12) * c->g[3]) >> 15;
    }
    v->ph[0] = p0, v->ph[1] = p1, v->e[0] = e;
    v->live = e > 4u;
}

static __attribute__((noinline)) void dv_bell_run(const dv_coef_t *c, dv_voice_t *v, int32_t *y, uint32_t n)
{
    uint32_t i, p0 = v->ph[0], p1 = v->ph[1], e = v->e[0];
    int32_t a, d;
    dv_slow(&v->q[0], c->kb[0], &a, &d);
    for (i = 0; i < n; i++) {
        int32_t x;
        p0 += c->inc[0];
        p1 += c->inc[1];
        x = (int32_t)((p0 >> 31) + (p1 >> 31)) * 16384 - 16384;   /* two squares, +-16384 each */
        x = dv_bp(&c->f[0], x, v->s) + ((x * c->g[1]) >> 15);
        a += d;
        y[i] = (x * (((a >> 5) + (((int32_t)(e >> 1) * c->g[0]) >> 15)) >> 2)) >> 13;
        e = (e * c->k[0]) >> 15;
    }
    v->ph[0] = p0, v->ph[1] = p1, v->e[0] = e;
    v->live = v->q[0] > DV_QUIET;
}

static __attribute__((noinline)) void dv_cym_run(const dv_coef_t *c, dv_voice_t *v, const int32_t *m, int32_t *y,
                                                 uint32_t n)
{
    uint32_t i;
    int32_t a, d, b, e;
    dv_slow(&v->q[0], c->kb[0], &a, &d);
    dv_slow(&v->q[1], c->kb[1], &b, &e);
    for (i = 0; i < n; i++) {
        int32_t hi = dv_bp(&c->f[0], m[i], v->s), lo = dv_bp(&c->f[1], m[i], v->s + 2), r, s;
        a += d;
        b += e;
        r = a >> 5;
        s = (b >> 5) * c->g[2] >> 15;
        y[i] = ((((hi * ((r + s) >> 1)) >> 15) * c->g[0]) >> 15) + ((((lo * s) >> 15) * c->g[1]) >> 15);
    }
    v->live = v->q[0] > DV_QUIET;
}

/* a model kit's metal: the source (c->span[0], Q15) and white noise (c->span[1]) mixed into y */
static __attribute__((noinline)) void dv_metal_mix(const dv_coef_t *c, dv_voice_t *v, const int32_t *m, int32_t *y,
                                                   uint32_t n)
{
    uint32_t i;
    int32_t gm = (int32_t)c->span[0], gn = (int32_t)c->span[1];
    for (i = 0; i < n; i++)
        y[i] = (m[i] * gm + DV_NOISE(v) * gn) >> 15;
}

/* ------------------------------------------------------------------- the API --- */
static void dv_init(dv_voice_t *v, uint32_t type)
{
    uint32_t i;
    for (i = 0; i < sizeof *v / 4u; i++)
        ((uint32_t *)v)[i] = 0;
    v->type = (uint8_t)type;
}

static void dv_trigger(dv_voice_t *v) { v->trig = 1; }
static void dv_choke(dv_voice_t *v) { v->choke = 1; }
static uint32_t dv_uses_metal(uint32_t type) { return type == DVT_HATC || type == DVT_HATO || type == DVT_CYM; }

/* a hit: every state from rest (phases, filters, noise seed), so that each hit renders the same */
static void dv_strike(const dv_coef_t *c, dv_voice_t *v)
{
    dv_init(v, c->type);
    v->live = 1;
    v->rng = DV_SEED + c->type;
    v->e[0] = v->e[1] = v->e[2] = 65536u;
    v->q[0] = 1 << 30;
    switch (c->type) {
    case DVT_PUNCH:
    case DVT_ROUND:
        v->cnt = c->hold;
        v->n = c->click;
        v->e[2] = c->k[2] ? 65536u : 0;                  /* the rise, if any */
        break;
    case DVT_CLAP:
        v->q[0] = 0;
        v->cnt = (uint16_t)(c->click - 1u);              /* teeth after the first */
        v->n = c->hold;
        break;
    case DVT_CYM:
        v->q[1] = 1 << 30;
        break;
    }
}

/* the voice at its level (c->out), Q15, under full scale: linear to -6 dBFS, a soft knee above */
static __attribute__((noinline)) void dv_out(const dv_coef_t *c, int32_t *y, uint32_t n)
{
    uint32_t i;
    for (i = 0; i < n; i++) {
        int32_t s = (y[i] * c->out) >> 12, a = s < 0 ? -s : s;
        if (a > 16384) {
            a = 16384 + (softclip((a - 16384) * 2) >> 1);   /* (at most 32179) */
            s = s < 0 ? -a : a;
        }
        y[i] = s;
    }
}

/* one block of the voice into y (Q15 at its level); metal: the metal source's block, or 0 */
static void dv_run(const dv_coef_t *c, dv_voice_t *v, const int32_t *metal, int32_t *y, uint32_t n)
{
    uint32_t i;
    if (v->trig)
        dv_strike(c, v);
    if (!v->live || v->type != c->type || (dv_uses_metal(c->type) && !metal)) {
        for (i = 0; i < n; i++)
            y[i] = 0;
        v->live = 0;
        return;
    }
    if (dv_uses_metal(c->type) && c->span[1]) {          /* a model kit's metal: the source and white noise */
        dv_metal_mix(c, v, metal, y, n);
        metal = y;                                       /* (the runs read a sample before they write it) */
    }
    switch (c->type) {
    case DVT_PUNCH:
    case DVT_ROUND:
        dv_kick_run(c, v, y, n);
        break;
    case DVT_SNARE:
        dv_snare_run(c, v, y, n);
        break;
    case DVT_CLAP:
        dv_clap_run(c, v, y, n);
        break;
    case DVT_HATC:
    case DVT_HATO:
        dv_hat_run(c, v, metal, y, n);
        break;
    case DVT_TOM:
    case DVT_CONGA:
        dv_tom_run(c, v, y, n);
        break;
    case DVT_RIM:
    case DVT_CLAVE:
        dv_rim_run(c, v, y, n);
        break;
    case DVT_BELL:
        dv_bell_run(c, v, y, n);
        break;
    default:
        dv_cym_run(c, v, metal, y, n);
        break;
    }
    dv_out(c, y, n);
}

/* the run a type plays through (DVT_*): itself, or its model kit's lane's */
static uint32_t dv_run_type(uint32_t type)
{
    return DV_KITOF(type) - 1u < DV_NKIT ? DV_KIT[DV_KITOF(type) - 1u][type & 7u].run : type < DVT_COUNT ? type : 0u;
}

/* the designed parameters of a type (LEVEL 100, no accent); a model kit's lane: 64 each */
static void dv_default(dv_param_t *p, uint32_t type)
{
    uint32_t own = DV_KITOF(type) == 0;
    p->type = (uint8_t)type;
    p->decay = own ? DV_DEF[type].def[0] : 64;
    p->tone = own ? DV_DEF[type].def[1] : 64;
    p->extra = own ? DV_DEF[type].def[2] : 64;
    p->level = 100;
    p->accent = 0;
    p->tune = 0;
}
