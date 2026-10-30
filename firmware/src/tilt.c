/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
 * Copyright (C) 2026 CDM Gaming (TILT-FM) */
/* TILT-FM: TILT's quick-start ideas inside Felucca's UI. Included at the end of ui_layer.c (every build that has the
 * layers has this: the firmware, the browser emulator, the host tests).
 * The TILT layer: ENV held (its tap is still ENV's page), a SET layer as SEQ TOOLS: OCT- puts back the selected
 * track's sequence as the layer opened, SAVE held undoes the last action. The white keys from F3:
 *   F3 NEW SONG  the NEW SONG screen (below)
 *   G3 STUCK     the selected track: COOK if it holds something (a NOTE step, a drum hit), else RANDOM: the SEQ TOOLS
 *                actions themselves (ui_tools.c), in the track's scale (ROOT, SCL), one undo each
 *   A3 TIMER     (below)
 * The song's scenes have a layer of their own, ARP held (SCENE LAYER, below) */
enum { TT_NEW, TT_STUCK, TT_TIMER, TT_N };              /* the white keys from F3 (key_place) */
static uint8_t tilt_timer_min;                          /* the TIMER: 0 off, else its minutes */

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

/* ------------------------------------------------------------ NEW SONG --- */
/* A screen of its own (ui.c tilt_ns_open: name_on() with it), TFM's dice (tfm_rng.c, tfm_instr.c, tfm_code.c: TILT's
 * roll and setup codes): KEY, SCALE, TEMPO (and swing), LINEUP, the four tracks' instruments and sounds, CODE; a row
 * each, F3 .. A4 the rows' keys:
 *   a white key F3 .. A4   HOLD its row (lit: held) and select it; again: let go
 *   SELECT                 the row; KNOB 1 its value (TEMPO: KNOB 2 the swing; a track: KNOB 1 every instrument's
 *                          sounds, KNOB 2 the instrument; CODE: KNOB 1 the character, KNOB 2 the place, KNOB 3 the length)
 *   OCT+  NUDGE            every row not held rolled again: the key, scale, tempo and swing as TILT rolls them (its
 *                          tempo bands, scale weights, generator), the tracks by the LINEUP:
 *                            BAND      T1 a bass, T2 harmony, T3 a lead (or harmony), T4 drums (TILT's groups)
 *                            WILD      any instrument on any track, the wildcards (VOX, WHEELHOUSE) too; the same one
 *                                      twice is allowed, never with the same sound
 *                            BAND + 1  BAND with one track (not held) wild
 *                          a drum instrument keeps its drum grid wherever it lands (the grid is the DRUM engine's)
 *   PLAY  START            the project made from it: each track's instrument and sound (tfm_load: as PRESETS loads a
 *                          sound, one undo a track), every track's ROOT and SCL, the tempo and swing; the melodic
 *                          tracks' QNT WHITE (the white keys walk the scale from C4, the root; SCALE LEDS shows them),
 *                          a kit's keys its own; the transport starts. With the TIMER set the steps and the automation
 *                          of every track go too (a fresh project: PLAY twice when there was something to clear)
 *   OCT-                   back, nothing changed
 * CODE is the setup as TILT's share code (both ways: what TILT rolls, entered here, is the same song's start; what TFM
 * rolls opens in TILT with the same key, scale, tempo, swing and instruments). The jam of the day: TILT's daily code,
 * entered. The first open rolls everything; the values and the holds stay between opens, and each open is a NUDGE */
enum { NS_KEY, NS_SCALE, NS_BPM, NS_LINEUP, NS_T1, NS_CODE = NS_T1 + NTRK, NS_ROWS };
#define TILT_NS 1u                                      /* tilt_ns_open: NEW SONG; TIME (below) */
#define TILT_TIME 2u
#define NS_ROW_H 19                                     /* 9 rows from Y_LABEL: 28 .. 199, the footer at 202 */
#define NS_AGAIN_MS 3000u
#include "tfm_rng.c"                                    /* TFM: TILT's generator, its roll's tables, a setup */
#include "tfm_instr.c"                                  /* TFM: the instruments over the engines, their macros */
#include "tfm_code.c"                                   /* TFM: TILT's setup codes */
static tfm_setup_t tfm_jam_s;                           /* TODAY'S JAM: the setup TILT's code of the day gave (the editor's "Send today's jam") */
static uint8_t tfm_jam_has;                             /* (a jam waits on the start screen) */
static int tfm_jam_accept(const uint8_t *a, uint32_t n) /* a code (ASCII, from the editor): decoded, kept, START offers it */
{
    char code[TFM_CODE_MAX + 1u];
    tfm_setup_t sj;
    uint32_t i;
    for (i = 0; i < n && i < TFM_CODE_MAX && a[i]; i++)
        code[i] = (char)a[i];
    code[i] = 0;
    if (!tfm_decode(code, &sj))
        return 0;
    tfm_jam_s = sj;
    tfm_jam_has = 1;
    ui.force = 1;
    return 1;
}
static int (*tfm_jam_hook)(const uint8_t *a, uint32_t n) = tfm_jam_accept;
#include "tfm_jam.c"                                     /* TFM: HOME > JAM */
#include "tfm_sheet.c"                                   /* TFM: the STEP SHEET (OCT+ held on STEP) */
/* (tfm_ui.c: TFM-UI, included at the end of this file; tilt.c's own forward declarations of what it uses) */
static struct {
    uint8_t row, rolled, lineup;                        /* the selected row; (rolled: once, the first open); LU_* */
    uint16_t hold;                                      /* bit r: row r held (NUDGE leaves it) */
    tfm_setup_t s;
    uint8_t again;                                      /* START pressed once with steps to clear (timed) */
    uint8_t cpos, cedit, cbad;                          /* CODE: the place (a symbol), being edited, not a code */
    char code[TFM_CODE_MAX + 1u];
    uint32_t again_t, sig;
} ns;
static uint8_t ns_from_start;                           /* (opened from the start screen: OCT- goes back there) */
/* TFM-UI (tfm_dice.c): in TFM mode (not CLASSIC) the modal screens are drawn and driven by these */
static uint8_t tfm_jam_has;                             /* TILT sent a jam over USB and it was not played yet */
static void tu_dice_draw(void);
static void tu_dice_input(uint32_t pressed, uint32_t notes, uint32_t oct);
static void tu_dice_nudge(void);
static void tu_start_draw(void);
static void tu_time_draw(void);
static uint8_t tu_setup_on;
static uint8_t tfm_dice_timer(void);

static void ns_encode(void)                             /* the code shown: the setup now (not while it is typed) */
{
    if (!ns.cedit)
        tfm_encode(&ns.s, ns.code);
    ns.cbad = 0;
}
static void ns_nudge(void)
{
    tfm_rng_t r;
    uint32_t i, keep = 0, wild = NTRK, free = 0;
    r.s = rng();
    keep |= (ns.hold >> NS_KEY) & 1u ? 1u : 0u;
    keep |= (ns.hold >> NS_SCALE) & 1u ? 2u : 0u;
    keep |= (ns.hold >> NS_BPM) & 1u ? 12u : 0u;        /* (TEMPO holds the swing too) */
    tfm_roll_feel(&r, &ns.s, keep);
    for (i = 0; i < NTRK; i++)
        free += !((ns.hold >> (NS_T1 + i)) & 1u);
    if (ns.lineup == LU_BAND1 && free) {                /* BAND + 1: one track not held goes wild */
        uint32_t k = tfm_pick(&r, free);
        for (wild = 0; wild < NTRK; wild++)
            if (!((ns.hold >> (NS_T1 + wild)) & 1u) && !k--)
                break;
    }
    for (i = 0; i < NTRK; i++)
        if (!((ns.hold >> (NS_T1 + i)) & 1u))
            tfm_roll_track(&r, &ns.s, i, ns.lineup, wild);
    ns.again = 0;
    ns.cedit = 0;
    ns_encode();
}
static void tilt_ns_show(void)
{
    if (!tu_classic())
        tu_setup_on = 0;
    if (chain_busy()) {
        ui_message("STOP THE SONG FIRST");
        return;
    }
    if (!ns.rolled) {                                   /* (the generator: from the time of the first open) */
        rng_state ^= fm1_ms * 2654435761u | 1u;
        ns.rolled = 1;
        ns.hold = 0;
    }
    if (tu_classic()) {
        ns_nudge();
    } else {
        tu_dice_nudge();
        tfm_dice_timer();
    }
    tilt_ns_open = TILT_NS;
    ui.force = 1;
}

/* the project: the sounds, the key and scale on every track, the tempo; timed, the steps go too */
static int ns_has_steps(void)
{
    uint32_t i;
    for (i = 0; i < NTRK; i++)
        if (!seq_is_empty(&trk[i]) || motion_count(&trk[i]))
            return 1;
    return 0;
}
static void tilt_timer_start(void);
static int tfm_newsong_keep(void);                      /* SETUP's NEW SONG rule: 1 KEEP SCENES (tfm_dice.c) */
static uint8_t tfm_new_slot = 0xFF;                     /* the song slot a new song goes into, once picked */
/* round 3: PLAY on DICE makes a NEW SONG: the song it leaves stays saved, the new one is a fresh one in an empty slot (all
 * three used: SONGS asks which to replace). 0 go on, 1 not now */
