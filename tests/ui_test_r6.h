/* TFM-UI round 6: the presets audit (item 4) and the loudness (item 10): a host render of each preset playing a phrase from a
 * clean start, simple features of the sound, the distances between the presets of one instrument. Included after round 5 */
#define R6(c) do { if (!(c)) { ok = 0; fprintf(stderr, "R6 line %d ", __LINE__); } } while (0)
#define R6_FS 44100u
#define R6_N (R6_FS * 2u)
static int16_t r6_buf[R6_N];
/* a preset rendered: C4 E4 G4 eighths then C5 held (a kit: kick, snare, hat, kick held), 2 s from a clean start */
/* round 8: every render from the same start whatever ran before (the voices' noise and LFOs, the matrix's random) */
static void r6_fresh(void)
{
    uint32_t q;
    seq_stop();                                         /* (no song playing under it: earlier tests may leave one) */
    song.playing = 0;
    transport_req = 0;
    chain.running = chain.armed = 0;
    for (q = 0; q < NTRK; q++) {
        memset(trk[q].step, 0, sizeof trk[q].step);     /* (no steps, the other three silent: track 0 alone is heard) */
        if (q)
            trk[q].p[P_MUTE] = 1;
        memset(trk[q].v, 0, sizeof trk[q].v);
        trk[q].lfo_ph = 0; trk[q].lfo_val = 0; trk[q].lfo_fade = 0; trk[q].lfo_rnd = 0; trk[q].lfo_prev = 0;
    }
    mod_seed = 0x2545F491u;
    memset(eng_mem, 0, sizeof eng_mem);                 /* (the parts' engine state: the drum lanes, FM6 notes, strings) */
    memset(eng_mem_owner, 0, sizeof eng_mem_owner);
    memset(&fx, 0, sizeof fx); memset(&hl, 0, sizeof hl);
    vage = 0; uni_seed = 0x1234567u; memset(mod_w, 0, sizeof mod_w);
    memset(fm6_lfo, 0, sizeof fm6_lfo); memset(drw_t, 0, sizeof drw_t); memset(drw_v, 0, sizeof drw_v);   /* (FM6's LFOs, */
    { uint32_t q; for (q = 0; q < NTRK; q++) { formant_nz[q] = 0x2545F491; phys_nz[q] = 0x6A09E667; } }   /* WHEEL's rotor, the noises) */ memset(drum_mute, 0, sizeof drum_mute);   /* (the buses: the delay and chorus write heads, the chorus phase, HALL) */
    lim_env = 0; dc_l = dc_r = dce_l = dce_r = 0;     /* (the master limiter and its DC blockers: a loud render before held it down) */
    memset(dj_s1, 0, sizeof dj_s1); memset(dj_s2, 0, sizeof dj_s2); memset((void *)dj_f, 0, sizeof dj_f);
}
static int32_t r6_mac = -1, r6_macv;                   /* (round 8 item 3: the macro a render moves, -1 none; its place) */
static void r6_render(uint32_t ins, uint32_t pre)     /* (round 6 item 10: without the mix trims: item 4 audits the sound itself) */
{
    static const uint8_t MEL[4] = {60, 64, 67, 72}, KIT[4] = {36, 38, 42, 36};
    static const uint32_t ON[4] = {0, 11025, 22050, 33075}, OFF[4] = {8820, 19845, 30870, 66150};
    track_t *t = &trk[0];
    int32_t o[2 * CTL];
    uint32_t f, n = 0, k;
    ui_power_on();
    memset(dly_buf, 0, sizeof dly_buf);
    memset(cho_buf, 0, sizeof cho_buf);
    rev_clear();
    tu_off = 0;
    song.master_q12 = 4096;
    song.g[G_BPM] = 120;
    rng_state = 0x12345678u;
    r6_fresh();
    song.sel = 0;
    tfm_load(t, ins, pre);
    if (r6_mac >= 0)                                    /* (round 8: one macro moved, the rest as the preset sets them) */
        tfm_macro_put(t, (uint32_t)r6_mac, r6_macv, 0);
    t->p[P_ROOT] = 0; t->p[P_SCALE] = 1; t->p[P_QUANT] = QN_WHITE;
    lvl_trim_fn = 0;
    panic_req = (1u << NTRK) - 1u; mix_block(o, CTL);
    for (f = 0; f < 256u && (t->xf_on || t->eng_req != t->engine); f++) mix_block(o, CTL);
    for (n = 0; n < R6_N; n += CTL) {
        uint32_t i;
        for (k = 0; k < 4u; k++) {
            uint32_t nt = TFM_INS[ins % TI_N].group == IG_DRUMS ? KIT[k] : MEL[k];   /* (round 8: every kit plays the beat) */
            if (ON[k] >= n && ON[k] < n + CTL) trk_note_on(t, nt, 100);
            if (OFF[k] >= n && OFF[k] < n + CTL) trk_note_off(t, nt);
        }
        mix_block(o, CTL);
        for (i = 0; i < CTL && n + i < R6_N; i++)
            r6_buf[n + i] = (int16_t)clamp((o[2 * i] + o[2 * i + 1]) / 2, -32768, 32767);
    }
    lvl_trim_fn = tfm_lvl_trim;
}
/* the features: 0 brightness (log2 of the RMS frequency, early), 1 its change late vs early (octaves), 2 attack (log2 ms to
 * 90 % of the first note's top), 3 sustain (the held note at 0.6 s over its top), 4 motion (the held note's level wobble), 5 level (dB / 6) */
