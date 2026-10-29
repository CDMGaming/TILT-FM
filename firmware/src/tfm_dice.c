/* TFM-UI phase 2 (tfm-spec/TFM-UI-SPEC.md 3): START, DICE, SETUP and TIME, the modal screens of tilt.c in TFM's look.
 * tilt.c keeps their logic (the options, the rolls, the autosave protections); in TFM mode (not CLASSIC) it calls what
 * is here. Included from tilt.c after tfm_ui.c.
 *
 * The saved setup (SETUP): 16 bytes of favorites.factory[13] (SLICE's row: its favourites use the first byte), so the
 * settings record is unchanged; 0 in every byte is every default (settings from before: the defaults).
 *   rules T1..T4   0 default, else option + 1: 0 RANDOM, 1..6 TYPE (DRUMS BASS CHORDS LEAD PAD FX), 7.. one INSTRUMENT
 *   KEY            0 random, else key + 1          SCALE   0 random (no chromatic), else TILT scale + 1
 *   TEMPO          low, high (0: 90 .. 130)         TIMER   the default minutes' step (0 OFF, 5, 10, 15, 20) */
#define CFG(i) (favorites.factory[13][16 + (i)])
enum { CF_RULE = 0, CF_KEY = 4, CF_SCALE = 5, CF_LO = 6, CF_HI = 7, CF_TIMER = 8 };
#define RO_N (1u + 6u + TI_N)
static const char *const TY_NAME[6] = {"DRUMS", "BASS", "CHORDS", "LEAD", "PAD", "FX"};
static const uint8_t TY_INS[TI_N] = {      /* the types an instrument is (bits: DRUMS BASS CHORDS LEAD PAD FX) */
    1,        /* THUMP LAB */
    2,        /* SQUELCH */
    2,        /* WOBBLER */
    4,        /* CHORDETTE */
    8,        /* RAZOR */
    16,       /* HAZE */
    8,        /* PIXEL */
    4,        /* TINE */
    8 | 32,   /* GLINT */
    4 | 8,    /* PLUCK */
    32 | 16,  /* VOX */
    4 | 16};  /* WHEELHOUSE */
static const uint8_t RULE_DEF[NTRK] = {1, 2, 3, 4};   /* a band: DRUMS, BASS, CHORDS, LEAD */
static uint32_t cfg_opt(uint32_t i) { return CFG(CF_RULE + i) ? (uint32_t)CFG(CF_RULE + i) - 1u : RULE_DEF[i % NTRK]; }
static void cfg_rule_text(uint32_t opt, char *b)
{
    if (opt == 0u) {
        str_cpy(b, "RANDOM", 24);
    } else if (opt <= 6u) {
        str_cpy(b, "TYPE \xB7 ", 24);
        str_cpy(b + str_len(b), TY_NAME[opt - 1u], 8);
    } else {
        str_cpy(b, TFM_INS[(opt - 7u) % TI_N].name, 24);
    }
}
/* the scales' full names, in TILT's order (TILT_SCALE gives Felucca's value) */
static const char *const SC_FULL[11] = {"MAJOR", "MINOR", "DORIAN", "PHRYGIAN", "LYDIAN", "MIXOLYDIAN", "HARM MINOR",
                                        "MIN PENTA", "MAJ PENTA", "BLUES", "CHROMATIC"};
static const char *tu_scale_full(uint32_t v)
{
    uint32_t k;
    for (k = 0; k < 11u; k++)
        if (TILT_SCALE[k] == v)
            return SC_FULL[k];
    return TP[P_SCALE].names[v];
}
static const uint8_t TEMPO_R[6][2] = {{60, 90}, {80, 110}, {90, 130}, {110, 150}, {130, 170}, {60, 170}};
static uint32_t cfg_tempo_idx(void)
{
    uint32_t i;
    for (i = 0; i < 6u; i++)
        if (TEMPO_R[i][0] == (CFG(CF_LO) ? CFG(CF_LO) : 90u) && TEMPO_R[i][1] == (CFG(CF_HI) ? CFG(CF_HI) : 130u))
            return i;
    return 2u;
}
static void cfg_lohi(uint32_t *lo, uint32_t *hi)
{
    *lo = CFG(CF_LO) ? CFG(CF_LO) : 90u;
    *hi = CFG(CF_HI) ? CFG(CF_HI) : 130u;
}
static int cfg_default(void)
{
    uint32_t i;
    for (i = 0; i < 16u; i++)
        if (CFG(i))
            return 0;
    return 1;
}

