/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* A song uses the sequences of four saved projects, with the current sounds.
 * Sources are copied in the main loop before PLAY. The ISR changes only their
 * index and five timing parameters; the editable steps stay untouched.
 *
 * Lanes (1.2): a section (a row) names the slot each track plays (core.h chain_row_t). Each track reads its steps (with
 * CHANCE, RATCH, NUDGE), its automation and locks, LEN / DIV / SWING / GATE and QUANTIZE from its own slot's copy for
 * that track only; all four slots in use are copied before PLAY (chain_prepare: the four sources were there already
 * for whole rows, so lanes cost no more RAM). Sections keep one boundary for all four tracks: a section lasts `repeat`
 * loops of its clock track (chain_clock): the track not silent there with the longest loop (LEN x DIV; the lowest of
 * equal ones. TILT-FM: up to Felucca 1.6.1 the first track not silent, so a 1-bar loop on T1 cut a 2-bar line on T2 in
 * half), or, every track silent, a 4/4 bar (CHAIN_SIL_*). At each section all four tracks
 * start together from their step 1, as rows always did.
 * A silent track ("-", CHAIN_SILENT) plays nothing in that section: at its start the track's sequenced notes are
 * released (seq_release, as at every section) and its automation goes back to the sound's own values (motion_restore);
 * no step, automation or lock of any slot plays there. Live keys and the arp still play it (the sound stays). */
typedef struct {
    step_t step[NTRK][NSTEP];
    int16_t timing[NTRK][4];
    uint8_t qnt[NTRK];                       /* each track's QUANTIZE (P_SQNT) as saved */
    motion_store_t motion;
} chain_pattern_t;
static chain_config_t chain_config;
static struct {
    chain_pattern_t source[4];
    chain_config_t config;
    int16_t timing[NTRK][4];                 /* the editable LEN DIV SWING GATE and QUANTIZE, back on STOP */
    uint8_t qnt[NTRK];
    volatile uint8_t armed, running, row, remaining;
    uint8_t lane[NTRK];                      /* the slot each track plays in this section, CHAIN_SILENT none */
    uint8_t clk;                             /* the track whose loop counts the section (chain_clock) */
    uint8_t rec;
    uint32_t carry;
} chain;
/* a silent lane: its steps (empty: nothing plays) and its timing (a 4/4 bar of 1/16: the section's length when every
 * track is silent) */
static const step_t chain_silent[NSTEP] = {{{0}}};
#define CHAIN_SIL_LEN 16
#define CHAIN_SIL_DIV 2                      /* 1/16 */

static void seq_release(track_t *t);
static void motion_rebase(track_t *t);
/* TILT-FM: SOUNDS SCENE (the TILT layer's B3; ui.c PREF_SONG_SCENE, a settings bit: off, CURRENT, every older song).
 * chain_prepare copies each slot the song uses its four tracks' sounds (their values, engine, preset, FM6 patch) and
 * the sounds as PLAY finds them; at each section a track takes its scene's sound where it differs from the one
 * playing (chain_sound_put: the values but the ones the song sets, LEN DIV SWING GATE QUANTIZE, and MUTE, which stays
 * the performer's; the engine asked for, voice.c fading the old one out; the FM6 patch), and STOP puts the sounds of
 * before back. CURRENT: as Felucca, every section with the sounds as they are */
typedef struct {
    int16_t p[NTRK][P_COUNT];
    uint8_t eng[NTRK], preset[NTRK];
    uint8_t fm6[NTRK][FP_SIZE + 1u];
} chain_sound_t;
static struct {
    chain_sound_t src[4], live;
    uint8_t on;
} chain_snd __attribute__((section(".pool")));
static int chain_owns(uint32_t id);
static void chain_sound_put(track_t *t, const chain_sound_t *c, uint32_t k)
{
    uint32_t i, d = c->eng[k] != t->eng_req || memcmp(fm6_patch[k], c->fm6[k], sizeof fm6_patch[k]);
    for (i = 0; i < P_COUNT && !d; i++)
        d = !chain_owns(i) && i != P_MUTE && t->p[i] != c->p[k][i];
    if (!d)
        return;
    for (i = 0; i < P_COUNT; i++)
        if (!chain_owns(i) && i != P_MUTE)
            t->p[i] = c->p[k][i];
    t->eng_req = c->eng[k];
    t->preset = c->preset[k];
    if (memcmp(fm6_patch[k], c->fm6[k], sizeof fm6_patch[k])) {
        memcpy(fm6_patch[k], c->fm6[k], sizeof fm6_patch[k]);
        fm6_pgen[k]++;
    }
    motion_rebase(t);
}
static void seq_stop(void);
static uint32_t div_samples(uint32_t div);
static uint32_t step_samples(const track_t *t, uint32_t period, uint32_t idx);