#define R6_NF 6u
static double r6_rmsf(uint32_t a, uint32_t b)            /* sqrt(sum d^2 / sum x^2) * fs / 2pi: the spectrum's RMS frequency */
{
    double sx = 0, sd = 0;
    uint32_t i;
    for (i = a + 1u; i < b && i < R6_N; i++) {
        double x = r6_buf[i], d = (double)r6_buf[i] - r6_buf[i - 1u];
        sx += x * x;
        sd += d * d;
    }
    return sx > 1e3 ? sqrt(sd / sx) * R6_FS / 6.2831853 : 20.0;
}
static double r6_rms(uint32_t a, uint32_t b)
{
    double s = 0;
    uint32_t i;
    for (i = a; i < b && i < R6_N; i++)
        s += (double)r6_buf[i] * r6_buf[i];
    return sqrt(s / (double)(b - a)) + 1e-3;
}
static void r6_features(double *v)
{
    uint32_t i, fr = 441u, at = 0;                      /* 10 ms frames */
    double top = 0, hold_top = 0, w = 0, m = 0, mm = 0, n = 0;
    for (i = 0; i < 25u; i++) {                         /* the first note: its top and when it gets to 90 % of it */
        double r = r6_rms(i * fr, (i + 1u) * fr);
        if (r > top) top = r;
    }
    for (i = 0; i < 25u; i++)
        if (r6_rms(i * fr, (i + 1u) * fr) >= 0.9 * top) { at = i; break; }
    for (i = 75; i < 85u; i++) {                        /* the held note's top (0.75 .. 0.85 s) */
        double r = r6_rms(i * fr, (i + 1u) * fr);
        if (r > hold_top) hold_top = r;
    }
    for (i = 100; i < 140u; i++) {                      /* its wobble (1.0 .. 1.4 s): the frames' spread over their mean */
        double r = r6_rms(i * fr, (i + 1u) * fr);
        m += r; mm += r * r; n++;
    }
    m /= n;
    w = sqrt(mm / n - m * m > 0 ? mm / n - m * m : 0) / (m + 1.0);
    v[0] = log2(r6_rmsf(0, 33075));
    v[1] = log2(r6_rmsf(44100, 66150) / r6_rmsf(0, 33075));
    v[2] = log2(10.0 * at + 5.0);
    v[3] = r6_rms(60 * fr, 61 * fr) / (hold_top + 1.0) * 2.0;   /* (x2: a sustain counts as much as an octave) */
    v[4] = w * 4.0;
    v[5] = 20.0 * log10(r6_rms(0, 66150) / 32768.0) / 6.0;
}
static double r6_dist(const double *a, const double *b)
{
    double s = 0;
    uint32_t i;
    for (i = 0; i < R6_NF; i++)
        s += (a[i] - b[i]) * (a[i] - b[i]);
    return sqrt(s);
}
#define R6_CLOSE 0.60                                   /* two presets nearer than this sound like one (from the table: see the report) */
static double r6_feat[TI_N][5][R6_NF];
static int test_r6_presets(void)
{
    int bad = 0, ok = 1, verbose = getenv("R6AUDIT") != 0;
    uint32_t i, a, b, f;
    double dmin_all = 1e9;
    for (i = 0; i < TI_N; i++)
        for (a = 0; a < 5u; a++) {
            r6_render(i, a);
            r6_features(r6_feat[i][a]);
            if (verbose) {
                fprintf(stderr, "AUDIT %-10s %u %-14s", TFM_INS[i].name, a, TFM_INS[i].snd[a].name);
                for (f = 0; f < R6_NF; f++) fprintf(stderr, " %6.2f", r6_feat[i][a][f]);
                fprintf(stderr, "%c", 10);
            }
        }
    for (i = 0; i < TI_N; i++) {
        double dmin = 1e9;
        uint32_t pa = 0, pb = 0;
        for (a = 0; a < 5u; a++)
            for (b = a + 1u; b < 5u; b++) {
                double d = r6_dist(r6_feat[i][a], r6_feat[i][b]);
                if (verbose) fprintf(stderr, "DIST %-10s %u-%u %.2f%c", TFM_INS[i].name, a, b, d, 10);
                if (d < dmin) { dmin = d; pa = a; pb = b; }
            }
        fprintf(stderr, "PRESETS %-10s nearest %s / %s %.2f%c", TFM_INS[i].name, TFM_INS[i].snd[pa].name, TFM_INS[i].snd[pb].name, dmin, 10);
        if (dmin < dmin_all) dmin_all = dmin;
        R6(dmin >= R6_CLOSE);
    }
    bad += check("round 6 item 4: the five named presets of every instrument sound apart (brightness, its change, attack, sustain, motion, level: no two nearer than R6_CLOSE)", ok);
    return bad;
}
/* item 5: START v2: the reels stop one by one left to right (a held one never runs), ENV from anywhere opens START rolled,
 * SAVE on WAIT is SONGS, PLAY on WAIT continues */
static int test_r6_start(void)
{
    int bad = 0, ok = 1;
    uint32_t t0;
    ui_power_on();
    tu_off = 0;
    memset(&ns, 0, sizeof ns);
    st.has = 0; st.crash = 0; tilt_ns_open = TILT_START; sv_boot();
    R6(sv.mode == SV_WAIT && !sv.last_ok);
    ns.rolled = 1; ns.hold = 1u << (3u + 2u);           /* T3 held (a roll before: the first one clears the holds) */
    sv_roll();
    t0 = sv.t0;
    R6(sv.mode == SV_SPIN && sv_stopped(2) && !sv_stopped(0) && !sv_stopped(1) && !sv_stopped(3));
    fm1_ms = t0 + 700u;
    R6(sv_stopped(0) && !sv_stopped(1) && !sv_stopped(3));
    fm1_ms = t0 + 1000u;
    R6(sv_stopped(1) && !sv_stopped(3));
    fm1_ms = t0 + 1600u; frame();
    R6(sv.mode == SV_ROLLED && sv_stopped(3));
    press(B_OCTDN);                                     /* OCT-: back to WAIT */
    R6(sv.mode == SV_WAIT && tilt_ns_open == TILT_START);
    press(B_SAVE);                                      /* SAVE: SONGS */
    R6(tilt_ns_open == 0 && tu.scr == TU_SONGS);
    tu_goto(TU_MIX); frames(16);
    press(B_ENV);                                       /* ENV from anywhere: START, rolled */
    R6(tilt_ns_open == TILT_START && sv.mode == SV_ROLLED);
    press(B_HOME);
    bad += check("round 6 item 5: START v2 (the reels stop left to right, a held one stays; OCT- back; SAVE SONGS; ENV from anywhere: START rolled)", ok);
    return bad;
}
/* item 8: SONGS: SEQ is SAVE JAM HERE: the jam log as the chain, then saved into the slot picked (the NAME screen; a song
 * there: asked first) */
static int test_r6_jam_here(void)
{
    int bad = 0, ok = 1;
    ui_power_on();
    tu_off = 0;
    r5_scenes();
    jl_n = 0;
    tu_goto(TU_SONGS);
    tsg_sel = 1;
    frames(48);
    stap(B_SEQ);                                        /* no jam yet: says so */
    R6(str_eq(ui.msg, "NO JAM PLAYED YET") && !nm.kind && !ui.confirm);
    jl_push(0); jl_push(1); jl_push(0);
    stap(B_SEQ);
    R6(nm.kind == NK_SONG_SAVE && nm.slot == 1u && chain_config.count == 3u && chain_config.row[0].slot[0] == 0u &&
       chain_config.row[1].slot[0] == 1u && chain_config.row[2].slot[0] == 0u);
    str_cpy(nm.s, "JAMMED", sizeof nm.s); nm.len = nm.cur = 6;
    name_ok();
    R6(!nm.kind && sng_cur == 1u && song_stored(1) && str_eq(song_name_of(1), "JAMMED"));
    tsg_sel = 0;                                        /* over a stored song: asked first (Cancel the default) */
    jl_push(2);
    stap(B_SEQ);
    R6(ui.confirm == CF_SONG_OVR && !nm.kind);
    ui.confirm = 0;
    tu_goto(TU_JAM);
    bad += check("round 6 item 8: SONGS: SEQ saves the jam here (the jam log as the chain, the NAME screen; over a song: asks; no jam: says so)", ok);
    return bad;
}
/* item 9: CHORDETTE names the chord notes up from the root (C E G), whatever the voicing, an inversion keeps the root name */
static int test_r6_chord_order(void)
{
    int bad = 0, ok = 1;
    char name[10], notes[24];
    r4_face(TI_CHORDETTE);
    trk[0].p[P_ROOT] = 0;
    trk[0].p[P_CHRD] = 0;
    {
        uint32_t i;
        for (i = 0; i < 9u; i++) {                      /* the CHORD switch: find the major triad */
            tfm_macro_put(&trk[0], 1, (int32_t)(i * 128u / 9u), 0);   /* (CHORD: macro 2 since round 8) */
            trk[0].p[P_VOIC] = VC_CLOSE;
            tf_chord_name(60, name, notes);
            if (str_eq(name, "C"))
                break;
        }
    }
    R6(str_eq(name, "C") && str_eq(notes, "C E G"));
    trk[0].p[P_VOIC] = VC_OPEN; tf_chord_name(60, name, notes);
    R6(str_eq(name, "C") && str_eq(notes, "C E G"));
    trk[0].p[P_VOIC] = VC_INV1; tf_chord_name(60, name, notes);
    R6(str_eq(name, "C") && str_eq(notes, "C E G"));
    trk[0].p[P_VOIC] = VC_CLOSE;
    bad += check("round 6 item 9: CHORDETTE lists the chord notes up from the root (C E G with OPEN and an inversion too)", ok);
    return bad;
}
/* item 10: loudness. ITU BS.1770 style: K-weighting (its shelf and high-pass, re-derived for 44.1 kHz), 400 ms blocks
 * every 100 ms, the absolute gate (-70) and the relative one (-10 LU) for the integrated loudness, the loudest 3 s for the
 * short-term one. Each preset plays a part that fits its role (drums a beat, basses C2-C3, chords and pads C3-C4, leads
 * and bells C4-C6) at its default mix level, from a clean start */
