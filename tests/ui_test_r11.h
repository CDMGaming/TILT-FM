/* TILT-FM round 11 (tfm-spec/ROUND-11.md): included by ui_test.c after ui_test_r6.h */
#define R11(c) do { if (!(c)) { ok = 0; printf("  R11 line %d\n", __LINE__); } } while (0)

/* item 2: the assignable SELECT knob */
static int32_t r11_val(uint32_t c)                      /* what choice c moves, as it is now */
{
    switch (c) {
    case SA_FILTER: return dj_now(0);
    case SA_ECHO: return dj_now(1);
    case SA_SPACE: return dj_now(2);
    case SA_MORPH: return tfm_macro_get(TSEL, 0);
    case SA_LEVEL: return TSEL->p[P_LEVEL];
    case SA_MOD: return TSEL->mw;
    case SA_SCENE: return (int32_t)scene_cur() * 16 + (int32_t)tsc.cue;
    case SA_SWING: return song.g[G_SWING];
    case SA_TEMPO: return song.g[G_BPM];
    }
    return 0;
}
static void r11_base(void)                              /* every value off its end, T1 a THUMP LAB (a drum track with MORPH) */
{
    uint32_t i;
    for (i = 0; i < DJ_N; i++)
        dj_put(i, 0);
    dj_mb = 0;
    tfm_load(&trk[0], TI_THUMP, 0);
    song.sel = 0;
    tfm_macro_set(TSEL, 0, 30);
    TSEL->p[P_LEVEL] = 60;
    TSEL->mw = 0;
    song.g[G_SWING] = 0;
    song.g[G_BPM] = 120;
    tilt_scene_uncue();
}
static int test_r11_select(void)
{
    static const uint8_t SCR[4] = {TU_JAM, TU_MIX, TU_SOUND, TU_DRUMS};
    int bad = 0, ok = 1;
    uint32_t i, c, n;
    int32_t v0;
    ui_power_on();
    tu_off = 0;
    R11(CFG(CF_SEL) == 0u && CFG(CF_GSEL) == 0u && sa_get(0) == SA_FILTER && sa_get(1) == SA_SCENE);
    bad += check("round 11 SELECT: a fresh install, SELECT = FILTER and GLO + SELECT = SCENE", ok);

    ok = 1;
    r5_scenes();                                        /* (A..D stored: SCENE has somewhere to go) */
    tilt_scene_go(0, 1);
    for (i = 0; i < 4u; i++)
        for (c = SA_FILTER; c < SA_N; c++) {
            r11_base();
            sa_put(0, c);
            tu_goto(SCR[i]);
            frames(4);
            v0 = r11_val(c);
            turn(EN_SELECT, 3);
            frames(2);
            if (r11_val(c) == v0)
                printf("  R11 SELECT %s on screen %u did not move (%d)\n", SA_NAME[c], (unsigned)SCR[i], (int)v0);
            R11(r11_val(c) != v0);
            R11(tsl_on() && str_eq(tsl.n, SA_NAME[c]));   /* (the header's readout) */
            if (c == SA_TEMPO || c == SA_SWING) {
                R11(dj_now(0) == 0 && TSEL->p[P_LEVEL] == 60);   /* (only its own value) */
            }
        }
    r11_base();
    sa_put(0, SA_OFF);
    tu_goto(TU_JAM); frames(2);
    turn(EN_SELECT, 3); frames(2);
    R11(dj_now(0) == 0 && song.g[G_BPM] == 120 && TSEL->p[P_LEVEL] == 60);
    fm1_ms += TSL_MS + 10u; frames(2);
    R11(!tsl_on());                                     /* (the readout goes after about a second) */
    bad += check("round 11 SELECT: each of the nine choices moves its value on JAM, MIX, SOUND and DRUMS; OFF nothing", ok);

    ok = 1;                                             /* the lists keep SELECT for moving */
    r11_base();
    sa_put(0, SA_TEMPO);
    tu_goto(TU_SONGS); frames(4);
    n = tsg_sel;
    turn(EN_SELECT, 1); frames(2);
    R11(tsg_sel != n && song.g[G_BPM] == 120);
    tu_goto(TU_NOTES); frames(4);
    turn(EN_SELECT, 2); frames(2);
    R11(song.g[G_BPM] == 120);
    bad += check("round 11 SELECT: SONGS and NOTES keep SELECT (the list, the pitch scroll), the choice untouched", ok);

    ok = 1;                                             /* GLO + SELECT: the second row */
    r11_base();
    sa_put(0, SA_FILTER);
    sa_put(1, SA_TEMPO);
    tu_goto(TU_JAM); frames(4);
    btn_down(B_GLO); frames(30);
    turn(EN_SELECT, 3); frames(2);
    R11(song.g[G_BPM] > 120 && dj_now(0) == 0);
    btn_up(B_GLO); frames(30); fm1_ms += 3000u; frames(4);   /* (the quiet after a layer, timed out) */
    v0 = song.g[G_BPM];
    turn(EN_SELECT, 3); frames(2);
    R11(dj_now(0) > 0 && song.g[G_BPM] == v0);
    r11_base();
    CFG(CF_GSEL) = 0;                                   /* (the default: SCENE) */
    tu_goto(TU_JAM); frames(4);
    v0 = r11_val(SA_SCENE);
    btn_down(B_GLO); frames(30);
    turn(EN_SELECT, 1); frames(2);
    btn_up(B_GLO); frames(30); fm1_ms += 3000u; frames(4);
    R11(r11_val(SA_SCENE) != v0);
    bad += check("round 11 SELECT: GLO + SELECT turns the second row (default SCENE), SELECT alone the first", ok);

    ok = 1;                                             /* REC on while playing: six record, three do not */
    for (c = SA_FILTER; c < SA_N; c++) {
        uint32_t m0, id = c <= SA_SPACE ? MO_DJ + (c - SA_FILTER) : c == SA_MORPH ? 0xFFu : c == SA_LEVEL ? P_LEVEL :
                          c == SA_MOD ? MO_MW : 0xFEu;
        r11_base();
        motion_clear(TSEL);
        sa_put(0, c);
        tu_goto(TU_JAM); frames(4);
        song.rec = 1u << song.sel;
        seq_start();
        frames(4);
        m0 = motion_count(TSEL);
        turn(EN_SELECT, 4); frames(4);
        if (c <= SA_MOD) {
            R11(motion_count(TSEL) > m0);
            if (id < 0xF0u) {
                uint32_t k, found = 0;
                for (k = 0; k < motion.count; k++)
                    found |= (motion.event[k].place >> 6) == song.sel && MOTION_ID(&motion.event[k]) == id;
                R11(found);
            }
        } else {
            R11(motion_count(TSEL) == m0);
        }
        song.rec = 0;
        press(B_PLAY); events_block(32); frames(4);
        tilt_scene_uncue();
    }
    {   /* MOD's moves play back: the mod wheel as recorded */
        r11_base();
        motion_clear(TSEL);
        sa_put(0, SA_MOD);
        song.rec = 1u << song.sel;
        seq_start(); frames(4);
        turn(EN_SELECT, 6); frames(4);
        song.rec = 0;
        press(B_PLAY); events_block(32); frames(4);
        TSEL->mw = 0;
        seq_start();
        for (n = 0; n < 4000u && !TSEL->mw; n++) { int32_t o[2 * CTL]; mix_block(o, CTL); }
        R11(TSEL->mw > 0);
        press(B_PLAY); events_block(32); frames(4);
    }
    bad += check("round 11 SELECT: with REC on, FILTER ECHO SPACE MORPH LEVEL MOD record as motion (MOD plays back), SCENE SWING TEMPO not", ok);

    ok = 1;                                             /* the player's setting: saved with the settings, no size change */
    {
        persist_t p;
        sa_put(0, SA_MOD);
        sa_put(1, SA_SWING);
        memset(&p, 0, sizeof p);
        settings_export(&p);
        CFG(CF_SEL) = CFG(CF_GSEL) = 0;
        R11(settings_import(&p, (int)sizeof p) == 1 && sa_get(0) == SA_MOD && sa_get(1) == SA_SWING);
    }
    n = mtab_rows(MTAB_CONTROL);                        /* (MENU > CONTROL > SELECT: CLASSIC's) */
    tu_off = 1;
    R11(mtab_rows(MTAB_CONTROL) == n + 1u && mtab_first(MTAB_CONTROL) + n == MI_SELECT);
    tu_off = 0;
    bad += check("round 11 SELECT: kept across a restart (the settings record), MENU's SELECT row only in CLASSIC", ok);
    sa_put(0, SA_FILTER);
    CFG(CF_SEL) = CFG(CF_GSEL) = 0;
    ui_power_on();
    return bad;
}