/* a rule asks for the drum kit: TYPE DRUMS, or the instrument itself */
static int ro_asks_drums(uint32_t opt) { return opt == 1u || (opt >= 7u && (TY_INS[(opt - 7u) % TI_N] & 1u)); }
static int ins_is_kit(uint32_t ins) { return (TY_INS[ins % TI_N] & 1u) != 0u; }
/* RANDOM never rolls a second drum kit: another track has one (rolled before it, held, or its rule asks for it) */
static int kit_taken(const tfm_setup_t *s, uint32_t i)
{
    uint32_t j;
    for (j = 0; j < NTRK; j++) {
        if (j == i)
            continue;
        if (j < i || ((ns.hold >> (3u + j)) & 1u) ? ins_is_kit(s->ins[j]) : ro_asks_drums(cfg_opt(j)))
            return 1;
    }
    return 0;
}
/* a track's instrument by its rule; the same instrument and sound twice never */
static void tu_roll_track(tfm_rng_t *r, tfm_setup_t *s, uint32_t i)
{
    uint32_t opt = cfg_opt(i), k, tries, n, j, ins = TI_SQUELCH;
    for (tries = 0; tries < 8u; tries++) {
        if (opt == 0u) {                                /* RANDOM: any instrument, a second kit never */
            int nokit = kit_taken(s, i);
            for (j = 0, n = 0; j < TI_N; j++)
                n += !(nokit && ins_is_kit(j));
            k = tfm_pick(r, n ? n : 1u);
            for (j = 0; j < TI_N; j++)
                if (!(nokit && ins_is_kit(j)) && !k--) {
                    ins = j;
                    break;
                }
        } else if (opt <= 6u) {
            uint32_t bit = 1u << (opt - 1u);
            for (j = 0, n = 0; j < TI_N; j++)
                n += (TY_INS[j] & bit) != 0u;
            k = tfm_pick(r, n ? n : 1u);
            for (j = 0; j < TI_N; j++)
                if ((TY_INS[j] & bit) && !k--) {
                    ins = j;
                    break;
                }
        } else {
            ins = (opt - 7u) % TI_N;
        }
        s->ins[i] = (uint8_t)ins;
        s->pre[i] = (uint8_t)tfm_pick(r, TFM_INS[ins].nsnd);
        for (k = 0; k < NTRK && (k == i || s->ins[k] != s->ins[i] || s->pre[k] != s->pre[i]); k++)
            ;
        if (k == NTRK)
            break;
    }
}
/* the dice: everything not held rolled (the held rows' bits are ns.hold: 0 KEY, 1 SCALE, 2 TEMPO, 3..6 T1..T4) */
static void tu_dice_nudge(void)
{
    tfm_rng_t r;
    uint32_t i, keep = 0, lo, hi;
    r.s = rng();
    keep |= (ns.hold >> 0) & 1u ? 1u : 0u;
    keep |= (ns.hold >> 1) & 1u ? 2u : 0u;
    keep |= (ns.hold >> 2) & 1u ? 12u : 0u;
    tfm_roll_feel(&r, &ns.s, keep);
    if (!(keep & 1u) && CFG(CF_KEY))
        ns.s.key = (uint8_t)(CFG(CF_KEY) - 1u);
    if (!(keep & 2u)) {
        if (CFG(CF_SCALE))
            ns.s.scale = TILT_SCALE[(CFG(CF_SCALE) - 1u) % 11u];
        else if (ns.s.scale == 0u)                      /* (no chromatic by the dice) */
            ns.s.scale = TILT_SCALE[TILT_SW_SCALE[tfm_pick(&r, 10)]];
    }
    cfg_lohi(&lo, &hi);
    if (!(keep & 4u))
        ns.s.bpm = (uint16_t)tfm_int(&r, (int32_t)lo, (int32_t)hi);
    for (i = 0; i < NTRK; i++)
        if (!((ns.hold >> (3u + i)) & 1u))
            tu_roll_track(&r, &ns.s, i);
    ns.again = 0;
}