static int tfm_new_song_begin(void)
{
    uint32_t n;
    if (transport_busy()) {
        ui_message("STOP TO START A NEW SONG");
        return 1;
    }
    if (song_store_cur())
        return 1;                                       /* (the song now, saved: it said why not) */
    if (tfm_new_slot < NSONG) {
        n = tfm_new_slot;
    } else {
        for (n = 0; n < NSONG && (n == sng_cur || song_stored(n)); n++)
            ;
        if (n >= NSONG) {                               /* every slot holds a song: which one? (Cancel: DICE again) */
            tilt_ns_open = 0;
            tsg_new = 1;
            tsg_sel = (uint8_t)((sng_cur + 1u) % NSONG);
            tu_goto(TU_SONGS);
            ui_message("REPLACE WHICH SONG?");
            ui.force = 1;
            return 1;
        }
    }
    tfm_new_slot = 0xFF;
    if (song_stored(n) && song_clear(n))
        return 1;
    sng_cur = (uint8_t)n;                               /* the music is song n now: no chain, no name, scene A to be */
    song_blank_live();
    return 0;
}
static void ns_start(void)
{
    uint32_t i;
    int keep = !tu_classic() && tfm_newsong_keep(), fresh = !tu_classic() && !keep;
    int timed = tilt_timer_min != 0u && tu_classic();   /* (TFM: nothing is cleared: a new song is fresh, or keeps its scenes) */
    char b[24];
    const char *unit;
    if (chain_busy()) {
        ui_message("STOP THE SONG FIRST");
        return;
    }
    if (fresh && tfm_new_song_begin())
        return;
    if (timed && ns_has_steps() && !(ns.again && fm1_ms - ns.again_t < NS_AGAIN_MS)) {
        ns.again = 1;                                   /* (a fresh project clears every track: PLAY again) */
        ns.again_t = fm1_ms;
        ui_message("PLAY AGAIN: CLEARS ALL");
        return;
    }
    ns.again = 0;
    for (i = 0; i < NTRK; i++) {
        track_t *t = &trk[i];
        tfm_load(t, ns.s.ins[i], ns.s.pre[i]);
        t->p[P_ROOT] = ns.s.key;
        t->p[P_SCALE] = ns.s.scale;
        if (!drum_track(t))                             /* (the melodic tracks: every white key in the scale; a kit */
            t->p[P_QUANT] = QN_WHITE;                   /* maps its own keys) */
        if (timed) {                                    /* (as TRACK CLEAR and AUTOMATION CLEAR do) */
            load_begin(t, UNDO_PAT);
            track_defaults_steps(t);
            motion_clear(t);
            load_end(t);
            t->nheld = 0;
            t->arp_phys = 0;
        }
    }
    song.g[G_BPM] = ns.s.bpm;
    song.g[G_SWING] = ns.s.swing;
    if (fresh) {                                        /* scene A: the rolled sounds, empty patterns; B..H empty (the slot was cleared) */
        scene_blank(0);
        (void)project_save_as(0, 0);                    /* (stopped: scene A stored with the rolled sounds) */
        scene_based();
        (void)song_save(sng_cur);
    }
    tilt_ns_open = 0;
    go_home();
    ui.force = 1;
    param_format(&TP[P_ROOT], ns.s.key, b, &unit);
    str_cpy(b + str_len(b), " ", 2);
    str_cpy(b + str_len(b), TP[P_SCALE].names[ns.s.scale], 8);
    str_cpy(b + str_len(b), " ", 2);
    fmt_int(b + str_len(b), ns.s.bpm);
    ui_say("NEW SONG ", b);
    ui.msg_t = 120;                                     /* (~2 s: longer than a message, it says what was made) */
    if (!song.playing && !seq_counting() && transport_req != 1u)
        transport_req = 1;
    if (tilt_timer_min != 0u)
        tilt_timer_start();
    if (tilt_start_pending)                             /* (the start screen's NEW SONG: this is the baseline; nothing */
        autosave_rebase();                              /* written until it changes) */
    ns_from_start = 0;
}

/* every instrument's sounds in one list (instrument by instrument): track t's place there, and back */
static uint32_t ns_snd_n(void)
{
    uint32_t i, n = 0;
    for (i = 0; i < TI_N; i++)
        n += TFM_INS[i].nsnd;
    return n;
}
static uint32_t ns_snd_pos(uint32_t t)
{
    uint32_t i, n = 0;
    for (i = 0; i < ns.s.ins[t] % TI_N; i++)
        n += TFM_INS[i].nsnd;
    return n + ns.s.pre[t];
}
static void ns_snd_at(uint32_t t, uint32_t n)
{
    uint32_t i;
    for (i = 0; i < TI_N && n >= TFM_INS[i].nsnd; i++)
        n -= TFM_INS[i].nsnd;
    ns.s.ins[t] = (uint8_t)(i % TI_N);
    ns.s.pre[t] = (uint8_t)n;
}
/* CODE: the symbols typed (dashes out); a change: decoded when it is a code (the setup then), "?" when not */
static uint32_t ns_code_len(void)
{
    uint32_t i, n = 0;
    for (i = 0; ns.code[i]; i++)
        n += ns.code[i] != '-';
    return n;
}
static char *ns_code_at(uint32_t k)                     /* symbol k (not a dash), 0 past the end */
{
    uint32_t i;
    for (i = 0; ns.code[i]; i++)
        if (ns.code[i] != '-' && !k--)
            return &ns.code[i];
    return 0;
}
static void ns_code_set(uint32_t len)                   /* the symbols again in groups of four, len of them */
{
    char raw[TFM_CODE_MAX + 1u], *p;
    uint32_t i, o = 0;
    for (i = 0; i < len && i < 32u; i++) {
        p = ns_code_at(i);
        raw[i] = p ? *p : '0';
    }
    for (i = 0; i < len && i < 32u; i++) {
        if (i && !(i & 3u))
            ns.code[o++] = '-';
        ns.code[o++] = raw[i];
    }
    ns.code[o] = 0;
}
static void ns_code_try(void)
{
    tfm_setup_t s = ns.s;
    ns.cedit = 1;
    if (tfm_decode(ns.code, &s)) {
        ns.s = s;
        ns.cbad = 0;
        ui_message("CODE OK");
    } else {
        ns.cbad = 1;
    }
}
static void ns_knob(uint32_t k, int32_t s)
{
    uint32_t r = ns.row, t, total, n;
    if (r == NS_CODE) {
        uint32_t len = ns_code_len();
        char *p;
        if (k == 0u && (p = ns_code_at(ns.cpos)) != 0) {
            const char *q = TFM_B32;
            uint32_t v = 0;
            while (q[v] && q[v] != *p)
                v++;
            *p = TFM_B32[(v + (uint32_t)(s % 32 + 32)) % 32u];
            ns_code_try();
        } else if (k == 1u) {
            ns.cpos = (uint8_t)clamp((int32_t)ns.cpos + s, 0, len ? (int32_t)len - 1 : 0);
        } else if (k == 2u) {
            ns_code_set((uint32_t)clamp((int32_t)len + s, 1, 32));
            ns.cpos = (uint8_t)clamp(ns.cpos, 0, (int32_t)ns_code_len() - 1);
            ns_code_try();
        }
        ns.again = 0;
        return;
    }
    if (k > 1u)
        return;
    if (r == NS_KEY && !k) {
        ns.s.key = (uint8_t)(((int32_t)ns.s.key + s % 12 + 12) % 12);
    } else if (r == NS_SCALE && !k) {
        int32_t m = TP[P_SCALE].max + 1;
        ns.s.scale = (uint8_t)(((int32_t)ns.s.scale + s % m + m) % m);
    } else if (r == NS_BPM) {
        if (!k)
            ns.s.bpm = (int16_t)clamp(ns.s.bpm + accel(EN_K1, s, GP[G_BPM].max - GP[G_BPM].min), GP[G_BPM].min, GP[G_BPM].max);
        else
            ns.s.swing = (uint8_t)clamp((int32_t)ns.s.swing + s, 0, 100);
    } else if (r == NS_LINEUP && !k) {
        ns.lineup = (uint8_t)(((int32_t)ns.lineup + s % LU_N + LU_N) % LU_N);
    } else if (r >= NS_T1 && r < NS_CODE) {
        t = r - NS_T1;
        if (!k) {
            total = ns_snd_n();
            n = (ns_snd_pos(t) + (uint32_t)(s % (int32_t)total + (int32_t)total)) % total;
            ns_snd_at(t, n);
        } else {
            ns.s.ins[t] = (uint8_t)(((int32_t)ns.s.ins[t] + s % TI_N + TI_N) % TI_N);
            ns.s.pre[t] = 0;
        }
    }
    ns.again = 0;
    ns.cedit = 0;
    ns_encode();
}

static void te_input(uint32_t pressed, uint32_t notes, uint32_t oct);
static void st_input(uint32_t pressed, uint32_t notes, uint32_t oct);
static uint32_t st_leds(void);
static void st_draw(void);
static void tilt_ns_input(uint32_t pressed, uint32_t notes, uint32_t oct)
{
    uint32_t k, p;
    int32_t s;
    if (tilt_ns_open == TILT_TIME) {
        te_input(pressed, notes, oct);
        return;
    }
    if (tilt_ns_open == TILT_START) {
        st_input(pressed, notes, oct);
        return;
    }
    if (!tu_classic()) {                                /* TFM-UI: DICE, SETUP (tfm_dice.c) */
        tu_dice_input(pressed, notes, oct);
        return;
    }
    song.grid = 2;                                      /* (silent keys: keys_mode, as NAME's) */
    for (k = 0; k < 27u; k++)
        if (((notes >> k) & 1u) && !key_black(k) && (p = key_place(k)) < NS_ROWS) {
            if (p != NS_LINEUP && p != NS_CODE)         /* (LINEUP and CODE are never rolled: nothing to hold) */
                ns.hold ^= (uint16_t)(1u << p);
            ns.row = (uint8_t)p;
        }
    if ((s = panel_enc(EN_SELECT)) != 0)
        ns.row = (uint8_t)clamp((int32_t)ns.row + s, 0, NS_ROWS - 1);
    if (ns.row != NS_CODE && ns.cedit) {                /* (CODE left: it shows the setup again) */
        ns.cedit = 0;
        ns_encode();
    }
    for (k = 0; k < 3u; k++)
        if ((s = panel_enc(EN_K1 + k)) != 0)
            ns_knob(k, s);
    enc_drop();                                         /* the other knobs do nothing here */
    if (oct & 2u)
        ns_nudge();
    else if (oct & 1u) {
        name_close();                                   /* back: nothing changed */
        if (ns_from_start) {                            /* (opened from the start screen: back to it) */
            ns_from_start = 0;
            tilt_ns_open = TILT_START;
            ui.force = 1;
        }
    }
    else if ((pressed >> panel.btn[B_PLAY]) & 1u)
        ns_start();
}