#define R6L_N (R6_FS * 4u)                              /* 2 bars at 120 */
static int16_t r6l_buf[2][R6L_N];
enum { R6R_DRUMS, R6R_BASS, R6R_PAD, R6R_LEAD };
static uint32_t r6_role(uint32_t ins)
{
    if (TFM_INS[ins % TI_N].group == IG_DRUMS)          /* (round 8: the three kits) */
        return R6R_DRUMS;
    if (TFM_INS[ins].group == IG_BASS)
        return R6R_BASS;
    if (TFM_INS[ins].group == IG_LEAD || ins == TI_GLINT)
        return R6R_LEAD;
    return R6R_PAD;
}
typedef struct { uint32_t on, off; uint8_t note; } r6l_ev_t;
static uint32_t r6l_part(uint32_t ins, int32_t tr, r6l_ev_t *ev)     /* the part's notes (sample times), transposed tr */
{
    static const uint8_t BASS[8] = {36, 36, 48, 36, 43, 36, 46, 48}, LEAD[8] = {60, 67, 72, 64, 76, 79, 84, 72};
    static const uint8_t PAD[4][3] = {{48, 52, 55}, {45, 48, 52}, {50, 53, 57}, {47, 50, 55}};
    uint32_t n = 0, i, k, role = r6_role(ins);
    const uint32_t E = R6_FS / 4u;                      /* an eighth at 120 */
    if (role == R6R_DRUMS) {
        for (i = 0; i < 16u; i++) {                     /* hats on the eighths, kicks on 1 and 3 (and the and of 3), snares on 2 and 4 */
            ev[n].on = i * E; ev[n].off = i * E + E / 2u; ev[n++].note = 42;
            if ((i & 7u) == 0u || (i & 7u) == 4u || (i & 7u) == 5u) { ev[n].on = i * E; ev[n].off = i * E + E; ev[n++].note = 36; }
            if ((i & 7u) == 2u || (i & 7u) == 6u) { ev[n].on = i * E; ev[n].off = i * E + E; ev[n++].note = 38; }
        }
        return n;
    }
    if (role == R6R_BASS) {
        for (i = 0; i < 16u; i++) { ev[n].on = i * E; ev[n].off = i * E + E * 8u / 10u; ev[n++].note = (uint8_t)(BASS[i & 7u] + tr); }
        return n;
    }
    if (role == R6R_LEAD) {
        for (i = 0; i < 8u; i++) { ev[n].on = i * 2u * E; ev[n].off = i * 2u * E + E * 18u / 10u; ev[n++].note = (uint8_t)(LEAD[i] + tr); }
        return n;
    }
    for (i = 0; i < 4u; i++)                            /* chords on the half bars (CHORDETTE: its root alone, it makes the chord) */
        for (k = 0; k < (ins == TI_CHORDETTE ? 1u : 3u); k++) {
            ev[n].on = i * 4u * E; ev[n].off = i * 4u * E + E * 38u / 10u; ev[n++].note = (uint8_t)(PAD[i][k] + tr);
        }
    return n;
}
/* track 0 plays what is loaded on it (a preset, macros moved), the part transposed tr; the master as everyone hears it */
static uint8_t r6l_only;                                /* (a kit: one note of the beat alone, 0 all) */
static int32_t r6l_peak;                                /* the part's peak before the master (the track's: fx.c mix_part) */
static void r6l_play(uint32_t ins, int32_t tr)
{
    static r6l_ev_t ev[64];
    track_t *t = &trk[0];
    int32_t o[2 * CTL];
    uint32_t n, k, ne = r6l_part(ins, tr, ev);
    panic_req = (1u << NTRK) - 1u; mix_block(o, CTL);
    for (n = 0; n < 256u && (t->xf_on || t->eng_req != t->engine); n++) mix_block(o, CTL);
    r6l_peak = 0;
    for (n = 0; n < R6L_N; n += CTL) {
        uint32_t i;
        for (k = 0; k < ne; k++) {
            if (r6l_only && ev[k].note != r6l_only) continue;
            if (ev[k].off >= n && ev[k].off < n + CTL) trk_note_off(t, ev[k].note);
            if (ev[k].on >= n && ev[k].on < n + CTL) trk_note_on(t, ev[k].note, 100);
        }
        t->peak = 0;
        mix_block(o, CTL);
        if (t->peak > r6l_peak) r6l_peak = t->peak;
        for (i = 0; i < CTL && n + i < R6L_N; i++) {
            r6l_buf[0][n + i] = (int16_t)clamp(o[2 * i], -32768, 32767);
            r6l_buf[1][n + i] = (int16_t)clamp(o[2 * i + 1], -32768, 32767);
        }
    }
    for (k = 0; k < ne; k++) trk_note_off(t, ev[k].note);
}
static void r6l_setup(uint32_t ins, uint32_t pre)
{
    track_t *t = &trk[0];
    ui_power_on();
    memset(dly_buf, 0, sizeof dly_buf);
    memset(cho_buf, 0, sizeof cho_buf);
    rev_clear();
    tu_off = 0;
    song.master_q12 = 4096;
    song.g[G_BPM] = 120;
    rng_state = 0x12345678u;
    r6_fresh();
    song.sel = 0;
    tfm_load(t, ins, pre);
    t->p[P_ROOT] = 0; t->p[P_SCALE] = 1; t->p[P_QUANT] = QN_WHITE;
}
/* BS.1770 K-weighting at fs (the two biquads re-derived from the standard's analogue prototypes) */
typedef struct { double b0, b1, b2, a1, a2, z1, z2; } r6bq_t;
static double r6bq(r6bq_t *f, double x)
{
    double y = f->b0 * x + f->z1;
    f->z1 = f->b1 * x - f->a1 * y + f->z2;
    f->z2 = f->b2 * x - f->a2 * y;
    return y;
}
static void r6_kw(r6bq_t *sh, r6bq_t *hp)
{
    double fs = R6_FS, G = 3.99984385397, Q = 0.7071752369554193, fc = 1681.9744509555319;
    double A = pow(10.0, G / 40.0), w = 2.0 * 3.14159265358979 * fc / fs, cs = cos(w), al = sin(w) / (2.0 * Q), sa = 2.0 * sqrt(A) * al;
    double a0 = (A + 1.0) - (A - 1.0) * cs + sa;
    memset(sh, 0, sizeof *sh);
    memset(hp, 0, sizeof *hp);
    sh->b0 = A * ((A + 1.0) + (A - 1.0) * cs + sa) / a0;
    sh->b1 = -2.0 * A * ((A - 1.0) + (A + 1.0) * cs) / a0;
    sh->b2 = A * ((A + 1.0) + (A - 1.0) * cs - sa) / a0;
    sh->a1 = 2.0 * ((A - 1.0) - (A + 1.0) * cs) / a0;
    sh->a2 = ((A + 1.0) - (A - 1.0) * cs - sa) / a0;
    Q = 0.5003270373253953; fc = 38.13547087613982;
    w = 2.0 * 3.14159265358979 * fc / fs; cs = cos(w); al = sin(w) / (2.0 * Q);
    a0 = 1.0 + al;
    hp->b0 = (1.0 + cs) / 2.0 / a0; hp->b1 = -(1.0 + cs) / a0; hp->b2 = (1.0 + cs) / 2.0 / a0;
    hp->a1 = -2.0 * cs / a0; hp->a2 = (1.0 - al) / a0;
}
/* the integrated loudness (LUFS, gated) and the loudest short-term (3 s) of r6l_buf */
#define R6L_HOP (R6_FS / 10u)
static double r6l_ms[R6L_N / R6L_HOP + 1u];                /* each 100 ms: the sum of both channels' K-weighted squares */
static void r6_lufs(double *integ, double *st)
{
    uint32_t c, i, nb = R6L_N / R6L_HOP, b;
    double gs = 0, l, rel, sum = 0;
    uint32_t gn = 0;
    memset(r6l_ms, 0, sizeof r6l_ms);
    for (c = 0; c < 2u; c++) {
        r6bq_t sh, hp;
        r6_kw(&sh, &hp);
        for (i = 0; i < R6L_N; i++) {
            double y = r6bq(&hp, r6bq(&sh, r6l_buf[c][i] / 32768.0));
            r6l_ms[i / R6L_HOP] += y * y;
        }
    }
    for (b = 0; b + 4u <= nb; b++) {                    /* 400 ms blocks: the absolute gate */
        double z = (r6l_ms[b] + r6l_ms[b + 1] + r6l_ms[b + 2] + r6l_ms[b + 3]) / (4.0 * R6L_HOP);
        l = -0.691 + 10.0 * log10(z + 1e-20);
        if (l > -70.0) { gs += z; gn++; }
    }
    rel = gn ? -0.691 + 10.0 * log10(gs / gn) - 10.0 : -70.0;
    gs = 0; gn = 0;
    for (b = 0; b + 4u <= nb; b++) {                    /* the relative gate */
        double z = (r6l_ms[b] + r6l_ms[b + 1] + r6l_ms[b + 2] + r6l_ms[b + 3]) / (4.0 * R6L_HOP);
        l = -0.691 + 10.0 * log10(z + 1e-20);
        if (l > -70.0 && l > rel) { gs += z; gn++; }
    }
    *integ = gn ? -0.691 + 10.0 * log10(gs / gn) : -99.0;
    *st = -99.0;
    for (b = 0; b + 30u <= nb; b++) {                   /* 3 s windows */
        sum = 0;
        for (i = 0; i < 30u; i++) sum += r6l_ms[b + i];
        l = -0.691 + 10.0 * log10(sum / (30.0 * R6L_HOP) + 1e-20);
        if (l > *st) *st = l;
    }
}
static double r6_loud(uint32_t ins, uint32_t pre, int32_t tr, double *st)
{
    double i, s;
    r6l_setup(ins, pre);
    r6l_play(ins, tr);
    r6_lufs(&i, &s);
    if (st) *st = s;
    return i;
}
/* R6LOUD=1: the tables (each preset, the octaves, the macros); the checks always */
static void r6_loud_tables(void)
{
    uint32_t i, a, m;
    double s;
    for (a = 0; a < 5u; a++) {                          /* THUMP LAB: the kick, the snare, the hats alone */
        double k, sn, h;
        r6l_only = 36; k = r6_loud(TI_THUMP, a, 0, 0);
        r6l_only = 38; sn = r6_loud(TI_THUMP, a, 0, 0);
        r6l_only = 42; h = r6_loud(TI_THUMP, a, 0, 0);
        r6l_only = 0;
        fprintf(stderr, "KIT %-14s kick %6.1f snare %6.1f hats %6.1f%c", TFM_INS[TI_THUMP].snd[a].name, k, sn, h, 10);
    }
    for (i = 0; i < TI_N; i++) {
        for (a = 0; a < 5u; a++) {
            double l = r6_loud(i, a, 0, &s), pk = 20.0 * log10(r6l_peak / 32768.0 + 1e-9), lo = r6_loud(i, a, -12, 0), hi = r6_loud(i, a, 12, 0);
            fprintf(stderr, "LOUD %-10s %u %-14s I %6.1f S %6.1f  -1oct %6.1f  +1oct %6.1f  lvl %d  pk %5.1f%c", TFM_INS[i].name, a,
                    TFM_INS[i].snd[a].name, l, s, lo, hi, trk[0].p[P_LEVEL], pk, 10);
        }
        for (m = 0; m < 8u; m++) {
            const tfm_macro_t *mc = &TFM_INS[i].mac[m];
            double v[3];
            uint32_t k;
            if (mc->nsw)
                continue;
            for (k = 0; k < 3u; k++) {
                r6l_setup(i, 0);
                tfm_macro_put(&trk[0], m, (int32_t)(k * 127u / 2u), 0);
                r6l_play(i, 0);
                r6_lufs(&v[k], &s);
            }
            fprintf(stderr, "MACRO %-10s %-7s 0 %6.1f  50 %6.1f  100 %6.1f%c", TFM_INS[i].name, mc->name, v[0], v[1], v[2], 10);
        }
    }
}
/* the share of r6l_buf's energy under ~100 Hz (two one-pole low-passes): a kick's low end */
static double r6_lowfrac(void)
{
    double k = 1.0 - exp(-2.0 * 3.14159265358979 * 100.0 / R6_FS), y1 = 0, y2 = 0, sl = 0, sa = 0;
    uint32_t i;
    for (i = 0; i < R6L_N; i++) {
        double x = r6l_buf[0][i] / 32768.0;
        y1 += k * (x - y1);
        y2 += k * (y1 - y2);
        sl += y2 * y2;
        sa += x * x;
    }
    return sl / (sa + 1e-12);
}
static int r6_cmpd(const void *a, const void *b) { double x = *(const double *)a, y = *(const double *)b; return x < y ? -1 : x > y; }
/* the macros that brighten, resonate, drive or add space: their loudness over the whole turn (0 / 50 / 100) */
static const char *const R6_MSTEADY[] = {
    "THUMP LAB:SMACK", "THUMP LAB:DIRT", "THUMP LAB:ROOM", "THUMP LAB:ECHO", "SQUELCH:CUTOFF", "SQUELCH:RESO", "SQUELCH:ENV MOD",
    "SQUELCH:BITE", "SQUELCH:GNAW", "WOBBLER:GROWL", "WOBBLER:CUT", "WOBBLER:DIRT", "CHORDETTE:WARM", "CHORDETTE:SHIMMER",
    "CHORDETTE:SPACE", "RAZOR:SCREAM", "RAZOR:CUT", "HAZE:SPACE", "HAZE:TONE", "HAZE:SHINE", "PIXEL:CRUSH", "PIXEL:TONE",
    "TINE:BARK", "TINE:TINE", "TINE:WARMTH", "TINE:SPACE", "GLINT:METAL", "GLINT:BITE", "GLINT:SPACE", "GLINT:SHIMMER",
    "PLUCK:BRIGHT", "PLUCK:SPACE", "PLUCK:BOW", "VOX:BUZZ", "VOX:Q", "VOX:SPACE", "WHEELHOUSE:DRIVE", "WHEELHOUSE:CLICK",
    "WHEELHOUSE:TOP", "WHEELHOUSE:SPACE", 0};