/* the TIMER's default from SETUP, when DICE opens and no TIMER runs (tilt.c tilt_ns_show) */
static uint8_t tfm_dice_timer(void)
{
    static const uint8_t TM_[5] = {0, 5, 10, 15, 20};
    if (!tm.on && CFG(CF_TIMER) % 5u)
        tilt_timer_min = TM_[CFG(CF_TIMER) % 5u];
    return tilt_timer_min;
}
/* ---- widgets ---- */
static void tu_text_sp(int32_t xc, int32_t ymid, const aafont_t *f, const char *s, int32_t sp, uint16_t fg, uint16_t bg)
{
    int32_t w = 0, x, i;
    for (i = 0; s[i]; i++) {
        char c[2] = {s[i], 0};
        w += text_w(f, c) + (s[i + 1] ? sp : 0);
    }
    x = xc - w / 2;
    for (i = 0; s[i]; i++) {
        char c[2] = {s[i], 0};
        tu_text(x, ymid, f, c, fg, bg);
        x += text_w(f, c) + sp;
    }
}
/* an option row (START, TIME): key, name, a hint at the right; the selected one amber */
static void tu_opt_row(int32_t y, int32_t h, const char *key, const char *name, const char *hint, int sel, uint16_t namec,
                       uint16_t hintc)
{
    uint16_t bg = sel ? TC_AMBER : TC_SURF;
    tu_rrect(12, y, 216, h, 6, bg, TC_BG);
    tu_text(20, y + h / 2, &AF_TS, key, sel ? TC_BG : TC_QUIET, bg);
    tu_text(40, y + h / 2, &AF_TL, name, sel ? TC_BG : namec, bg);
    tu_text_r(220, y + h / 2, &AF_TS, hint, sel ? TC_BG : hintc, bg);
}