static uint32_t tilt_ns_leds(void)                       /* the held rows' keys lit (TIME: its four choices') */
{
    uint32_t k, m = 0;
    if (tilt_ns_open == TILT_TIME)
        return 0x55u;                                   /* (F3 G3 A3 B3: keys 0 2 4 6) */
    if (tilt_ns_open == TILT_START)
        return st_leds();
    for (k = 0; k < 27u; k++)
        if (!key_black(k) && key_place(k) < (tu_classic() ? NS_ROWS : 7u) && ((ns.hold >> key_place(k)) & 1u))   /* (F3 .. A4) */
            m |= 1u << k;
    return m;
}

/* a row: its key ("F3"), the name, the value at the right; the selected row on SURF, a held one's value THEME with a
 * lock (THEME: amber on the AMBER palette) */
static void ns_value(uint32_t r, char *b)              /* (b holds 40) */
{
    const char *unit;
    if (r == NS_KEY) {
        param_format(&TP[P_ROOT], ns.s.key, b, &unit);
    } else if (r == NS_SCALE) {
        str_cpy(b, TP[P_SCALE].names[ns.s.scale], 8);
    } else if (r == NS_BPM) {
        fmt_int(b, ns.s.bpm);
        str_cpy(b + str_len(b), ns.s.swing ? " BPM  SWG " : " BPM", 11);
        if (ns.s.swing) {
            fmt_int(b + str_len(b), ns.s.swing);
            str_cpy(b + str_len(b), "%", 2);
        }
    } else if (r == NS_LINEUP) {
        str_cpy(b, LU_NAME[ns.lineup % LU_N], 12);
    } else if (r == NS_CODE) {
        str_cpy(b, ns.code, 40);
        if (ns.cbad)
            str_cpy(b + str_len(b), " ?", 3);
    } else {
        tfm_snd_name(ns.s.ins[r - NS_T1], ns.s.pre[r - NS_T1], b, 28);
    }
}
static void ns_draw_row(uint32_t r)
{
    static const char *const LBL[NS_T1] = {"KEY", "SCALE", "TEMPO", "LINEUP"};
    uint32_t sel = r == ns.row, held = (ns.hold >> r) & 1u && r != NS_LINEUP && r != NS_CODE;
    uint16_t bg = sel ? T_SURF : T_BG;
    int32_t y = CAP_IN(S, NS_ROW_H), x;
    char n[3] = {W_NOTE[r], (char)(r < 4u ? '3' : '4'), 0}, b[40];
    cv_begin(240, NS_ROW_H, T_BG);
    if (sel)
        cv_rrect(3, 1, 234, NS_ROW_H - 2, 4, T_SURF, T_BG);
    cv_text_on(9, y, &AF_S, n, held ? T_THEME : T_DIM, bg);
    if (r < NS_T1) {
        cv_text_on(31, y, &AF_S, LBL[r], T_MID, bg);
    } else if (r == NS_CODE) {
        cv_text_on(31, y, &AF_S, "CODE", T_MID, bg);
    } else {
        b[0] = 'T';
        b[1] = (char)('1' + r - NS_T1);
        b[2] = 0;
        cv_text_on(31, y, &AF_S, b, T_MID, bg);          /* (the role: the instrument's name says it; LINEUP the rule) */
    }
    ns_value(r, b);
    x = held ? 214 : 231;
    if (r == NS_CODE && sel && ns.cedit) {              /* (typing: the symbol at the place underlined) */
        uint32_t i, k = 0;
        int32_t w = text_w(&AF_S, b), cx = x - w;
        for (i = 0; b[i]; i++) {
            char c[2] = {b[i], 0};
            if (b[i] != '-' && k++ == ns.cpos)
                cv_rect(cx, y + AF_S_CAP_Y + AF_S_CAP_H + 2, text_w(&AF_S, c), 1, T_THEME);
            cx += text_w(&AF_S, c);
        }
    }
    cv_text_r(x, y, &AF_S, b, held ? T_THEME : r == NS_CODE && ns.cbad ? T_DIM : sel ? T_TEXT : T_MID, bg);
    if (held)
        cv_icon_on(219, (NS_ROW_H - 12) / 2, 12, ICON_X_LOCK, T_THEME, bg);
    cv_blit(0, Y_LABEL + r * NS_ROW_H);
}
static void ns_draw_foot(void)
{
    khint_t a[3], b[3];
    a[0] = (khint_t){KC_OCTUP, "NUDGE"};
    a[1] = (khint_t){KC_PLAY, ns.again ? "AGAIN: CLEAR" : "START"};
    a[2] = (khint_t){KC_OCTDN, "BACK"};
    if (ns.row == NS_CODE) {
        b[0] = (khint_t){KC_K1, "CHAR"};
        b[1] = (khint_t){KC_K2, "MOVE"};
        b[2] = (khint_t){KC_K3, "LENGTH"};
    } else if (ns.row == NS_BPM || ns.row >= NS_T1) {   /* (two knobs: both named; SELECT goes without saying) */
        b[0] = (khint_t){KC_K1, ns.row == NS_BPM ? "BPM" : "SOUND"};
        b[1] = (khint_t){KC_K2, ns.row == NS_BPM ? "SWING" : "INSTR"};
        b[2] = (khint_t){KC_KEYS, "HOLD"};
    } else {
        b[0] = (khint_t){KC_SELECT, "ROW"};
        b[1] = (khint_t){KC_K1, "CHANGE"};
        b[2] = (khint_t){KC_KEYS, "HOLD"};
    }
    cv_begin(240, H_FOOT, T_BG);
    cv_key_row(8, 232, 2, a, 3, 7u, T_BG);
    cv_key_row(8, 232, 21, b, 3, 7u, T_BG);
    cv_blit(0, Y_FOOT);
}
static void te_draw(void);
static void tilt_ns_draw(void)
{
    uint8_t st[12] = {ns.row, (uint8_t)ns.hold, (uint8_t)(ns.hold >> 8), ns.lineup, ns.again, tilt_timer_min, ns.cpos,
                      ns.cedit, ns.cbad};
    uint32_t r, sig;
    if (tilt_ns_open == TILT_TIME) {
        te_draw();
        return;
    }
    if (tilt_ns_open == TILT_START) {
        st_draw();
        return;
    }
    if (!tu_classic()) {                                /* TFM-UI: DICE, SETUP (tfm_dice.c) */
        tu_dice_draw();
        return;
    }
    if (ns.again && fm1_ms - ns.again_t >= NS_AGAIN_MS)
        ns.again = 0;
    sig = fnv(fnv(fnv(2166136261u, st, sizeof st), &ns.s, sizeof ns.s), ns.code, sizeof ns.code) + ux.gen * 7919u;
    if (ui.force) {                                     /* (no cards: no frame's gaps, LINE's dividers; the rows' band) */
        lcd_fill(0, H_HEAD, 240, Y_FOOT - H_HEAD, T_BG);
        ns.sig = ~sig;
    }
    draw_head();
    if (sig != ns.sig) {
        ns.sig = sig;
        for (r = 0; r < NS_ROWS; r++)
            ns_draw_row(r);
        ns_draw_foot();
    }
    if (ui.msg_t && !--ui.msg_t && ui.msg2[0]) {
        str_cpy(ui.msg, ui.msg2, sizeof ui.msg);
        ui.msg2[0] = 0;
        ui.msg_t = 60;
    }
    if (ui.bpm_t)
        ui.bpm_t--;
    ui.force = 0;
}

/* ------------------------------------------------------------- TIMER --- */
/* OFF / 5 / 10 / 15 / 20 minutes (the TILT layer: A3 steps it, KNOB 1 turns it). Setting it starts the countdown, so
 * does NEW SONG's START with it set; it runs in real time (stopped or playing). In the header, left of the battery, on
 * every screen: M:SS in a 3 x 5 dot matrix, 1 px dots 2 px apart (9 px high, "20:00" 29 px wide: the octave or the
 * song row still fits between it and the BPM, ui_draw.c draw_head); amber (fixed, as the splash: dim, from 1:00
 * bright, the last 10 s flashing) where the palette has colour, the palette's own ink on GREY and MONO */
#define TM_DIM 0xABA0u                                  /* #A87400 */
#define TM_BRIGHT 0xFD80u                               /* #FFB000 (the splash's) */
#define TM_DOT 1
#define TM_PITCH 2
static const uint16_t TM_DIGIT[10] = {                  /* 5 rows of 3 (bit 14 the top row's left dot) */
    075557, 026227, 071747, 071717, 055711, 074717, 074757, 071111, 075757, 075717};
static struct {
    uint8_t on;                                         /* counting */
    uint32_t end;                                       /* fm1_ms when it reaches 0:00 */
    uint32_t left;                                      /* the seconds shown (frozen at 0 once it is over) */
} tm;
static const uint8_t TM_STEPS[5] = {0, 5, 10, 15, 20};
#ifndef TILT_MIN_MS
#define TILT_MIN_MS 60000u                              /* a minute (a test build of the emulator: shorter) */
#endif

