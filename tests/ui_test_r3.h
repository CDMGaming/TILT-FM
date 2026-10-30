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
    stap(B_SEQ);
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
    turn(EN_K3, 1); turn(EN_K4, 1);                                          /* (round 4: K2 on an empty step inserts the last note) */
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
/* 2: REC decides what the keys do on STEP, NOTES, DRUMS: off = play only; on + stopped = write at the cursor; on + playing = live */
static void r3_key(uint32_t k) { fm1_in.notes |= 1u << k; host_notes |= 1u << k; frame(); fm1_in.notes &= ~(1u << k); frame(); }
static int test_r3_rec(void)
{
    int bad = 0, ok = 1, scr;
    uint32_t i, kC4, n0, t0, v0, n1, t1, v1;
    ui_power_on();
    tu_off = 0;
    tfm_load(&trk[0], TI_SQUELCH, 0); tfm_load(&trk[1], TI_THUMP, 0);
    for (scr = 1; scr < 3; scr++) {                     /* NOTES, DRUMS (round 4: STEP is BARS now, no keys there) */
        track_t *t;
        song.sel = scr == 2 ? 1 : 0;
        t = &trk[song.sel];
        tu_goto(TU_JAM);
        for (i = 0; i < NSTEP; i++) step_clear(&t->step[i]);
        t->p[P_SLEN] = 16;
        stap(B_SEQ);
        R3(tu.scr == (scr == 1 ? TU_NOTES : TU_DRUMS));
        if (scr == 2) tu_goto(TU_DRUMS);
        kC4 = key_at(0, 4);
        ui.cursor = 3;
        song.rec = 0;
        n0 = r3_notes(t, &t0, &v0);
        for (i = 0; i < NSTEP; i++) n0 += t->step[i].hit;
        r3_key(kC4); r3_key(kC4);                       /* PLAY ONLY, stopped: nothing written, the cursor stays */
        n1 = r3_notes(t, &t1, &v1);
        for (i = 0; i < NSTEP; i++) n1 += t->step[i].hit;
        R3(n1 == n0 && ui.cursor == 3u);
        transport_req = 1; events_block(32);
        for (i = 0; i < 40u; i++) events_block(32);
        r3_key(kC4);                                    /* PLAY ONLY, playing: nothing recorded */
        for (i = 0; i < 400u; i++) events_block(32);
        n1 = r3_notes(t, &t1, &v1);
        for (i = 0; i < NSTEP; i++) n1 += t->step[i].hit;
        R3(n1 == n0);
        seq_stop(); transport_req = 0; events_block(32);
        song.rec = 1u << song.sel;                      /* WRITE STEPS: REC on, stopped */
        ui.cursor = 3;
        r3_key(kC4);
        n1 = r3_notes(t, &t1, &v1);
        for (i = 0; i < NSTEP; i++) n1 += t->step[i].hit;
        R3(n1 == n0 + 1u && (scr == 2 || ui.cursor == 4u));
        for (i = 0; i < NSTEP; i++) step_clear(&t->step[i]);
        n0 = 0;
        transport_req = 1; events_block(32);            /* REC LIVE: REC on, playing */
        for (i = 0; i < 40u; i++) events_block(32);
        fm1_in.notes |= 1u << kC4; host_notes |= 1u << kC4; frame();
        for (i = 0; i < 60u; i++) events_block(32);
        fm1_in.notes &= ~(1u << kC4); frame();
        for (i = 0; i < 3000u; i++) events_block(32);
        n1 = r3_notes(t, &t1, &v1);
        for (i = 0; i < NSTEP; i++) n1 += t->step[i].hit;
        R3(n1 >= 1u);
        seq_stop(); transport_req = 0; events_block(32);
        song.rec = 0;
    }
    bad += check("TFM-UI round 3: REC off the keys only play (stopped or playing, STEP / NOTES / DRUMS); REC on stopped they write; REC on playing they record live", ok);
    return bad;
}
/* 3 + 4: PLAY on DICE makes a NEW SONG (fresh, or KEEP SCENES); the TIMER is 10 minutes by default and K4 on DICE sets this song's */
static int r3_empty_song(void)                          /* every pattern empty, no automation */
{
    uint32_t k;
    for (k = 0; k < NTRK; k++)
        if (!seq_is_empty(&trk[k]) || motion_count(&trk[k]))
            return 0;
    return 1;
}
static void r3_two_scenes(void)                         /* the music is song 1 with scenes A and B (a note each), a chain, a name, stored */
{
    sng_cur = 0;
    memset(proj_slot, 0, sizeof proj_slot);
    memset(song_rec, 0, sizeof song_rec);
    chain_defaults(&chain_config);
    scene_claim();
    str_cpy(proj_name, "OLD", sizeof proj_name);
    sc_note(0, 0, 55);
    tilt_scene_go(1, 0);
    sc_note(0, 0, 66);
    tilt_scene_go(0, 0);
    chain_config.count = 2;
    chain_config.row[0] = chain_row_of(0, 2);
    chain_config.row[1] = chain_row_of(1, 1);
}
static int test_r3_newsong(void)
{
    int bad = 0, ok = 1, n;
    project_t p;
    uint32_t cfg_t;
    ui_power_on();
    tu_off = 0;
    memset(&favorites.factory[13][16], 0, 16);          /* SETUP as it comes: FRESH, timer default */
    r3_two_scenes();
    tilt_ns_open = 0; tm.on = 0; tilt_timer_min = 0;
    tilt_ns_show();
    R3(tilt_timer_min == 10u);                          /* the TIMER is on by default: 10:00 */
    cfg_t = CFG(CF_TIMER);
    turn(EN_K4, 1);                                     /* K4 on DICE: this song's timer, not the default */
    R3(tilt_timer_min == 15u && CFG(CF_TIMER) == cfg_t);
    turn(EN_K4, -3);
    R3(tilt_timer_min == 0u && CFG(CF_TIMER) == cfg_t);
    turn(EN_K4, 2);
    R3(tilt_timer_min == 10u);
    press(B_PLAY); frames(32);                          /* PLAY: a NEW SONG, fresh, in the first empty slot */
    R3(sng_cur == 1u && tilt_ns_open == 0 && r3_empty_song() && !proj_name[0] && chain_config.count == 0);
    R3(scene_cur() == 0u && project_used(0) && !project_used(1) && !project_used(2) && !project_used(7));
    R3(song_stored(0) && song_stored(1) && str_eq(song_name_of(0), "OLD") && tm.on);
    R3(scn_get(0 * NSCN + 0, &p) && p.t[0].step[0].note[0] == 55u && scn_get(0 * NSCN + 1, &p) && p.t[0].step[0].note[0] == 66u);
    seq_stop(); transport_req = 0; tm.on = 0; events_block(32);
    /* all three slots used: SONGS asks which to replace; Cancel (OCT-) is DICE again with nothing changed */
    song_save_as(2, "THREE");
    R3(song_stored(0) && song_stored(1) && song_stored(2) && sng_cur == 2u);
    tilt_timer_min = 0;
    tilt_ns_show();
    press(B_PLAY); frames(32);
    R3(tu.scr == TU_SONGS && tsg_new && tilt_ns_open == 0 && sng_cur == 2u);
    press(B_OCTDN); frames(32);
    R3(!tsg_new && tilt_ns_open == TILT_START && sv.mode == SV_ROLLED && sng_cur == 2u && song_stored(0) && song_stored(1));
    press(B_PLAY); frames(32);
    R3(tsg_new);
    stap(B_HOME);                                       /* HOME cancels too */
    R3(!tsg_new && tilt_ns_open == TILT_START);
    press(B_PLAY); frames(32);
    turn(EN_SELECT, 1);
    n = tsg_sel;
    press(B_OCTUP); frames(32);                         /* OCT+: replace the song picked */
    R3(!tsg_new && sng_cur == (uint32_t)n && r3_empty_song() && project_used(0) && !project_used(1) && chain_config.count == 0);
    R3(song_stored(0) + song_stored(1) + song_stored(2) == 3);
    seq_stop(); transport_req = 0; tm.on = 0; events_block(32);
    /* KEEP SCENES: the sounds re-roll, the patterns, the scenes and the chain stay; the song is the same */
    r3_two_scenes();
    CFG(CF_NEW) = 1;
    tilt_ns_open = 0;
    tilt_ns_show();
    press(B_PLAY); frames(32);
    R3(sng_cur == 0u && tilt_ns_open == 0 && trk[0].step[0].note[0] == 55u && project_used(1) && chain_config.count == 2 &&
       str_eq(proj_name, "OLD") && !r3_empty_song());
    seq_stop(); transport_req = 0; tm.on = 0; events_block(32);
    CFG(CF_NEW) = 0;
    /* SETUP: TIMER default 10, OFF selectable; the stored byte 0 (older settings) is 10 */
    CFG(CF_TIMER) = 0;
    R3(cfg_timer_idx() == 2u && TM_LIST[cfg_timer_idx()] == 10u);
    CFG(CF_TIMER) = cfg_timer_val(0);
    R3(TM_LIST[cfg_timer_idx()] == 0u);
    CFG(CF_TIMER) = 0;
    bad += check("TFM-UI round 3: PLAY on DICE makes a fresh NEW SONG in an empty slot (the old one stays saved, A the rolled sounds, B-H and the chain empty); "
                 "all three used: SONGS asks, OCT-/HOME cancel; KEEP SCENES keeps them; the TIMER is 10 min by default, K4 on DICE is this song's", ok);
    return bad;
}
/* 5: every melodic instrument stays in key: the root and the fifth of the key for one beat at 120 BPM; the fundamental after the
 * first 1/16 note within +-35 cents of the written note or an octave of it (docs/TFM-INSTRUMENTS.md "In key"). Drums are not
 * measured; a sound with no clear fundamental (bells, noise) is listed as unmeasurable, not passed */
