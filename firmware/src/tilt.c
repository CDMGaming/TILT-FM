/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
 * Copyright (C) 2026 CDM Gaming (TILT-FM) */
/* TILT-FM: TILT's quick-start ideas inside Felucca's UI. Included at the end of ui_layer.c (every build that has the
 * layers has this: the firmware, the browser emulator, the host tests).
 * The TILT layer: ENV held (its tap is still ENV's page), a SET layer as SEQ TOOLS: OCT- puts back the selected
 * track's sequence as the layer opened, SAVE held undoes the last action. The white keys from F3:
 *   F3 NEW SONG  the NEW SONG screen (below)
 *   G3 STUCK     the selected track: COOK if it holds something (a NOTE step, a drum hit), else RANDOM: the SEQ TOOLS
 *                actions themselves (ui_tools.c), in the track's scale (ROOT, SCL), one undo each */
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
/* A screen of its own (ui.c tilt_ns_open: name_on() with it): KEY, SCALE, TEMPO and the four tracks' sounds (an engine
 * and one of its factory presets), a row each, F3 .. E4 the rows' keys:
 *   a white key F3 .. E4   HOLD its row (lit: held) and select it; again: let go
 *   SELECT                 the row; KNOB 1 its value (a track: any factory sound, in PRESETS order)
 *   OCT+  NUDGE            every row not held rolled again
 *   PLAY  START            the project made from it: the sounds loaded (as PRESETS does: one undo a track), every
 *                          track's ROOT and SCL, the tempo; the transport starts. With the TIMER set the steps and the
 *                          automation of every track go too (a fresh project: PLAY twice when there was something
 *                          to clear); without it they stay, as a sound load leaves them
 *   OCT-                   back, nothing changed
 * The rolls know the tracks' roles: T1 a bass, T2 and T3 a lead, pad, pluck or keys (not the same sound), T4 a drum kit
 * (DRUM, SLICE); the scale never chromatic; the tempo 75 .. 145. Factory presets only (a setup is then the same
 * everywhere: room for a short code later). The first open rolls everything; the values and the holds stay between
 * opens, and each open is a NUDGE */
enum { NS_KEY, NS_SCALE, NS_BPM, NS_T1, NS_ROWS = NS_T1 + NTRK };
#define TILT_NS 1u                                      /* tilt_ns_open: NEW SONG; TIME (below) */
#define TILT_TIME 2u
#define NS_BPM_LO 75
#define NS_BPM_HI 145
#define NS_ROW_H 24                                     /* 7 rows from Y_LABEL: 28 .. 196, the footer at 202 */
#define NS_AGAIN_MS 3000u
static struct {
    uint8_t row, hold;                                  /* the selected row; bit r: row r held (NUDGE leaves it) */
    uint8_t root, scale, rolled;                        /* (rolled: once, the first open) */
    int16_t bpm;
    uint8_t eng[NTRK], pre[NTRK];
    uint8_t again;                                      /* START pressed once with steps to clear (timed) */
    uint32_t again_t, sig;
} ns;
static const char *const NS_ROLE[NTRK] = {"BASS", "MELODY", "MELODY", "DRUMS"};
static uint8_t ns_from_start;                           /* (opened from the start screen: OCT- goes back there) */
static const uint8_t NS_SCALES[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 12};   /* MAJ MIN DOR MIX PEN MPEN HARM PHRY LYD BLUES */

/* sound (e, k) fits track t's role: T4 a kit of the engines that are kits (DRUM, SLICE: category.c cat_of_engine; not
 * PHYS's HAND DRUM, a DRUM-listed pitched sound with no grid) */
