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
enum { CF_RULE = 0, CF_KEY = 4, CF_SCALE = 5, CF_LO = 6, CF_HI = 7, CF_TIMER = 8, CF_NEW = 9, CF_CAP = 10 };   /* (round 8: CAPTURE 0..2: 1 2 4 bars) */
static int tfm_newsong_keep(void) { return CFG(CF_NEW) == 1u; }   /* round 3: 0 FRESH (default), 1 KEEP SCENES */
static const uint8_t TM_LIST[5] = {0, 5, 10, 15, 20};
/* round 3: the TIMER default is 10 minutes: the stored byte 0 (every older setting) means 10, 1..4 are 5 15 .. as before
 * (1: 5, 2: 10, 3: 15, 4: 20) and 5 is OFF */
static uint32_t cfg_timer_idx(void) { uint32_t v = CFG(CF_TIMER); return v == 0u ? 2u : v == 5u ? 0u : v % 5u; }
static uint8_t cfg_timer_val(uint32_t idx) { return (uint8_t)(idx == 0u ? 5u : idx); }
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
    if (!tm.on)
        tilt_timer_min = TM_LIST[cfg_timer_idx()];
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

/* ================================================================== START v2 ================================== */
/* round 6 item 5 (tfm-spec/screens/StartWait, StartSpin, StartRolled): DICE merged into START. WAIT: the last game's band
 * asleep in their cabinets, PLAY = CONTINUE, OCT+ rolls a new band, SAVE = SONGS, ENV = SETUP. SPIN: the cabinets run like
 * reels (the instrument names streaming past) and stop one by one, left to right (the held ones stay), about 1.5 s; any
 * button skips to the end. ROLLED: the new band awake (each cabinet its material, role and preset), F3 G3 A3 B3 hold / let
 * go a player, OCT+ rolls the rest again, PLAY starts the new song (fresh, as round 3) and the clock, OCT- back to WAIT.
 * ENV from anywhere opens it ROLLED (a fresh roll waiting for PLAY); ENV again is SETUP. Cool neutral greys; the display
 * stays TILT's amber dot matrix */