static void te_end(void);
static void tilt_timer_start(void)                      /* (tilt_timer_set and NEW SONG's START: a TIME on its way, its */
{                                                       /* bar, fade or quiet, is called off, MASTER back: te_end) */
    te_end();
    tm.on = tilt_timer_min != 0u;
    tm.end = fm1_ms + (uint32_t)tilt_timer_min * TILT_MIN_MS;
    tm.left = (uint32_t)tilt_timer_min * TILT_MIN_MS / 1000u;
}
static void tilt_timer_set(uint32_t min)
{
    char b[12];
    tilt_timer_min = (uint8_t)min;
    tilt_timer_start();
    if (min) {
        fmt_int(b, (int32_t)min);
        str_cpy(b + str_len(b), " MIN", 5);
        ui_say("TIMER ", b);
    } else {
        ui_message("TIMER OFF");
    }
}
static uint32_t tm_step_of(uint32_t min)               /* its place in TM_STEPS */
{
    uint32_t i;
    for (i = 0; i < sizeof TM_STEPS && TM_STEPS[i] != min; i++)
        ;
    return i % sizeof TM_STEPS;
}
static void tilt_timer_over(void);
static void te_poll(void);
static void tilt_scene_poll(void);
static void tilt_timer_poll(void)
{
    tilt_scene_poll();                                  /* (the scene remembered: SCENES, below) */
    te_poll();                                          /* (TIME: the bar, the fade) */
    if (!tm.on)
        return;
    if ((int32_t)(tm.end - fm1_ms) <= 0) {
        tm.on = 0;
        tm.left = 0;
        tilt_timer_over();
        return;
    }
    tm.left = (tm.end - fm1_ms + 999u) / 1000u;
}
/* the countdown's text, "9:59" / "20:00" */
static void tm_text(char *b)
{
    uint32_t m = tm.left / 60u, s = tm.left % 60u, n;
    fmt_int(b, (int32_t)m);
    n = str_len(b);
    b[n] = ':';
    b[n + 1u] = (char)('0' + s / 10u);
    b[n + 2u] = (char)('0' + s % 10u);
    b[n + 3u] = 0;
}
static int32_t tm_char_w(char c) { return c == ':' ? TM_DOT : 3 * TM_PITCH - 1; }
static int32_t tilt_head_w(void)
{
    char b[8];
    int32_t w = 0, i;
    if (!tm.on)
        return 0;
    tm_text(b);
    for (i = 0; b[i]; i++)
        w += tm_char_w(b[i]) + (b[i + 1] ? TM_PITCH : 0);
    return w;
}
/* 0 hidden (flashing), 1 dim, 2 bright */
static uint32_t tm_level(void)
{
    if (tm.left <= 10u)
        return (fm1_ms / 250u) & 1u ? 0u : 2u;
    return tm.left <= 60u ? 2u : 1u;
}
static uint32_t tilt_head_sig(void) { return tm.on ? tm.left * 4u + tm_level() + 1u : 0u; }
static void tilt_head_draw(int32_t xr)
{
    char b[8];
    uint32_t lv = tm_level(), i, r, c;
    uint32_t grey = settings.palette == UI_GREY_INDEX || settings.palette == UI_BW_INDEX;   /* (no colour there) */
    uint16_t ink = grey ? (lv == 2u ? T_TEXT : T_MID) : lv == 2u ? TM_BRIGHT : TM_DIM;
    int32_t x = xr - tilt_head_w(), y = (H_HEAD - (5 * TM_PITCH - 1)) / 2;
    if (!lv)
        return;
    tm_text(b);
    for (i = 0; b[i]; i++) {
        if (b[i] == ':') {                              /* the colon: two dots */
            cv_rect(x, y + TM_PITCH, TM_DOT, TM_DOT, ink);
            cv_rect(x, y + 3 * TM_PITCH, TM_DOT, TM_DOT, ink);
        } else {
            uint32_t g = TM_DIGIT[(b[i] - '0') % 10];
            for (r = 0; r < 5u; r++)
                for (c = 0; c < 3u; c++)
                    if ((g >> (14u - 3u * r - c)) & 1u)
                        cv_rect(x + (int32_t)c * TM_PITCH, y + (int32_t)r * TM_PITCH, TM_DOT, TM_DOT, ink);
        }
        x += tm_char_w(b[i]) + TM_PITCH;
    }
}

/* -------------------------------------------------------------- TIME --- */
/* At 0:00: playing, the bar finishes (the clock's 1/16 step back at the bar's first; 8 s at most, a bar at 40 BPM is
 * 6), then the sound fades out over TE_FADE_MS (MASTER x master_fade: the pot still sets the level), the transport
 * stops, a moment of quiet for the tails (TE_QUIET_MS; at most TE_QUIET_MAX_MS, the transport stopped or not: MASTER
 * is never left down, every way out of the bar / fade / quiet puts it back, te_end); stopped already: at once. Then the TIME screen: TIME big in the splash's
 * dot matrix, the project written as the autosave (the timed take: project.c autosave_now; RESTORE LAST ON brings it
 * back at power-on; nothing new since the last autosave or the start screen's choice: none, "NOTHING NEW TO SAVE"),
 * and four choices, F3 .. B3 or SELECT / KNOB 1 and OCT+:
 *   PLAY IT BACK  from the top     SAVE          SAVE > PROJECT: a slot, KNOB 4 SAVE (its name, as ever)
 *   EXTRA BALL    5 more minutes, the music on again                KEEP WORKING  the timer off (OCT- too)
 * The menu (HOME held) closes it as KEEP WORKING. Due while NAME, a dialog, the MENU or NEW SONG is open, TIME waits
 * until that closes (MASTER back meanwhile): what is being typed or asked is never closed under the player */
enum { TE_NONE, TE_BAR, TE_FADE, TE_QUIET, TE_WAIT };   /* (TE_WAIT: due, NAME / a dialog / MENU / NEW SONG open) */
enum { TO_PLAY, TO_SAVE, TO_EXTRA, TO_KEEP, TO_N };
#define TE_BAR_MS 8000u
#define TE_FADE_MS 2000u
#define TE_QUIET_MS 700u
#define TE_QUIET_MAX_MS 3000u                           /* (the transport still running then, an external clock: on) */
#define TE_ROW_H 18
#define TE_ROW_Y 124                                    /* 4 rows: 124 .. 196, the footer at 202 */
static const char *const TO_NAME[TO_N] = {"PLAY IT BACK", "SAVE", "EXTRA BALL", "KEEP WORKING"};
static const char *const TO_HINT[TO_N] = {"FROM THE TOP", "TO A SCENE", "+5 MIN", "TIMER OFF"};
static struct {
    uint8_t st, moved, pick, saved;
    uint32_t t, sig;
} te;
static const uint8_t TE_WORD[4][7] = {                  /* T I M E: rows, top first, bit 4 the left column */
    {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04},
    {0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E},
    {0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11},
    {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F},
};

static void te_end(void)                                /* the bar / fade / quiet over: MASTER as the pot says */
{
    te.st = TE_NONE;
    master_fade = 4096u;
}
static int te_ready(void) { return !nm.kind && !ui.confirm && !ui.menu && tilt_ns_open != TILT_NS; }
static void te_show(void)                               /* (te_ready: nothing else open to close) */
{
    te_end();
    te.saved = (uint8_t)autosave_now();
    te.pick = TO_PLAY;
    tilt_ns_open = TILT_TIME;
    ui.force = 1;
}
static void te_show_when_ready(void)
{
    if (te_ready()) {
        te_show();
    } else {
        te_end();
        te.st = TE_WAIT;
    }
}
static void tilt_timer_over(void)
{
    if (song.playing || seq_counting()) {
        te.st = TE_BAR;
        te.moved = 0;
        te.t = fm1_ms;
    } else {
        te_show_when_ready();
    }
}
static void te_poll(void)
{
    uint32_t e = fm1_ms - te.t;
    if (te.st == TE_BAR) {
        if (clk_step != 0u)
            te.moved = 1;
        if (!song.playing || (te.moved && clk_step == 0u) || e >= TE_BAR_MS) {
            te.st = TE_FADE;                            /* (the bar is done: the next one's first step) */
            te.t = fm1_ms;
        }
    } else if (te.st == TE_FADE) {
        if (e >= TE_FADE_MS) {
            master_fade = 0;
            if (song.playing || seq_counting())
                transport_req = 2;
            te.st = TE_QUIET;
            te.t = fm1_ms;
        } else {
            master_fade = (uint16_t)(4096u - 4096u * e / TE_FADE_MS);
        }
    } else if (te.st == TE_QUIET && e >= TE_QUIET_MS && (!song.playing || e >= TE_QUIET_MAX_MS)) {
        te_show_when_ready();
    } else if (te.st == TE_WAIT && te_ready()) {
        te_show();
    }
}

static void te_do(uint32_t o)
{
    uint32_t i;
    tilt_ns_open = 0;
    go_home();
    ui.force = 1;
    if (o == TO_PLAY) {
        transport_req = 1;
        ui_message("THE TAKE");
    } else if (o == TO_SAVE && !tu_classic()) {         /* TFM-UI: KEEP AS A SONG: SONGS, and the NAME screen for it */
        uint32_t sl = scene_cur() < NSCN ? scene_cur() : 0u;
        for (i = 0; i < NSCN && scene_cur() >= NSCN; i++)
            if (!project_used(i)) {
                sl = i;
                break;
            }
        (void)sl;
        tu_goto(TU_SONGS);
        name_open(NK_SONG_SAVE, sng_cur);
    } else if (o == TO_SAVE) {
        for (i = 0; i < NPAGES && PAGES[i].graph != GR_SLOTS; i++)
            ;
        ui.home = 0;
        ui.page = (uint8_t)i;
        ui_message("A SCENE, THEN [K4] SAVE");
    } else if (o == TO_EXTRA) {
        tm.on = 1;
        tm.end = fm1_ms + 5u * TILT_MIN_MS;
        tm.left = 5u * TILT_MIN_MS / 1000u;
        transport_req = 1;
        ui_message("EXTRA BALL: +5 MIN");
    } else {
        ui_message("KEEP WORKING");
    }
}
static void te_input(uint32_t pressed, uint32_t notes, uint32_t oct)
{
    uint32_t k, p;
    int32_t s;
    song.grid = 2;
    for (k = 0; k < 27u; k++)
        if (((notes >> k) & 1u) && !key_black(k) && (p = key_place(k)) < TO_N) {
            te_do(p);
            enc_drop();
            return;
        }
    if ((s = panel_enc(EN_SELECT)) != 0 || (s = panel_enc(EN_K1)) != 0)
        te.pick = (uint8_t)clamp((int32_t)te.pick + (s > 0 ? 1 : -1), 0, TO_N - 1);
    enc_drop();
    if (oct & 2u)
        te_do(te.pick);
    else if (oct & 1u)
        te_do(TO_KEEP);
    else if ((pressed >> panel.btn[B_PLAY]) & 1u)
        te_do(TO_PLAY);
}