static int ns_role_ok(uint32_t t, uint32_t e, uint32_t k)
{
    uint32_t cat = preset_cat(e, k);
    if (t == NTRK - 1u)
        return cat_of_engine(e) == CAT_DRUM;
    if (cat_of_engine(e) == CAT_DRUM)
        return 0;
    if (t == 0u)
        return cat == CAT_BASS;
    return cat == CAT_LEAD || cat == CAT_PAD || cat == CAT_PLUCK || cat == CAT_KEYS;
}
/* the factory sounds in PRESETS order (engines.c ENGINE_ORDER, the aliases skipped): the n-th as *e, *k; 1 = found */
static int ns_fact_at(uint32_t n, uint32_t *e, uint32_t *k)
{
    uint32_t r, i;
    for (r = 0; r < NENG_SHOWN; r++) {
        const engine_t *en = ENGINES[eng_vis(r)];
        for (i = 0; i < en->npresets; i++)
            if (preset_orig(en, i) == i && !n--) {
                *e = eng_vis(r);
                *k = i;
                return 1;
            }
    }
    return 0;
}
static uint32_t ns_fact_n(void)
{
    uint32_t r, n = 0;
    for (r = 0; r < NENG_SHOWN; r++)
        n += preset_shown(eng_vis(r));
    return n;
}
static uint32_t ns_fact_pos(uint32_t e, uint32_t k)    /* (not in the list: 0) */
{
    uint32_t n, ne, nk, total = ns_fact_n();
    for (n = 0; n < total; n++)
        if (ns_fact_at(n, &ne, &nk) && ne == e && nk == k)
            return n;
    return 0;
}
/* track t's sound at random among its role's */
static void ns_roll_track(uint32_t t)
{
    uint32_t total = ns_fact_n(), n, c = 0, pick, e = 0, k = 0, tries;
    for (n = 0; n < total; n++)
        if (ns_fact_at(n, &e, &k) && ns_role_ok(t, e, k))
            c++;
    for (tries = 0; c && tries < 4u; tries++) {
        pick = rng() % c;
        for (n = 0; n < total; n++)
            if (ns_fact_at(n, &e, &k) && ns_role_ok(t, e, k) && !pick--)
                break;
        ns.eng[t] = (uint8_t)e;
        ns.pre[t] = (uint8_t)k;
        if (t != 2u || ns.eng[2] != ns.eng[1] || ns.pre[2] != ns.pre[1])   /* (T2, T3: two different sounds) */
            break;
    }
}
static void ns_roll(uint32_t r)
{
    if (r == NS_KEY)
        ns.root = (uint8_t)(rng() % 12u);
    else if (r == NS_SCALE)                             /* (the ten that start a song well: params.c N_SCALE, not CHR, */
        ns.scale = NS_SCALES[rng() % sizeof NS_SCALES]; /* LOC, MEL, WHOLE and the diminished; KNOB 1 has all) */
    else if (r == NS_BPM)
        ns.bpm = (int16_t)(NS_BPM_LO + (int32_t)(rng() % (uint32_t)(NS_BPM_HI - NS_BPM_LO + 1)));
    else if (r < NS_ROWS)
        ns_roll_track(r - NS_T1);
}
static void ns_nudge(void)
{
    uint32_t r;
    for (r = 0; r < NS_ROWS; r++)
        if (!((ns.hold >> r) & 1u))
            ns_roll(r);
    ns.again = 0;
}