enum { SV_WAIT, SV_SPIN, SV_ROLLED };
#define SV_SPIN_MS 1500u
#define SV_BG RGB(14, 15, 18)
#define SV_CARD RGB(27, 29, 35)
#define SV_EDGE RGB(46, 49, 58)
#define SV_QUIET RGB(150, 154, 166)
static struct {
    uint8_t mode, last_ok, last_ins[NTRK];
    uint32_t t0;
} sv;
static void tsg_time_text(char *b, uint32_t bars);     /* (tfm_song.c) */
static void autosave_track(uint32_t k, uint8_t *eng, uint8_t *pre);   /* (project.c: the autosave just read, track k) */
static const char *const SV_ROLE[IG_N] = {"DRUMS", "BASS", "CHORDS", "LEAD"};
static void sv_boot(void)                               /* power-on (tilt_start_boot): the last game's band, from the autosave read */
{
    uint32_t k;
    sv.mode = SV_WAIT;
    sv.last_ok = (uint8_t)(st.has && !st.crash);
    for (k = 0; k < NTRK; k++) {
        uint8_t e = 0, pr = 0;
        uint32_t p, i;
        autosave_track(k, &e, &pr);
        p = pr;
        i = p >= TFM_PRE0 ? (p - TFM_PRE0) / 16u : TI_N;
        sv.last_ins[k] = (uint8_t)(sv.last_ok && i < TI_N && TFM_INS[i].eng == e ? i : TI_NONE);
    }
}
static void sv_roll(void)                               /* OCT+ / NEXT BALL: roll (the held ones stay) and spin */
{
    if (!tilt_start_pending) {
        scene_claim();
        tilt_start_pending = 1;
    }
    ns_from_start = 1;
    tilt_ns_show();                                     /* (the dice: tu_dice_nudge, the timer's default) */
    tilt_ns_open = TILT_START;
    sv.mode = SV_SPIN;
    sv.t0 = fm1_ms;
    ui.force = 1;
}
static int sv_stopped(uint32_t k)                       /* reel k has stopped (a held one never ran) */
{
    return sv.mode != SV_SPIN || ((ns.hold >> (3u + k)) & 1u) || fm1_ms - sv.t0 >= 600u + 300u * k;
}
static void sv_frame(void)                              /* (each frame) the spin ends on its own */
{
    if (sv.mode == SV_SPIN && fm1_ms - sv.t0 >= SV_SPIN_MS) {
        sv.mode = SV_ROLLED;
        ui.force = 1;
    }
}
static void sv_cab(uint32_t k)
{
    int32_t x0 = 5 + 58 * (int32_t)k, y0 = 60, w = 54, h = 75;
    uint32_t ins = sv.mode == SV_WAIT ? sv.last_ins[k] : ns.s.ins[k] % TI_N, held = (ns.hold >> (3u + k)) & 1u;
    const tu_mat_t *m = ins < TI_N ? &TU_MAT[ins] : &TU_MAT_NONE;
    int sleep = sv.mode == SV_WAIT;
    uint16_t top = sleep ? tu_mix(SV_BG, m->top, 55) : m->top, bot = sleep ? tu_mix(SV_BG, m->bot, 55) : m->bot;
    uint16_t ink = sleep ? tu_mix(SV_BG, m->ink, 60) : m->ink, mid = tu_mix(top, bot, 50);
    int32_t i;
    if (held && sv.mode != SV_WAIT)                     /* a held player: outlined amber */
        tu_cut(x0 - 2, y0 - 2, w + 4, h + 4, TC_AMBER);
    tu_cut(x0, y0, w, h, bot);
    tu_cut(x0, y0, w, h / 2, top);
    for (i = 8; i < h - 8; i += 4)
        tu_rect(x0, y0 + i, w, 4, tu_mix(top, bot, (uint32_t)i * 100u / (uint32_t)h));
    tu_rect(x0 + 2, y0, w - 4, 3, sleep ? tu_mix(SV_BG, TU_TC[k], 50) : TU_TC[k]);
    if (!sv_stopped(k)) {                               /* a reel: the names stream past */
        uint32_t ph = (fm1_ms - sv.t0) / 40u, r;
        for (r = 0; r < 4u; r++) {
            uint32_t n = (ph / 3u + r * 5u + k * 3u) % TI_N;
            int32_t y = y0 + 12 + (int32_t)r * 17 + (int32_t)(ph % 3u) * 6 - 6;
            if (y > y0 + 6 && y < y0 + h - 6)
                tu_text_c(x0 + w / 2, y, &AF_TS, TFM_INS[n].name, tu_mix(mid, ink, r == 1u || r == 2u ? 80 : 35), mid);
        }
        return;
    }
    if (ins >= TI_N) {
        tu_text_c(x0 + w / 2, y0 + 36, &AF_TS, sleep ? "z z" : "--", ink, mid);
        return;
    }
    tu_text_c(x0 + w / 2, y0 + 13, &AF_TS, TFM_INS[ins].name, ink, tu_mix(top, bot, 15));
    if (sleep) {
        tu_text_c(x0 + w / 2, y0 + 40, &AF_TL, "z z", ink, mid);
        return;
    }
    tu_text_c(x0 + w / 2, y0 + 32, &AF_TS, SV_ROLE[TFM_INS[ins].group % IG_N], tu_mix(mid, ink, 60), mid);
    tu_text_c(x0 + w / 2, y0 + 46, &AF_TS, TFM_INS[ins].snd[ns.s.pre[k] % TFM_INS[ins].nsnd].name, ink, mid);
    {
        char kb[12] = "HELD \xB7 F3";
        kb[7] = W_NOTE[k];
        if (held) {
            tu_cut(x0 + 4, y0 + h - 15, w - 8, 12, TC_AMBER);
            tu_text_c(x0 + w / 2, y0 + h - 9, &AF_TS, kb, SV_BG, TC_AMBER);
        } else {
            tu_cut(x0 + 4, y0 + h - 15, w - 8, 12, tu_mix(bot, SV_BG, 45));
            tu_text_c(x0 + w / 2, y0 + h - 9, &AF_TS, kb + 7, tu_mix(bot, ink, 70), tu_mix(bot, SV_BG, 45));
        }
    }
}
static void sv_display(void)                            /* y 0 .. 58: TILT in dots, and what the mode says */
{
    char b[28];
    tu_rect(0, 0, TU_W, 58, SV_BG);
    tu_rrect(5, 5, 230, 48, 6, SV_EDGE, SV_BG);
    tu_rrect(7, 7, 226, 44, 5, RGB(10, 7, 4), SV_EDGE);
    dmd_str(14, 15, 3, "TILT", TC_AMBER, DMD_DIM);
    tu_text(14, 40, &AF_TS, TILT_BUILD, RGB(150, 112, 50), RGB(10, 7, 4));   /* round 7: which build this is */
    if (sv.mode == SV_WAIT) {
        uint32_t n = song_nscenes(sng_cur), bars = song_bars_n(sng_cur, tfm_bar());
        tu_text(92, 15, &AF_PS, sv.last_ok ? "LAST GAME" : "NEW GAME", TC_AMBER, RGB(10, 7, 4));
        str_cpy(b, song_name_of(sng_cur), 16);
        tu_text(92, 30, &AF_TM, sv.last_ok ? (b[0] ? b : "UNTITLED") : "roll a band", RGB(255, 217, 160), RGB(10, 7, 4));
        if (sv.last_ok) {
            fmt_int(b, (int32_t)n);
            str_cpy(b + str_len(b), n == 1u ? " SCENE \xB7 " : " SCENES \xB7 ", 12);
            tsg_time_text(b + str_len(b), bars);
            tu_text(92, 44, &AF_TS, b, RGB(185, 138, 58), RGB(10, 7, 4));
        }
    } else if (sv.mode == SV_SPIN) {
        uint32_t i, ph = fm1_ms / 60u;
        tu_text(92, 15, &AF_PS, "ROLLING", TC_AMBER, RGB(10, 7, 4));
        for (i = 0; i < 6u; i++)
            b[i] = (char)('A' + (ph * 7u + i * 5u) % 26u);
        b[6] = 0;
        tu_text(92, 30, &AF_TM, b, RGB(255, 217, 160), RGB(10, 7, 4));
        tu_text(92, 44, &AF_TS, "KEY \xB7 SCALE \xB7 TEMPO", RGB(185, 138, 58), RGB(10, 7, 4));
    } else {
        tu_note_name(60u + ns.s.key % 12u, b);
        b[str_len(b) - 1u] = 0;                         /* (the octave's digit off: "A") */
        str_cpy(b + str_len(b), " ", 2);
        str_cpy(b + str_len(b), tu_scale_full(ns.s.scale), 12);
        tu_text(92, 17, &AF_TM, b, TC_AMBER, RGB(10, 7, 4));
        fmt_int(b, ns.s.bpm);
        str_cpy(b + str_len(b), " BPM", 6);
        tu_text(92, 37, &AF_TM, b, TC_AMBER, RGB(10, 7, 4));
        str_cpy(b, "TIMER ", 8);
        if (tilt_timer_min) {
            fmt_int(b + 6, tilt_timer_min);
            str_cpy(b + str_len(b), ":00", 4);
        } else {
            str_cpy(b + 6, "OFF", 4);
        }
        tu_text_r(226, 37, &AF_TS, b, RGB(255, 217, 160), RGB(10, 7, 4));
    }
}
static void sv_band(void)                               /* y 58 .. 138 */
{
    uint32_t k;
    tu_rect(0, 58, TU_W, 80, SV_BG);
    for (k = 0; k < NTRK; k++)
        sv_cab(k);
}
static void sv_card(int32_t x, const char *key, const char *title, const char *sub, uint16_t tc)
{
    tu_cut(x, 174, 74, 38, SV_CARD);
    tu_text(x + 5, 181, &AF_TS, key, TC_AMBER, SV_CARD);
    tu_text(x + 5, 193, &AF_TM, title, tc, SV_CARD);
    tu_text(x + 5, 205, &AF_TS, sub, SV_QUIET, SV_CARD);
}
static void sv_bottom(void)                             /* y 138 .. 215: the PLAY button and the three cards */
{
    int32_t x;
    tu_rect(0, 138, TU_W, 77, SV_BG);
    if (sv.mode == SV_SPIN)
        return;
    for (x = 0; x < 230; x++) {                         /* PLAY: a gradient through the track colours */
        uint32_t seg = (uint32_t)x * 3u / 230u, f = ((uint32_t)x * 3u % 230u) * 100u / 230u;
        static const uint8_t ORD[4] = {0, 2, 1, 1};     /* (blue, pink, yellow) */
        uint16_t c = tu_mix(TU_TC[ORD[seg]], TU_TC[ORD[seg + 1u]], f);
        tu_rect(5 + x, 141 + (x < 3 || x > 226 ? 2 : 0), 1, 30 - (x < 3 || x > 226 ? 4 : 0), c);
    }
    tu_cut(10, 146, 26, 20, SV_BG);
    tu_text_c(23, 156, &AF_TS, "PLAY", TC_AMBER, SV_BG);
    tu_text(42, 156, &AF_TL, sv.mode == SV_WAIT && sv.last_ok ? "CONTINUE" : "START", SV_BG, tu_mix(TU_TC[0], TU_TC[2], 30));
    tu_text_r(228, 156, &AF_TS, sv.mode == SV_WAIT && sv.last_ok ? "wake the band" : "the clock starts", SV_BG, TU_TC[1]);
    if (sv.mode == SV_WAIT) {
        sv_card(5, "OCT+", "NEW ROLL", "shuffle a band", TC_AMBER);
        sv_card(83, "SAVE", "SONGS", "3 slots", TC_TEXT);
        sv_card(161, "ENV", "SETUP", "roll rules", TC_TEXT);
    } else {
        sv_card(5, "OCT+", "RE-ROLL", "the unheld ones", TC_AMBER);
        sv_card(83, "F3-B3", "HOLD", "keep a player", TC_TEXT);
        sv_card(161, "OCT-", "BACK", "your last song", TC_TEXT);
    }
}
static void tu_start_draw(void)
{
    uint32_t sig = sv.mode * 7u + (uint32_t)ns.hold * 31u + fnv(2166136261u, &ns.s, sizeof ns.s) + tilt_timer_min * 977u + sv.last_ok * 3u +
                   (sv.mode == SV_SPIN ? fm1_ms / 40u : 0u);
    uint32_t k;
    sv_frame();
    for (k = 0; k < NTRK; k++)
        sig = sig * 31u + sv.last_ins[k];
    tu_pbg = SV_BG;
    tu_pline = SV_EDGE;
    tu_pq = SV_QUIET;
    tu_zone(1, 0, 58, SV_BG, sig, sv_display);
    tu_zone(2, 58, 80, SV_BG, sig, sv_band);
    tu_zone(3, 138, 77, SV_BG, sig, sv_bottom);
    if (sv.mode == SV_WAIT)
        tu_foot(3, "PLAY", sv.last_ok ? "CONTINUE" : "START", "OCT+", "ROLL", "SAVE", "SONGS", 0, 0, 0, 0);
    else if (sv.mode == SV_SPIN)
        tu_foot(2, "HELD", "PLAYERS STAY", "THE REST", "STOP ONE BY ONE", 0, 0, 0, 0, 0, 0);
    else
        tu_foot(3, "PLAY", "START", "OCT+", "RE-ROLL", "ENV", "SETUP", 0, 0, 0, 0);
    tu_pbg = TC_BG;
    tu_pline = TC_SURF;
    tu_pq = TC_QUIET;
    ui.force = 0;
}
static void sv_rolled(void)                             /* tilt_ns_show in TFM: START rolled, a fresh roll waiting for PLAY */
{
    tilt_ns_open = TILT_START;
    sv.mode = SV_ROLLED;
}
static void sv_setup(void)                              /* ENV: SETUP (leaving it comes back here) */
{
    tilt_ns_open = TILT_NS;
    tu_setup_on = 1;
    ui.force = 1;
}
static void sv_input(uint32_t pressed, uint32_t notes, uint32_t oct)
{
    uint32_t k, p;
    int32_t s;
    song.grid = 2;                                      /* (silent keys: keys_mode, as NAME's) */
    sv_frame();
    if (sv.mode == SV_SPIN) {                           /* any button (or key) skips to the end */
        if (pressed || notes || oct) {
            sv.mode = SV_ROLLED;
            ui.force = 1;
        }
        enc_drop();
        return;
    }
    if ((pressed >> panel.btn[B_ENV]) & 1u) {
        sv_setup();
        enc_drop();
        return;
    }
    {   /* a screen button closes START and opens its screen (SAVE on WAIT: SONGS, below) */
        static const uint8_t SB[7] = {B_SEQ, B_EDIT, B_GLO, B_FX, B_SCL, B_LFO, B_ARP};
        for (k = 0; k < 7u; k++)
            if ((pressed >> panel.btn[SB[k]]) & 1u) {
                tilt_ns_open = 0;
                name_close();
                ns_from_start = 0;
                tu_tap(SB[k]);
                enc_drop();
                return;
            }
        if (sv.mode == SV_ROLLED && ((pressed >> panel.btn[B_SAVE]) & 1u)) {
            tilt_ns_open = 0;
            name_close();
            tu_tap(B_SAVE);
            enc_drop();
            return;
        }
    }
    if (sv.mode == SV_WAIT) {
        enc_drop();
        if ((pressed >> panel.btn[B_PLAY]) & 1u) {
            if (sv.last_ok)
                st_do(SO_CONT);
            else
                sv_roll();
        } else if (oct & 2u) {
            sv_roll();
        } else if ((pressed >> panel.btn[B_SAVE]) & 1u) {
            st_do(SO_LOAD);
        }
        return;
    }
    for (k = 0; k < 27u; k++)                           /* ROLLED: F3 G3 A3 B3 hold / let go a player */
        if (((notes >> k) & 1u) && !key_black(k) && (p = key_place(k)) < NTRK) {
            ns.hold ^= (uint16_t)(1u << (3u + p));
            ui.force = 1;
        }
    if ((s = panel_enc(EN_K4)) != 0 && !tm.on) {       /* K4: this song's timer (OFF 5 10 15 20) */
        uint32_t ix = 0, j;
        for (j = 0; j < 5u; j++)
            if (TM_LIST[j] == tilt_timer_min)
                ix = j;
        tilt_timer_min = TM_LIST[(uint32_t)clamp((int32_t)ix + s, 0, 4)];
        ui.force = 1;
    }
    enc_drop();
    if (oct & 2u) {
        sv_roll();
    } else if (oct & 1u) {
        sv.mode = SV_WAIT;
        ui.force = 1;
    } else if ((pressed >> panel.btn[B_PLAY]) & 1u) {
        sv.mode = SV_WAIT;
        ns_start();
    }
}

