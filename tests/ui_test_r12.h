/* TILT-FM round 12 (tfm-spec/ROUND-12.md): included by ui_test.c after ui_test_r11.h */
#define R12(c) do { if (!(c)) { ok = 0; printf("  R12 line %d\n", __LINE__); } } while (0)

/* item 1: FX's CHARACTER page */
static int test_r12_char(void)
{
    static const struct { uint8_t type; const char *l[3]; } TY[] = {
        {1, {"DRIVE", "TONE", "LEVEL"}}, {3, {"DRIVE", "TONE", "LEVEL"}}, {5, {"BITS", "RATE", "LPF"}},
        {6, {"RATE", "DEPTH", "FEEDBACK"}}, {8, {"RATE", "DEPTH", "FEEDBACK"}}};
    int bad = 0, ok = 1;
    uint32_t i, k;
    ui_power_on();
    tu_off = 0;
    tu_goto(TU_JAM); frames(4);
    stap(B_FX); R12(tu.scr == TU_FX);
    stap(B_FX); R12(tu.scr == TU_CHAR);
    stap(B_FX); R12(tu.scr == TU_SPACE);
    stap(B_FX); R12(tu.scr == TU_FX);
    bad += check("round 12 FX: FX cycles TRACK FX -> CHARACTER -> SPACE -> TRACK FX", ok);

    ok = 1;
    tu_goto(TU_CHAR); frames(2);
    for (i = 0; i < NELEM(TY); i++) {
        TSEL->p[P_ITYPE] = TY[i].type;
        tu_fx_fill();
        for (k = 0; k < 3u; k++)
            R12(str_eq(ro.kl[k], TY[i].l[k]));
        R12(str_eq(ro.kl[3], "AMOUNT") && ro.pages == 2u && ro.line && !str_eq(ro.line, "PICK A CHARACTER ON TRACK FX (K3)"));
    }
    bad += check("round 12 FX: CHARACTER's K1-K3 named as the type means them (DRIVE TONE LEVEL, BITS RATE LPF, RATE DEPTH FEEDBACK), K4 AMOUNT", ok);

    ok = 1;
    TSEL->p[P_ITYPE] = 8;
    TSEL->p[P_IA] = TSEL->p[P_IB] = TSEL->p[P_IC] = 40;
    TSEL->p[P_IMIX] = 40;
    for (k = 0; k < 4u; k++) {
        uint32_t id = k < 3u ? P_IA + k : P_IMIX, j;
        int16_t was[4] = {TSEL->p[P_IA], TSEL->p[P_IB], TSEL->p[P_IC], TSEL->p[P_IMIX]};
        turn(EN_K1 + k, 4); frames(2);
        R12(TSEL->p[id] > was[k]);
        for (j = 0; j < 4u; j++)
            R12(j == k || TSEL->p[j < 3u ? P_IA + j : P_IMIX] == was[j]);
    }
    R12(TSEL->p[P_ITYPE] == 8);                         /* (the character stays) */
    {   /* REC on while playing: recorded as motion, as the other FX knobs */
        uint32_t m0;
        motion_clear(TSEL);
        song.rec = 1u << song.sel;
        seq_start(); frames(4);
        m0 = motion_count(TSEL);
        turn(EN_K2, 4); frames(4);
        R12(motion_count(TSEL) > m0);
        song.rec = 0;
        press(B_PLAY); events_block(32); frames(4);
    }
    bad += check("round 12 FX: CHARACTER's K1-K4 move P_IA P_IB P_IC and P_IMIX, each alone, recorded as motion", ok);

    ok = 1;
    TSEL->p[P_ITYPE] = 0;
    tu_fx_fill();
    R12(ro.line && str_eq(ro.line, "PICK A CHARACTER ON TRACK FX (K3)"));
    {
        int16_t was[4] = {TSEL->p[P_IA], TSEL->p[P_IB], TSEL->p[P_IC], TSEL->p[P_IMIX]};
        for (k = 0; k < 4u; k++) { turn(EN_K1 + k, 4); frames(2); }
        R12(TSEL->p[P_IA] == was[0] && TSEL->p[P_IB] == was[1] && TSEL->p[P_IC] == was[2] && TSEL->p[P_IMIX] == was[3] && !TSEL->p[P_ITYPE]);
    }
    bad += check("round 12 FX: CHARACTER OFF says PICK A CHARACTER ON TRACK FX (K3), the knobs do nothing", ok);
    ui_power_on();
    return bad;
}

