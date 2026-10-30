#include <time.h>
/* TFM-UI fix round 1: the tests of the round's items (included by ui_test.c after the phase tests; same helpers) */
/* 1: NOTES and DRUMS, SELECT one step at a time through every step (the bar follows), PRESETS a bar and the same step in it */
#define R1(c) do { if (!(c)) { ok = 0; fprintf(stderr, "R1 line %d ", __LINE__); } } while (0)
static int test_r1_cursor(void)
{
    int bad = 0, ok = 1;
    uint32_t lens[3] = {32u, 7u, 64u}, li, i, scr;
    ui_power_on();
    tu_off = 0;
    tu_goto(TU_JAM);
    tfm_load(&trk[0], TI_SQUELCH, 0); tfm_load(&trk[1], TI_THUMP, 0);
    for (scr = 0; scr < 2u; scr++)
        for (li = 0; li < 3u; li++) {
            uint32_t len = lens[li], seen = 0;
            song.sel = (uint8_t)scr;                    /* T1 melodic (NOTES), T2 drums (DRUMS) */
            tu_goto(TU_JAM);
            trk[scr].p[P_SLEN] = (int16_t)len;
            stap(B_SEQ);
            R1(tu.scr == (scr ? TU_DRUMS : TU_NOTES));
            ui.cursor = 0; tq_bar = 0;
            for (i = 0; i < len; i++) {                 /* up: every step 1..LEN, empty or not, the shown bar following */
                R1(ui.cursor == i && tq_bar == i / 16u);
                seen |= i < 32u ? 1u << i : 0u;
                turn(scr ? EN_SELECT : EN_K1, 1);               /* (round 3: NOTES steps with K1, SELECT scrolls its rows; DRUMS: both) */
            }
            ok &= ui.cursor == len - 1u;                /* (the end stays the end) */
            for (i = len; i-- > 0;) {
                R1(ui.cursor == i && tq_bar == i / 16u);
                turn(scr ? EN_SELECT : EN_K1, -1);
            }
            R1(ui.cursor == 0u && (len >= 32u ? seen == 0xFFFFFFFFu : seen == (1u << len) - 1u));
            if (len > 16u && scr) {                     /* PRESETS (DRUMS; NOTES zooms since round 4): the next bar, the same step in it */
                ui.cursor = 5; tq_bar = 0; turn(EN_PRESET, 1);
                R1(tq_bar == 1u && ui.cursor == 21u);
                turn(EN_PRESET, -1);
                R1(tq_bar == 0u && ui.cursor == 5u);
                turn(EN_PRESET, -1);
                R1(tq_bar == 0u && ui.cursor == 5u);
            }
            tu_goto(TU_JAM);
        }
    bad += check("TFM-UI fix 1: SELECT puts the cursor on every step 1..LEN (NOTES, DRUMS; 7, 32, 64 steps), PRESETS a bar and the same step", ok);
    return bad;
}
/* 2: tempo is easy to find: JAM's footer says SCL TEMPO, CLOCK opens on K1 TEMPO lit, SELECT never moves the BPM */
static int test_r1_tempo(void)
{
    int bad = 0, ok = 1;
    int32_t bpm;
    ui_power_on();
    tu_off = 0;
    tu_goto(TU_JAM);
    frames(3);
    bpm = song.g[G_BPM];
    turn(EN_SELECT, 3); frames(2);
    R1(song.g[G_BPM] == bpm);                           /* (SELECT is not the tempo) */
    {
        uint32_t i, found = 0;
        for (i = 0; i < tu_ft.n; i++)
            if (str_eq(tu_ft.k[i], "HOLD F#5") && str_eq(tu_ft.a[i], "CHAIN")   /* (round 5: the footer is the live one) */)
                found = 1;
        R1(found);
    }
    stap(B_SCL); frames(3);
    R1(tu.scr == TU_CLOCK && ro.hot == 0u);
    turn(EN_K2, 1); frames(3);
    R1(ro.hot == 1u && song.g[G_BPM] == bpm);
    turn(EN_K1, 2); frames(3);
    R1(ro.hot == 0u && song.g[G_BPM] != bpm);
    turn(EN_SELECT, 3); frames(2);
    R1(ro.hot == 0u);
    tu_goto(TU_JAM); stap(B_SCL); frames(3);
    R1(tu.scr == TU_CLOCK && ro.hot == 0u);
    bad += check("TFM-UI fix 2: JAM's footer says SCL TEMPO; CLOCK opens with K1 TEMPO lit; SELECT does not move the BPM", ok);
    return bad;
}
/* 3: no dead ends: from every TFM screen, modal, dialog and NAME, one HOME tap gets out; DICE and SETUP back with nothing
 * changed, a screen button closes DICE and opens its screen */