#define R6_PRESET_LU 1.5                                /* a named preset from its instrument's reference (the median) */
#define R6_KICK_LOW 0.45                                /* (56 % now; 33 % with the kick held up at ~160 Hz before) */
#define R6_MACRO_LU 5.0                                 /* such a macro over its whole turn */
static int test_r6_loud(void)
{
    int bad = 0, ok = 1, kt_ok = 1, kit_ok = 1, mac_ok = 1, pk_ok = 1;
    uint32_t i, a, m;
    double s;
    if (getenv("R6LOUD"))
        r6_loud_tables();
    for (i = 0; i < TI_N; i++) {
        double l[5], srt[5], ref, rise = 0;
        uint32_t role = r6_role(i);
        for (a = 0; a < 5u; a++) {
            l[a] = srt[a] = r6_loud(i, a, 0, &s);
            if (r6l_peak >= 32768) { pk_ok = 0; fprintf(stderr, "R6 PEAK %s %u %d ", TFM_INS[i].name, a, (int)r6l_peak); }
            rise += r6_loud(i, a, 12, 0) - l[a];
        }
        qsort(srt, 5, sizeof srt[0], r6_cmpd);
        ref = srt[2];
        for (a = 0; a < 5u; a++) {                      /* (PLUCK's NYLON and MUTED: their pick's peak meets the ceiling first) */
            double lim = i == TI_PLUCK && (a == 1u || a == 2u) ? 3.0 : R6_PRESET_LU;
            if (fabs(l[a] - ref) > lim) { ok = 0; fprintf(stderr, "R6 LU %s %u %.1f ref %.1f ", TFM_INS[i].name, a, l[a], ref); }
        }
        if (role == R6R_DRUMS || role == R6R_BASS ? ref < -19.0 || ref > -15.5 : i == TI_PLUCK ? ref < -22.5 : ref < -19.5 || ref > -16.5) {
            ok = 0;
            fprintf(stderr, "R6 REF %s %.1f ", TFM_INS[i].name, ref);
        }
        if (rise / 5.0 > 2.0) { kt_ok = 0; fprintf(stderr, "R6 KT %s %.1f ", TFM_INS[i].name, rise / 5.0); }
        for (m = 0; m < 8u; m++) {
            const tfm_macro_t *mc = &TFM_INS[i].mac[m];
            char nm[32];
            uint32_t k, on = 0;
            double v[3], lo, hi;
            snprintf(nm, sizeof nm, "%s:%s", TFM_INS[i].name, mc->name);
            for (k = 0; R6_MSTEADY[k]; k++)
                on |= !strcmp(R6_MSTEADY[k], nm);
            if (!on || mc->nsw)
                continue;
            for (k = 0; k < 3u; k++) {
                r6l_setup(i, 0);
                tfm_macro_put(&trk[0], m, (int32_t)(k * 127u / 2u), 0);
                r6l_play(i, 0);
                r6_lufs(&v[k], &s);
            }
            lo = v[0] < v[1] ? (v[0] < v[2] ? v[0] : v[2]) : (v[1] < v[2] ? v[1] : v[2]);
            hi = v[0] > v[1] ? (v[0] > v[2] ? v[0] : v[2]) : (v[1] > v[2] ? v[1] : v[2]);
            if (hi - lo > R6_MACRO_LU) { mac_ok = 0; fprintf(stderr, "R6 MACRO %s %.1f ", nm, hi - lo); }
        }
    }
    {                                                   /* THUMP LAB WAREHOUSE (the first kit): kick, snare, hats alone */
        double k, sn, h, low;
        r6l_only = 36; k = r6_loud(TI_THUMP, 0, 0, 0); low = r6_lowfrac();
        r6l_only = 38; sn = r6_loud(TI_THUMP, 0, 0, 0);
        r6l_only = 42; h = r6_loud(TI_THUMP, 0, 0, 0);
        r6l_only = 0;
        fprintf(stderr, "KIT WAREHOUSE kick %.1f snare %.1f hats %.1f, the kick under 100 Hz %.0f %%%c", k, sn, h, low * 100.0, 10);
        kit_ok = fabs(k - sn) <= 3.0 && fabs(k - h) <= 3.0 && low >= R6_KICK_LOW;
    }
    bad += check("round 6 item 10: levels: every named preset within 1.5 LU of its instrument (PLUCK NYLON / MUTED 3), the instruments matched (drums and bass -19..-15.5 LUFS, the rest -19.5..-16.5)", ok);
    bad += check("round 6 item 10: no instrument alone peaks over 0 dBFS before the master (no loudness bought from the limiter)", pk_ok);
    bad += check("round 6 item 10: key tracking: an octave up rises 2 LU at most (an instrument's mean)", kt_ok);
    bad += check("round 6 item 10: THUMP LAB: the kick as loud as the snare and the hats on its own (3 LU), with its low end", kit_ok);
    bad += check("round 6 item 10: the brightness, resonance, drive and space macros keep their loudness (5 LU over the turn)", mac_ok);
    return bad;
}
/* ------------------------------------------------------------ round 7 item 1: the instruments stay --- */
#define R7(c) do { if (!(c)) { ok = 0; fprintf(stderr, "R7 line %d ", __LINE__); } } while (0)
static uint32_t r7_all(const uint8_t *want)             /* every track still has its instrument */
{
    uint32_t k, n = 0;
    for (k = 0; k < NTRK; k++)
        n += tfm_ins_of(&trk[k]) == want[k] && trk[k].preset >= TFM_PRE0;
    return n == NTRK;
}
static int test_r7_instruments(void)
{
    static const uint8_t A[NTRK] = {TI_THUMP, TI_SQUELCH, TI_CHORDETTE, TI_PIXEL}, B[NTRK] = {TI_THUMP, TI_WOBBLER, TI_HAZE, TI_GLINT};
    int bad = 0, ok = 1;
    uint32_t k, n;
    ui_power_on();
    tu_off = 0;
    sng_cur = 0;
    memset(proj_slot, 0, sizeof proj_slot);
    memset(song_rec, 0, sizeof song_rec);
    chain_defaults(&chain_config);
    scene_claim();
    for (k = 0; k < NTRK; k++) tfm_load(&trk[k], A[k], 1u);
    sc_note(0, 0, 55);
    tilt_scene_go(1, 0);                                /* B: other instruments on three tracks */
    for (k = 0; k < NTRK; k++) tfm_load(&trk[k], B[k], 2u);
    sc_note(0, 0, 66);
    tilt_scene_go(0, 0);
    R7(r7_all(A));
    chain_config.count = 2;
    chain_config.row[0] = chain_row_of(0, 1);
    chain_config.row[1] = chain_row_of(1, 1);
    ui_prefs3 |= PREF_SONG_SCENE;                       /* SOUNDS SCENE: each scene brings its sounds */
    tu_goto(TU_SONG);
    frames(16);
    R7(chain_prepare() == 0);
    events_block(32);
    for (n = 0; n < 4000u && chain.running && chain.row == 0u; n++) { events_block(32); tilt_scene_poll(); }
    R7(chain.running && chain.row == 1u);
    R7(r7_all(B));                                      /* B with its instruments, bytes and all */
    seq_stop(); transport_req = 0; events_block(32);
    R7(r7_all(A));                                      /* (stopped: the live sounds come back, each with its byte) */
    /* the chain again from JAM (the same path), then stopped */
    tu_goto(TU_JAM);
    frames(16);
    R7(chain_prepare() == 0);
    events_block(32);
    for (n = 0; n < 4000u && chain.running && chain.row == 0u; n++) { events_block(32); tilt_scene_poll(); }
    seq_stop(); transport_req = 0; events_block(32);
    R7(r7_all(A));
    /* JAM shows the jam macros, SOUND the face, EDIT toggles its pages there and never drops into DEEP */
    song.sel = 1; tu_goto(TU_JAM); frames(16);
    R7(tfm_ins_of(TSEL) == TI_SQUELCH && tu.scr == TU_JAM);
    tu_goto(TU_SOUND); frames(16);
    press(B_EDIT); frames(16);
    R7(tu.scr == TU_SOUND && tu.page == 1);
    press(B_EDIT); frames(16);
    R7(tu.scr == TU_SOUND && tu.page == 0);
    /* a power cycle: the autosave's way (capture, pack, unpack, restore) */
    {
        static project_t q, r;
        static project_store_t ps;
        project_capture(&q);
        proj_pack(&ps, &q);
        memset(&r, 0, sizeof r);
        R7(proj_unpack(&r, ps.raw, PROJ_STORE_SIZE) == 1);
        for (k = 0; k < NTRK; k++) tfm_load(&trk[k], TI_PLUCK, 0u);   /* (the power-on state: other sounds) */
        R7(project_restore_runtime(&r) == 0);
        R7(r7_all(A));
        R7(chain_config.count == 2u);
    }
    /* MUTATE SOUND keeps the instrument */
    song.sel = 2; prf_mut = 1u;
    prf_mutate();
    R7(tfm_ins_of(&trk[2]) == TI_CHORDETTE && trk[2].preset >= TFM_PRE0);
    bad += check("round 7 item 1: instruments stay after a SONG chain with SOUNDS SCENE, a chain from JAM, a power cycle (pack, restore) and MUTATE SOUND (every track keeps its instrument byte; SOUND keeps its face)", ok);
    return bad;
}
/* round 7 item 2: a track with no TFM instrument stays on TFM screens: EDIT toggles the pages, never DEEP; DEEP only from the MENU */
static int test_r7_noinst(void)
{
    int bad = 0, ok = 1;
    uint32_t i;
    ui_power_on();
    tu_off = 0;
    song.sel = 0;
    tfm_load(&trk[0], TI_WOBBLER, 1u);
    trk[0].preset = 4;                                  /* a raw engine preset: no instrument */
    R7(tfm_ins_of(&trk[0]) == TI_NONE);
    tu_goto(TU_SOUND);
    frames(32);
    for (i = 0; i < 3u; i++) {
        press(B_EDIT); frames(32);
        R7(tu.scr == TU_SOUND && tu.page == (i & 1u ? 0u : 1u));
    }
    R7(!tu_deep_on());
    tu_goto(TU_JAM);
    frames(32);
    R7(tu.scr == TU_JAM && !tu_deep_on());
    bad += check("round 7 item 2: a track with no instrument keeps SOUND on its TFM face (EDIT toggles the pages, never DEEP; DEEP is the MENU row only)", ok);
    return bad;
}
/* round 7 item 3: the build name: START shows it, MENU > INFO's first row is it, ABOUT's TILT-FM line has it (tools/build.py: TILT_BUILD) */
static int test_r7_build(void)
{
    int bad = 0, ok = 1;
    R7(sizeof TILT_BUILD > 1 && sizeof TILT_BUILD <= 17);
    st.has = 1; st.crash = 0; tilt_ns_open = TILT_START; sv.mode = SV_WAIT; sv.last_ok = 1;
    tu_off = 0; ui.force = 1; frames(32);
    {
        uint32_t k, lit = 0;                            /* the build's pixels on START's display (x 14.., y 40..): not the display's blank */
        for (k = 0; k < 60u; k++)
            lit += host_screen[(41u + k % 8u) * 240u + 14u + k] != host_screen[(41u + k % 8u) * 240u + 200u];
        R7(lit > 0u);
    }
    tilt_ns_open = 0;
    bad += check("round 7 item 3: the build name (TILT_BUILD) is drawn on START's display", ok);
    return bad;
}
/* ------------------------------------------------------------ round 8 --- */
#define R8(c) do { if (!(c)) { ok = 0; fprintf(stderr, "R8 line %d ", __LINE__); } } while (0)
/* item 1: hold ARP = DJ: K1 FILTER (LP left, HP right, off in the middle), K2 ECHO (+ feedback past 70 %), K3 SPACE, K4 DIRT
 * (the INSERT, its type kept), ALGORITHM / F#3..C#4 pick the track, let go: back (a glide), OCT+ LATCH, OCT- all back */
