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
    R4(r4_count(7, 180, 25, 200, TC_AMBER) < 5u);                           /* A is not the scene (the music is in B..) */
    tsc.cue = 3;
    lit = 0;
    for (f = 0; f < 40u; f++) {
        frames(48);
        lit += r4_count(69 - 1, 177, 69 + 20, 201, RGB(255, 226, 168)) > 10u;
    }
    R4(lit > 3u && lit < 38u);                                               /* it blinks */
    tsc.cue = SCENE_NONE;
    bad += check("round 4 item 3: JAM v2 (selection, mute, REC dot, timer on / off and its dots, the queued scene's lamp)", ok);
    return bad;
}
/* 4: NOTES v2 */
static int r4_in_view(const track_t *t, uint32_t i)      /* every note of step i is on a row of the window (the note's floor in the scale) */
{
    uint32_t j;
    int32_t top = tq_base, bot = tq_up(t, top, 1 - TQ_ROWS);
    for (j = 0; j < t->step[i].n; j++) {
        int32_t fl = tq_floor(t, t->step[i].note[j]);
        if (fl > top || fl < bot)
            return 0;
    }
    return 1;
}
static int test_r4_notes(void)
{
    int bad = 0, ok = 1;
    uint32_t i, z, r, kC4 = key_at(0, 4);
    track_t *t;
    ui_power_on();
    tu_off = 0;
    song.rec = 0;
    song.playing = 0;
    tu_goto(TU_JAM);
    tfm_load(&trk[0], TI_SQUELCH, 0);
    song.sel = 0;
    t = &trk[0];
    t->p[P_SLEN] = 64;
    t->p[P_ROOT] = 2;                                    /* D, and a minor scale: the rows follow the key */
    t->p[P_SCALE] = 1;
    for (i = 0; i < NSTEP; i++) step_clear(&t->step[i]);
    for (i = 0; i < 16u; i++) t->step[i] = (step_t){{(uint8_t)(62 + (i & 7u))}, 1, ST_NOTE, 0, 90, 0, 0};          /* bar 1: D4 up */
    for (i = 16; i < 32u; i += 2u) t->step[i] = (step_t){{(uint8_t)(36 + (i - 16u) * 5u)}, 1, ST_NOTE, 0, 90, 0, 0};   /* bar 2: C2 .. C7 */
    t->step[63] = (step_t){{49}, 1, ST_NOTE, 0, 100, 0, 0};                                                          /* bar 4: one note, out of the key maybe */
    stap(B_SEQ);
    R4(tu.scr == TU_NOTES && tq_zoom == 0u);
    /* the rows are the scale's notes, whatever the key */
    frames(64);
    for (r = 0; r < TQ_ROWS; r++)
        R4(tl_in_scale(t, tq_up(t, tq_base, -(int32_t)r)));
    t->p[P_ROOT] = 7; t->p[P_SCALE] = 0; frames(64);
    tq_fit_sig = 0xFFFFFFFFu; frames(64);
    for (r = 0; r < TQ_ROWS; r++)
        R4(tl_in_scale(t, tq_up(t, tq_base, -(int32_t)r)));
    t->p[P_ROOT] = 2; t->p[P_SCALE] = 1; tq_fit_sig = 0xFFFFFFFFu;
    /* every step reachable and its note in view, at every zoom; the zoom keeps the cursor */
    for (z = 0; z < 3u; z++) {
        uint32_t v0, span;
        ui.cursor = 0; tq_bar = 0; frames(64);
        for (i = 0; i < 64u; i++) {
            R4(ui.cursor == i);
            if (t->step[i].n)
                R4(r4_in_view(t, i));
            tq_view(&v0, &span);
            R4(ui.cursor >= v0 && ui.cursor < v0 + span);
            if (!ok)
                fprintf(stderr, "R4 zoom %u step %u ", (unsigned)z, (unsigned)i);
            turn(EN_K1, 1);
            frames(32);
        }
        turn(EN_PRESET, 1); frames(32);
        R4(ui.cursor == 63u);
    }
    R4(tq_zoom == 2u);
    turn(EN_PRESET, 1); R4(tq_zoom == 2u);                                   /* (no further) */
    ui.cursor = 20; tq_bar = 1;
    turn(EN_PRESET, -1); frames(32);
    {
        uint32_t v0, span;
        R4(tq_zoom == 1u && ui.cursor == 20u);
        tq_view(&v0, &span);
        R4(v0 == 0u && span == 32u);
        turn(EN_PRESET, -1); frames(32);
        tq_view(&v0, &span);
        R4(tq_zoom == 0u && ui.cursor == 20u && v0 == 16u && span == 16u);
    }
    /* the strip: 4 bars, the bar shown outlined amber; ▲ for notes out of the rows */
    frames(64);
    R4(r4_count(TQ_GX + 52, 26, TQ_GX + 52 + 48, 26, TC_AMBER) > 10u && r4_count(TQ_GX, 26, TQ_GX + 48, 26, TC_AMBER) == 0u);
    R4(r4_count(210, 50, 230, 56, TC_AMBER) > 2u);                           /* bar 2 spans five octaves: some notes are above the rows */
    /* K2 on an empty step inserts the last note used there, REC off */
    ui.cursor = 40; tq_bar = 2; last_note = 69; frames(32);
    R4(!t->step[40].n && !song.rec);
    turn(EN_K2, 1);
    R4(t->step[40].time == ST_NOTE && t->step[40].n == 1u && tl_in_scale(t, t->step[40].note[0]) && t->step[40].note[0] == tq_floor(t, 69));
    turn(EN_K2, 1);                                                          /* (now a note: K2 moves it) */
    R4(t->step[40].note[0] > tq_floor(t, 69));
    /* REC on, stopped, a key held: K1 sets the length of the note being entered; the cursor lands after it */
    song.rec = 1; tq_bar = 0; ui.cursor = 44; frames(32);
    fm1_in.notes |= 1u << kC4; host_notes |= 1u << kC4; frame();
    R4(t->step[44].time == ST_NOTE && t->step[44].n == 1u);
    turn(EN_K1, 1); turn(EN_K1, 1);
    R4(t->step[45].time == ST_TIE && t->step[46].time == ST_TIE && ui.cursor == 44u);
    fm1_in.notes &= ~(1u << kC4); frame(); frame();
    R4(t->step[47].time != ST_TIE && ui.cursor == 47u);
    song.rec = 0;
    /* a GRAB scrolls with the note: K2 takes it up through the rows, the window follows */
    ui.cursor = 22; tq_bar = 1; frames(48);
    R4(t->step[22].n == 1u);
    press(B_OCTUP); frames(32);
    R4(tq_grab == 1u);
    for (i = 0; i < 12u; i++) {
        turn(EN_K2, 1); frames(32);
        R4(r4_in_view(t, 22));
    }
    press(B_OCTUP); frames(32);
    R4(!tq_grab);
    bad += check("round 4 item 4: NOTES v2 (strip, folded rows in the key, every step and note in view at every zoom, zoom keeps the cursor, K2 inserts, K1 sets the length while a key is held)", ok);
    return bad;
}
/* 5: SOUND faces v2: the live parts */
static void r4_face(uint32_t ins)
{
    ui_power_on();
    tu_off = 0;
    song.rec = 0; song.playing = 0;
    tfm_load(&trk[0], ins, 0);
    song.sel = 0;
    tu_goto(TU_SOUND);
    tu.page = 0;
    memset(fs, 0, sizeof fs);
    frames(100);
}
static uint32_t r4_hash(int32_t x0, int32_t y0, int32_t x1, int32_t y1)      /* the pixels of a box, hashed */
{
    int32_t x, y;
    uint32_t h = 2166136261u;
    for (y = y0; y <= y1; y++)
        for (x = x0; x <= x1; x++)
            h = (h ^ host_screen[(uint32_t)y * 240u + (uint32_t)x]) * 16777619u;
    return h;
}
static int test_r4_faces(void)
{
    int bad = 0, ok = 1;
    uint32_t a, b, i;
    char name[10], notes[24];
    /* THUMP LAB: a pad lights on its hit */
    r4_face(TI_THUMP);
    a = r4_hash(8 + 57 + 6, 28 + 6, 8 + 57 + 46, 28 + 28);
    fs[0].lane[1] = 6; frames(32);
    b = r4_hash(8 + 57 + 6, 28 + 6, 8 + 57 + 46, 28 + 28);
    R4(a != b);
    R4(r4_hash(8 + 6, 28 + 6, 8 + 46, 28 + 28) == r4_hash(8 + 6, 28 + 6, 8 + 46, 28 + 28));
    frames(300); fs[0].lane[1] = 0; frames(32);
    R4(r4_hash(8 + 57 + 6, 28 + 6, 8 + 57 + 46, 28 + 28) == a);                  /* and out again */
    /* SQUELCH: the ACCENT lamp lights on an accented note */
    r4_face(TI_SQUELCH);
    a = r4_hash(180, 30, 200, 50);
    song.playing = 1; trk[0].step[0].flags |= SF_ACCENT; trk[0].seq_idx = 0;       /* an accented step sounding */
    fs[0].flash = 6; frames(16);
    b = r4_hash(180, 30, 200, 50);
    song.playing = 0;
    R4(a != b);
    /* WOBBLER: the LED ring = WOB, the lit tile = RATE */
    r4_face(TI_WOBBLER);
    tfm_macro_put(&trk[0], 1, 10, 0); frames(64);
    a = r4_hash(60, 30, 180, 155);
    tfm_macro_put(&trk[0], 1, 120, 0); frames(64);
    b = r4_hash(60, 30, 180, 155);
    R4(a != b);
    tfm_macro_put(&trk[0], 2, 0, 0); frames(64);
    a = r4_hash(36, 166, 210, 183);
    tfm_macro_put(&trk[0], 2, 127, 0); frames(64);
    b = r4_hash(36, 166, 210, 183);
    R4(a != b);
    /* CHORDETTE: the LCD says the chord's real name and notes (D minor 7: Dm7, D F A C) */
    r4_face(TI_CHORDETTE);
    trk[0].p[P_ROOT] = 0;
    for (i = 0; i < 9u; i++) {                                                   /* the CHORD switch: find MIN7 */
        tfm_macro_put(&trk[0], 1, (int32_t)(i * 128u / 9u), 0);
        tf_chord_name(62, name, notes);
        if (str_eq(name, "Dm7"))
            break;
    }
    R4(str_eq(name, "Dm7") && str_eq(notes, "D F A C"));
    tfm_macro_put(&trk[0], 1, 0, 0);
    tf_chord_name(60, name, notes);
    R4(name[0] == 'C' && notes[0] == 'C');
    /* PIXEL: its screen draws the current WAVE (five different drawings) */
    r4_face(TI_PIXEL);
    {
        uint32_t h[5];
        for (i = 0; i < 5u; i++) {
            tfm_macro_put(&trk[0], 2, (int32_t)(i * 128u / 5u + 2u), 0);
            frames(64);
            h[i] = r4_hash(22, 40, 140, 95);
        }
        R4(h[0] != h[1] && h[1] != h[2] && h[2] != h[3] && h[3] != h[4] && h[0] != h[4]);
    }
    /* TINE: the lever follows TREM (OFF / MONO / STER) */
    r4_face(TI_TINE);
    tfm_macro_put(&trk[0], 3, 0, 0); frames(64); a = r4_hash(14, 90, 116, 104);
    tfm_macro_put(&trk[0], 3, 127, 0); frames(64); b = r4_hash(14, 90, 116, 104);
    R4(a != b);
    /* PLUCK: the string vibrates on a note and is still when it has died away */
    r4_face(TI_PLUCK);
    a = r4_hash(24, 62, 216, 92);
    fs[0].amp = 100; frames(48);
    b = r4_hash(24, 62, 216, 92);
    R4(a != b);
    /* VOX: the mouth opens with the sound and the eyes glow on a note; the vowel shapes it */
    r4_face(TI_VOX);
    a = r4_hash(30, 70, 80, 112);
    fs[0].env = 6000; fs[0].flash = 5; frames(48);
    b = r4_hash(30, 70, 80, 112);
    R4(a != b);
    R4(r4_hash(32, 47, 42, 57) != 0u);
    tfm_macro_put(&trk[0], 1, 100, 0); frames(64);
    R4(r4_hash(30, 70, 80, 112) != b);
    /* WHEELHOUSE: the drawbars show the STOPS registration, the tile = ROTOR */
    r4_face(TI_WHEEL);
    tfm_macro_put(&trk[0], 1, 0, 0); frames(64); a = r4_hash(10, 28, 150, 125);
    tfm_macro_put(&trk[0], 1, 40, 0); frames(64); b = r4_hash(10, 28, 150, 125);
    R4(a != b);
    tfm_macro_put(&trk[0], 3, 0, 0); frames(64); a = r4_hash(150, 93, 230, 107);
    tfm_macro_put(&trk[0], 3, 127, 0); frames(64); b = r4_hash(150, 93, 230, 107);
    R4(a != b);
    /* HAZE / GLINT / RAZOR: the controls move with the macros */
    r4_face(TI_HAZE);
    tfm_macro_put(&trk[0], 0, 10, 0); frames(64); a = r4_hash(30, 70, 55, 165);
    tfm_macro_put(&trk[0], 0, 120, 0); frames(64); b = r4_hash(30, 70, 55, 165);
    R4(a != b);
    r4_face(TI_GLINT);
    tfm_macro_put(&trk[0], 0, 10, 0); frames(64); a = r4_hash(26, 90, 58, 124);
    tfm_macro_put(&trk[0], 0, 120, 0); frames(64); b = r4_hash(26, 90, 58, 124);
    R4(a != b);
    /* MACROS 2: the other four macros in the same control style */
    r4_face(TI_RAZOR);
    a = r4_hash(20, 70, 215, 175);
    tu.page = 1; frames(80);
    b = r4_hash(20, 70, 215, 175);
    R4(a != b);
    tfm_macro_put(&trk[0], 4, 5, 0); frames(64); a = r4_hash(20, 70, 215, 175);
    tfm_macro_put(&trk[0], 4, 120, 0); frames(64); b = r4_hash(20, 70, 215, 175);
    R4(a != b);
    bad += check("round 4 item 5: SOUND faces v2 (pads, accent lamp, wobbler ring and tiles, chord name, wave screen, tremolo lever, string, mouth and eyes, drawbars and rotor tile, MACROS 2)", ok);
    return bad;
}
/* 6: BARS replaces STEP */
static int r4_same_bar(const track_t *t, uint32_t a, uint32_t b)       /* two bars hold the same steps */
{
    uint32_t i;
    for (i = 0; i < 16u; i++)
        if (memcmp(&t->step[a * 16u + i], &t->step[b * 16u + i], sizeof(step_t)))
            return 0;
    return 1;
}
static void r4_hold_oct(void)                            /* OCT+ held past 0.45 s (frames a few hundred ms apart) */
{
    int q;
    btn_down(B_OCTUP);
    for (q = 0; q < 4; q++) {
        fm1_ms += 200u;
        frame();
    }
}
static int test_r4_bars(void)
{
    int bad = 0, ok = 1;
    uint32_t i;
    track_t *t;
    step_t keep[NSTEP];
    ui_power_on();
    tu_off = 0;
    song.rec = 0; song.playing = 0;
    tfm_load(&trk[0], TI_SQUELCH, 0); tfm_load(&trk[1], TI_THUMP, 0);
    song.sel = 0;
    t = &trk[0];
    t->p[P_SLEN] = 16;
    for (i = 0; i < NSTEP; i++) step_clear(&t->step[i]);
    for (i = 0; i < 16u; i++) t->step[i] = (step_t){{(uint8_t)(60 + i)}, 1, ST_NOTE, 0, 90, 0, 0};
    tu_goto(TU_JAM);
    stap(B_SEQ);                                                              /* NOTES, then BARS */
    R4(tu.scr == TU_NOTES);
    stap(B_SEQ);
    R4(tu.scr == TU_BARS && !song.seq_mode);
    frames(64);
    /* LENGTH: growing repeats the bars there are, shrinking keeps the first n; the longest is what 64 steps hold */
    turn(EN_K2, 1);
    R4(t->p[P_SLEN] == 32 && r4_same_bar(t, 0, 1));
    turn(EN_K2, 1);
    R4(t->p[P_SLEN] == 64 && r4_same_bar(t, 0, 3) && r4_same_bar(t, 0, 2));
    turn(EN_K2, 1);
    R4(t->p[P_SLEN] == 64);
    turn(EN_K2, -1);
    R4(t->p[P_SLEN] == 32);
    turn(EN_K2, -1);
    R4(t->p[P_SLEN] == 16 && t->step[3].note[0] == 63 && t->step[20].note[0] == 64);          /* (the first bar kept) */
    turn(EN_K2, -1);
    R4(t->p[P_SLEN] == 16);
    /* DOUBLE: OCT+ tapped copies every bar after itself */
    press(B_OCTUP); frames(32);
    R4(t->p[P_SLEN] == 32 && r4_same_bar(t, 0, 1));
    turn(EN_K3, 1); R4(tb_bar == 1u);
    turn(EN_K3, 1); R4(tb_bar == 1u);                                         /* (no further than the last bar) */
    /* CLEAR BAR: EDIT clears the selected bar only */
    for (i = 0; i < NSTEP; i++) keep[i] = t->step[i];
    stap(B_EDIT);
    R4(!t->step[16].n && !t->step[31].n && t->step[0].n == 1u && t->step[15].n == 1u && t->p[P_SLEN] == 32);
    undo_swap();                                                              /* all undoable */
    R4(!memcmp(t->step, keep, sizeof keep) && t->p[P_SLEN] == 32);
    /* SHIFT: the whole track a step, wrapping */
    for (i = 0; i < NSTEP; i++) keep[i] = t->step[i];
    turn(EN_K4, 1);
    R4(t->step[1].note[0] == keep[0].note[0] && t->step[0].note[0] == keep[31].note[0] && t->step[31].note[0] == keep[30].note[0]);
    R4(tb_shift == 1);
    undo_swap();
    R4(!memcmp(t->step, keep, sizeof keep) && t->p[P_SLEN] == 32);
    turn(EN_K4, -1);
    R4(t->step[0].note[0] == keep[1].note[0] && t->step[31].note[0] == keep[0].note[0]);
    undo_swap();
    /* COPY: hold OCT+, K3 the target, OCT+ pastes; OCT- cancels */
    for (i = 0; i < 16u; i++) t->step[16u + i].note[0] = (uint8_t)(80 + i);    /* bar 2 is its own */
    tb_bar = 0; frames(32);
    r4_hold_oct();
    R4(tb_copy == 1u && tb_tgt == 1u);
    btn_up(B_OCTUP); frame(); frame(); frame();
    R4(tb_copy == 1u && t->p[P_SLEN] == 32);                                  /* (the hold's release is no DOUBLE) */
    stap(B_OCTDN);
    R4(tb_copy == 0u && t->step[16].note[0] == 80);                           /* cancelled: nothing pasted */
    r4_hold_oct();
    btn_up(B_OCTUP); frame(); frame(); frame();
    R4(tb_copy == 1u);
    press(B_OCTUP); frames(32);                                               /* paste: bar 1 onto bar 2 */
    R4(tb_copy == 0u && r4_same_bar(t, 0, 1) && t->p[P_SLEN] == 32);
    undo_swap();
    R4(t->step[16].note[0] == 80);
    /* the track: K1; a bar of the other track (a drum track draws by lanes) */
    turn(EN_K1, 1);
    R4(song.sel == 1u && tb_bar < tb_nbars(&trk[1]));
    turn(EN_K1, -1);
    /* the scene's length: the longest track; shorter tracks repeat (dashed) up to it */
    R4(tb_scene_bars() == 2u && tb_nbars(&trk[1]) == 1u);
    stap(B_SEQ);
    R4(tu.scr == TU_NOTES);
    bad += check("round 4 item 6: BARS replaces STEP (SEQ NOTES then BARS, K2 length 1/2/4, DOUBLE, K3 bar, EDIT clears a bar, K4 shift wraps, hold OCT+ COPY and paste, OCT- cancels, every change one undo)", ok);
    return bad;
}