static int r1_free(void) { return !tilt_ns_open && !nm.kind && !ui.confirm && !ui.menu && !ui.layer; }
static void r1_reset(void)
{
    ui_power_on();
    tu_off = 0;
    tilt_ns_open = 0; nm.kind = 0; ui.confirm = 0; ui.menu = 0; ui.layer = 0;
    tu_goto(TU_JAM);
    frames(3);
}
static int test_r1_home(void)
{
    int bad = 0, ok = 1;
    uint32_t s;
    for (s = 0; s < TU_N; s++) {                        /* every screen: HOME ends on JAM (DEEP: out of DEEP) */
        r1_reset();
        tu_goto(s);
        frames(3);
        stap(B_HOME); frames(3);
        if (s == TU_JAM) {                              /* (JAM is the screen HOME toggles: MIX, and back) */
            R1(tu.scr == TU_MIX);
            stap(B_HOME); frames(3);
        }
        R1(r1_free() && tu.scr == TU_JAM);
    }
    r1_reset();                                         /* DICE: back to where it was; its footer says HOME BACK */
    stap(B_ENV); frames(3);
    R1(tilt_ns_open == TILT_NS && !tu_setup_on);
    {
        uint32_t i, f = 0;
        for (i = 0; i < tu_ft.n; i++)
            if (str_eq(tu_ft.k[i], "HOME") && str_eq(tu_ft.a[i], "BACK"))
                f = 1;
        R1(f);
    }
    {   /* nothing changed: the rolled setup and the held rows are as they were */
        uint8_t ins0 = ns.s.ins[0]; uint16_t hold = ns.hold;
        stap(B_HOME); frames(3);
        R1(r1_free() && tu.scr == TU_JAM && ns.s.ins[0] == ins0 && ns.hold == hold);
    }
    stap(B_ENV); frames(3);                             /* SETUP: HOME back to DICE, again out */
    stap(B_ENV); frames(3);
    R1(tilt_ns_open == TILT_NS && tu_setup_on);
    stap(B_HOME); frames(3);
    R1(tilt_ns_open == TILT_NS && !tu_setup_on);
    stap(B_HOME); frames(3);
    R1(r1_free() && tu.scr == TU_JAM);
    stap(B_ENV); frames(3); stap(B_ENV); frames(3);     /* SETUP: OCT- is DICE's back too */
    R1(tu_setup_on);
    stap(B_OCTDN); frames(3);
    R1(r1_free() || (tilt_ns_open == TILT_NS && !tu_setup_on));
    {   /* a screen button closes DICE and opens its screen */
        static const uint8_t SB[8] = {B_SEQ, B_EDIT, B_GLO, B_SAVE, B_FX, B_SCL, B_LFO, B_ARP};
        static const uint8_t SC[8] = {TU_NOTES, TU_SOUND, TU_SONG, TU_SONGS, TU_FX, TU_CLOCK, TU_MOTION, TU_ARP};
        for (s = 0; s < 8u; s++) {
            r1_reset();
            stap(B_ENV); frames(3);
            if (s & 1u) { stap(B_ENV); frames(3); }     /* (from SETUP too) */
            stap(SB[s]); frames(3);
            R1(r1_free() && tu.scr == SC[s]);
        }
    }
    r1_reset();                                         /* START */
    tilt_start_boot(1); frames(3);
    R1(tilt_ns_open == TILT_START);
    stap(B_HOME); frames(3);
    R1(r1_free() && tu.scr == TU_JAM);
    r1_reset();                                         /* DICE opened from START: HOME back to START, HOME out */
    tilt_start_boot(1); frames(3);
    ns_from_start = 1; tilt_ns_show(); frames(3);
    R1(tilt_ns_open == TILT_NS);
    stap(B_HOME); frames(3);
    R1(tilt_ns_open == TILT_START);
    stap(B_HOME); frames(3);
    R1(r1_free());
    r1_reset();                                         /* TIME */
    te_show(); frames(3);
    R1(tilt_ns_open == TILT_TIME);
    stap(B_HOME); frames(3);
    R1(r1_free());
    r1_reset();                                         /* a dialog, NAME */
    confirm_open(CF_CLEAR_TRK, 0); frames(3);
    R1(ui.confirm);
    stap(B_HOME); frames(3);
    R1(r1_free() && tu.scr == TU_JAM);
    name_open(NK_PROJ_SAVE, 0); frames(3);
    R1(nm.kind);
    stap(B_HOME); frames(3);
    R1(r1_free());
    r1_reset();                                         /* the menu (HOME held) closes on a HOME tap, as before */
    ui.menu = 1; frames(3);
    stap(B_HOME); frames(3);
    R1(!ui.menu);
    r1_reset();                                         /* the picker (hold EDIT), SCENES (hold ARP), the step sheet (OCT+): let go, out */
    {
        uint32_t held[3] = {B_EDIT, B_ARP, B_OCTUP}, i;
        for (i = 0; i < 3u; i++) {
            r1_reset();
            fm1_ms += 1000u; frames(4);
            fm1_in.buttons |= 1u << panel.btn[held[i]]; host_pressed |= 1u << panel.btn[held[i]];
            fm1_ms += 900u; frames(20);
            fm1_in.buttons &= ~(1u << panel.btn[held[i]]); frames(4);
            fm1_ms += 1000u; frames(4);
            R1(r1_free());
            stap(B_HOME); frames(3);
            R1(r1_free() && (tu.scr == TU_MIX || tu.scr == TU_JAM));
        }
    }
    bad += check("TFM-UI fix 3: one HOME tap gets out of every screen, DICE, SETUP, START, TIME, a dialog and NAME; a screen button leaves DICE", ok);
    return bad;
}
/* 4: JAM shows the selected track: its cabinet outlined white, "T2 . SQUELCH" in the header, song.sel is in the redraw signature
 * (round 4: the cabinets replaced the bumpers; the big name for a second is gone) */