/* YIN (de Cheveigne and Kawahara 2002): the cumulative mean normalised difference d'(tau) of a 2048-sample window, the first
 * dip under 0.15 (else the deepest one) between half an octave under and two over the note, its minimum refined by a parabola.
 * conf = 1 - d'(tau): a sound with no single period (noise, inharmonic bells) has a low one */
static double r3_f0(const int16_t *x, uint32_t n, double fs, double fexp, double *conf)
{
    enum { W = 2048 };
    static double d[W + 8];
    uint32_t tmin = (uint32_t)(fs / (fexp * 2.6)), tmax = (uint32_t)(fs / (fexp * 0.45)), tau, i, best = 0;
    double run = 0, bv = 1e30, a, b, c, off;
    if (tmax > W - 4u) tmax = W - 4u;
    if (tmin < 2u) tmin = 2u;
    (void)n;
    d[0] = 1.0;
    for (tau = 1; tau <= tmax + 1u; tau++) {
        double s = 0;
        for (i = 0; i < W; i++) {
            double df = (double)x[i] - (double)x[i + tau];
            s += df * df;
        }
        run += s;
        d[tau] = run > 0 ? s * tau / run : 1.0;
    }
    for (tau = tmin; tau <= tmax; tau++)
        if (d[tau] < 0.15) {                            /* the first dip: walk down to its bottom */
            while (tau + 1u <= tmax && d[tau + 1u] < d[tau]) tau++;
            best = tau;
            break;
        }
    if (!best)
        for (tau = tmin; tau <= tmax; tau++)
            if (d[tau] < bv) { bv = d[tau]; best = tau; }
    *conf = 1.0 - d[best];
    if (best <= tmin || best >= tmax) return fs / best;
    a = d[best - 1]; b = d[best]; c = d[best + 1];
    off = (a - c) / (2.0 * (a - 2.0 * b + c) + 1e-30);
    return fs / ((double)best + off);
}
/* the worst deviation (cents, octave-folded) over the key's root and fifth, or 999 when it cannot be measured; the sound is
 * loaded afresh for each note (nothing of the one before rings on), macro < 0: none, else that macro at v */