/* ================================================================== START ===================================== */
static const char *const SO_TFM_NAME[SO_N] = {"CONTINUE", "BLANK", "NEW SONG", "SONGS", "TODAY'S JAM"};
static const char *const SO_TFM_HINT[SO_N] = {"THE LAST SESSION", "EMPTY TRACKS", "roll the dice", "load one", "sent from TILT"};
static const uint8_t SO_ROW[4] = {SO_NEW, SO_CONT, SO_JAM, SO_LOAD};   /* F3 NEW SONG, G3 CONTINUE, A3 TODAY'S JAM, B3 SONGS */
static void tu_start_title(void)
{
    tu_rect(0, 0, TU_W, 58, TC_BG);
    tu_text_sp(120, 30, &AF_PL, "TILT", 4, TC_AMBER, TC_BG);
    tu_text_sp(120, 53, &AF_TS, "INSERT IDEA \xB7 PRESS START", 1, TC_QUIET, TC_BG);
}
static void tu_start_rows(void)
{
    uint32_t i;
    int32_t y = 73;
    tu_rect(0, 58, TU_W, 157, TC_BG);
    for (i = 0; i < 4u; i++) {
        uint32_t o = SO_ROW[i], on = so_shown(o), sel = on && o == st.pick;
        char key[3] = {W_NOTE[i], '3', 0}, hint[32], pn[16];
        int32_t h = sel ? 26 : 23;
        if (!on) {
            if (o == SO_CONT && st.crash) {
                tu_text_c(120, y + 12, &AF_TS, "LAST SESSION SKIPPED - CRASH", TC_REC, TC_BG);
                y += 28;
            }
            continue;
        }
        str_cpy(hint, SO_TFM_HINT[o], sizeof hint);
        project_cur_name(pn);
        if (o == SO_CONT && pn[0]) {                    /* "NEON DRIFT · 3 scenes" */
            uint32_t n = 0, s;
            for (s = 0; s < NSCN; s++)
                n += (uint32_t)project_used(s);
            str_cpy(hint, pn, 14);
            str_cpy(hint + str_len(hint), " \xB7 ", 5);
            fmt_int(hint + str_len(hint), (int32_t)n);
            str_cpy(hint + str_len(hint), n == 1u ? " scene" : " scenes", 8);
        }
        tu_opt_row(y, h, key, SO_TFM_NAME[o], hint, (int)sel, TC_TEXT, o == SO_JAM ? TC_CYAN : TC_QUIET);
        y += h + 5;
    }
}
static void tu_start_draw(void)
{
    uint32_t sig = st.pick * 7u + st.has * 3u + st.crash * 5u + tfm_jam_has * 17u + 1u;
    tu_zone(2, 0, 58, TC_BG, 1u, tu_start_title);
    tu_zone(3, 58, 124, TC_BG, sig, tu_start_rows);
    tu_zone(4, 182, 33, TC_BG, 1u, tu_gap_draw);
    tu_foot(3, "OCT+", "START", "SELECT", "PICK", "F3-B3", "GO", 0, 0, 0, 0);
    ui.force = 0;
}

/* ================================================================== TIME ====================================== */
static const char *const TO_TFM_NAME[TO_N] = {"PLAY IT BACK", "KEEP AS A SONG", "EXTRA BALL", "KEEP WORKING"};
static const char *const TO_TFM_HINT[TO_N] = {"from the top", "name it", "+5 min", "no clock"};
static void tu_time_top(void)
{
    char b[40];
    uint32_t bars, cur, n = 0, s;
    tu_rect(0, 0, TU_W, 76, TC_BG);
    tu_text_sp(120, 32, &AF_PL, "TIME", 5, TC_AMBER, TC_BG);
    tu_bars(&cur, &bars);
    for (s = 0; s < NSCN; s++)
        n += (uint32_t)project_used(s);
    str_cpy(b, te.saved == 1u ? "TAKE SAVED" : te.saved == 2u ? "NOTHING NEW TO SAVE" : "TAKE NOT SAVED", 24);
    str_cpy(b + str_len(b), " \xB7 ", 5);
    fmt_int(b + str_len(b), (int32_t)bars);
    str_cpy(b + str_len(b), " BARS \xB7 ", 10);
    fmt_int(b + str_len(b), (int32_t)n);
    str_cpy(b + str_len(b), " SCENES", 8);
    tu_text_c(120, 60, &AF_TM, b, te.saved == 1u ? TC_GOOD : TC_REC, TC_BG);
}
static void tu_time_rows(void)
{
    uint32_t i;
    int32_t y = 12 + 76 - 12;
    tu_rect(0, 76, TU_W, 139, TC_BG);
    for (i = 0; i < TO_N; i++) {
        int sel = i == te.pick;
        char key[3] = {W_NOTE[i], '3', 0};
        int32_t h = sel ? 25 : 23;
        tu_opt_row(y, h, key, TO_TFM_NAME[i], TO_TFM_HINT[i], sel, i == TO_EXTRA ? TC_CYAN : TC_TEXT, TC_QUIET);
        y += h + 4;
    }
}
static void tu_time_draw(void)
{
    uint32_t sig = te.pick * 7u + te.saved * 3u;
    tu_zone(2, 0, 76, TC_BG, sig, tu_time_top);
    tu_zone(3, 76, 70, TC_BG, sig, tu_time_rows);
    tu_zone(5, 146, 69, TC_BG, sig, tu_time_rows);
    tu_foot(3, "OCT+", "DO IT", "SELECT", "PICK", "F3-B3", "GO", 0, 0, 0, 0);
    ui.force = 0;
}