static int test_r8_dj(void)
{
    int bad = 0, ok = 1;
    static int32_t buf[CTL];
    uint32_t i, n;
    int16_t dly, rev, imix, ia, fb;
    double e0, e1;
    /* the filter itself: Nyquist through the LP far closed, DC through the HP */
    dj_f[0] = -60; dj_s1[0] = dj_s2[0] = 0;
    for (n = 0; n < 4u; n++) {
        for (i = 0; i < CTL; i++) buf[i] = i & 1u ? 20000 : -20000;
        dj_filter(&trk[0], buf, CTL);
    }
    e0 = 0; for (i = 0; i < CTL; i++) e0 += fabs((double)buf[i]);
    R8(e0 / CTL < 2000.0);                              /* (-20 dB at least) */
    dj_f[0] = 60; dj_s1[0] = dj_s2[0] = 0;
    for (n = 0; n < 200u; n++) {
        for (i = 0; i < CTL; i++) buf[i] = 20000;
        dj_filter(&trk[0], buf, CTL);
    }
    e1 = 0; for (i = 0; i < CTL; i++) e1 += fabs((double)buf[i]);
    R8(e1 / CTL < 200.0);
    dj_f[0] = 2;                                        /* (the middle: off, untouched) */
    for (i = 0; i < CTL; i++) buf[i] = i & 1u ? 1234 : -1234;
    dj_filter(&trk[0], buf, CTL);
    R8(buf[0] == -1234 && buf[1] == 1234);
    dj_f[0] = 0;
    /* the layer */
    ui_power_on();
    tu_off = 0;
    tu_goto(TU_JAM);
    tfm_load(&trk[0], TI_SQUELCH, 0);
    tfm_load(&trk[1], TI_HAZE, 0);
    trk[1].p[P_ITYPE] = 0;                              /* (no INSERT: DIRT runs SOFT while it moves) */
    song.sel = 1;
    frames(32);
    dly = trk[1].p[P_DLY]; rev = trk[1].p[P_REV]; imix = trk[1].p[P_IMIX]; ia = trk[1].p[P_IA]; fb = song.g[G_DFDBK];
    r5_perf_open();
    R8(prf_on() && tu.scr == TU_JAM);
    turn(EN_K1, -10); frames(16);
    R8(dj_f[1] < -3);
    for (n = 0; n < 20u; n++) turn(EN_K2, 10);
    frames(16);
    R8(trk[1].p[P_DLY] == TP[P_DLY].max && song.g[G_DFDBK] > fb);
    turn(EN_K3, 10); frames(16);
    R8(trk[1].p[P_REV] > rev);
    turn(EN_K4, 10); frames(16);
    R8(trk[1].p[P_ITYPE] == 1 && trk[1].p[P_IMIX] > 0 && trk[1].p[P_IA] >= ia);
    /* ALGORITHM and F#3 pick the track */
    turn(EN_ALGO, 1); frames(16);
    R8(song.sel == 2u);
    r5_key(0);
    R8(song.sel == 0u);
    /* let go: a glide back to where it started (exactly) */
    r5_perf_close();
    for (n = 0; n < 40u; n++) { fm1_ms += 16u; frame(); }
    R8(dj_f[1] == 0 && trk[1].p[P_DLY] == dly && trk[1].p[P_REV] == rev && trk[1].p[P_IMIX] == imix && trk[1].p[P_IA] == ia &&
       trk[1].p[P_ITYPE] == 0 && song.g[G_DFDBK] == fb);
    /* LATCH: OCT+ tapped while holding, let go: the moves stay; OCT- (no key rolling): all back */
    song.sel = 0; frames(16);
    dly = trk[0].p[P_DLY];
    r5_perf_open();
    press(B_OCTUP); frames(16);
    R8(dj_latched());
    for (n = 0; n < 6u; n++) turn(EN_K2, 10);
    frames(16);
    r5_perf_close();
    for (n = 0; n < 40u; n++) { fm1_ms += 16u; frame(); }
    R8(trk[0].p[P_DLY] > dly && dj_latched());
    r5_perf_open();
    press(B_OCTDN); frames(16);
    R8(!dj_latched() && trk[0].p[P_DLY] == dly);
    /* with a white key held (it rolls), OCT- / OCT+ step the roll rate */
    prf_rate = 2;
    fm1_in.notes |= 1u << key_at(0, 4); host_notes |= 1u << key_at(0, 4); frame();
    press(B_OCTUP); frames(16);
    R8(prf_rate == 3u && !dj_latched());
    press(B_OCTDN); press(B_OCTDN); frames(16);
    R8(prf_rate == 1u);
    fm1_in.notes &= ~(1u << key_at(0, 4)); frame(); frame();
    r5_perf_close();
    bad += check("round 8 item 1: hold ARP = DJ (FILTER LP / HP / off, ECHO with feedback, SPACE, DIRT on the INSERT; ALGORITHM and F#3 pick; let go: back exactly; OCT+ LATCH, OCT- all back; OCT-/+ the roll rate while rolling)", ok);
    return bad;
}
/* item 2: MORPH (macro 1, JAM's knob): from the sound as set (0) to its contrast (100); in key; the level within 3 LU over the
 * turn; back at 0 exactly as it was; another macro turned while it is out moves its base */
