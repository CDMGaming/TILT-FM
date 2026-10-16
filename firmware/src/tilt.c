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
    tilt_ns_open = 1;
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

static void tilt_ns_input(uint32_t pressed, uint32_t notes, uint32_t oct)
{
    uint32_t k, p;
    int32_t s;
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
    else if (oct & 1u)
        name_close();                                   /* back: nothing changed */
    else if ((pressed >> panel.btn[B_PLAY]) & 1u)
        ns_start();
}

static uint32_t tilt_ns_leds(void)                       /* the held rows' keys lit */
{
    uint32_t k, m = 0;
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
static void tilt_ns_draw(void)
{
    uint8_t st[8 + 2 * NTRK] = {ns.row, ns.hold, ns.root, ns.scale, (uint8_t)ns.bpm, (uint8_t)(ns.bpm >> 8), ns.again,
                                tilt_timer_min};
    uint32_t r, sig;
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
static void tilt_timer_start(void) { }

/* --------------------------------------------------------- the layer --- */
static void tilt_key(uint32_t k, uint32_t p)
{
    if (key_black(k))
        return;
    if (p == TT_NEW)
        tilt_ns_show();
    else if (p == TT_STUCK)
        tilt_stuck();
}
static void tilt_knob(uint32_t k, int32_t s) { (void)k; (void)s; }
static uint32_t tilt_can(uint32_t k, uint32_t p) { return !key_black(k) && (p == TT_NEW || p == TT_STUCK) && !chain_busy(); }
static uint32_t tilt_on(uint32_t k, uint32_t p) { (void)k; (void)p; return 0; }
static uint32_t tilt_sig(void) { return (tilt_used(TSEL) != 0u) * 31u + (uint32_t)chain_busy() * 3u + song.sel * 977u; }

static void tilt_cards(void)
{
    uint32_t c;
    for (c = 0; c < 4u; c++)
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
    b[0] = 'T';
    b[1] = (char)('1' + song.sel);
    str_cpy(b + 2, chain_busy() ? ": STOP TO EDIT" : tilt_used(TSEL) ? ": COOK IT" : ": RANDOM", sizeof b - 2);
    fill = lc_box(6, 100, 228, 18, lc_fill(st, &ink));
    GFX_HOOK_ALIGN(6, 100, 234, 118, AL_HV, "tilt layer stuck line centred");
    cv_text_in(6, 100 + CAP_IN(S, 18), 228, &AF_S, b, ink, fill);
}