static void te_draw_word(void)                          /* TIME, 205 x 61 at y 36: lit amber, unlit dim (the splash's) */
{
    uint32_t grey = settings.palette == UI_GREY_INDEX || settings.palette == UI_BW_INDEX, col, row;
    uint16_t lit = grey ? T_TEXT : SPL_AMBER, unlit = grey ? T_RAISE : SPL_AMBER_DIM;
    for (col = 0; col < SPL_COLS; col++) {
        uint32_t ch = col / 6u, cx = col % 6u;
        for (row = 0; row < 7u; row++) {
            uint32_t x = SPL_X0 + col * SPL_PITCH, y = 36u + row * SPL_PITCH;
            uint16_t c = cx < 5u && (TE_WORD[ch][row] >> (4u - cx)) & 1u ? lit : unlit;
            lcd_fill(x + 1u, y, SPL_DOT - 2u, SPL_DOT, c);
            lcd_fill(x, y + 1u, SPL_DOT, SPL_DOT - 2u, c);
        }
    }
}
static void te_draw_rows(void)
{
    char n[3] = {0, '3', 0};
    uint32_t i;
    cv_begin(240, 16, T_BG);                            /* the take: saved or not */
    GFX_HOOK_ALIGN(0, 0, 240, 16, AL_HV, "time take line centred");
    cv_text_in(0, CAP_IN(S, 16), 240, &AF_S, te.saved == 1u ? "TIMED TAKE SAVED" : te.saved == 2u ? "NOTHING NEW TO SAVE" :
               "TIMED TAKE NOT SAVED", te.saved ? T_MID : T_ACCENT, T_BG);   /* (project.c autosave_now: 1, 2, 0) */
    cv_blit(0, 104);
    for (i = 0; i < TO_N; i++) {
        uint32_t sel = i == te.pick;
        uint16_t bg = sel ? T_SURF : T_BG;
        int32_t y = CAP_IN(S, TE_ROW_H);
        n[0] = W_NOTE[i];
        cv_begin(240, TE_ROW_H, T_BG);
        if (sel)
            cv_rrect(3, 0, 234, TE_ROW_H, 4, T_SURF, T_BG);
        cv_text_on(9, y, &AF_S, n, T_DIM, bg);
        cv_text_on(31, y, &AF_S, TO_NAME[i], sel ? T_THEME : T_TEXT, bg);
        cv_text_r(231, y, &AF_S, TO_HINT[i], T_MID, bg);
        cv_blit(0, TE_ROW_Y + i * TE_ROW_H);
    }
}
static void te_draw(void)
{
    uint32_t sig = te.pick * 7u + te.saved * 3u + ux.gen * 7919u;
    if (!tu_classic()) {                                /* TFM-UI: the drawing (tfm_dice.c) */
        tu_time_draw();
        return;
    }
    if (ui.force) {
        lcd_fill(0, H_HEAD, 240, Y_FOOT - H_HEAD, T_BG);
        te_draw_word();
        te.sig = ~sig;
    }
    draw_head();
    if (sig != te.sig) {
        khint_t a[3];
        te.sig = sig;
        te_draw_rows();
        a[0] = (khint_t){KC_OCTUP, "DO IT"};
        a[1] = (khint_t){KC_SELECT, "PICK"};
        a[2] = (khint_t){KC_KEYS, "F3 - B3"};
        cv_begin(240, H_FOOT, T_BG);
        cv_key_row(8, 232, 9, a, 3, 7u, T_BG);
        cv_blit(0, Y_FOOT);
    }
    if (ui.msg_t && !--ui.msg_t && ui.msg2[0]) {
        str_cpy(ui.msg, ui.msg2, sizeof ui.msg);
        ui.msg2[0] = 0;
        ui.msg_t = 60;
    }
    ui.force = 0;
}

/* --------------------------------------------------------- the layer --- */
/* ------------------------------------------------------------ SCENES --- */
/* C4..F4: SCENE A..D (project.c: the PROJECT slots are the song's scenes; a scene is the four tracks, the song's
 * tempo, effects, sections and name stay). Stopped: the scene the music is in, when edited since it was loaded or
 * stored (SCENE A*), is stored to its slot first (as SAVE > PROJECT saves: its flash at once), then the other one's
 * tracks are loaded; an empty one keeps the sounds and the mix with no steps and no automation. Then the autosave is
 * written (the settings remember the scene: tilt_scene_poll; both at once, so CONTINUE finds them agreeing) and the
 * start screen's choice is over (a change of scene is a change). The music in no scene (none was empty for it) and
 * edited: the dialog first (CF_SCENE_DROP), as nothing would keep it.
 * Playing: cued for the end of the bar (SCENE B: NEXT BAR; its key breathes, the strip says NEXT): the scene left is
 * stored in RAM now (and again while cued, as it is edited, up to the bar's last 16th), the cued one staged, and the
 * audio ISR flips at the bar's end (seq.c scene_flip); its key again, or the scene now's, takes the cue back. Stopped
 * before the bar ends: it goes at once. While a song plays: STOP TO CHANGE SCENE. The key LEDs: the scene now lit, a
 * stored one at the mid level, an empty one dark, the cued one breathing */
static void tilt_scene_uncue(void)
{
    scene_armed = 0;
    tsc.cue = SCENE_NONE;
}
static void tilt_scene_go(uint32_t s, int drop)
{
    uint32_t cur = scene_cur(), stored = 0, used;
    char b[24] = "SCENE A";
    s &= NSCN - 1u;
    b[6] = (char)('A' + s);
    if (chain_busy()) {
        ui_message("STOP TO CHANGE SCENE");
        return;
    }
    if (transport_busy()) {                             /* playing (or counting in): at the bar's end */
        if (s == cur || s == tsc.cue) {
            if (tsc.cue != SCENE_NONE)
                ui_message("SCENE CHANGE OFF");
            else
                ui_message(b);
            tilt_scene_uncue();
            return;
        }
        if (cur == SCENE_NONE && scene_edited() && !drop) {
            confirm_open(CF_SCENE_DROP, s);
            return;
        }
        tilt_scene_uncue();
        if (cur != SCENE_NONE && scene_edited() && scene_store_ram(cur))
            return;
        if (scene_stage_from(s)) {
            ui_message("STOP TO CHANGE SCENE");
            return;
        }
        tsc.cue = (uint8_t)s;
        RING_PUBLISH();
        scene_armed = 1;
        tilt_start_pending = 0;
        str_cpy(b + 7, ": NEXT BAR", sizeof b - 7);
        ui_message(b);
        ui.force = 1;
        return;
    }
    tilt_scene_uncue();
    if (s == cur) {
        ui_message(b);
        return;
    }
    if (scene_edited()) {
        if (cur == SCENE_NONE && !drop) {
            confirm_open(CF_SCENE_DROP, s);
            return;
        }
        if (cur != SCENE_NONE) {
            if (project_save_as(cur, 0))                /* (it said why: SAVE ERROR) */
                return;
            stored = 1;
        }
    }
    used = (uint32_t)project_used(s);
    if (used ? scene_load(s) != 0 : (scene_blank(s), 0)) {
        ui_message("SCENE NOT LOADED");
        return;
    }
    tilt_start_pending = 0;
    autosave_now();
    ui_message(b);
    if (stored) {
        str_cpy(b, "A STORED", sizeof b);
        b[0] = (char)('A' + cur);
        str_cpy(ui.msg2, b, sizeof ui.msg2);
    } else if (!used) {
        str_cpy(ui.msg2, "EMPTY: THE SAME SOUNDS", sizeof ui.msg2);
    }
    ui.force = 1;
}
/* G4 (the SCENE layer) ADD TO SONG: a SONG section at the end with the four tracks on the scene now, x1 (core.h chain_row_t; the SONG
 * page edits it as any): ADDED: SECTION 3. The song plays a scene as stored: one edited (or never stored) is stored
 * first, stopped (playing: STOP TO STORE SCENE, nothing added). In no scene: NO SCENE TO ADD; sixteen sections:
 * SONG FULL; a song playing: STOP TO EDIT, as the SONG page */
static void tilt_scene_add(void)
{
    uint32_t cur = scene_cur();
    char b[24] = "ADDED: SECTION ";
    if (chain_busy()) {
        ui_message("STOP TO EDIT");
        return;
    }
    if (cur == SCENE_NONE) {
        ui_message("NO SCENE TO ADD");
        return;
    }
    if (chain_config.count >= CHAIN_ROWS) {
        ui_message("SONG FULL");
        return;
    }
    if (scene_edited() || !project_used(cur)) {
        if (transport_busy()) {
            ui_message("STOP TO STORE SCENE");
            return;
        }
        if (project_save_as(cur, 0))
            return;
    }
    chain_config.row[chain_config.count] = chain_row_of(cur, 1);
    chain_config.count++;
    fmt_int(b + str_len(b), chain_config.count);
    ui_message(b);
    ui.force = 1;
}
/* the settings follow the scene the music is in (ui.c PREF_SCENE; deferred while playing, as every settings write).
 * Not while the start screen shows: the one remembered is CONTINUE's to restore */
/* every main loop pass: a flip done (seq.c scene_armed 2): the music is that scene now; a cue left when the transport
 * stopped: it goes now (stopped); while cued, the scene left edited again is stored again (not in the bar's last
 * 16ths: the flip must not land while it is read); a song started meanwhile takes the cue back; stopped, the scenes
 * stored in RAM go to flash */
