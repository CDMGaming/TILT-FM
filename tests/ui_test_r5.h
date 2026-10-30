/* TFM-UI round 5: live arranging from JAM (included by ui_test.c after round 4; same helpers) */
#define R5(c) do { if (!(c)) { ok = 0; fprintf(stderr, "R5 line %d ", __LINE__); } } while (0)
static void r5_key(uint32_t idx)                        /* a tap of black key idx 0..10 */
{
    int32_t k = jl_key(idx);
    fm1_in.notes |= 1u << k; host_notes |= 1u << k; frame();
    fm1_in.notes &= ~(1u << k); frame();
}
static void r5_down(uint32_t idx) { int32_t k = jl_key(idx); fm1_in.notes |= 1u << k; host_notes |= 1u << k; frame(); }
static void r5_up(uint32_t idx) { int32_t k = jl_key(idx); fm1_in.notes &= ~(1u << k); frame(); }
static void r5_scenes(void)                             /* song 1: A (T1: 2 bars, note 55), B (T1: 1 bar, note 66), C, D stored too */
{
    uint32_t s;
    sng_cur = 0;
    memset(proj_slot, 0, sizeof proj_slot);
    memset(song_rec, 0, sizeof song_rec);
    chain_defaults(&chain_config);
    scene_claim();
    for (s = 0; s < 4u; s++) {
        if (s) tilt_scene_go(s, 0);
        sc_note(0, 0, 55u + 11u * (s & 1u) + s);
        trk[0].p[P_SLEN] = s == 0u ? 32 : 16;
    }
    tilt_scene_go(0, 0);
    chain_defaults(&chain_config);
}
static int test_r5_live(void)
{
    int bad = 0, ok = 1;
    uint32_t n, step;
    ui_power_on();
    tu_off = 0;
    r5_scenes();
    R5(scene_cur() == 0u && project_used(1) && project_used(2) && project_used(3) && trk[0].p[P_SLEN] == 32);
    tu_goto(TU_JAM);
    frames(32);
    /* the black keys play nothing on JAM; F#3..A#4 are the scenes */
    R5(jl_key(0) >= 0 && jl_key(10) >= 0 && jl_key(11) < 0 && jam_silent == 1u);
    R5(kb_map(&trk[0], (uint32_t)jl_key(3)) == KB_SILENT);
    /* PLAY: the scene plays; a scene key queues (and the key light breathes), the launch is at the LOOP's end */
    transport_req = 1; events_block(32); frames(16);
    R5(song.playing);
    r5_key(1);
    R5(tsc.cue == 1u && scene_armed == 1u);
    {
        uint32_t br = 0;
        jl_leds(&br, 0);
        R5(((br >> jl_key(1)) & 1u) != 0u);
    }
    step = div_samples(2);
    for (n = 0; n < 4000u && trk[0].seq_idx < 17u; n++) { events_block(32); tilt_scene_poll(); }
    R5(scene_cur() == 0u && tsc.cue == 1u);                                  /* (past the first bar: still A) */
    for (n = 0; n < 6000u && scene_cur() == 0u; n++) { events_block(32); tilt_scene_poll(); }
    R5(scene_cur() == 1u && tsc.cue == SCENE_NONE && trk[0].step[0].note[0] == 67u);
    R5(jl_n >= 2u && jl[jl_n - 1u].sc == 1u);                                  /* the log: A, then B */
    (void)step;
    /* tap again: cancelled; several taps: the last wins; the playing scene's key keeps it looping */
    r5_key(2); R5(tsc.cue == 2u);
    r5_key(2); R5(tsc.cue == SCENE_NONE);
    r5_key(2); r5_key(3); R5(tsc.cue == 3u);
    r5_key(1); R5(tsc.cue == SCENE_NONE && scene_cur() == 1u);
    /* FILL: D#5 held */
    r5_down(9); R5(seq_fill == 1u); r5_up(9); R5(seq_fill == 0u);
    /* C#5 does nothing */
    r5_key(8); R5(tsc.cue == SCENE_NONE);
    transport_req = 2; events_block(32); frames(16);
    R5(!song.playing);
    /* copy: OCT+ and a scene key (stopped): the scene copied there and the music in it */
    tilt_scene_go(0, 1);
    R5(scene_cur() == 0u);
    btn_down(B_OCTUP); frame();
    r5_key(4);
    btn_up(B_OCTUP); frame();
    R5(project_used(4) && scene_cur() == 4u && trk[0].step[0].note[0] == 55u && trk[0].p[P_SLEN] == 32);
    /* clear: OCT- and a scene key: asks first (Cancel the default), nothing gone */
    btn_down(B_OCTDN); frame();
    r5_key(4);
    btn_up(B_OCTDN); frame();
    R5(project_used(4));
    ui.confirm = 0;
    bad += check("round 5 item 1: black keys launch scenes (queued to the loop's end, cancel, last wins, tap playing, FILL, copy, clear asks, the key lights)", ok);
    return bad;
}
static int test_r5_chain(void)
{
    int bad = 0, ok = 1;
    uint32_t n;
    chain_config_t old;
    ui_power_on();
    tu_off = 0;
    r5_scenes();
    tu_goto(TU_JAM);
    frames(32);
    chain_config.count = 1;
    chain_config.row[0] = chain_row_of(3, 1);
    old = chain_config;
    /* hold F#5, tap A A B B (the same scene twice is a repeat), OCT- takes the last tap, then B again */
    r5_down(10);
    R5(jw.held == 1u);
    r5_key(0); r5_key(0); r5_key(1); r5_key(1);
    R5(jw.n == 4u);
    press(B_OCTDN); frames(16);
    R5(jw.n == 3u);
    r5_key(1);
    r5_up(10);
    R5(!jw.held && chain_config.count == 2u && chain_config.row[0].slot[0] == 0u && chain_config.row[0].repeat == 2u &&
       chain_config.row[1].slot[0] == 1u && chain_config.row[1].repeat == 2u);
    /* one undo brings the old chain back */
    press(B_OCTDN); frames(16);
    R5(chain_config.count == 1u && !memcmp(&chain_config.row[0], &old.row[0], sizeof old.row[0]));
    press(B_OCTDN); frames(16);                                               /* (and again: the new one) */
    R5(chain_config.count == 2u);
    /* PLAY plays the chain: the display's boxes follow; a scene key drops to live (that scene queued) */
    press(B_PLAY); frames(16);
    for (n = 0; n < 200u && !chain.running; n++) { events_block(32); tilt_scene_poll(); }
    R5(chain.running && chain.row == 0u);
    r5_key(2);
    R5(!chain.running && tsc.cue == 2u && song.playing);
    transport_req = 2; events_block(32); frames(16);
    /* a blank scene in the chain plays silence (round 4): C..H blank here */
    chain_config.count = 1;
    chain_config.row[0] = chain_row_of(6, 1);
    R5(chain_prepare() == 0 && chain.config.row[0].slot[0] == CHAIN_SILENT);
    chain.armed = 0; transport_req = 0;
    /* let go with no taps: back to the chain if there is one (from live) */
    chain_config = old;
    jm_chain = 0;
    r5_down(10); r5_up(10);
    R5(jm_chain == 1u);
    bad += check("round 5 item 2: CHAIN on F#5 (taps in order, a repeat, OCT- a tap, replace and one undo, PLAY plays it, a scene key drops to live, a blank plays silence)", ok);
    return bad;
}
static int test_r5_log(void)
{
    int bad = 0, ok = 1;
    chain_config_t c;
    ui_power_on();
    jl_n = 0;
    jl_push(0); jl[0].loops = 2;
    jl_push(1); jl[1].loops = 1;
    jl_push(1); jl[2].loops = 1;
    jl_push(2);
    jl_to_chain(&c);
    R5(c.count == 3u && c.row[0].slot[0] == 0u && c.row[0].repeat == 2u && c.row[1].slot[0] == 1u && c.row[1].repeat == 2u &&
       c.row[2].slot[0] == 2u && c.row[2].repeat == 1u);
    for (jl_n = 0; jl_n < JL_N; ) jl_push(jl_n & 7u);
    jl_push(5);
    R5(jl_n == JL_N && jl[JL_N - 1u].sc == 5u);                              /* capped: the oldest dropped */
    jl_n = 0;
    jl_push(3); jl_push(4);
    chain_defaults(&chain_config);
    R5(jl_keep() && chain_config.count == 2u && chain_config.row[0].slot[0] == 3u && chain_config.row[1].slot[0] == 4u);
    bad += check("round 5 item 3: the jam is logged and KEEP AS A SONG turns it into the chain (repeats = loops, capped at 64 entries)", ok);
    return bad;
}