static void tilt_ns_show(void)
{
    if (chain_busy()) {
        ui_message("STOP THE SONG FIRST");
        return;
    }
    if (!ns.rolled) {                                   /* (the generator: from the time of the first open) */
        rng_state ^= fm1_ms * 2654435761u | 1u;
        ns.rolled = 1;
        ns.hold = 0;
    }
    ns_nudge();
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
static void ns_start(void)
{
    uint32_t i;
    int timed = tilt_timer_min != 0u;
    char b[24];
    const char *unit;
    if (chain_busy()) {
        ui_message("STOP THE SONG FIRST");
        return;
    }
    if (timed && ns_has_steps() && !(ns.again && fm1_ms - ns.again_t < NS_AGAIN_MS)) {
        ns.again = 1;                                   /* (a fresh project clears every track: PLAY again) */
        ns.again_t = fm1_ms;
        ui_message("PLAY AGAIN: CLEARS ALL");
        return;
    }
    ns.again = 0;
    for (i = 0; i < NTRK; i++) {
        track_t *t = &trk[i];
        if (t->eng_req != ns.eng[i])
            set_engine_of(t, ns.eng[i]);
        apply_preset_to(t, ns.pre[i]);
        t->p[P_ROOT] = ns.root;
        t->p[P_SCALE] = ns.scale;
        if (timed) {                                    /* (as TRACK CLEAR and AUTOMATION CLEAR do) */
            load_begin(t, UNDO_PAT);
            track_defaults_steps(t);
            motion_clear(t);
            load_end(t);
            t->nheld = 0;
            t->arp_phys = 0;
        }
    }
    song.g[G_BPM] = ns.bpm;
    tilt_ns_open = 0;
    go_home();
    ui.force = 1;
    param_format(&TP[P_ROOT], ns.root, b, &unit);
    str_cpy(b + str_len(b), " ", 2);
    str_cpy(b + str_len(b), TP[P_SCALE].names[ns.scale], 8);
    str_cpy(b + str_len(b), " ", 2);
    fmt_int(b + str_len(b), ns.bpm);
    ui_say("NEW SONG ", b);
    ui.msg_t = 120;                                     /* (~2 s: longer than a message, it says what was made) */
    if (!song.playing && !seq_counting() && transport_req != 1u)
        transport_req = 1;
    if (timed)
        tilt_timer_start();
    if (tilt_start_pending)                             /* (the start screen's NEW SONG: this is the baseline; nothing */
        autosave_rebase();                              /* written until it changes) */
    ns_from_start = 0;
}

static void ns_knob(int32_t s)
{
    uint32_t r = ns.row, t, total, n, e, k;
    if (r == NS_KEY) {
        ns.root = (uint8_t)(((int32_t)ns.root + s % 12 + 12) % 12);
    } else if (r == NS_SCALE) {
        int32_t m = TP[P_SCALE].max + 1;
        ns.scale = (uint8_t)(((int32_t)ns.scale + s % m + m) % m);
    } else if (r == NS_BPM) {
        ns.bpm = (int16_t)clamp(ns.bpm + accel(EN_K1, s, GP[G_BPM].max - GP[G_BPM].min), GP[G_BPM].min, GP[G_BPM].max);
    } else {
        t = r - NS_T1;
        total = ns_fact_n();
        n = (ns_fact_pos(ns.eng[t], ns.pre[t]) + (uint32_t)(s % (int32_t)total + (int32_t)total)) % total;
        if (ns_fact_at(n, &e, &k)) {
            ns.eng[t] = (uint8_t)e;
            ns.pre[t] = (uint8_t)k;
        }
    }
    ns.again = 0;
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
    song.grid = 2;                                      /* (silent keys: keys_mode, as NAME's) */
    for (k = 0; k < 27u; k++)
        if (((notes >> k) & 1u) && !key_black(k) && (p = key_place(k)) < NS_ROWS) {
            ns.hold ^= (uint8_t)(1u << p);
            ns.row = (uint8_t)p;
        }
    if ((s = panel_enc(EN_SELECT)) != 0)
        ns.row = (uint8_t)clamp((int32_t)ns.row + s, 0, NS_ROWS - 1);
    if ((s = panel_enc(EN_K1)) != 0)
        ns_knob(s);
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
        if (!key_black(k) && key_place(k) < NS_ROWS && ((ns.hold >> key_place(k)) & 1u))
            m |= 1u << k;
    return m;
}

/* a row: its key ("F3"), the name, the value at the right; the selected row on SURF, a held one's value THEME with a
 * lock (THEME: amber on the AMBER palette) */
static void ns_value(uint32_t r, char *b)              /* (b holds 28) */
{
    const char *unit;
    if (r == NS_KEY) {
        param_format(&TP[P_ROOT], ns.root, b, &unit);
    } else if (r == NS_SCALE) {
        str_cpy(b, TP[P_SCALE].names[ns.scale], 8);
    } else if (r == NS_BPM) {
        fmt_int(b, ns.bpm);
        str_cpy(b + str_len(b), " BPM", 5);
    } else {
        const engine_t *e = ENGINES[ns.eng[r - NS_T1] % NENGINES];
        str_cpy(b, e->name, 12);
        str_cpy(b + str_len(b), " ", 2);
        if (e->npresets)
            str_cpy(b + str_len(b), e->presets[ns.pre[r - NS_T1] % e->npresets].name, 16);
    }
}
static void ns_draw_row(uint32_t r)
{
    static const char *const LBL[NS_T1] = {"KEY", "SCALE", "TEMPO"};
    uint32_t sel = r == ns.row, held = (ns.hold >> r) & 1u;
    uint16_t bg = sel ? T_SURF : T_BG;
    int32_t y = CAP_IN(S, NS_ROW_H);
    char n[3] = {W_NOTE[r], (char)(r < 4u ? '3' : '4'), 0}, b[28];
    cv_begin(240, NS_ROW_H, T_BG);
    if (sel)
        cv_rrect(3, 1, 234, NS_ROW_H - 2, 4, T_SURF, T_BG);
    cv_text_on(9, y, &AF_S, n, held ? T_THEME : T_DIM, bg);
    if (r < NS_T1) {
        cv_text_on(31, y, &AF_S, LBL[r], T_MID, bg);
    } else {
        b[0] = 'T';
        b[1] = (char)('1' + r - NS_T1);
        b[2] = 0;
        cv_text_on(31, y, &AF_S, b, T_MID, bg);
        cv_text_on(48, y, &AF_S, NS_ROLE[r - NS_T1], T_DIM, bg);
    }
    ns_value(r, b);
    cv_text_r(held ? 214 : 231, y, &AF_S, b, held ? T_THEME : sel ? T_TEXT : T_MID, bg);
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
    b[0] = (khint_t){KC_SELECT, "ROW"};
    b[1] = (khint_t){KC_K1, "CHANGE"};
    b[2] = (khint_t){KC_KEYS, "HOLD"};
    cv_begin(240, H_FOOT, T_BG);
    cv_key_row(8, 232, 2, a, 3, 7u, T_BG);
    cv_key_row(8, 232, 21, b, 3, 7u, T_BG);
    cv_blit(0, Y_FOOT);
}
static void te_draw(void);
static void tilt_ns_draw(void)
{
    uint8_t st[8 + 2 * NTRK] = {ns.row, ns.hold, ns.root, ns.scale, (uint8_t)ns.bpm, (uint8_t)(ns.bpm >> 8), ns.again,
                                tilt_timer_min};
    uint32_t r, sig;
    if (tilt_ns_open == TILT_TIME) {
        te_draw();
        return;
    }
    if (tilt_ns_open == TILT_START) {
        st_draw();
        return;
    }
    for (r = 0; r < NTRK; r++) {
        st[8 + r] = ns.eng[r];
        st[8 + NTRK + r] = ns.pre[r];
    }
    if (ns.again && fm1_ms - ns.again_t >= NS_AGAIN_MS)
        ns.again = 0;
    sig = fnv(2166136261u, st, sizeof st) + ux.gen * 7919u;
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
static void tilt_timer_poll(void)
{
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
static const char *const TO_HINT[TO_N] = {"FROM THE TOP", "TO A PROJECT SLOT", "+5 MIN", "TIMER OFF"};
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
    } else if (o == TO_SAVE) {
        for (i = 0; i < NPAGES && PAGES[i].graph != GR_SLOTS; i++)
            ;
        ui.home = 0;
        ui.page = (uint8_t)i;
        ui_message("A SLOT, THEN [K4] SAVE");
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

/* the map: F3 NEW SONG, G3 STUCK, and under them what STUCK will do to the selected track */
static void layer_tilt(void)
{
    uint32_t st = chain_busy() ? LS_DIM : LS_OFF;
    char n[3] = {'F', 0, 0}, b[24];
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
enum { SO_CONT, SO_BLANK, SO_NEW, SO_LOAD, SO_N };
static const char *const SO_NAME[SO_N] = {"CONTINUE", "BLANK", "NEW SONG", "LOAD"};
static const char *const SO_HINT[SO_N] = {"THE LAST SESSION", "EMPTY TRACKS", "ROLL ONE", "A SAVED PROJECT"};
static struct {
    uint8_t has, crash, pick;                           /* an autosave to continue; skipped after a crash; the selection */
    uint32_t sig;
} st;

static int so_shown(uint32_t o) { return o == SO_CONT ? st.has && !st.crash : o < SO_N; }
static void tilt_start_boot(int clean)
{
    st.has = (uint8_t)autosave_peek(clean);             /* (after a crash: only that one is there, never parsed) */
    st.crash = (uint8_t)(!clean && st.has);
    st.pick = so_shown(SO_CONT) ? SO_CONT : SO_BLANK;
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
        if (!autosave_boot(1))                          /* (it said RESTORED; its baseline: the session loaded) */
            ui_message("NOTHING TO CONTINUE");
    } else if (o == SO_BLANK) {
        ui_message("BLANK");
    } else if (o == SO_NEW) {
        tilt_start_pending = 1;
        ns_from_start = 1;
        tilt_ns_show();
    } else {
        uint32_t i;
        tilt_start_pending = 1;
        for (i = 0; i < NPAGES && PAGES[i].graph != GR_SLOTS; i++)
            ;
        ui.home = 0;
        ui.page = (uint8_t)i;
        ui_message("A SLOT, THEN [K3] LOAD");
    }
}
static void st_move(int32_t s)
{
    uint32_t p = st.pick, i;
    for (i = 0; i < SO_N; i++) {                        /* the next shown one that way (they stop at the ends) */
        int32_t q = (int32_t)p + (s > 0 ? 1 : -1);
        if (q < 0 || q >= SO_N)
            return;
        p = (uint32_t)q;
        if (so_shown(p)) {
            st.pick = (uint8_t)p;
            return;
        }
    }
}
static void st_input(uint32_t pressed, uint32_t notes, uint32_t oct)
{
    uint32_t k, p;
    int32_t s;
    (void)pressed;                                      /* (PLAY, REC, the page buttons: nothing here) */
    song.grid = 2;
    for (k = 0; k < 27u; k++)
        if (((notes >> k) & 1u) && !key_black(k) && (p = key_place(k)) < SO_N && so_shown(p)) {
            enc_drop();
            st_do(p);
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
    for (o = 0; o < SO_N; o++)
        if (so_shown(o))
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
    draw_text_line(0, ST_WY + 7u * ST_PITCH + 10u, 240, &AF_S, "TILT-FM / Felucca " FELUCCA_VERSION, T_MID, T_BG, 1);
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