/* item 2: hold ENV, a small sheet: F3 MUTATE (K2 its mode, SETUP's row the same value), G3 STUCK */
static uint32_t r12_notes(const track_t *t)
{
    uint32_t n, q = 0;
    for (n = 0; n < NSTEP; n++)
        q += t->step[n].time == ST_NOTE && t->step[n].n;
    return q;
}
static void r12_lkey(uint32_t place) { uint32_t k = key_at(0, place); r9_key(k, 1); frames(2); r9_key(k, 0); frames(2); }
static int test_r12_env(void)
{
    int bad = 0, ok = 1;
    uint32_t q, tmin;
    char b[28];
    track_t *t;
    ui_power_on();
    tu_off = 0;
    layer_seen |= 1u;
    song.sel = 2;
    t = TSEL;
    tfm_load(t, TI_SQUELCH, 0);
    for (q = 0; q < NSTEP; q++) step_clear(&t->step[q]);
    t->p[P_SLEN] = 16;
    CFG(CF_MUT) = 0;
    tu_goto(TU_JAM); fm1_ms += 4000u; scrn.st = SCR_ON; scrn.idle = fm1_ms; frames(8);
    btn_down(B_ENV); frames(500);
    R12(ui.layer == LAYER_TILT && tu_hold_on());
    ui.force = 1; tu_hold_draw();
    R12(str_eq(tu_ft.k[0], "F3") && str_eq(tu_ft.a[0], "MUTATE") && str_eq(tu_ft.a[1], "STUCK") && str_eq(tu_ft.a[2], "MODE"));
    r12_lkey(0);                                        /* F3: MUTATE (PATTERN) */
    R12(r12_notes(t) > 0u);
    undo_swap();
    R12(r12_notes(t) == 0u);                            /* (one undo) */
    r12_lkey(1);                                        /* G3: STUCK on an empty track: it starts one */
    R12(r12_notes(t) > 0u);
    undo_swap();
    R12(r12_notes(t) == 0u);
    tmin = tilt_timer_min;
    r12_lkey(2); r12_lkey(3);                           /* A3, B3: nothing now (MUTATE moved to F3, TIMER to SETUP) */
    turn(EN_K1, 3); frames(2);                          /* K1: nothing (the TIMER is SETUP's) */
    R12(r12_notes(t) == 0u && tilt_timer_min == tmin && tilt_ns_open == 0u);
    turn(EN_K2, 1); frames(2);                          /* K2: the mode, PATTERN -> SOUND */
    R12(prf_mut_now() == 1u && CFG(CF_MUT) == 1u);
    tu_su_row_text(12, b);
    R12(str_eq(b, "SOUND"));                            /* (SETUP's MUTATE row: the same value) */
    turn(EN_K2, 1); frames(2);
    R12(prf_mut_now() == 2u);
    btn_up(B_ENV); frames(30); fm1_ms += 3000u; frames(4);
    ui.layer = ui.ly = ui.lock = 0; frames(4);
    CFG(CF_MUT) = 0; frames(2);                         /* SETUP's row changed: the hold's mode follows */
    R12(prf_mut_now() == 0u);
    bad += check("round 12 hold ENV: F3 MUTATE, G3 STUCK (one undo each), K2 the mode = SETUP's MUTATE row, A3 B3 K1 nothing, the footer F3 MUTATE G3 STUCK K2 MODE", ok);

    ok = 1;                                             /* CLASSIC keeps Felucca's map: A3 MUTATE */
    tu_off = 1;
    for (q = 0; q < NSTEP; q++) step_clear(&t->step[q]);
    tilt_key(key_at(0, 2), TT_MUTATE);
    R12(r12_notes(t) > 0u && !tu_hold_on());
    tu_off = 0;
    bad += check("round 12 hold ENV: CLASSIC keeps Felucca's map (A3 MUTATE)", ok);
    ui_power_on();
    return bad;
}

/* item 3: hold SEQ v2 (the drawing; the tools are ui_layer.c's, their own tests unchanged) */
static int test_r12_seq_v2(void)
{
    int bad = 0, ok = 1;
    track_t *t;
    ui_power_on();
    tu_off = 0;
    layer_seen |= 1u;
    song.sel = 0;
    t = TSEL;
    tfm_load(t, TI_THUMP, 0);
    t->p[P_SLEN] = 16;
    tu_goto(TU_JAM); fm1_ms += 4000u; scrn.st = SCR_ON; scrn.idle = fm1_ms; frames(8);
    btn_down(B_SEQ); frames(500);
    R12(ui.layer == LAYER_SEQ && tu_hold_on());
    ui.force = 1; tu_hold_draw();
    R12(str_eq(tu_ft.k[2], "OCT-") && str_eq(tu_ft.a[2], "AS OPENED"));
    tl_act_p = 0;
    r12_lkey(12);                                       /* D5: x2 */
    R12(t->p[P_SLEN] == 32 && tl_act_p == 13u && fm1_ms - tl_act_t < 300u);   /* (the tile flashes) */
    btn_up(B_SEQ); frames(30); fm1_ms += 3000u; frames(4);
    ui.layer = ui.ly = ui.lock = 0; frames(4);
    bad += check("round 12 hold SEQ v2: footer WHITE TOOL . BLACK LANE . OCT- AS OPENED, D5 doubles the phrase and its tile flashes", ok);
    ui_power_on();
    return bad;
}