static double r8_morph_lu(uint32_t ins, int32_t pos, double *feat)
{
    double l, s;
    r6l_setup(ins, 0);
    tfm_macro_put(&trk[0], 0, pos, 0);
    r6l_play(ins, 0);
    r6_lufs(&l, &s);
    if (feat) {                                         /* (the audit's features of the same, its own render) */
        r6_render(ins, 0);
        (void)feat;
    }
    return l;
}
static int test_r8_morph(void)
{
    int bad = 0, ok = 1, lv = 1, key = 1, back = 1;
    uint32_t i;
    for (i = 0; i < TI_N; i++) {
        double l0 = r8_morph_lu(i, 0, 0), l1 = r8_morph_lu(i, 64, 0), l2 = r8_morph_lu(i, 127, 0), lo, hi;
        lo = l0 < l1 ? (l0 < l2 ? l0 : l2) : (l1 < l2 ? l1 : l2);
        hi = l0 > l1 ? (l0 > l2 ? l0 : l2) : (l1 > l2 ? l1 : l2);
        fprintf(stderr, "MORPH %-10s LUFS 0 %6.1f  50 %6.1f  100 %6.1f  (%.1f LU)%c", TFM_INS[i].name, l0, l1, l2, hi - lo, 10);
        if (hi - lo > 3.0) lv = 0;
        if (TFM_INS[i].group != IG_DRUMS) {             /* (a kit has no pitch) */
            double w1 = r3_pitch(i, 0, 0, 50), w2 = r3_pitch(i, 0, 0, 100);
            if ((w1 != 999.0 && w1 > 35.0) || (w2 != 999.0 && w2 > 35.0)) {
                key = 0;
                fprintf(stderr, "R8 MORPH PITCH %s %.0f %.0f ", TFM_INS[i].name, w1, w2);
            }
        }
    }
    {   /* back at 0: the sound exactly as it was; a macro turned while out moves the base */
        track_t *t = &trk[0];
        int16_t p0[P_COUNT];
        ui_power_on();
        tfm_load(t, TI_SQUELCH, 0);
        memcpy(p0, t->p, sizeof p0);
        tfm_macro_put(t, 0, 127, 0);
        back &= t->p[P_E5] != p0[P_E5] && tfm_macro_get(t, 0) == 127;
        tfm_macro_put(t, 0, 0, 0);
        back &= !memcmp(p0, t->p, sizeof p0) && tfm_macro_get(t, 0) == 0;
        tfm_macro_put(t, 0, 64, 0);
        tfm_macro_put(t, 1, 0, 0);                      /* CUTOFF down while MORPH is out */
        {
            int16_t lowcut = (int16_t)tfm_tgt_val(&TFM_INS[TI_SQUELCH].mac[1].t[0], 0);
            tfm_macro_put(t, 0, 0, 0);
            back &= t->p[P_E4] == lowcut;               /* (back at 0: the base is where CUTOFF put it) */
        }
    }
    bad += check("round 8 item 2: MORPH on every instrument keeps its level (3 LU over the turn)", lv);
    bad += check("round 8 item 2: MORPH stays in key at 50 and 100 (YIN, +-35 cents)", key);
    bad += check("round 8 item 2: MORPH back at 0 is the sound exactly as it was; a macro turned while it is out moves its base", back && ok);
    return bad;
}
/* item 3: how far each macro moves the sound, 0 -> 100, in the audit's features (on the first named preset, the rest as it
 * sets them); a continuous one under R8_MACRO_MIN is widened. R8MACRO=1 prints the table (tfm-spec/MACRO-RATINGS.md) */