static void tilt_scene_follow(void)
{
    uint32_t k, s = tsc.cue;
    char b[16] = "SCENE A";
    if (scene_armed == 2u) {
        tilt_scene_uncue();
        scene_enter(s);
        undo.trk = 0;                                   /* (ui.c: the undo copy was the scene before) */
        for (k = 0; k < NTRK; k++) {
            trk[k].user = 0;
            pat_sig[k] = ~steps_sig(&trk[k]);           /* (a scene's steps are the user's) */
            fm6_adopt(k);
        }
        pat_from_set((s & (NSCN - 1u)) + 1u);
        sync_reload = 1;                                /* (the editor) */
        b[6] = (char)('A' + (s & (NSCN - 1u)));
        ui_message(b);
        ui.force = 1;
    } else if (scene_armed == 1u && chain_busy()) {
        tilt_scene_uncue();
    } else if (scene_armed == 1u && !transport_busy()) {
        tilt_scene_uncue();
        tilt_scene_go(s, 1);
    } else if (scene_armed == 1u && scene_cur() != SCENE_NONE && clk_step < 14u && scene_edited()) {
        (void)scene_store_ram(scene_cur());
    }
    if (!transport_busy())
        scene_flush();
}
static void scene_keys_poll(void);
static void tilt_scene_poll(void)
{
    uint32_t v = scene_cur() < NSCN ? scene_cur() + 1u : 0u, m = (uint32_t)(sng_cur & 3u) | 4u | v << 4;
    tilt_scene_follow();
    scene_keys_poll();
    if (tilt_ns_open == TILT_START || SCN_MEM == m)
        return;
    SCN_MEM = (uint8_t)m;                               /* (round 2: the song and the scene, project.c SCN_MEM) */
    settings_save();
}

static void tilt_key(uint32_t k, uint32_t p)
{
    if (key_black(k))
        return;
    if (p == TT_NEW)
        tilt_ns_show();
    else if (p == TT_STUCK)
        tilt_stuck();
    else if (p == TT_TIMER)                             /* A3: OFF 5 10 15 20, each press the next */
        tilt_timer_set(TM_STEPS[(tm_step_of(tilt_timer_min) + 1u) % sizeof TM_STEPS]);
}
static void tilt_knob(uint32_t k, int32_t s)            /* KNOB 1: the TIMER */
{
    if (k == 0u && s)
        tilt_timer_set(TM_STEPS[(uint32_t)clamp((int32_t)tm_step_of(tilt_timer_min) + (s > 0 ? 1 : -1), 0,
                                                (int32_t)sizeof TM_STEPS - 1)]);
}
static uint32_t tilt_can(uint32_t k, uint32_t p)
{
    return !key_black(k) && (((p == TT_NEW || p == TT_STUCK) && !chain_busy()) || p == TT_TIMER);
}
static uint32_t tilt_on(uint32_t k, uint32_t p) { return !key_black(k) && p == TT_TIMER && tilt_timer_min; }
static uint32_t tilt_sig(void)
{
    return (tilt_used(TSEL) != 0u) * 31u + (uint32_t)chain_busy() * 3u + song.sel * 977u + tilt_timer_min * 7919u;
}
static void tilt_cards(void)
{
    char v[4];
    uint32_t c;
    fmt_int(v, tilt_timer_min);
    draw_column(0, "TIMER", tilt_timer_min ? v : "OFF", tilt_timer_min ? "MIN" : "", tilt_timer_min ? VAL(0u) : T_DIM,
                (int32_t)tm_step_of(tilt_timer_min) * 250, ICON_TIME);
    for (c = 1; c < 4u; c++)
        draw_column(c, "", "", "", T_THEME, -1, ICON_NONE);
}
/* the map: F3 NEW SONG, G3 STUCK, A3 TIMER, and under them what STUCK will do to the selected track */
static void layer_tilt(void)
{
    uint32_t st = chain_busy() ? LS_DIM : LS_OFF;
    char n[3] = {'F', '3', 0}, b[24];
    uint16_t ink, fill;
    lcell(LC_X(TT_NEW), 4, LC_H, n, ICON_STEPS, 0, "NEW", st, 0);   /* (NEW SONG: wider than a cell) */
    n[0] = 'G';
    lcell(LC_X(TT_STUCK), 4, LC_H, n, ICON_PROB, 0, "STUCK", st, 0);
    n[0] = 'A';
    lcell(LC_X(TT_TIMER), 4, LC_H, n, ICON_TIME, 0, "TIMER", tilt_timer_min ? LS_SEL : LS_OFF, 0);
    b[0] = 'T';
    b[1] = (char)('1' + song.sel);
    str_cpy(b + 2, chain_busy() ? ": STOP TO EDIT" : tilt_used(TSEL) ? ": COOK IT" : ": RANDOM", sizeof b - 2);
    fill = lc_box(6, 100, 228, 18, lc_fill(st, &ink));
    GFX_HOOK_ALIGN(6, 100, 234, 118, AL_HV, "tilt layer stuck line centred");
    cv_text_in(6, 100 + CAP_IN(S, 18), 228, &AF_S, b, ink, fill);
}

/* ------------------------------------------------------- SCENE LAYER --- */
/* ARP held (its tap is still ARP's pages; a double tap locks it), a SET layer of the song's scenes:
 *   C4..F4         a tap: go to scene A..D (tilt_scene_go: stopped at once, playing at the bar's end). It goes when the
 *                  key is let go, if nothing else was done with it held:
 *   hold + C4..F4  the held scene copied into the pressed one (the held one the scene now: its live state, stored
 *                  first); a used one: OVERWRITE C? first (CF_SCENE_OVR, NO picked); "A > C COPIED"
 *   hold + OCT-    the held scene cleared: CLEAR C? (USED IN SONG when a section plays it; CF_SCENE_CLR, NO picked);
 *                  the scene now: GO TO ANOTHER SCENE (as a copy into it)
 *   G4             ADD TO SONG (tilt_scene_add)
 *   OCT- alone     the last copy or clear undone (one level: project.c scene_undo)
 * A copy, a clear and its undo write flash: stopped only. The map: SCENES C4-F4 over the four boxes, each its key and
 * letter (the scene now THEME, a dot after the letter when edited; a stored one outlined; an empty one dim, dashed; the
 * cued one blinking), G4 ADD under them. LEDs: the scene now lit, a stored one at the mid level, the cued one
 * breathing, an empty one dark; G4 breathes when it can add */
#define SK_SCENE 4u                                     /* C4: the white key place of scene A */
static uint32_t scene_keys(void) { return tu_classic() ? 4u : NSCN; }   /* the layer has four boxes; TFM's keys are A..H */
#define SK_ADD 8u                                       /* G4 */
static struct { uint8_t held, k, used; } skh = {SCENE_NONE, 0, 0};
/* TFM: QUICK CHAIN: G4 held, the scene keys tapped in order: each tap a section of that scene at the song's end (the
 * same scene again: one more REPS of the last section). G4 tapped alone: ADD as before (on its release) */
