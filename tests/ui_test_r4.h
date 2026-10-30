/* TFM-UI round 4: the tests of the round's items (included by ui_test.c after round 3; same helpers) */
#define R4(c) do { if (!(c)) { ok = 0; fprintf(stderr, "R4 line %d ", __LINE__); } } while (0)
/* 1: blank scenes play silence */
static int test_r4_blank(void)
{
    int bad = 0, ok = 1;
    uint32_t n, k, pl;
    ui_power_on();
    tu_off = 0;
    r3_two_scenes();                                    /* song 1: A (note 55), B (note 66); C..H blank */
    R4(project_used(0) && project_used(1) && !project_used(2));
    chain_config.count = 4;
    chain_config.row[0] = chain_row_of(0, 1);
    chain_config.row[1] = chain_row_of(1, 1);
    chain_config.row[2] = chain_row_of(2, 2);           /* C: blank, two repeats */
    chain_config.row[3] = chain_row_of(0, 1);
    R4(chain_prepare() == 0 && chain.armed);
    R4(chain.config.row[2].slot[0] == CHAIN_SILENT && chain.config.row[2].slot[3] == CHAIN_SILENT && chain.config.row[1].slot[0] == 1u);
    R4(chain_config.row[2].slot[0] == 2u);              /* the song as written is untouched */
    events_block(32);
    R4(chain.running && chain.row == 0u && trk[0].seq_notes[0] == 55);
    pl = 16u * div_samples(2);                          /* LEN 16 of 1/16: a 4/4 bar */
    n = 1u + lanes_until(0, 4000);
    R4(chain.row == 1u && lanes_near(n - 1u, pl) && trk[0].seq_notes[0] == 66);
    n = 1u + lanes_until(1, 4000);
    R4(chain.row == 2u);
    {
        int quiet = 1;
        n = 0;
        while (n < 8000u && chain.running && chain.row == 2u) {
            events_block(32);
            n++;
            for (k = 0; k < NTRK; k++) quiet &= chain.row != 2u || (!trk[k].seq_n && seq_steps(&trk[k]) == chain_silent);
        }
        R4(quiet);
        R4(chain.row == 3u && lanes_near(n, 2u * pl));  /* an all-blank section: 1 bar x its 2 repeats */
    }
    R4(trk[0].seq_notes[0] == 55);                      /* and A plays on after it */
    seq_stop(); transport_req = 0; events_block(32);
    R4(tsg_row_bars(&chain_config.row[2]) == 2u && tsg_row_bars(&chain_config.row[0]) >= 1u);
    /* no blank / empty message in the quick chain */
    chain_defaults(&chain_config);
    ui.msg[0] = 0;
    quick_chain(2);
    R4(chain_config.count == 1u && chain_config.row[0].slot[0] == 2u && !str_eq(ui.msg, "SCENE EMPTY"));
    /* SONG's knob: "-" A B .. H, "-" first, no wrap */
    chain_defaults(&chain_config);
    chain_config.count = 1;
    chain_config.row[0] = (chain_row_t){{CHAIN_SILENT, 0, 7, 3}, 1};
    tu_goto(TU_SONG);
    ui.song_row = 0;
    frames(8);
    turn(EN_K1, -1);                                    /* "-" stays "-" */
    R4(chain_config.row[0].slot[0] == CHAIN_SILENT);
    turn(EN_K1, 1);
    R4(chain_config.row[0].slot[0] == 0u);
    turn(EN_K1, 1);
    R4(chain_config.row[0].slot[0] == 1u);
    for (k = 0; k < 8u; k++) turn(EN_K1, 1);
    R4(chain_config.row[0].slot[0] == 7u);              /* H, and no wrap */
    turn(EN_K1, 1);
    R4(chain_config.row[0].slot[0] == 7u);
    turn(EN_K2, -1);                                    /* A -> "-" (left of A) */
    R4(chain_config.row[0].slot[1] == CHAIN_SILENT);
    turn(EN_K3, 1);
    R4(chain_config.row[0].slot[2] == 7u);
    turn(EN_K4, -1);
    R4(chain_config.row[0].slot[3] == 2u);
    /* an old chain keeps its meaning: stored A..D 0..3, "-" 4, E..H 5..8 */
    R4(chain_dec_b(0) == 0u && chain_dec_b(3) == 3u && chain_dec_b(4) == CHAIN_SILENT && chain_dec_b(8) == 7u);
    bad += check("round 4 item 1: blank scenes play silence (section lengths, no error, knob order \"-\" A..H)", ok);
    return bad;
}
/* 2: REC state on every header but START, DICE, TIME: four dots, a red ring = armed, a red disc = recording now, the selected larger */
static uint32_t r4_ink(uint32_t x0, uint32_t x1, uint16_t bg)       /* pixels not the background in the header's row 11, x0..x1 */
{
    uint32_t x, n = 0;
    for (x = x0; x <= x1; x++)
        n += px_at(x, 11) != bg;
    return n;
}
static uint16_t r4_c(uint16_t c) { uint16_t v = tu_c(c); return (uint16_t)(v >> 8 | v << 8); }   /* (the screen buffer holds the panel's byte order) */
static int test_r4_recdots(void)
{
    static const uint8_t SCR[] = {TU_JAM, TU_MIX, TU_SOUND, TU_NOTES, TU_DRUMS, TU_SCENES, TU_SONG, TU_SONGS, TU_FX, TU_SPACE, TU_ARP, TU_CLOCK, TU_CLOCK2, TU_MOTION};
    int bad = 0, ok = 1;
    uint32_t i, k;
    ui_power_on();
    tu_off = 0;
    tm.on = 0;
    for (i = 0; i < sizeof SCR; i++) {
        uint32_t st;
        for (st = 0; st < 3u; st++) {                   /* none armed; T2 armed, stopped; T2 armed and playing */
            song.sel = 2;
            song.rec = st ? 2u : 0u;
            song.playing = st == 2u;
            tu_goto(SCR[i]);
            frames(80);
            for (k = 0; k < NTRK; k++) {
                uint32_t cx = 14u + 10u * k, rb = k == 2u ? 3u : 2u;
                uint16_t bg = px_at(cx, 3);
                int armed = k == 1u && st, rec = armed && st == 2u;
                uint32_t ring = r4_ink(cx + rb + 1u, cx + rb + 2u, bg) + r4_ink(cx - rb - 2u, cx - rb - 1u, bg);
                R4(armed ? ring >= 2u : ring == 0u);                                    /* a ring only when armed */
                R4(rec ? px_at(cx, 11) != r4_c(TU_TC[k]) && px_at(cx, 11) == px_at(cx + 1u, 11)   /* a disc, not the track's dot */
                       : px_at(cx, 11) == r4_c(TU_TC[k]));
                if (!ok) {
                    fprintf(stderr, "R4 screen %u state %u track %u ", (unsigned)SCR[i], (unsigned)st, (unsigned)k);
                    ok = 1; bad++;
                }
            }
        }
    }
    song.rec = 0; song.playing = 0;
    /* the selected track's dot is larger: T3 selected, none armed, JAM */
    song.sel = 2; tu_goto(TU_JAM); frames(80);
    R4(r4_ink(14 + 20 - 4, 14 + 20 + 4, px_at(60, 3)) > r4_ink(14 - 4, 14 + 4, px_at(60, 3)));
    /* arm on SEQ, go HOME: JAM shows it */
    tu_goto(TU_JAM); song.sel = 0; song.rec = 0;
    stap(B_SEQ); frames(32);
    stap(B_REC); frames(32);
    stap(B_HOME); frames(80);
    R4(tu.scr == TU_JAM && (song.rec & 1u) && r4_ink(14 + 4, 14 + 5, px_at(60, 3)) >= 1u);
    song.rec = 0;
    bad += check("round 4 item 2: REC dots in every header but START, DICE, TIME (unarmed, a red ring when armed, a red disc when recording; the selected larger)", ok);
    return bad;
}
/* 3: JAM v2: the scoreboard, four cabinets, the scene lamps */
static uint32_t r4_count(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint16_t c)    /* pixels of colour c (tu_c'd) in a box */
{
    int32_t x, y;
    uint32_t n = 0;
    for (y = y0; y <= y1; y++)
        for (x = x0; x <= x1; x++)
            n += swap16(host_screen[(uint32_t)y * 240u + (uint32_t)x]) == tu_c(c);
    return n;
}
static uint32_t r4_lum(int32_t x, int32_t y)
{
    uint16_t v = swap16(host_screen[(uint32_t)y * 240u + (uint32_t)x]);
    return ((v >> 11) & 31u) * 2u + ((v >> 5) & 63u) + (v & 31u) * 2u;
}
static int test_r4_jam(void)
{
    int bad = 0, ok = 1;
    uint32_t n, f, lit;
    ui_power_on();
    tu_off = 0;
    tm.on = 0;
    tilt_timer_min = 0;
    song.rec = 0;
    song.playing = 0;
    tfm_load(&trk[0], TI_SQUELCH, 0); tfm_load(&trk[1], TI_THUMP, 0); tfm_load(&trk[2], TI_HAZE, 0); tfm_load(&trk[3], TI_VOX, 0);
    song.sel = 2;
    tu_goto(TU_JAM);
    frames(100);
    /* selection: T3's cabinet outlined white, the others not; the header says T3 . HAZE */
    R4(r1_px(CAB_X(2) - 2, 120) == tu_c(TC_TEXT) && r1_px(CAB_X(1) - 2, 120) != tu_c(TC_TEXT));
    R4(tu_hd.l && tu_hd.l[0] == 'T' && tu_hd.l[1] == '3' && str_eq(tu_hd.l + 5, "HAZE"));
    /* timer off: no draining dots; the song's name and scene take the timer's place */
    R4(r4_count(14, 66, 140, 72, TC_AMBER) < 5u && r4_count(14, 66, 140, 72, TC_REC) < 5u);
    R4(r4_count(14, 31, 120, 63, TC_AMBER) > 20u);
    /* timer on: the time big and the dots under it, draining; red for the last minute */
    tilt_timer_min = 10; tm.on = 1; tm.left = 600; tm.end = fm1_ms + 600000u;
    frames(60);
    R4(r4_count(14, 66, 140, 72, TC_AMBER) >= 30u * 9u - 40u && r4_count(14, 66, 140, 72, TC_REC) < 5u);
    tm.left = 300; tm.end = fm1_ms + 300000u; frames(1100);
    lit = r4_count(14, 66, 140, 72, TC_AMBER);
    R4(lit > 0u && lit < 30u * 9u - 40u);                                    /* half of it gone */
    tm.left = 30; tm.end = fm1_ms + 30000u; frames(1100);
    R4(r4_count(14, 66, 140, 72, TC_REC) > 0u && r4_count(14, 66, 140, 72, TC_AMBER) < 5u);
    tm.on = 0; tilt_timer_min = 0; frames(60);
    /* mute: the cabinet dimmed, MUTED across it */
    n = r4_lum(CAB_X(1) + 27, 100);
    trk[1].p[P_MUTE] = 1; frames(100);
    R4(r4_lum(CAB_X(1) + 27, 100) < n);
    R4(r4_count(CAB_X(1) + 10, 108, CAB_X(1) + 44, 130, TC_TEXT) > 10u);     /* (MUTED, in the text colour) */
    trk[1].p[P_MUTE] = 0; frames(100);
    R4(r4_lum(CAB_X(1) + 27, 100) == n);
    /* the REC dot: dim unarmed, a red ring when armed, a red disc when it records */
    R4(r4_count(CAB_X(1) + 41, 75 + CAB_Y, CAB_X(1) + 49, 83 + CAB_Y, TC_REC) == 0u);
    song.rec = 2; frames(100);
    R4(r4_count(CAB_X(1) + 41, 75 + CAB_Y, CAB_X(1) + 49, 83 + CAB_Y, TC_REC) > 4u && r1_px(CAB_X(1) + 45, 79 + CAB_Y) != tu_c(TC_REC));
    song.playing = 1; frames(100);
    R4(r1_px(CAB_X(1) + 45, 79 + CAB_Y) == tu_c(TC_REC));
    song.playing = 0; song.rec = 0; frames(100);
    /* the scene lamps: the one in use lit amber, a queued one outlined and blinking */
    R4(r4_count(10, 184, 34, 206, TC_AMBER) < 5u);                           /* A is not the scene (the music is in B..) */
    tsc.cue = 3;
    lit = 0;
    for (f = 0; f < 40u; f++) {
        frames(48);
        lit += r4_count(94 - 1, 183, 94 + 25, 207, RGB(255, 226, 168)) > 10u;
    }
    R4(lit > 3u && lit < 38u);                                               /* it blinks */
    tsc.cue = SCENE_NONE;
    bad += check("round 4 item 3: JAM v2 (selection, mute, REC dot, timer on / off and its dots, the queued scene's lamp)", ok);
    return bad;
}