/* ================================================================== DICE ====================================== */
static uint8_t tu_su_top;                               /* SETUP: the first row of the window of four */
static void tu_dice_tiles(void)                         /* y 22 .. 76: KEY, SCALE, TEMPO */
{
    static const char *const LBL[3] = {"KEY", "SCALE", "TEMPO"};
    char v[16];
    const char *u;
    uint32_t i;
    for (i = 0; i < 3u; i++) {
        int32_t x = 8 + (int32_t)i * 76, w = 72;
        tu_rrect(x, 29, w, 41, 6, TC_SURF, TC_BG);
        tu_text_c(x + w / 2, 39, &AF_TS, LBL[i], TC_QUIET, TC_SURF);
        if (i == 0u) {
            param_format(&TP[P_ROOT], ns.s.key, v, &u);
            tu_text_c(x + w / 2, 55, &AF_TX, v, TC_TEXT, TC_SURF);
        } else if (i == 1u) {
            str_cpy(v, tu_scale_full(ns.s.scale), 14);
            tu_text_c(x + w / 2, 55, &AF_TL, v, TC_TEXT, TC_SURF);
        } else {
            fmt_int(v, ns.s.bpm);
            tu_text_c(x + w / 2, 55, &AF_TX, v, TC_TEXT, TC_SURF);
        }
        if ((ns.hold >> i) & 1u)
            tu_text_r(x + w - 5, 36, &AF_TS, "HELD", TC_AMBER, TC_SURF);
    }
}
static void tu_dice_rows(void)                          /* y 76 .. 190: the four tracks */
{
    uint32_t i;
    for (i = 0; i < NTRK; i++) {
        int32_t y = 76 + (int32_t)i * 25;
        char b[24], r[24];
        tu_rrect(8, y, 224, 22, 5, TC_DEEP, TC_BG);
        tu_badge(14, y + 11, i, TC_DEEP);
        tu_text_fit(35, y + 11, &AF_TL, TFM_INS[ns.s.ins[i] % TI_N].name, TC_TEXT, TC_DEEP, 70);
        tu_text_fit(110, y + 11, &AF_TM, TFM_INS[ns.s.ins[i] % TI_N].snd[ns.s.pre[i] % TFM_INS[ns.s.ins[i] % TI_N].nsnd].name,
                    TC_MID, TC_DEEP, 80);
        cfg_rule_text(cfg_opt(i), r);
        if ((ns.hold >> (3u + i)) & 1u) {
            str_cpy(b, "HELD", 8);
            tu_text_r(224, y + 11, &AF_TS, b, TC_AMBER, TC_DEEP);
            tu_text_r(198, y + 11, &AF_TS, r, TC_QUIET, TC_DEEP);
        } else {
            tu_text_r(224, y + 11, &AF_TS, r, TC_QUIET, TC_DEEP);
        }
    }
}
static void tu_dice_note(void)
{
    tu_rect(0, 176, TU_W, 39, TC_BG);
    static const char *const K[2] = {"PLAY", "ENV"}, *const A[2] = {"START", "SETUP"};
    tu_text(9, 184, &AF_TS, cfg_default() ? "Rules come from SETUP: a band." : "Rules come from SETUP: yours.", TC_QUIET, TC_BG);
    tu_hint_line(203, TC_BG, TC_QUIET, 2, K, A);
}
static void tu_head_dice(void)
{
    static const char *l = "DICE";
    char t[8];
    const char *name = cfg_default() ? "BAND" : "MY BAND";
    tu_rect(0, 0, TU_W, TU_HY, TC_BG);
    tu_rect(0, TU_HY - 1, TU_W, 1, TC_SURF);
    tu_text(9, 10, &AF_PS, l, TC_AMBER, TC_BG);
    {
        int32_t x = tu_text(86, 10, &AF_TS, "SETUP", TC_QUIET, TC_BG) + 3;
        tu_text(x, 10, &AF_TM, name, TC_TEXT, TC_BG);
    }
    if (tm.on) {
        tm_text(t);
    } else if (tilt_timer_min) {
        fmt_int(t, (int32_t)tilt_timer_min);
        str_cpy(t + str_len(t), ":00", 4);
    } else {
        t[0] = 0;
    }
    if (t[0])
        tu_text_r(231, 10, &AF_PS, t, TC_AMBER, TC_BG);
}
/* ---- SETUP ---- */
static void tu_su_row_text(uint32_t r, char *b)
{
    uint32_t lo, hi;
    const char *u;
    if (r < 4u) {
        cfg_rule_text(cfg_opt(r), b);
    } else if (r == 4u) {
        if (CFG(CF_KEY))
            param_format(&TP[P_ROOT], CFG(CF_KEY) - 1u, b, &u);
        else
            str_cpy(b, "RANDOM", 8);
    } else if (r == 5u) {
        if (CFG(CF_SCALE))
            str_cpy(b, SC_FULL[(CFG(CF_SCALE) - 1u) % 11u], 12);
        else
            str_cpy(b, "RANDOM, no chromatic", 24);
    } else if (r == 6u) {
        cfg_lohi(&lo, &hi);
        fmt_int(b, (int32_t)lo);
        str_cpy(b + str_len(b), " - ", 4);
        fmt_int(b + str_len(b), (int32_t)hi);
    } else if (r == 7u) {
        static const uint8_t TM_[5] = {0, 5, 10, 15, 20};
        if (CFG(CF_TIMER) % 5u) {
            fmt_int(b, TM_[CFG(CF_TIMER) % 5u]);
            str_cpy(b + str_len(b), " MIN", 5);
        } else {
            str_cpy(b, "OFF", 4);
        }
    } else {
        str_cpy(b, "FINISH BAR + FADE", 20);
    }
}
static void tu_su_rows(void)
{
    static const char *const LBL[9] = {"T1", "T2", "T3", "T4", "KEY", "SCALE", "TEMPO", "TIMER", "TIME'S UP"};
    uint32_t r;
    for (r = 0; r < 9u; r++) {
        int32_t y = 28 + (int32_t)r * 19;
        int lit = r >= tu_su_top && r < tu_su_top + 4u;
        uint32_t k = r - tu_su_top;
        char v[28], kb[3] = {'K', (char)('1' + k), 0};
        uint16_t bg = lit ? TC_SURF : RGB(26, 20, 12);
        tu_rrect(8, y, 224, 17, 4, bg, TC_BG);
        if (lit)
            tu_text(13, y + 8, &AF_TS, kb, TU_TC[k & 3u], bg);
        tu_text(33, y + 8, &AF_TM, LBL[r], lit ? TC_TEXT : TC_QUIET, bg);
        tu_su_row_text(r, v);
        tu_text_r(225, y + 8, &AF_TM, v, lit ? TU_TC[k & 3u] : TC_MID, bg);
    }
}
static void tu_su_note(void)
{
    tu_rect(0, 199, TU_W, 16, TC_BG);
    tu_text(9, 207, &AF_TS, "Rule: RANDOM, a TYPE or one INSTRUMENT", TC_QUIET, TC_BG);
}
static void tu_head_setup(void)
{
    tu_rect(0, 0, TU_W, TU_HY, TC_BG);
    tu_rect(0, TU_HY - 1, TU_W, 1, TC_SURF);
    tu_text(9, 10, &AF_PS, "SETUP", TC_AMBER, TC_BG);
    tu_text_r(231, 10, &AF_TS, "how the dice roll \xB7 saved", TC_QUIET, TC_BG);
}
static void tu_dice_draw(void)
{
    uint32_t sig = fnv(2166136261u, &ns.s, sizeof ns.s) + ns.hold * 977u + cfg_opt(0) + cfg_opt(1) * 3u + cfg_opt(2) * 5u +
                   cfg_opt(3) * 7u + (uint32_t)tilt_timer_min * 11u + (tm.on ? tm.left : 0u);
    if (ns.again && fm1_ms - ns.again_t >= NS_AGAIN_MS)
        ns.again = 0;
    if (tu_setup_on) {
        uint32_t i, ssig = tu_su_top * 7u;
        for (i = 0; i < 16u; i++)
            ssig = ssig * 31u + CFG(i);
        tu_zone(0, 0, TU_HY, TC_BG, 1u, tu_head_setup);
        tu_zone(2, TU_HY, 100, TC_BG, ssig, tu_su_rows);       /* (a band is 124 rows at most: two) */
        tu_zone(5, TU_HY + 100, 77, TC_BG, ssig, tu_su_rows);
        tu_zone(3, 199, 16, TC_BG, 1u, tu_su_note);
        tu_foot(3, "SELECT", "SCROLL", "K1-K4", "THE 4 LIT ROWS", "HOME", "BACK", 0, 0, 0, 0);
    } else {
        tu_zone(0, 0, TU_HY, TC_BG, sig, tu_head_dice);
        tu_zone(2, TU_HY, 54, TC_BG, sig, tu_dice_tiles);
        tu_zone(3, 76, 100, TC_BG, sig, tu_dice_rows);
        tu_zone(4, 176, 39, TC_BG, 1u + (uint32_t)cfg_default(), tu_dice_note);
        tu_foot(3, "KEYS", "HOLD", "OCT+", "RE-ROLL", "HOME", "BACK", 0, 0, 0, 0);
    }
    ui.force = 0;
}
static void tu_su_step(uint32_t row, int32_t s)
{
    int32_t d = s > 0 ? 1 : -1;
    uint32_t v, i;
    if (row < 4u) {
        v = cfg_opt(row);
        CFG(CF_RULE + row) = (uint8_t)(clamp((int32_t)v + d, 0, (int32_t)RO_N - 1) + 1);
    } else if (row == 4u) {
        CFG(CF_KEY) = (uint8_t)clamp((int32_t)CFG(CF_KEY) + d, 0, 12);
    } else if (row == 5u) {
        CFG(CF_SCALE) = (uint8_t)clamp((int32_t)CFG(CF_SCALE) + d, 0, 11);
    } else if (row == 6u) {
        i = (uint32_t)clamp((int32_t)cfg_tempo_idx() + d, 0, 5);
        CFG(CF_LO) = TEMPO_R[i][0];
        CFG(CF_HI) = TEMPO_R[i][1];
    } else if (row == 7u) {
        CFG(CF_TIMER) = (uint8_t)clamp((int32_t)(CFG(CF_TIMER) % 5u) + d, 0, 4);
        {
            static const uint8_t TM_[5] = {0, 5, 10, 15, 20};
            if (!tm.on)
                tilt_timer_min = TM_[CFG(CF_TIMER) % 5u];
        }
    }
    if (row < 4u) {                                     /* (a rule changed: that track rolls at once, unless held) */
        if (!((ns.hold >> (3u + row)) & 1u)) {
            tfm_rng_t r;
            r.s = rng();
            tu_roll_track(&r, &ns.s, row);
        }
    } else if (row == 4u && CFG(CF_KEY) && !(ns.hold & 1u)) {
        ns.s.key = (uint8_t)(CFG(CF_KEY) - 1u);
    } else if (row == 5u && CFG(CF_SCALE) && !(ns.hold & 2u)) {
        ns.s.scale = TILT_SCALE[(CFG(CF_SCALE) - 1u) % 11u];
    } else if (row == 6u && !(ns.hold & 4u)) {
        uint32_t lo, hi;
        cfg_lohi(&lo, &hi);
        ns.s.bpm = (uint16_t)clamp((int32_t)ns.s.bpm, (int32_t)lo, (int32_t)hi);
    }
    ui.force = 1;
}
/* DICE or SETUP closed, nothing changed (the rules are kept once SETUP is left); START, if it was opened from there */
static void tu_dice_back(int to_start)
{
    if (tu_setup_on) {
        tu_setup_on = 0;
        settings_save();
    }
    name_close();
    if (ns_from_start) {
        ns_from_start = 0;
        if (to_start)
            tilt_ns_open = TILT_START;
    }
    ui.force = 1;
}
/* HOME tapped over anything that takes the screen (ui_input.c): one tap out, nothing changed. 1 = it was taken here.
 * A dialog is cancelled; NAME is cancelled; SETUP goes back to DICE, DICE to where it was opened from (START, else the
 * screen before), START and TIME to JAM */
