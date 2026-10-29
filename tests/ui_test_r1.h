/* TFM-UI fix round 1: the tests of the round's items (included by ui_test.c after the phase tests; same helpers) */
/* 1: NOTES and DRUMS, SELECT one step at a time through every step (the bar follows), PRESETS a bar and the same step in it */
#define R1(c) do { if (!(c)) { ok = 0; fprintf(stderr, "R1 line %d scr=%u len=%u cur=%u bar=%u ", __LINE__, scr, len, (unsigned)ui.cursor, (unsigned)tq_bar); } } while (0)
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
            stap(B_SEQ); stap(B_SEQ);
            R1(tu.scr == (scr ? TU_DRUMS : TU_NOTES));
            ui.cursor = 0; tq_bar = 0;
            for (i = 0; i < len; i++) {                 /* up: every step 1..LEN, empty or not, the shown bar following */
                R1(ui.cursor == i && tq_bar == i / 16u);
                seen |= i < 32u ? 1u << i : 0u;
                turn(EN_SELECT, 1);
            }
            ok &= ui.cursor == len - 1u;                /* (the end stays the end) */
            for (i = len; i-- > 0;) {
                R1(ui.cursor == i && tq_bar == i / 16u);
                turn(EN_SELECT, -1);
            }
            R1(ui.cursor == 0u && (len >= 32u ? seen == 0xFFFFFFFFu : seen == (1u << len) - 1u));
            if (len > 16u) {                            /* PRESETS: the next bar, the same step in it */
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