/* ================================================================== TIME ====================================== */
static const char *const TO_TFM_NAME[TO_N] = {"PLAY IT BACK", "NEXT BALL", "EXTRA BALL", "KEEP WORKING"};   /* (round 6: NEXT BALL) */
static const char *const TO_TFM_HINT[TO_N] = {"from the top", "save it, roll a band", "+5 min", "no clock"};
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
    for (i = 0; i < TO_N; i++) {                        /* (round 6: in te_ord's order: PLAY IT BACK, EXTRA BALL, KEEP WORKING, NEXT BALL) */
        uint32_t o = te_ord(i);
        int sel = i == te.pick;
        char key[3] = {W_NOTE[i], '3', 0};
        int32_t h = sel ? 25 : 23;
        tu_opt_row(y, h, key, TO_TFM_NAME[o], TO_TFM_HINT[o], sel, o == TO_EXTRA ? TC_CYAN : o == TO_SAVE ? TC_AMBER : TC_TEXT, TC_QUIET);
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
    static const char *const K[3] = {"PLAY", "ENV", "K4"}, *const A[3] = {"START", "SETUP", "TIMER"};
    char tb[16] = "TIMER ";
    tu_text(9, 184, &AF_TS, cfg_default() ? "Rules come from SETUP: a band." : "Rules come from SETUP: yours.", TC_QUIET, TC_BG);
    if (tilt_timer_min) {
        fmt_int(tb + 6, tilt_timer_min);
        str_cpy(tb + str_len(tb), " MIN", 5);
    } else {
        str_cpy(tb + 6, "OFF", 4);
    }
    tu_text_r(231, 184, &AF_TS, tb, TC_AMBER, TC_BG);
    tu_hint_line(203, TC_BG, TC_QUIET, 3, K, A);
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
        if (TM_LIST[cfg_timer_idx()]) {
            fmt_int(b, TM_LIST[cfg_timer_idx()]);
            str_cpy(b + str_len(b), " MIN", 5);
        } else {
            str_cpy(b, "OFF", 4);
        }
    } else if (r == 8u) {
        str_cpy(b, tfm_newsong_keep() ? "KEEP SCENES" : "FRESH", 14);
    } else {                                            /* (round 8: JAM's C#5 CAPTURE takes the last 1, 2 or 4 bars) */
        str_cpy(b, CFG(CF_CAP) % 3u == 0u ? "1 BAR" : CFG(CF_CAP) % 3u == 1u ? "2 BARS" : "4 BARS", 8);
    }
}
static void tu_su_rows(void)
{
    static const char *const LBL[10] = {"T1", "T2", "T3", "T4", "KEY", "SCALE", "TEMPO", "TIMER", "NEW SONG", "CAPTURE"};
    uint32_t r;
    for (r = 0; r < 10u; r++) {                         /* (round 8: ten rows, 17 apart) */
        int32_t y = 28 + (int32_t)r * 17;
        int lit = r >= tu_su_top && r < tu_su_top + 4u;
        uint32_t k = r - tu_su_top;
        char v[28], kb[3] = {'K', (char)('1' + k), 0};
        uint16_t bg = lit ? TC_SURF : RGB(26, 20, 12);
        tu_rrect(8, y, 224, 16, 4, bg, TC_BG);
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
    tu_rec_dots(TC_BG);
    tu_text(TU_DX, 10, &AF_PS, "SETUP", TC_AMBER, TC_BG);
    tu_text_r(231, 10, &AF_TS, "how the dice roll", TC_QUIET, TC_BG);
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
        tu_zone(0, 0, TU_HY, TC_BG, tu_rec_sig(), tu_head_setup);
        tu_zone(2, TU_HY, 100, TC_BG, ssig, tu_su_rows);       /* (a band is 124 rows at most: two) */
        tu_zone(5, TU_HY + 100, 77, TC_BG, ssig, tu_su_rows);
        tu_zone(3, 199, 16, TC_BG, 1u, tu_su_note);
        tu_foot(3, "SELECT", "SCROLL", "K1-K4", "THE 4 LIT ROWS", "HOME", "BACK", 0, 0, 0, 0);
    } else {
        tu_zone(0, 0, TU_HY, TC_BG, sig, tu_head_dice);
        tu_zone(2, TU_HY, 54, TC_BG, sig, tu_dice_tiles);
        tu_zone(3, 76, 100, TC_BG, sig, tu_dice_rows);
        tu_zone(4, 176, 39, TC_BG, 1u + (uint32_t)cfg_default() + (uint32_t)tilt_timer_min * 7u, tu_dice_note);
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
        CFG(CF_TIMER) = cfg_timer_val((uint32_t)clamp((int32_t)cfg_timer_idx() + d, 0, 4));
        if (!tm.on)
            tilt_timer_min = TM_LIST[cfg_timer_idx()];
    } else if (row == 8u) {
        CFG(CF_NEW) = (uint8_t)clamp((int32_t)CFG(CF_NEW) + d, 0, 1);
    } else if (row == 9u) {
        CFG(CF_CAP) = (uint8_t)clamp((int32_t)(CFG(CF_CAP) % 3u) + d, 0, 2);
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
            tilt_ns_open = TILT_START;                  /* (round 6: SETUP's HOME is START) */
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
        if (!tu_setup_on) {
            settings_save();                            /* (the rules, written once left) */
            tilt_ns_open = TILT_START;                  /* (round 6: back to START) */
        }
        ui.force = 1;
        enc_drop();
        return;
    }
    if (tu_setup_on) {
        if ((s = panel_enc(EN_SELECT)) != 0) {
            tu_su_top = (uint8_t)clamp((int32_t)tu_su_top + (s > 0 ? 1 : -1), 0, 6);
            ui.force = 1;
        }
        for (k = 0; k < 4u; k++)
            if ((s = panel_enc(EN_K1 + k)) != 0)
                tu_su_step(tu_su_top + k, s);
        enc_drop();
        if (oct & 1u) {
            tu_setup_on = 0;
            settings_save();
            tilt_ns_open = TILT_START;                  /* (round 6: back to START) */
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
    if ((s = panel_enc(EN_K4)) != 0 && !tm.on) {       /* K4: this song's timer (OFF 5 10 15 20); the default is SETUP's */
        uint32_t ix = 0, j;
        for (j = 0; j < 5u; j++)
            if (TM_LIST[j] == tilt_timer_min)
                ix = j;
        tilt_timer_min = TM_LIST[(uint32_t)clamp((int32_t)ix + s, 0, 4)];
        ui.force = 1;
    }
    enc_drop();                                         /* (the other knobs do nothing here) */
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
