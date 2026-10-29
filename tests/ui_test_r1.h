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
            if (str_eq(tu_ft.k[i], "SCL") && str_eq(tu_ft.a[i], "TEMPO"))
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
        static const uint8_t SC[8] = {TU_STEP, TU_SOUND, TU_SONG, TU_SONGS, TU_FX, TU_CLOCK, TU_MOTION, TU_ARP};
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