static int tu_modal_home(void)
{
    if (tu_classic())
        return 0;
    if (ui.confirm) {
        ui.confirm = 0;
        ui.force = 1;
        return 1;
    }
    if (tilt_ns_open == TILT_NS) {
        if (tu_setup_on) {
            tu_setup_on = 0;
            settings_save();
            ui.force = 1;
        } else {
            tu_dice_back(1);
        }
        return 1;
    }
    if (tilt_ns_open == TILT_START) {
        ns_from_start = 0;
        name_close();
        go_home();
        return 1;
    }
    if (tilt_ns_open == TILT_TIME) {
        te_do(TO_KEEP);
        return 1;
    }
    if (nm.kind) {
        name_close();
        ui.force = 1;
        return 1;
    }
    return 0;
}
static void tu_dice_input(uint32_t pressed, uint32_t notes, uint32_t oct)
{
    uint32_t k, p;
    int32_t s;
    song.grid = 2;                                      /* (silent keys: keys_mode, as NAME's) */
    {   /* a screen button closes DICE (SETUP) and opens its screen */
        static const uint8_t SB[8] = {B_SEQ, B_EDIT, B_GLO, B_SAVE, B_FX, B_SCL, B_LFO, B_ARP};
        for (k = 0; k < 8u; k++)
            if ((pressed >> panel.btn[SB[k]]) & 1u) {
                tu_dice_back(0);
                tu_tap(SB[k]);
                enc_drop();
                return;
            }
    }
    if ((pressed >> panel.btn[B_ENV]) & 1u) {           /* ENV: SETUP, and back */
        tu_setup_on = (uint8_t)!tu_setup_on;
        if (!tu_setup_on)
            settings_save();                            /* (the rules, written once left) */
        ui.force = 1;
        enc_drop();
        return;
    }
    if (tu_setup_on) {
        if ((s = panel_enc(EN_SELECT)) != 0) {
            tu_su_top = (uint8_t)clamp((int32_t)tu_su_top + (s > 0 ? 1 : -1), 0, 5);
            ui.force = 1;
        }
        for (k = 0; k < 4u; k++)
            if ((s = panel_enc(EN_K1 + k)) != 0)
                tu_su_step(tu_su_top + k, s);
        enc_drop();
        if (oct & 1u) {
            tu_setup_on = 0;
            settings_save();
            ui.force = 1;
        }
        return;
    }
    for (k = 0; k < 27u; k++)
        if (((notes >> k) & 1u) && !key_black(k) && (p = key_place(k)) < 7u) {   /* F3 .. D4... a row: held, or not */
            ns.hold ^= (uint16_t)(1u << p);
            ns.row = (uint8_t)p;
            ui.force = 1;
        }
    enc_drop();                                         /* (the knobs do nothing here) */
    if (oct & 2u) {
        tu_dice_nudge();
        ui.force = 1;
    } else if (oct & 1u) {
        name_close();                                   /* back: nothing changed */
        if (ns_from_start) {
            ns_from_start = 0;
            tilt_ns_open = TILT_START;
            ui.force = 1;
        }
    } else if ((pressed >> panel.btn[B_PLAY]) & 1u) {
        ns_start();
    }
}
