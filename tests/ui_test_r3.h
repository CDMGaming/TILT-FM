/* TFM-UI round 3: the tests of the round's items (included by ui_test.c after round 2; same helpers) */
#define R3(c) do { if (!(c)) { ok = 0; fprintf(stderr, "R3 line %d ", __LINE__); } } while (0)
static uint32_t r3_notes(const track_t *t, uint32_t *ties, uint32_t *velsum)   /* the notes, the ties, the velocities of a pattern */
{
    uint32_t i, n = 0;
    *ties = *velsum = 0;
    for (i = 0; i < NSTEP; i++) {
        if (t->step[i].time == ST_NOTE && t->step[i].n) { n++; *velsum += t->step[i].vel * (1u + t->step[i].note[0]); }
        if (t->step[i].time == ST_TIE) (*ties)++;
    }
    return n;
}
/* 1: NOTES: K1 the step through every step, SELECT scrolls the rows, K2..K4 on the note at the cursor, OCT+ grabs / drops */
static int test_r3_notes(void)
{
    int bad = 0, ok = 1;
    uint32_t n0, t0, v0, n1, t1, v1, i, base;
    int8_t base0;
    track_t *t;
    ui_power_on();
    tu_off = 0;
    tu_goto(TU_JAM);
    tfm_load(&trk[0], TI_SQUELCH, 0);
    song.sel = 0;
    t = &trk[0];
    t->p[P_SLEN] = 32;
    for (i = 0; i < NSTEP; i++) step_clear(&t->step[i]);
    t->step[2] = (step_t){{60}, 1, ST_NOTE, 0, 90, 0, 0};                   /* a note, 3 steps long (two ties) */
    t->step[3].time = ST_TIE; t->step[4].time = ST_TIE;
    t->step[8] = (step_t){{67}, 1, ST_NOTE, 0, 70, 0, 0};
    t->step[9].time = ST_TIE;
    t->step[31] = (step_t){{64}, 1, ST_NOTE, 0, 110, 0, 0};
    stap(B_SEQ); stap(B_SEQ);
    R3(tu.scr == TU_NOTES);
    ui.cursor = 0; tq_bar = 0;
    for (i = 1; i < 32u; i++) {                                              /* K1: every step, empty ones too; the bar follows */
        turn(EN_K1, 1);
        R3(ui.cursor == i && tq_bar == i / 16u);
    }
    for (i = 31; i-- > 0;) {
        turn(EN_K1, -1);
        R3(ui.cursor == i);
    }
    /* SELECT scrolls the rows, the cursor stays */
    ui.cursor = 2; tq_bar = 0;
    frames(32);
    base0 = tq_base;
    turn(EN_SELECT, 1); frames(32);
    R3(ui.cursor == 2u && tq_base != base0 && tl_in_scale(t, tq_base));
    base = (uint32_t)(uint8_t)tq_base;
    turn(EN_SELECT, 1); frames(32);
    R3((uint32_t)(uint8_t)tq_base > base && ui.cursor == 2u);
    turn(EN_SELECT, -2); frames(32);
    R3(tq_base == base0 && ui.cursor == 2u);
    /* K2..K4 on an empty step do nothing */
    ui.cursor = 6;
    n0 = r3_notes(t, &t0, &v0);
    turn(EN_K2, 1); turn(EN_K3, 1); turn(EN_K4, 1);
    n1 = r3_notes(t, &t1, &v1);
    R3(n1 == n0 && t1 == t0 && v1 == v0);
    /* GRAB: OCT+ tapped on the note at step 3 (index 2) */
    ui.cursor = 2;
    press(B_OCTUP); frames(32);
    R3(tq_grab == 1u);
    n0 = r3_notes(t, &t0, &v0);
    turn(EN_K1, 1);                                                          /* one step later, its ties with it */
    R3(ui.cursor == 3u && t->step[3].time == ST_NOTE && t->step[3].note[0] == 60 && t->step[4].time == ST_TIE &&
       t->step[5].time == ST_TIE && t->step[2].time != ST_NOTE && t->step[2].time != ST_TIE && t->step[3].vel == 90);
    turn(EN_K1, -1);
    R3(ui.cursor == 2u && t->step[2].note[0] == 60 && t->step[3].time == ST_TIE && t->step[4].time == ST_TIE && t->step[5].time != ST_TIE);
    turn(EN_K1, 3);                                                          /* up to the next note: 5, 6 (the tail reaches step 8, the other note) */
    R3(ui.cursor == 5u && t->step[7].time == ST_TIE);                        /* (step index 7 is the last tie, 8 is the other note) */
    turn(EN_K1, 1);                                                          /* the tail would run into the note at 8: nowhere to go */
    R3(ui.cursor == 5u && t->step[8].note[0] == 67 && t->step[8].vel == 70);
    turn(EN_K2, 1);                                                          /* its pitch, in the scale */
    R3(t->step[5].note[0] > 60 && tl_in_scale(t, t->step[5].note[0]));
    {   /* behind: the start of the pattern stops it */
        turn(EN_K1, -5);
        R3(ui.cursor == 0u && t->step[0].time == ST_NOTE);
        turn(EN_K1, -1);
        R3(ui.cursor == 0u);
    }
    press(B_OCTUP); frames(32);                                              /* tapped again: DROP */
    R3(tq_grab == 0u);
    n1 = r3_notes(t, &t1, &v1);
    R3(n1 == n0 && t1 == t0);                                                /* no note lost or duplicated, the ties all there */
    turn(EN_K1, 1);                                                          /* dropped: K1 is the cursor again, the note stays */
    R3(ui.cursor == 1u && t->step[0].time == ST_NOTE);
    /* the last step and the end of the pattern */
    ui.cursor = 31; frames(32);
    press(B_OCTUP); frames(32);
    R3(tq_grab == 1u);
    turn(EN_K1, 1);
    R3(ui.cursor == 31u && t->step[31].time == ST_NOTE && t->step[31].vel == 110);
    press(B_OCTUP); frames(32);
    /* a tap on an empty step grabs nothing; a hold is the step sheet, not a grab */
    ui.cursor = 12;
    press(B_OCTUP); frames(32);
    R3(tq_grab == 0u);
    ui.cursor = 8;
    hold(B_OCTUP); frames(32);
    R3(tq_grab == 0u);
    /* leaving the screen drops it */
    ui.cursor = 8;
    press(B_OCTUP); frames(32);
    R3(tq_grab == 1u);
    tu_goto(TU_JAM);
    R3(tq_grab == 0u);
    bad += check("TFM-UI round 3 NOTES: K1 steps through every step, SELECT scrolls the rows, K2-K4 act on the note (nothing on an empty step), "
                 "OCT+ GRABs, K1 moves it with its ties (not past a note or the ends), K2 its pitch, OCT+ DROPs: nothing lost or doubled", ok);
    return bad;
}
