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
