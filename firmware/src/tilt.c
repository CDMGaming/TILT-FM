/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
 * Copyright (C) 2026 CDM Gaming (TILT-FM) */
/* TILT-FM: TILT's quick-start ideas inside Felucca's UI. Included at the end of ui_layer.c (every build that has the
 * layers has this: the firmware, the browser emulator, the host tests).
 * The TILT layer: ENV held (its tap is still ENV's page), a SET layer as SEQ TOOLS: OCT- puts back the selected
 * track's sequence as the layer opened, SAVE held undoes the last action. The white keys from F3:
 *   G3 STUCK   the selected track: COOK if it holds something (a NOTE step, a drum hit), else RANDOM: the SEQ TOOLS
 *              actions themselves (ui_tools.c), in the track's scale (ROOT, SCL), one undo each */
enum { TT_NEW, TT_STUCK, TT_TIMER, TT_N };              /* the white keys from F3 (key_place) */

/* the selected track holds something to cook (as tl_do counts it) */
static uint32_t tilt_used(const track_t *t)
{
    uint32_t len = tl_len(t), i, n = 0;
    int drum = drum_track(t);
    for (i = 0; i < len; i++)
        n += drum ? (uint32_t)__builtin_popcount(step_lanes(&t->step[i])) : (uint32_t)tl_gate(&t->step[i]);
    return n;
}

/* STUCK: COOK what is there, or RANDOM where there is nothing (tools_key: STOP TO EDIT, the undo, the message) */
static void tilt_stuck(void) { tools_key(tilt_used(TSEL) ? TL_COOK : TL_RANDOM); }

static void tilt_key(uint32_t k, uint32_t p)
{
    if (key_black(k))
        return;
    if (p == TT_STUCK)
        tilt_stuck();
}
static void tilt_knob(uint32_t k, int32_t s) { (void)k; (void)s; }
static uint32_t tilt_can(uint32_t k, uint32_t p) { return !key_black(k) && p == TT_STUCK && !chain_busy(); }
static uint32_t tilt_on(uint32_t k, uint32_t p) { (void)k; (void)p; return 0; }
static uint32_t tilt_sig(void) { return (tilt_used(TSEL) != 0u) * 31u + (uint32_t)chain_busy() * 3u + song.sel * 977u; }

static void tilt_cards(void)
{
    uint32_t c;
    for (c = 0; c < 4u; c++)
        draw_column(c, "", "", "", T_THEME, -1, ICON_NONE);
}

/* the map: G3 STUCK, and under it what it will do to the selected track */
static void layer_tilt(void)
{
    uint32_t st = chain_busy() ? LS_DIM : LS_OFF;
    char n[3] = {'G', 0, 0}, b[24];
    uint16_t ink, fill;
    lcell(LC_X(TT_STUCK), 4, LC_H, n, ICON_PROB, 0, "STUCK", st, 0);
    b[0] = 'T';
    b[1] = (char)('1' + song.sel);
    str_cpy(b + 2, chain_busy() ? ": STOP TO EDIT" : tilt_used(TSEL) ? ": COOK IT" : ": RANDOM", sizeof b - 2);
    fill = lc_box(6, 100, 228, 18, lc_fill(st, &ink));
    GFX_HOOK_ALIGN(6, 100, 234, 118, AL_HV, "tilt layer stuck line centred");
    cv_text_in(6, 100 + CAP_IN(S, 18), 228, &AF_S, b, ink, fill);
}