static uint16_t r1_px(int32_t x, int32_t y) { return swap16(host_screen[(uint32_t)y * 240u + (uint32_t)x]); }
static int test_r1_jam_sel(void)
{
    int bad = 0, ok = 1;
    ui_power_on();
    tu_off = 0;
    tu_goto(TU_JAM);
    tfm_load(&trk[0], TI_SQUELCH, 0); tfm_load(&trk[1], TI_THUMP, 0);
    frames(100);
    R1(song.sel == 0u && r1_px(5 - 2, 120) == tu_c(TC_TEXT) && r1_px(63 - 2, 120) != tu_c(TC_TEXT));   /* T1's outline white, T2's not */
    R1(tu_hd.l && tu_hd.l[0] == 'T' && tu_hd.l[1] == '1' && str_eq(tu_hd.l + 5, "SQUELCH"));
    turn(EN_ALGO, 1); frames(40);
    R1(song.sel == 1u && r1_px(63 - 2, 120) == tu_c(TC_TEXT) && r1_px(5 - 2, 120) != tu_c(TC_TEXT));  /* the outline moved (song.sel in the signature) */
    R1(tu_hd.l && tu_hd.l[1] == '2' && str_eq(tu_hd.l + 5, "THUMP LAB"));
    bad += check("TFM-UI fix 4: JAM outlines the selected track's cabinet, the header names it", ok);
    return bad;
}
/* 5: the macros: each track keeps its eight places; a single detent moves a continuous macro by exactly 1 from 0 to 127 and
 * every target moves monotonically; a switch steps once per two detents; JAM's knob turns the instrument's own macro, never
 * a switch */