#define R8_MACRO_MIN 0.40                                /* (from the table: the lowest fifth of the 85 continuous macros and MORPHs sat under it) */
static double r8_macro_dist(uint32_t ins, uint32_t m)
{
    double a[R6_NF], b[R6_NF];
    r6_mac = (int32_t)m; r6_macv = 0;
    r6_render(ins, 0); r6_features(a);
    r6_macv = 127;
    r6_render(ins, 0); r6_features(b);
    r6_mac = -1;
    return r6_dist(a, b);
}
static int test_r8_macros(void)
{
    int bad = 0, ok = 1;
    uint32_t i, m, n = 0, under = 0;
    int verbose = getenv("R8MACRO") != 0;
    for (i = 0; i < TI_N; i++)
        for (m = 0; m < 8u; m++) {
            const tfm_macro_t *mc = &TFM_INS[i].mac[m];
            double d = r8_macro_dist(i, m);
            int sw = mc->nsw && mc->nsw != TFM_MSW;
            if (verbose)
                fprintf(stderr, "MACRODIST %s|%u|%s|%s|%.2f%c", TFM_INS[i].name, m + 1u, mc->name, sw ? "SWITCH" : mc->nsw == TFM_MSW ? "MORPH" : "CONT", d, 10);
            if (!sw && (!strcmp(mc->name, "GLIDE") || !strcmp(mc->name, "SLIDE") || !strcmp(mc->name, "VIB")))
                continue;                               /* (they move pitch between notes and around one: the phrase hears neither) */
            if (!sw) {
                n++;
                if (d < R8_MACRO_MIN) {
                    under++;
                    fprintf(stderr, "R8 SUBTLE %s %s %.2f ", TFM_INS[i].name, mc->name, d);
                }
            }
        }
    R8(n > 75u && under == 0u);
    bad += check("round 8 item 3: every continuous macro (and MORPH) moves the sound at least R8_MACRO_MIN over its turn (the audit's distance, on the first preset)", ok);
    return bad;
}
/* item 4: three drum instruments on the DRUM engine's own kits, each with a cymbal, kick, snare or rim, both hats; drums on
 * the white keys only (F3..F4 the lanes, G4..G5 accented), the black keys never; CLASSIC keeps the GM map */