static int32_t r3_m2 = -1, r3_v2;                                   /* (a diagnostic: a second macro set too) */
static double r3_pitch(uint32_t ins, uint32_t pre, int32_t macro, int32_t v)
{
    static int16_t buf[22050];
    double worst = 0;
    uint32_t j, f;
    song.master_q12 = 4096;
    for (j = 0; j < 2u; j++) {
        track_t *t = &trk[0];
        uint32_t note = j ? 67u : 60u, n = 0;
        double fexp = 440.0 * pow(2.0, ((double)note - 69.0) / 12.0), fr, cents, conf;
        int32_t o[2 * CTL];
        ui_power_on();                                              /* (a clean start per note: no reverb or echo tail of the one before) */
        memset(dly_buf, 0, sizeof dly_buf);
        memset(cho_buf, 0, sizeof cho_buf);
        rev_clear();
        tu_off = 0;
        song.master_q12 = 4096;
        song.sel = 0;
        tfm_load(t, ins, pre);
        t->p[P_ROOT] = 0; t->p[P_SCALE] = 1; t->p[P_QUANT] = QN_WHITE;
        if (ins == TI_CHORDETTE) t->p[P_CHRD] = 0;                  /* (one note: the chord's own pitches are the scale's) */
        if (r3_m2 >= 0) tfm_macro_set(t, (uint32_t)r3_m2, r3_v2);
        if (macro >= 0) tfm_macro_set(t, (uint32_t)macro, v);
        if (ins == TI_WOBBLER && macro != 0) t->p[P_LD_FLT] = 0;      /* (the wob is an amplitude wobble the estimator reads as a bias; the pitch is the note's) */
        panic_req = (1u << NTRK) - 1u; mix_block(o, CTL);
        for (f = 0; f < 256u && (t->xf_on || t->eng_req != t->engine); f++) mix_block(o, CTL);   /* (the engine switch done) */
        trk_note_on(t, note, 100);
        for (f = 0; f < 22050u; f += CTL) {
            uint32_t i;
            mix_block(o, CTL);
            for (i = 0; i < CTL && n < 22050u; i++) buf[n++] = (int16_t)(o[2 * i] > 32767 ? 32767 : o[2 * i] < -32768 ? -32768 : o[2 * i]);
        }
        trk_note_off(t, note);
        fr = r3_f0(buf + 5512, 4096, 44100.0, fexp, &conf);
        cents = 1200.0 * log2(fr / fexp);
        cents -= 1200.0 * floor(cents / 1200.0 + 0.5);
        if (getenv("R3DIAG"))
            fprintf(stderr, "DIAG ins %u pre %u macro %d v %d note %u: f %.1f exp %.1f cents %.0f conf %.2f%c", ins, pre, (int)macro, (int)v, note, fr, fexp, cents, conf, 10);
        if (conf < 0.55) return 999.0;
        if (fabs(cents) > worst) worst = fabs(cents);
    }
    return worst;
}
static int test_r3_pitch(void)
{
    int bad = 0, ok = 1;
    static const uint8_t MEL[] = {TI_SQUELCH, TI_WOBBLER, TI_CHORDETTE, TI_RAZOR, TI_HAZE, TI_PIXEL, TI_TINE, TI_GLINT, TI_PLUCK, TI_VOX, TI_WHEEL};
    uint32_t a, p, m;
    ui_power_on();
    tu_off = 0;
    if (getenv("R3GRID")) {                                          /* (a diagnostic: R3GRID=<ins>,<macro a>,<macro b>: a 5 x 5 grid on each preset, the other macros as the preset has them) */
        uint32_t ins, ma, mb, pp, ia, ib;
        sscanf(getenv("R3GRID"), "%u,%u,%u", &ins, &ma, &mb);
        for (pp = 0; pp < 5u; pp++) {
            fprintf(stderr, "GRID %s preset %u: %s (rows) x %s (cols) at 0 25 50 75 100%c", TFM_INS[ins].name, pp, TFM_INS[ins].mac[ma].name, TFM_INS[ins].mac[mb].name, 10);
            for (ia = 0; ia < 5u; ia++) {
                fprintf(stderr, "  ");
                for (ib = 0; ib < 5u; ib++) {
                    r3_m2 = (int32_t)mb; r3_v2 = (int32_t)(ib * 127u / 4u);
                    fprintf(stderr, " %4.0f", r3_pitch(ins, pp, (int32_t)ma, (int32_t)(ia * 127u / 4u)));
                }
                fprintf(stderr, "%c", 10);
            }
            r3_m2 = -1;
        }
    }
    if (getenv("R3SWEEP")) {                                         /* (a diagnostic: R3SWEEP=<instrument>: every macro at 0 / 50 / 100 on each preset) */
        uint32_t ins = (uint32_t)atoi(getenv("R3SWEEP")), pp, mm;
        for (pp = 0; pp < 5u; pp++) {
            fprintf(stderr, "SWEEP %s preset %u:%c", TFM_INS[ins].name, pp, 10);
            for (mm = 0; mm < 8u; mm++) {
                int32_t v;
                fprintf(stderr, "  %s:", TFM_INS[ins].mac[mm].name);
                for (v = 0; v <= 100; v += 50) fprintf(stderr, " %.0f", r3_pitch(ins, pp, (int32_t)mm, v));
                fprintf(stderr, "%c", 10);
            }
        }
    }
    for (a = 0; a < sizeof MEL; a++) {
        uint32_t good = 0, un = 0;
        for (p = 0; p < 5u; p++) {
            double w = r3_pitch(MEL[a], p, -1, 0);
            if (w == 999.0) un++;
            else if (w <= 35.0) good++;
            else fprintf(stderr, "PITCH %s preset %u: %.0f cents%c", TFM_INS[MEL[a]].name, p, w, 10);
        }
        for (m = 0; m < 8u; m++) {                                  /* every continuous macro at 0 and 50, on the first preset */
            int32_t v;
            if (TFM_INS[MEL[a]].mac[m].nsw) continue;
            for (v = 0; v <= 50; v += 50) {
                double w = r3_pitch(MEL[a], 0, (int32_t)m, v);
                if (w != 999.0 && w > 35.0)
                    fprintf(stderr, "PITCH %s macro %s at %d: %.0f cents%c", TFM_INS[MEL[a]].name, TFM_INS[MEL[a]].mac[m].name, v, w, 10);
            }
        }
        fprintf(stderr, "PITCH %s: %u of 5 presets in key, %u unmeasurable (no single pitch)%c", TFM_INS[MEL[a]].name, good, un, 10);
        R3(good + un >= 3u);                                         /* (all eleven melodic instruments: THUMP LAB is a kit, no pitch) */
    }
    bad += check("TFM-UI round 4: every melodic instrument (SQUELCH WOBBLER CHORDETTE RAZOR HAZE PIXEL TINE GLINT PLUCK VOX WHEELHOUSE) has at least 3 presets that play the key's root and fifth in tune by YIN (+-35 cents, octaves allowed; a sound with no single period counts as measured, not wrong)", ok);
    return bad;
}