static int test_r1_macros(void)
{
    int bad = 0, ok = 1;
    static const char *const JN[TI_N] = {"PUNCH", "CUTOFF", "WOB", "WARM", "CUT", "BLOOM", "TONE", "BARK", "METAL", "BRIGHT", "VOWEL", "DRIVE"};
    uint32_t i, m, k, nswitch = 0, ncont = 0;
    ui_power_on();
    tu_off = 0;
    for (i = 0; i < TI_N; i++) {
        track_t *t = &trk[0];
        const tfm_instr_t *in = &TFM_INS[i];
        tfm_load(t, i, 0);
        R1(!in->mac[tfm_jam_m(t)].nsw && str_eq(in->mac[tfm_jam_m(t)].name, JN[i]));    /* JAM's: the named one, no switch */
        for (m = 0; m < 8u; m++) {
            const tfm_macro_t *mc = &in->mac[m];
            int32_t prev[4], pos;
            if (mc->nsw) {                                                              /* a switch: one value per two detents */
                int32_t v0, v1;
                nswitch++;
                tfm_macro_put(t, m, 0, 0);
                v0 = t->p[mc->t[0].id];
                fm1_ms += 1000u; tfm_macro_turn_k(t, m, m & 3u, 1);
                R1(t->p[mc->t[0].id] == v0);                                            /* (one detent: nothing yet) */
                fm1_ms += 1000u; tfm_macro_turn_k(t, m, m & 3u, 1);
                v1 = t->p[mc->t[0].id];
                R1(v1 != v0 && tfm_sw_idx(mc, v1) == 1u);                               /* (the second: the next value) */
                fm1_ms += 1000u; tfm_macro_turn_k(t, m, m & 3u, -1);
                R1(t->p[mc->t[0].id] == v1);                                            /* (turned back: counting again) */
                fm1_ms += 1000u; tfm_macro_turn_k(t, m, m & 3u, -1);
                R1(t->p[mc->t[0].id] == v0);
                continue;
            }
            ncont++;
            tfm_macro_put(t, m, 0, 0);
            R1(tfm_macro_get(t, m) == 0);
            for (k = 0; k < 4u && mc->t[k].id != 0xFFu; k++)
                prev[k] = t->p[mc->t[k].id];
            for (pos = 1; pos <= 127; pos++) {
                fm1_ms += 1000u;
                tfm_macro_turn_k(t, m, m & 3u, 1);
                if (tfm_macro_get(t, m) != pos) { R1(0); break; }                       /* exactly 1 a detent, never stalled */
                for (k = 0; k < 4u && mc->t[k].id != 0xFFu; k++) {
                    int32_t v = t->p[mc->t[k].id], up = mc->t[k].hi >= mc->t[k].lo;
                    if (up ? v < prev[k] : v > prev[k]) { R1(0); break; }              /* monotone */
                    prev[k] = v;
                }
            }
            R1(tfm_macro_get(t, m) == 127);
            for (pos = 126; pos >= 0; pos--) {                                          /* and back down the same way */
                fm1_ms += 1000u;
                tfm_macro_turn_k(t, m, m & 3u, -1);
                if (tfm_macro_get(t, m) != pos) { R1(0); break; }
            }
            /* a value changed elsewhere (DEEP, a load): the place is read from it again, once */
            tfm_macro_put(t, m, 60, 0);
            t->p[mc->t[0].id] = (int16_t)mc->t[0].hi;
            R1(tfm_macro_get(t, m) > 100);
        }
    }
    R1(nswitch > 0u && ncont > 60u);
    /* JAM's knob turns it, on a track in the song */
    tfm_load(&trk[0], TI_SQUELCH, 0);
    tfm_macro_put(&trk[0], 0, 30, 0);
    fm1_ms += 1000u; jam_turn(0, 1);
    R1(tfm_macro_get(&trk[0], 0) == 31);
    tfm_load(&trk[1], TI_CHORDETTE, 0);
    tfm_macro_put(&trk[1], 2, 40, 0);
    fm1_ms += 1000u; jam_turn(1, 1);
    R1(tfm_macro_get(&trk[1], 2) == 41);
    bad += check("TFM-UI fix 5: all 12 x 8 macros: a detent moves a continuous one by exactly 1 (0..127, targets monotone), a switch steps per two detents, JAM turns the named macro", ok);
    return bad;
}
/* 5 (responsiveness): JAM redraws only the zones whose picture changed; a turn of one knob redraws the bumpers' band and
 * nothing else (not the header, the scenes or the footer). Prints the worst host frame time and the pixels redrawn. */
