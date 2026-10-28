/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 CDM Gaming (TILT-FM) */
/* TFM: the dice. Ported from TILT (github.com/CDMGaming/tilt, src/engine/rng.ts, dice.ts, daily.ts; CDM Gaming's own,
 * relicensed GPL-3.0 here) so a TILT setup code and a TFM one are the same thing:
 *   tfm_rng      TILT's seeded generator (mulberry32), bit for bit: the same seed, the same numbers
 *   tfm_roll     TILT's roll of the key, the scale, the tempo and the swing (its TEMPO_BANDS, SCALE_WEIGHTS), in integer
 *                arithmetic that picks exactly what TILT's floating point does (u / 2^32 < x <=> u < x * 2^32)
 *   tfm_encode / tfm_decode   TILT's share codes (dice.ts encodeSetup / decodeSetup: format 1, one byte a slot; format 2,
 *                an instrument byte and a preset byte, or a generated sound's two-byte seed), Crockford base32 in groups
 *                of four. A code carries everything (key, scale, tempo, swing, the lineup): entering one gives what it
 *                gave in TILT, whatever either side's generator does next. TILT's lineup slots map onto TFM's four tracks
 *                in order (a drum instrument keeps its drum grid wherever it lands: the grid is the engine's); more than
 *                four: the first four; fewer: the rest rolled from the code itself (the same code, the same fill)
 * TILT's instruments map onto TFM's (tfm_instr.c): each TFM instrument is one TILT kind; a TILT kind with no TFM
 * instrument plays the closest one of its role (TILT_TO_TFM). A preset number is the instrument's own list, wrapped. */

/* ------------------------------------------------------------ the generator --- */
typedef struct { uint32_t s; } tfm_rng_t;
static uint32_t tfm_next(tfm_rng_t *r)                  /* TILT Rng.next() * 2^32, exactly */
{
    uint32_t t = (r->s = r->s + 0x6d2b79f5u);
    t = (t ^ (t >> 15)) * (t | 1u);
    t ^= t + (t ^ (t >> 7)) * (t | 61u);
    return t ^ (t >> 14);
}
static int32_t tfm_int(tfm_rng_t *r, int32_t lo, int32_t hi)   /* Rng.int(lo, hi): lo + floor(next * (hi - lo + 1)) */
{
    return lo + (int32_t)(((uint64_t)tfm_next(r) * (uint32_t)(hi - lo + 1)) >> 32);
}
static uint32_t tfm_pick(tfm_rng_t *r, uint32_t n)      /* Rng.pick: floor(next * n) */
{
    return (uint32_t)(((uint64_t)tfm_next(r) * n) >> 32);
}
static uint32_t tfm_weighted(tfm_rng_t *r, const uint8_t *w, uint32_t n)   /* Rng.weighted, integer weights */
{
    uint32_t i, total = 0, cum = 0;
    uint64_t x;
    for (i = 0; i < n; i++)
        total += w[i];
    x = (uint64_t)tfm_next(r) * total;                  /* r = next * total; the first whose running sum passes it */
    for (i = 0; i < n; i++) {
        cum += w[i];
        if (x < (uint64_t)cum << 32)
            return i;
    }
    return n - 1u;
}
static int tfm_chance(tfm_rng_t *r, uint32_t p32)       /* Rng.chance(p): next < p; p32 = ceil(p * 2^32) */
{
    return tfm_next(r) < p32;
}

/* ------------------------------------------------------------ TILT's tables --- */
/* music.ts SCALES, in TILT's order (the code's 4-bit scale number) -> params.c N_SCALE */
static const uint8_t TILT_SCALE[11] = {1, 2, 3, 8, 9, 4, 7, 6, 5, 12, 0};   /* major minor dorian phrygian lydian
                                                         * mixolydian harmminor minpenta majpenta blues chromatic */
/* dice.ts SCALE_WEIGHTS: minor 6, major 4, dorian 3, minpenta 3, majpenta 2, phrygian 2, mixolydian 2, harmminor 2,
 * lydian 1, blues 2 (as TILT scale numbers) */
static const uint8_t TILT_SW_SCALE[10] = {1, 0, 2, 7, 8, 3, 5, 6, 4, 9};
static const uint8_t TILT_SW_W[10] = {6, 4, 3, 3, 2, 2, 2, 2, 1, 2};
/* dice.ts TEMPO_BANDS: [lo, hi, swing chance], weight */
static const struct { uint8_t lo, hi; uint32_t swing32; } TILT_BAND[5] = {
    {70, 95, 2576980378u},                              /* 0.6 */
    {96, 118, 1288490189u},                             /* 0.3 */
    {119, 132, 858993460u},                             /* 0.2 */
    {133, 150, 644245095u},                             /* 0.15 */
    {151, 175, 429496730u},                             /* 0.1 */
};
static const uint8_t TILT_BAND_W[5] = {2, 3, 4, 2, 1};
static const uint8_t TILT_SWINGS[5] = {15, 25, 35, 45, 55};
/* dice.ts KIND_CODE (append-only in TILT): drums acid poly lead analog wobble pad chip fm wave sampler epiano pluck
 * organ anthem patch vox boom piano garage chop guitar -> the TFM instrument that plays it (tfm_instr.c TI_*) */
#define TILT_NKIND 22u

/* a setup: what NEW SONG holds */
typedef struct {
    uint8_t key, scale, swing;                          /* scale: N_SCALE; swing: G_SWING (0..100, TILT's) */
    int16_t bpm;
    uint8_t ins[NTRK], pre[NTRK];                       /* each track's instrument (tfm_instr.c) and its sound there */
} tfm_setup_t;

/* TILT's roll of the key, scale, tempo and swing (dice.ts roll / nudgeSetup: tempo band, tempo, swing, key, scale) */
static void tfm_roll_feel(tfm_rng_t *r, tfm_setup_t *s, uint32_t keep)   /* keep: bits 1 key, 2 scale, 4 tempo, 8 swing */
{
    uint32_t b = tfm_weighted(r, TILT_BAND_W, 5);
    int32_t bpm = tfm_int(r, TILT_BAND[b].lo, TILT_BAND[b].hi);
    uint32_t swing = tfm_chance(r, TILT_BAND[b].swing32) ? TILT_SWINGS[tfm_pick(r, 5)] : 0u;
    uint32_t key = (uint32_t)tfm_int(r, 0, 11);
    uint32_t sc = TILT_SCALE[TILT_SW_SCALE[tfm_weighted(r, TILT_SW_W, 10)]];
    if (!(keep & 4u)) s->bpm = (int16_t)bpm;
    if (!(keep & 8u)) s->swing = (uint8_t)swing;
    if (!(keep & 1u)) s->key = (uint8_t)key;
    if (!(keep & 2u)) s->scale = (uint8_t)sc;
}