static int test_r8_kits(void)
{
    int bad = 0, ok = 1, keys = 1;
    static const uint8_t KITS[3] = {TI_THUMP, TI_TRASH, TI_HAND};
    uint32_t i, k, l;
    ui_power_on();
    tu_off = 0;
    for (i = 0; i < 3u; i++) {
        track_t *t = &trk[0];
        uint32_t cym = 0, kick = 0, snr = 0, hc = 0, ho = 0;
        tfm_load(t, KITS[i], 0);
        R8(TFM_INS[KITS[i]].group == IG_DRUMS && drum_track(t) && tfm_ins_of(t) == KITS[i]);
        for (l = 0; l < NLANE; l++) {
            const char *n = drum_lane_name(t, l);
            cym |= str_eq(n, "CYM"); kick |= str_eq(n, "KICK"); snr |= str_eq(n, "SNARE") || str_eq(n, "RIM");
            hc |= str_eq(n, "HATCL"); ho |= str_eq(n, "HATOP");
        }
        R8(cym && kick && snr && hc && ho);
        for (k = 1; k < 5u; k++)                        /* (every named preset plays its instrument's kit) */
        { tfm_load(t, KITS[i], k); R8(t->p[P_E0] == TFM_INS[KITS[i]].vo[0].v); }
    }
    R8(trk[0].p[P_E0] != 0);
    {   /* three different kits */
        track_t a, b, c;
        tfm_load(&trk[0], TI_THUMP, 0); a = trk[0];
        tfm_load(&trk[0], TI_TRASH, 0); b = trk[0];
        tfm_load(&trk[0], TI_HAND, 0); c = trk[0];
        R8(a.p[P_E0] != b.p[P_E0] && b.p[P_E0] != c.p[P_E0] && a.p[P_E0] != c.p[P_E0]);
    }
    /* the keys: TFM, a drum track: the white keys from F3: lanes 1..8, then the same accented; the black keys silent */
    tfm_load(&trk[0], TI_TRASH, 0);
    song.sel = 0;
    tu_goto(TU_SOUND); frames(16);
    for (k = 0; k < 27u; k++) {
        uint32_t n;
        tfm_dwhite = 1;
        n = kb_map(&trk[0], k);
        if (key_black(k))
            keys &= n == KB_SILENT;
        else if (key_place(k) < 2u * NLANE)
            keys &= n == DRUM_LANE_NOTE[key_place(k) % NLANE] && (((dw_acc >> k) & 1u) == (key_place(k) >= NLANE));
    }
    tfm_dwhite = 0;                                     /* (CLASSIC: the GM map over every key, as before) */
    keys &= kb_map(&trk[0], 1) != KB_SILENT && kb_map(&trk[0], 7) == (uint32_t)clamp(29 + 12 * song.octave + 7, 0, 127);
    dw_acc = 0;
    bad += check("round 8 item 4: THUMP LAB (DEEP), TRASH KIT (10), HAND DRUMS (77): three kits, each with a cymbal, kick, snare or rim, both hats; every preset on its kit", ok);
    bad += check("round 8 item 4: drums on the white keys (F3..F4 the eight lanes, G4..G5 the same accented), the black keys silent; CLASSIC keeps the GM map", keys);
    return bad;
}
/* item 5: the pitched lane: a drum step's pitch in the scale, from note[0] of a hits-only step (64 + degrees); the hit of that
 * lane sounds that many scale steps from the lane's own pitch; DRUMS K2 sets it there; a save keeps it */
static int test_r8_pitched_lane(void)
{
    int bad = 0, ok = 1;
    track_t *t = &trk[0];
    step_t *st;
    drum_lane_t *K;
    int8_t base;
    ui_power_on();
    tu_off = 0;
    song.sel = 0;
    tfm_load(t, TI_THUMP, 0);
    t->p[P_ROOT] = 0; t->p[P_SCALE] = 1;                /* (major) */
    R8(drum_plane_fn(t) == 0u);
    tfm_load(&trk[1], TI_HAND, 0); R8(drum_plane_fn(&trk[1]) == 5u); tfm_load(&trk[1], TI_SQUELCH, 0); R8(drum_plane_fn(&trk[1]) == 0xFFu);
    R8(drum_deg_st(t, 2) == 4 && drum_deg_st(t, -1) == -1 && drum_deg_st(t, 7) == 12 && drum_deg_st(t, 0) == 0);
    st = &t->step[0];
    step_clear(st);
    st->time = ST_NOTE; st->hit = 1u; st->vel = 100;
    panic_req = 15u; { int32_t o[2 * CTL]; uint32_t f; for (f = 0; f < 64u; f++) mix_block(o, CTL); }
    trk_note_on(t, DRUM_LANE_NOTE[0], 100);             /* unpitched: the kick at its own pitch */
    K = drum_kit_of(t);
    R8(K != 0);
    base = K ? K[0].st : 0;
    trk_note_off(t, DRUM_LANE_NOTE[0]);
    st->note[0] = 64 + 2;                               /* two scale steps up */
    drum_pitch_set(t, st);
    trk_note_on(t, DRUM_LANE_NOTE[0], 100);
    R8(K && K[0].st == base + 4);
    drum_ppl[0] = 0;
    trk_note_off(t, DRUM_LANE_NOTE[0]);
    /* DRUMS: on the pitched lane, a hit at the cursor: K2 is PITCH */
    tu_goto(TU_DRUMS); frames(16);
    ui.lane = 0; cursor_set(0); frames(16);
    st->note[0] = 0;
    turn(EN_K2, 1); frames(16);
    R8(st->note[0] == 65u && ui.lane == 0u);
    turn(EN_K2, -1); frames(16);
    R8(st->note[0] == 0u);
    ui.lane = 1; frames(16);
    turn(EN_K2, 1); frames(16);
    R8(ui.lane == 2u && st->note[0] == 0u);             /* (another lane: K2 picks the lane, as before) */
    st->note[0] = 64 + 1;                               /* another lane's hit on a pitched step keeps the pitch */
    grid_hit(t, 0, 2, 1);
    R8(st->note[0] == 65u && (st->hit & 4u));
    grid_hit(t, 0, 2, 0);
    st->note[0] = 0;
    {   /* a save keeps it (the four note bytes are stored for every step) */
        static project_t q, r;
        static project_store_t ps;
        st->note[0] = 64 - 3;
        project_capture(&q);
        proj_pack(&ps, &q);
        R8(proj_unpack(&r, ps.raw, PROJ_STORE_SIZE) == 1 && r.t[0].step[0].note[0] == 61u && r.t[0].step[0].hit == 1u && r.t[0].step[0].n == 0u);
    }
    tu_goto(TU_JAM);
    bad += check("round 8 item 5: the pitched lane (THUMP LAB's kick, TRASH KIT's and HAND DRUMS' tom) plays a step's pitch in the scale (note[0] of a hits-only step), DRUMS K2 sets it, a save keeps it", ok);
    return bad;
}