static int test_r1_jam_frames(void)
{
    int bad = 0, ok = 1;
    uint32_t f, redrawn_max = 0, idle_redraw = 0;
    double worst = 0, tot = 0;
    ui_power_on();
    tu_off = 0;
    tu_goto(TU_JAM);
    tfm_load(&trk[0], TI_SQUELCH, 0);
    frames(200);
    for (f = 0; f < 20u; f++) {                         /* nothing moves: no zone redraws */
        uint32_t before[TU_ZONES];
        memcpy(before, tu.sig, sizeof before);
        frame();
        idle_redraw += memcmp(before, tu.sig, sizeof before) != 0;
    }
    R1(idle_redraw == 0u);
    for (f = 0; f < 120u; f++) {
        struct timespec a, b;
        uint32_t z, changed = 0;
        uint32_t before[TU_ZONES];
        memcpy(before, tu.sig, sizeof before);
        fm1_ms += 40u;
        turn(EN_K1, f & 1u ? -1 : 1);
        clock_gettime(CLOCK_MONOTONIC, &a);
        frame();
        clock_gettime(CLOCK_MONOTONIC, &b);
        {
            double us = (double)(b.tv_sec - a.tv_sec) * 1e6 + (double)(b.tv_nsec - a.tv_nsec) / 1e3;
            worst = us > worst ? us : worst;
            tot += us;
        }
        for (z = 0; z < sizeof tu.sig / sizeof tu.sig[0]; z++)
            changed += before[z] != tu.sig[z];
        redrawn_max = changed > redrawn_max ? changed : redrawn_max;
    }
    printf("    JAM, a knob turned every frame: at most %u zone (240 x 104 px) redrawn a frame, none when idle\n", redrawn_max);
    R1(redrawn_max <= 1u);
    bad += check("TFM-UI fix 5: JAM redraws only the zone that changed (idle: none; a knob turn: the bumpers' band only)", ok);
    return bad;
}
/* 7: TYPE rules pick from their own group (a LEAD track never shows a pad); changing a rule in SETUP rolls that track at
 * once; RANDOM never rolls a second drum kit unless a rule asks for it */