static struct { uint8_t held, k, used; } qc;
static void quick_chain(uint32_t s)
{
    chain_row_t *last = chain_config.count ? &chain_config.row[chain_config.count - 1u] : 0;
    char b[24] = "SONG: ";
    uint32_t i, n;
    qc.used = 1;
    if (chain_busy()) {
        ui_message("STOP TO EDIT");
        return;
    }
    if (last && !memcmp(last->slot, chain_row_of(s, 1).slot, sizeof last->slot) && last->repeat < 16u) {
        last->repeat++;
    } else if (chain_config.count >= CHAIN_ROWS) {
        ui_message("SONG FULL");
        return;
    } else {
        chain_config.row[chain_config.count++] = chain_row_of(s, 1);
    }
    n = str_len(b);                                     /* the song's last sections, as letters ("A A B") */
    for (i = chain_config.count > 4u ? chain_config.count - 4u : 0u; i < chain_config.count && n + 4u < sizeof b; i++) {
        b[n++] = (char)('A' + chain_config.row[i].slot[0] % NSCN);
        if (chain_config.row[i].repeat > 1u)
            b[n++] = 'x', b[n++] = (char)('0' + chain_config.row[i].repeat % 10u);
        b[n++] = ' ';
    }
    b[n ? n - 1u : 0u] = 0;
    ui_message(b);
    ui.force = 1;
}
static void scene_copy_ui(uint32_t src, uint32_t dst, int sure)
{
    char b[24] = "A > C COPIED";
    src &= NSCN - 1u;
    dst &= NSCN - 1u;
    if (transport_busy()) {
        ui_message("STOP TO COPY SCENE");
        return;
    }
    if (dst == scene_cur()) {
        ui_message("GO TO ANOTHER SCENE");
        return;
    }
    if (src == scene_cur() && (scene_edited() || !project_used(src)) && project_save_as(src, 0))
        return;                                         /* (its live state first; it said why not) */
    if (!project_used(src)) {
        str_cpy(b, "A IS EMPTY", sizeof b);
        b[0] = (char)('A' + src);
        ui_message(b);
        return;
    }
    if (project_used(dst) && !sure) {
        confirm_open(CF_SCENE_OVR, src << 3 | dst);
        return;
    }
    if (scene_copy(src, dst))
        return;
    b[0] = (char)('A' + src);
    b[4] = (char)('A' + dst);
    ui_message(b);
    ui.force = 1;
}
static int scene_in_song(uint32_t s)                    /* a SONG section plays scene s on some track */
{
    uint32_t r, k;
    for (r = 0; r < chain_config.count && r < CHAIN_ROWS; r++)
        for (k = 0; k < NTRK; k++)
            if (chain_config.row[r].slot[k] == s)
                return 1;
    return 0;
}
static void scene_clear_ui(uint32_t s, int sure)
{
    char b[24] = "A CLEARED";
    s &= NSCN - 1u;
    b[0] = (char)('A' + s);
    if (transport_busy()) {
        ui_message("STOP TO CLEAR SCENE");
    } else if (s == scene_cur()) {
        ui_message("GO TO ANOTHER SCENE");
    } else if (!project_used(s)) {
        str_cpy(b + 1, " IS EMPTY", sizeof b - 1);
        ui_message(b);
    } else if (!sure) {
        confirm_open(CF_SCENE_CLR, s);
    } else if (!scene_clear(s)) {
        ui_message(b);
        ui.force = 1;
    }
}
static void scene_undo_ui(void)
{
    uint32_t s = scene_undo_slot();
    char b[24] = "A PUT BACK";
    if (transport_busy()) {
        ui_message("STOP TO UNDO");
    } else if (s == SCENE_NONE) {
        ui_message("NOTHING TO UNDO");
    } else if (s == scene_cur()) {
        ui_message("GO TO ANOTHER SCENE");
    } else if (!scene_undo()) {
        b[0] = (char)('A' + s);
        ui_message(b);
        ui.force = 1;
    }
}
static void scene_key(uint32_t k, uint32_t p)
{
    if (key_black(k))
        return;
    if (!tu_classic() && k == 26u) {                    /* TFM-UI: C4..C5 are the scenes A..H, G5 held chains */
        qc.held = 1;
        qc.k = (uint8_t)k;
        qc.used = 0;
        return;
    }
    if (tu_classic() && p == SK_ADD) {                  /* (the layer: on its release, unless the scenes were tapped) */
        qc.held = 1;
        qc.k = (uint8_t)k;
        qc.used = 0;
        return;
    }
    if (p < SK_SCENE || p >= SK_SCENE + scene_keys())
        return;
    p -= SK_SCENE;
    if (qc.held && ((fm1_in.notes >> qc.k) & 1u)) {     /* TFM: QUICK CHAIN */
        quick_chain(p);
        return;
    }
    if (skh.held != SCENE_NONE && skh.held != p && ((fm1_in.notes >> skh.k) & 1u)) {
        skh.used = 1;                                   /* hold + another: copy */
        scene_copy_ui(skh.held, p, 0);
        return;
    }
    skh.held = (uint8_t)p;
    skh.k = (uint8_t)k;
    skh.used = 0;
}
/* the layer's OCT- (ui_layer.c layer_oct): a scene key held, clear it; alone, undo */
static void scene_oct(void)
{
    if (skh.held != SCENE_NONE && ((fm1_in.notes >> skh.k) & 1u)) {
        skh.used = 1;
        scene_clear_ui(skh.held, 0);
    } else {
        scene_undo_ui();
    }
}
/* each pass (tilt_scene_poll): the scene key held let go, nothing else done with it: go there */
static void scene_keys_poll(void)
{
    if (qc.held && !((fm1_in.notes >> qc.k) & 1u)) {    /* (TFM: G4 let go: ADD if it chained nothing) */
        qc.held = 0;
        if (!qc.used)
            tilt_scene_add();
    }
    if (skh.held == SCENE_NONE || ((fm1_in.notes >> skh.k) & 1u))
        return;
    if (!skh.used && !ui.confirm)
        tilt_scene_go(skh.held, 0);
    skh.held = SCENE_NONE;
}
static int tilt_addable(void) { return scene_cur() != SCENE_NONE && chain_config.count < CHAIN_ROWS && !chain_busy(); }
static uint32_t scene_can(uint32_t k, uint32_t p)       /* (breathing: the cued scene, G4 when it can add) */
{
    return !key_black(k) && ((tu_classic() && p == SK_ADD && tilt_addable()) ||
                             (p >= SK_SCENE && p < SK_SCENE + scene_keys() && p - SK_SCENE == tsc.cue));
}
static uint32_t scene_on(uint32_t k, uint32_t p)
{
    return !key_black(k) && p >= SK_SCENE && p < SK_SCENE + scene_keys() && p - SK_SCENE == scene_cur();
}
/* the keys at the mid level: the stored scenes (not the one now, lit, nor the cued one, breathing) */
static uint32_t tilt_mid(void)
{
    uint32_t k, m = 0, p;
    for (k = 0; k < 27u; k++)
        if (!key_black(k) && (p = key_place(k)) >= SK_SCENE && p < SK_SCENE + scene_keys() && p - SK_SCENE != scene_cur() &&
            p - SK_SCENE != tsc.cue && graph_project_used(p - SK_SCENE))
            m |= 1u << k;
    return m;
}
static uint32_t scene_blink(void) { return tsc.cue != SCENE_NONE && ((ui.frame >> 3) & 1u); }
static uint32_t scene_sig_ui(void)
{
    uint32_t s, h = scene_cur() * 3u + (uint32_t)scene_edited() * 5u + chain_config.count * 11u + tsc.cue * 13u +
                    scene_blink() * 17u + (uint32_t)chain_busy() * 19u;
    for (s = 0; s < NSCN; s++)
        h = h * 7u + (uint32_t)graph_project_used(s);
    return h;
}
static void scene_cards(void)
{
    uint32_t c;
    for (c = 0; c < 4u; c++)
        draw_column(c, "", "", "", T_THEME, -1, ICON_NONE);
}
static void scene_dash(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t c)   /* a dashed 1 px outline */
{
    int32_t i;
    for (i = 2; i + 3 <= w - 2; i += 6) {
        cv_rect(x + i, y, 3, 1, c);
        cv_rect(x + i, y + h - 1, 3, 1, c);
    }
    for (i = 2; i + 3 <= h - 2; i += 6) {
        cv_rect(x, y + i, 1, 3, c);
        cv_rect(x + w - 1, y + i, 1, 3, c);
    }
}
#define SB_Y 18                                         /* the boxes, under their label */
#define SB_H 44
static void layer_scene(void)
{
    uint32_t s, cur = scene_cur();
    char n[3] = {'G', '4', 0};
    cv_text_on(LC_X(0), 4, &AF_S, "SCENES  C4-F4", T_MID, T_SURF);
    for (s = 0; s < 4u; s++) {
        static const char NOTE[4] = {'C', 'D', 'E', 'F'};
        uint32_t used = (uint32_t)graph_project_used(s), now = s == cur, cued = s == tsc.cue;
        int32_t x = LC_X(s), lx;
        char l[2] = {(char)('A' + s), 0}, kn[3] = {NOTE[s], '4', 0};
        uint16_t fill = T_SURF, ink = used ? T_THEME : T_DIM, key = T_MID;
        if (now || (cued && scene_blink())) {           /* the scene now, the cued one lit in turn: filled */
            fill = now ? T_THEME : T_ACCENT;
            ink = key = now ? T_INK : T_BG;
            cv_rrect(x, SB_Y, LC_W, SB_H, 4, fill, T_SURF);
        } else if (used || cued) {                      /* stored (cued, dark in turn): outlined */
            cv_rrect(x, SB_Y, LC_W, SB_H, 4, cued ? T_ACCENT : T_MID, T_SURF);
            cv_rrect(x + 1, SB_Y + 1, LC_W - 2, SB_H - 2, 3, T_SURF, cued ? T_ACCENT : T_MID);
        } else {                                        /* empty: dim, dashed */
            scene_dash(x, SB_Y, LC_W, SB_H, T_DIM);
        }
        cv_text_on(x + 5, SB_Y + 4, &AF_S, kn, key, fill);
        lx = cv_text_on(x + 6, SB_Y + SB_H - 5 - AF_M_CAP_H - AF_M_CAP_Y, &AF_M, l, ink, fill);
        if (now && scene_edited())                      /* edited since stored: a dot after the letter */
            cv_rect(lx + 2, SB_Y + SB_H - 5 - 4, 3, 3, ink);
    }
    lcell(LC_X(0), SB_Y + SB_H + 4, 34, n, ICON_ORDER, 0, "ADD", tilt_addable() ? LS_OFF : LS_DIM, 0);
}

/* -------------------------------------------------------- START SCREEN --- */
/* MENU > SYSTEM > START SCREEN ON (the default; ui.c PREF_START_OFF): after the splash, before the music, a choice
 * (main.c: tilt_start_boot in place of autosave_boot). Nothing is loaded until one is made, and the autosave's
 * baseline is the music as it stands (project.c autosave_peek / autosave_rebase): nothing is written until it changes.
 *   F3 CONTINUE  the autosave, as RESTORE LAST loads it (autosave_boot: its baseline the loaded session, so continuing
 *                without an edit writes nothing). Shown only when there is one that loads (RESTORE LAST ON) and the
 *                last boot did not crash, hang or fail (as autosave_boot's `allowed`); after a crash with one there:
 *                "LAST SESSION SKIPPED - CRASH" in its place. The selection starts on it; else on BLANK
 *   G3 BLANK     the power-on set as it is (Felucca's: ANALOG ACID, FM6 PAD, LOFI PULSE LD, DRUM KIT; empty patterns,
 *                120 BPM): what RESTORE LAST OFF gives
 *   A3 NEW SONG  the NEW SONG screen (OCT- there: back here); its START is the choice, not an edit
 *   B3 LOAD      SAVE > PROJECT (a slot, KNOB 3 LOAD, OCT+); the project loaded is the choice, not an edit
 * BLANK, NEW SONG and LOAD keep the last session's autosave as it is until the music changes after the choice: BLANK's
 * baseline is the power-on set; NEW SONG's START and a LOAD while tilt_start_pending reset it to what they made
 * KNOB 1, SELECT or PRESETS move the selection, OCT+ takes it; a white key takes its own at once. Nothing else does
 * anything here: the keys are silent (name_on), no layer, no REC, no MENU (ui_input.c), the knobs only select */
enum { SO_CONT, SO_BLANK, SO_NEW, SO_LOAD, SO_JAM, SO_N };   /* (SO_JAM: TFM's TODAY'S JAM; not in CLASSIC) */
static const char *const SO_NAME[SO_N] = {"CONTINUE", "BLANK", "NEW SONG", "LOAD", "TODAY'S JAM"};
static const char *const SO_HINT[SO_N] = {"THE LAST SESSION", "EMPTY TRACKS", "ROLL ONE", "A SAVED SCENE", "SENT FROM TILT"};
/* the options' places on F3 G3 A3 B3: CLASSIC CONTINUE BLANK NEW LOAD; TFM NEW CONTINUE TODAY'S JAM SONGS */
static const uint8_t SO_ORD_C[4] = {SO_CONT, SO_BLANK, SO_NEW, SO_LOAD};
static const uint8_t SO_ORD_T[4] = {SO_NEW, SO_CONT, SO_JAM, SO_LOAD};
static const uint8_t *so_ord(void) { return tu_classic() ? SO_ORD_C : SO_ORD_T; }
static struct {
    uint8_t has, crash, pick;                           /* an autosave to continue; skipped after a crash; the selection */
    uint32_t sig;
} st;