static chain_row_t chain_row_of(uint32_t slot, uint32_t repeat)   /* a section with all four tracks on one slot */
{
    chain_row_t r;
    memset(r.slot, (int)slot, sizeof r.slot);
    r.repeat = (uint8_t)repeat;
    return r;
}
static void chain_defaults(chain_config_t *c)
{
    uint32_t i;
    memset(c, 0, sizeof *c);
    for (i = 0; i < CHAIN_ROWS; i++)
        c->row[i].repeat = 1;
}
static int chain_valid(const chain_config_t *c)
{
    uint32_t i, k;
    if (c->count > CHAIN_ROWS)
        return 0;
    for (i = 0; i < c->count; i++) {
        for (k = 0; k < NTRK; k++)
            if (c->row[i].slot[k] > CHAIN_SILENT)
                return 0;
        if (!c->row[i].repeat || c->row[i].repeat > 16u)
            return 0;
    }
    return 1;
}
/* the chain of FUN6..FUN9 (rows {slot, repeat}) -> today's: each row a section with the four tracks on its slot. 0: not
 * a valid one (as chain_valid then) */
static int chain_from_v9(chain_config_t *c, const chain_v9_t *o)
{
    uint32_t i;
    chain_defaults(c);
    if (o->count > CHAIN_ROWS)
        return 0;
    for (i = 0; i < o->count; i++) {
        if (o->row[i].slot >= 4u || !o->row[i].repeat || o->row[i].repeat > 16u)
            return 0;
        c->row[i] = chain_row_of(o->row[i].slot, o->row[i].repeat);
    }
    c->count = o->count;
    return 1;
}
/* LEN steps of DIV in 1/48 quarters (a 4/4 bar: 192), clamped as chain_prepare does (project.c project_info's too) */
static uint32_t len48_of(int32_t len, int32_t div)
{
    uint32_t n = (uint32_t)clamp(len, TP[P_SLEN].min, TP[P_SLEN].max), d = (uint32_t)clamp(div, TP[P_SDIV].min, TP[P_SDIV].max);
    return n * (d < 6u ? 48u / DIV_DEN[d] : d < 10u ? 48u << (d - 5u) : 48u / DIV_DEN[d % 6u]);   /* (fx.c div_samples) */
}
/* the track whose loops count section r's length (TILT-FM): of the tracks not silent there, the one whose loop is the
 * longest, len48[k] (its LEN x DIV in 1/48 quarters), the lowest of equal ones; every track silent: track 1 (a bar) */