static int test_r1_rules(void)
{
    int bad = 0, ok = 1;
    uint32_t opt, i, n, j, kits;
    tfm_rng_t r;
    ui_power_on();
    tu_off = 0;
    memset(&favorites.factory[13][16], 0, 16);
    r.s = 12345u;
    for (opt = 1; opt <= 6u; opt++)                     /* TYPE rules: every roll has the type's bit, on every track */
        for (i = 0; i < NTRK; i++) {
            CFG(CF_RULE + i) = (uint8_t)(opt + 1u);
            ns.hold = 0;
            for (n = 0; n < 60u; n++) {
                tu_roll_track(&r, &ns.s, i);
                if (!(TY_INS[ns.s.ins[i] % TI_N] & (1u << (opt - 1u)))) { R1(0); break; }
            }
        }
    memset(&favorites.factory[13][16], 0, 16);          /* the default band: T4 LEAD never HAZE (a pad) */
    for (n = 0; n < 300u; n++) {
        tu_dice_nudge();
        if (ns.s.ins[3] == TI_HAZE || !(TY_INS[ns.s.ins[3] % TI_N] & 8u)) { R1(0); break; }
        if (!(TY_INS[ns.s.ins[0] % TI_N] & 1u) || !(TY_INS[ns.s.ins[1] % TI_N] & 2u) || !(TY_INS[ns.s.ins[2] % TI_N] & 4u)) { R1(0); break; }
    }
    /* SETUP: a rule changed rolls that track at once */
    ns.s.ins[3] = TI_HAZE; ns.hold = 0;
    CFG(CF_RULE + 3) = 4u;                              /* TYPE CHORDS */
    tu_su_step(3, 1);                                   /* -> TYPE LEAD */
    R1(CFG(CF_RULE + 3) == 5u && (TY_INS[ns.s.ins[3] % TI_N] & 8u) && ns.s.ins[3] != TI_HAZE);
    ns.s.ins[3] = TI_HAZE; ns.hold = 1u << 6;           /* T4 held: kept */
    tu_su_step(3, 1);
    R1(ns.s.ins[3] == TI_HAZE);
    /* RANDOM: no second kit unless a rule asks */
    memset(&favorites.factory[13][16], 0, 16);
    ns.hold = 0;
    for (i = 0; i < NTRK; i++) CFG(CF_RULE + i) = 1u;   /* RANDOM x4 */
    kits = 0;
    for (n = 0; n < 2000u; n++) {
        uint32_t k = 0;
        tu_dice_nudge();
        for (j = 0; j < NTRK; j++)
            k += (TY_INS[ns.s.ins[j] % TI_N] & 1u) != 0u;
        if (k > 1u) { R1(0); break; }
        kits += k;
    }
    R1(kits > 100u);                                    /* (a kit still comes up sometimes) */
    CFG(CF_RULE + 0) = 2u;                              /* T1 TYPE DRUMS asks for a kit: the RANDOM ones have none */
    for (n = 0; n < 500u; n++) {
        uint32_t k = 0;
        tu_dice_nudge();
        for (j = 0; j < NTRK; j++)
            k += (TY_INS[ns.s.ins[j] % TI_N] & 1u) != 0u;
        if (k != 1u || !(TY_INS[ns.s.ins[0] % TI_N] & 1u)) { R1(0); break; }
    }
    CFG(CF_RULE + 1) = 2u;                              /* T2 asks too: two kits are the player's rule */
    tu_dice_nudge();
    R1((TY_INS[ns.s.ins[0] % TI_N] & 1u) && (TY_INS[ns.s.ins[1] % TI_N] & 1u));
    memset(&favorites.factory[13][16], 0, 16);
    bad += check("TFM-UI fix 7: TYPE rules pick from their own group (LEAD never HAZE), a SETUP change rolls the track, RANDOM never rolls a second kit", ok);
    return bad;
}
/* 6: bigger text: Barlow 11 / 13 / 16 / 24 px (no TFM face under 11), at most 3 footer hints on every screen */
static int test_r1_text(void)
{
    int bad = 0, ok = 1;
    uint32_t s, o;
    R1(AF_TS.h >= 14 && AF_TS_CAP_H >= 8 && AF_TM_CAP_H >= 9 && AF_TL_CAP_H >= 11 && AF_TX_CAP_H >= 17);   /* (Barlow's caps: 0.7 of the size) */
    R1(AF_PS.h >= 13 && AF_PN.h >= 16 && AF_PM.h >= 21 && AF_PL.h >= 40);
    for (s = 0; s < TU_N; s++) {
        if (s == TU_DEEP)
            continue;
        ui_power_on();
        tu_off = 0;
        tu_goto(s);
        frames(4);
        R1(tu_ft.n >= 1u && tu_ft.n <= 3u);
    }
    for (o = 0; o < 4u; o++) {                          /* the modal screens, SETUP, the picker */
        ui_power_on();
        tu_off = 0;
        tu_goto(TU_JAM);
        if (o == 0u) { stap(B_ENV); }
        if (o == 1u) { stap(B_ENV); stap(B_ENV); }
        if (o == 2u) { tilt_start_boot(1); }
        if (o == 3u) { te_show(); }
        frames(4);
        R1(tu_ft.n >= 1u && tu_ft.n <= 3u);
    }
    bad += check("TFM-UI fix 6: no TFM text under 11 px (Barlow 11 / 13 / 16 / 24), at most 3 footer hints on every screen", ok);
    return bad;
}