static int so_shown(uint32_t o)
{
    if (!tu_classic())                                  /* (TFM: no BLANK; TODAY'S JAM only when TILT sent one) */
        return o == SO_CONT ? st.has && !st.crash : o == SO_JAM ? tfm_jam_has != 0u : o == SO_NEW || o == SO_LOAD;
    return o == SO_CONT ? st.has && !st.crash : o < SO_JAM;
}
static void tilt_start_boot(int clean)
{
    st.has = (uint8_t)autosave_peek(clean);             /* (after a crash: only that one is there, never parsed) */
    st.crash = (uint8_t)(!clean && st.has);
    st.pick = so_shown(SO_CONT) ? SO_CONT : tu_classic() ? SO_BLANK : SO_NEW;
    tilt_ns_open = TILT_START;
    ui.force = 1;
}
static void st_do(uint32_t o)
{
    if (!so_shown(o))
        return;
    tilt_ns_open = 0;
    go_home();
    ui.force = 1;
    autosave_rebase();                                  /* (the power-on set: nothing written until a change) */
    if (o == SO_CONT || o == SO_BLANK)                  /* (no load or START to come as the choice: a NEW SONG left */
        tilt_start_pending = 0;                         /* with OCT- before this keeps it no more) */
    if (o == SO_CONT) {
        if (!autosave_boot(1))                          /* (it said RESTORED; its baseline: the session loaded; and its */
            ui_message("NOTHING TO CONTINUE");          /* scene, project.c scene_boot) */
    } else if (o == SO_BLANK) {
        scene_claim();                                  /* (an empty scene, if there is one: project.c) */
        ui_message("BLANK");
    } else if (o == SO_NEW) {
        scene_claim();
        tilt_start_pending = 1;
        ns_from_start = 1;
        tilt_ns_show();
    } else if (o == SO_JAM) {                           /* TFM-UI: today's jam: its sounds, key, scale, tempo, played once */
        scene_claim();
        tilt_start_pending = 1;
        ns.s = tfm_jam_s;
        tfm_jam_has = 0;
        ns_start();
        ui_message("TODAY'S JAM");
    } else if (!tu_classic()) {                         /* TFM-UI: SONGS */
        tilt_start_pending = 1;
        tu_goto(TU_SONGS);
    } else {
        uint32_t i;
        tilt_start_pending = 1;
        for (i = 0; i < NPAGES && PAGES[i].graph != GR_SLOTS; i++)
            ;
        ui.home = 0;
        ui.page = (uint8_t)i;
        ui_message("A SCENE, THEN [K3] LOAD");
    }
}
static void st_move(int32_t s)
{
    const uint8_t *ord = so_ord();
    uint32_t i, at = 0;
    int32_t q;
    for (i = 0; i < 4u; i++)
        if (ord[i] == st.pick)
            at = i;
    for (q = (int32_t)at + (s > 0 ? 1 : -1); q >= 0 && q < 4; q += s > 0 ? 1 : -1)   /* the next shown one that way */
        if (so_shown(ord[q])) {                         /* (they stop at the ends) */
            st.pick = ord[q];
            return;
        }
}
static void st_input(uint32_t pressed, uint32_t notes, uint32_t oct)
{
    uint32_t k, p;
    int32_t s;
    (void)pressed;                                      /* (PLAY, REC, the page buttons: nothing here) */
    song.grid = 2;
    for (k = 0; k < 27u; k++)
        if (((notes >> k) & 1u) && !key_black(k) && (p = key_place(k)) < 4u && so_shown(so_ord()[p])) {
            enc_drop();
            st_do(so_ord()[p]);
            return;
        }
    if ((s = panel_enc(EN_SELECT)) != 0 || (s = panel_enc(EN_K1)) != 0 || (s = panel_enc(EN_PRESET)) != 0)
        st_move(s);
    enc_drop();
    if (oct & 2u)
        st_do(st.pick);
}
static uint32_t st_leds(void)                           /* the shown options' keys lit */
{
    uint32_t o, m = 0;
    static const uint8_t KEY[4] = {0, 2, 4, 6};         /* (F3 G3 A3 B3) */
    for (o = 0; o < 4u; o++)
        if (so_shown(so_ord()[o]))
            m |= 1u << KEY[o];
    return m;
}
/* the look (attract mode): the whole screen but the footer, no header. "TILT" in the splash's dot matrix, smaller (5 px
 * dots 6 px apart, 137 x 41 at y 18), and the credit under it; the four rows from ST_ROW_Y: the selected one's name amber
 * between two pointers, its hint a "PRESS START" flashing (2 Hz); amber where the palette has colour, its ink on GREY
 * and MONO (the TIMER's rule) */
#define ST_DOT 5u
#define ST_PITCH 6u
#define ST_WY 18u
#define ST_ROW_Y 108
#define ST_ROW_H 22
static int st_grey(void) { return settings.palette == UI_GREY_INDEX || settings.palette == UI_BW_INDEX; }
static void st_word(void)
{
    uint32_t col, row, x0 = (240u - (SPL_COLS * ST_PITCH - (ST_PITCH - ST_DOT))) / 2u;
    uint16_t lit = st_grey() ? T_TEXT : SPL_AMBER, unlit = st_grey() ? T_RAISE : SPL_AMBER_DIM;
    for (col = 0; col < SPL_COLS; col++) {
        uint32_t ch = col / 6u, cx = col % 6u;
        for (row = 0; row < 7u; row++) {
            uint32_t x = x0 + col * ST_PITCH, y = ST_WY + row * ST_PITCH;
            uint16_t c = cx < 5u && (SPLASH_TILT[ch][row] >> (4u - cx)) & 1u ? lit : unlit;
            lcd_fill(x + 1u, y, ST_DOT - 2u, ST_DOT, c);
            lcd_fill(x, y + 1u, ST_DOT, ST_DOT - 2u, c);
        }
    }
    draw_text_line(0, ST_WY + 7u * ST_PITCH + 10u, 240, &AF_S, "TILT-FM " TILT_VERSION " / Felucca " FELUCCA_VERSION, T_MID, T_BG, 1);
}
static void st_pointer(int32_t x, int32_t y, int right, uint16_t c)   /* a 4 x 7 triangle, its base column at x: */
{                                                                      /* right, pointing right (its point x + 3) */
    int32_t j;
    for (j = 0; j < 4; j++)
        cv_rect(right ? x + j : x - j, y + j, 1, 7 - 2 * j, c);
}
static void st_draw(void)
{
    uint32_t blink = (fm1_ms / 250u) & 1u, o;
    if (!tu_classic()) {                                /* TFM-UI: the drawing (tfm_dice.c) */
        tu_start_draw();
        return;
    }
    uint32_t sig = st.pick * 7u + st.has * 3u + st.crash * 5u + blink * 11u + ux.gen * 7919u;
    uint16_t amber = st_grey() ? T_TEXT : SPL_AMBER;
    if (ui.force) {
        lcd_fill(0, 0, 240, Y_FOOT, T_BG);
        st_word();
        st.sig = ~sig;
    }
    if (sig != st.sig) {
        char n[3] = {0, '3', 0};
        khint_t a[3];
        st.sig = sig;
        for (o = 0; o < SO_N; o++) {
            uint32_t sel = o == st.pick, on = so_shown(o);
            uint16_t bg = sel ? T_SURF : T_BG;
            int32_t y = CAP_IN(S, ST_ROW_H);
            n[0] = W_NOTE[o];
            cv_begin(240, ST_ROW_H, T_BG);
            if (sel)
                cv_rrect(3, 0, 234, ST_ROW_H, 4, T_SURF, T_BG);
            if (on) {
                cv_text_on(9, y, &AF_S, n, T_DIM, bg);
                if (sel) {
                    st_pointer(27, (ST_ROW_H - 7) / 2, 1, amber);                               /* (a "▶" */
                    st_pointer(36 + text_w(&AF_S, SO_NAME[o]) + 7, (ST_ROW_H - 7) / 2, 0, amber);   /* .. "◀") */
                }
                cv_text_on(36, y, &AF_S, SO_NAME[o], sel ? amber : T_TEXT, bg);
                if (!sel)
                    cv_text_r(231, y, &AF_S, SO_HINT[o], T_MID, bg);
                else if (!blink)
                    cv_text_r(231, y, &AF_S, "PRESS START", amber, bg);
            } else if (o == SO_CONT && st.crash) {
                GFX_HOOK_ALIGN(0, 0, 240, ST_ROW_H, AL_HV, "start crash line centred");
                cv_text_in(0, y, 240, &AF_S, "LAST SESSION SKIPPED - CRASH", T_ACCENT, bg);
            }
            cv_blit(0, (uint32_t)(ST_ROW_Y + (int32_t)o * ST_ROW_H));
        }
        a[0] = (khint_t){KC_OCTUP, "START"};
        a[1] = (khint_t){KC_K1, "PICK"};
        a[2] = (khint_t){KC_KEYS, "F3 - B3"};
        cv_begin(240, H_FOOT, T_BG);
        cv_key_row(8, 232, 9, a, 3, 7u, T_BG);
        cv_blit(0, Y_FOOT);
    }
    ui.force = 0;
}

#include "tfm_ui.c"                                        /* TFM-UI: TFM's screens, the button map */
#include "tfm_dice.c"                                      /* TFM-UI: START, DICE, SETUP, TIME */
#include "tfm_faces.c"                                     /* TFM-UI: the instruments' faces, SOUND */
#include "tfm_seq.c"                                       /* TFM-UI: STEP, NOTES, DRUMS */
#include "tfm_song.c"                                      /* TFM-UI: SCENES, SONG, SONGS */
#include "tfm_pages.c"                                     /* TFM-UI: FX, SPACE, ARP, CLOCK, MOTION */