static uint32_t chain_clock_by(const chain_row_t *r, const uint32_t *len48)
{
    uint32_t k, c = NTRK;
    for (k = 0; k < NTRK; k++)
        if (r->slot[k] < CHAIN_SILENT && (c == NTRK || len48[k] > len48[c]))
            c = k;
    return c < NTRK ? c : 0u;
}
/* .. as a song plays it: each lane's LEN and DIV in its slot's copy (chain_prepare; once a section, chain_apply) */
static uint32_t chain_clock(const chain_row_t *r)
{
    uint32_t k, len48[NTRK];
    for (k = 0; k < NTRK; k++)
        len48[k] = r->slot[k] < 4u ? len48_of(chain.source[r->slot[k]].timing[k][0], chain.source[r->slot[k]].timing[k][1])
                                   : 0u;
    return chain_clock_by(r, len48);
}
/* the first track not silent in r (every track silent: track 1): the one slot a row of before lanes names (editor v6) */
static uint32_t chain_lead(const chain_row_t *r)
{
    uint32_t k;
    for (k = 0; k < NTRK; k++)
        if (r->slot[k] < CHAIN_SILENT)
            return k;
    return 0;
}
/* a track parameter a playing song sets from its slots (the editor and the pages leave it alone meanwhile) */
static int chain_owns(uint32_t id) { return (id >= P_SLEN && id <= P_SGATE) || id == P_SQNT; }
static const step_t *seq_steps(const track_t *t)
{
    uint32_t k = (uint32_t)(t - trk), s;
    if (!chain.running)
        return t->step;
    s = chain.lane[k];
    return s < 4u ? chain.source[s].step[k] : chain_silent;
}
static void motion_restore(track_t *t);
/* a section starts (chain_start, chain_tick: once a section, not per block; not inlined: off the audio ISR's loop) */
static __attribute__((noinline)) void chain_apply(void)
{
    uint32_t i;
    const chain_row_t *r = &chain.config.row[chain.row];
    chain.clk = (uint8_t)chain_clock(r);
    for (i = 0; i < NTRK; i++) {
        track_t *t = &trk[i];
        uint32_t s = r->slot[i];
        chain.lane[i] = (uint8_t)(s < 4u ? s : CHAIN_SILENT);
        seq_release(t);
        motion_restore(t);
        if (s < 4u && chain_snd.on)
            chain_sound_put(t, &chain_snd.src[s], i);   /* (TILT-FM: SOUNDS SCENE) */
        if (s < 4u) {
            memcpy(&t->p[P_SLEN], chain.source[s].timing[i], sizeof chain.timing[i]);
            t->p[P_SQNT] = chain.source[s].qnt[i];
        } else {
            t->p[P_SLEN] = CHAIN_SIL_LEN;
            t->p[P_SDIV] = CHAIN_SIL_DIV;
            t->p[P_SSWING] = 0;
            t->p[P_SQNT] = 1;
        }
        t->seq_idx = (uint16_t)(t->p[P_SLEN] - 1);
        t->seq_pos = 0x7FFFFFFFu;
        t->rh_n = t->rskip_n = 0;
    }
}
static void chain_start(void)
{
    uint32_t i;
    if (!chain.armed)
        return;
    chain.rec = song.rec;
    song.rec = 0;
    for (i = 0; i < NTRK; i++) {
        memcpy(chain.timing[i], &trk[i].p[P_SLEN], sizeof chain.timing[i]);
        chain.qnt[i] = (uint8_t)trk[i].p[P_SQNT];
    }
    chain.row = 0;
    chain.remaining = chain.config.row[0].repeat;
    chain.carry = 0;
    chain.running = 1;
    chain.armed = 0;
    chain_apply();
}
static void chain_stop(void)
{
    uint32_t i;
    chain.armed = 0;
    if (!chain.running)
        return;
    for (i = 0; i < NTRK; i++) {
        if (chain_snd.on) {                             /* (TILT-FM: SOUNDS SCENE: the sounds of before PLAY) */
            motion_restore(&trk[i]);
            chain_sound_put(&trk[i], &chain_snd.live, i);
        }
        memcpy(&trk[i].p[P_SLEN], chain.timing[i], sizeof chain.timing[i]);
        trk[i].p[P_SQNT] = chain.qnt[i];
    }
    chain_snd.on = 0;
    song.rec = chain.rec;
    chain.running = 0;
}
static void chain_tick(uint32_t n)
{
    const track_t *t = &trk[chain.clk % NTRK];
    uint32_t length;
    if (!chain.running || t->seq_pos >= 0x7FFFFFFFu || t->seq_idx + 1u != (uint32_t)t->p[P_SLEN])
        return;
    length = step_samples(t, div_samples((uint32_t)t->p[P_SDIV]), t->seq_idx);
    if (t->seq_pos + n < length)
        return;
    if (chain.remaining > 1u) {
        chain.remaining--;
        return;
    }
    if (chain.row + 1u >= chain.config.count) {
        seq_stop();
        return;
    }
    chain.carry = t->seq_pos + n - length;
    chain.row++;
    chain.remaining = chain.config.row[chain.row].repeat;
    chain_apply();
}
