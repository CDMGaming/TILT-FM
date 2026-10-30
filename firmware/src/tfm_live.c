/* TFM-UI round 5 (tfm-spec/ROUND-5.md, screens/JamFinal*.dc.html): live arranging from JAM. The 11 black keys, left to right
 * F#3 G#3 A#3 | C#4 D#4 | F#4 G#4 A#4 | C#5 D#5 | F#5: the first eight launch the scenes A-H (queued, they start when the
 * scene playing has finished its loop: seq.c scene_tick), C#5 is unused, D#5 held is FILL, F#5 held is CHAIN (the taps of
 * the scene keys are the song's new chain). Every scene that starts is logged with how long it played (the jam is
 * remembered: TIME's KEEP AS A SONG and SONGS turn it into the SONG chain). Included from tilt.c after tfm_pages.c */
static void scene_keep_live(void)                       /* round 6: the scene now stored as it sounds (project.c chain_prepare) */
{
    uint32_t cur = scene_cur();
    if (tu_classic())                                   /* (CLASSIC: Felucca's project slots, as they were) */
        return;
    if (cur < NSCN && (scene_edited() || !project_used(cur)))
        (void)scene_store_ram(cur);
}
static void (*chain_live_fn)(void) = scene_keep_live;
#define JL_N 64u                                        /* the log: the last 64 launches */
static struct { uint8_t sc, loops; } jl[JL_N];
static uint32_t jl_n;
static uint8_t jl_playing, jl_row = 0xFF, jl_prev_idx;
static chain_config_t jw_undo;                          /* the chain before the last replace (one level) */
static uint8_t jw_undo_ok, jm_chain;                    /* jm_chain: PLAY plays the chain (a scene key makes it live) */
static uint8_t jl_fill_prev, jl_rechain;

static int32_t jl_key(uint32_t idx)                     /* the key (0..26) of black key idx 0..10, -1 none */
{
    uint32_t k, n = 0;
    for (k = 0; k < 27u; k++)
        if (key_black(k)) {
            if (n == idx)
                return (int32_t)k;
            n++;
        }
    return -1;
}
static int jl_held(uint32_t idx) { int32_t k = jl_key(idx); return k >= 0 && ((fm1_in.notes >> k) & 1u); }
static int jl_oct_held(uint32_t b) { return (int)((fm1_in.buttons >> panel.btn[b]) & 1u); }
static int jl_on(void) { return !tu_classic() && tu_on() && tu.scr == TU_JAM && !ui.layer && !ui.menu; }

static void jl_push(uint32_t sc)                        /* a scene started: logged (the oldest dropped when full) */
{
    if (sc >= NSCN)
        return;
    if (jl_n == JL_N) {
        uint32_t i;
        for (i = 0; i + 1u < JL_N; i++)
            jl[i] = jl[i + 1u];
        jl_n--;
    }
    jl[jl_n].sc = (uint8_t)sc;
    jl[jl_n].loops = 0;
    jl_n++;
}
/* the log as a chain: consecutive launches of one scene are one section whose repeats are the loops played (at least 1) */
static void jl_to_chain(chain_config_t *c)
{
    uint32_t i;
    chain_defaults(c);
    for (i = 0; i < jl_n; i++) {
        uint32_t rep = jl[i].loops ? jl[i].loops : 1u;
        if (c->count && c->row[c->count - 1u].slot[0] == jl[i].sc && c->row[c->count - 1u].repeat + rep <= 16u) {
            c->row[c->count - 1u].repeat = (uint8_t)(c->row[c->count - 1u].repeat + rep);
            continue;
        }
        if (c->count == CHAIN_ROWS) {
            uint32_t q;
            for (q = 0; q + 1u < CHAIN_ROWS; q++)
                c->row[q] = c->row[q + 1u];
            c->count--;
        }
        c->row[c->count] = chain_row_of(jl[i].sc, rep > 16u ? 16u : rep);
        c->count++;
    }
}
static int jl_keep(void)                                /* the jam as the song's chain (KEEP AS A SONG, SAVE JAM AS SONG) */
{
    if (!jl_n || chain_busy())
        return 0;
    scene_keep_live();
    jw_undo = chain_config;
    jw_undo_ok = 1;
    jl_to_chain(&chain_config);
    return 1;
}
/* the clock track of the music playing: the longest loop (as seq.c scene_tick) */
static uint32_t jl_clock(void)
{
    uint32_t k, c = 0, best = 0;
    for (k = 0; k < NTRK; k++) {
        uint32_t l = len48_of(trk[k].p[P_SLEN], trk[k].p[P_SDIV]);
        if (l > best) {
            best = l;
            c = k;
        }
    }
    return chain.running ? chain.clk % NTRK : c;
}
/* the bars until the queued scene starts (the loop's end), at least 1 */
static uint32_t jl_bars_left(void)
{
    const track_t *t = &trk[jl_clock()];
    uint32_t len = (uint32_t)clamp(t->p[P_SLEN], 1, NSTEP), unit = len48_of((int32_t)len, t->p[P_SDIV]) / len, idx = t->seq_idx % len;
    uint32_t left = (len - idx) * unit, bar = 192u;
    return (left + bar - 1u) / bar ? (left + bar - 1u) / bar : 1u;
}
static void jl_drop_to_live(void)                       /* a scene key while the chain plays: the chain stops, the music goes on */
{
    if (!chain_busy())
        return;
    fm1_irq_off();
    chain_stop();
    fm1_irq_on();
    jm_chain = 0;
}
static void jl_scene_key(uint32_t s)
{
    uint32_t cur = scene_cur();
    if (jw.held) {                                      /* CHAIN: a tap is a section */
        if (jw.n < sizeof jw.sc)
            jw.sc[jw.n++] = (uint8_t)s;
        ui.force = 1;
        return;
    }
    if (jl_oct_held(B_OCTUP)) {                         /* OCT+ and a scene key: the playing scene copied there and queued */
        if (cur >= NSCN || s == cur) {
            ui_message("GO TO ANOTHER SCENE");
            return;
        }
        if (!transport_busy())
            scene_copy_ui(cur, s, 0);
        else if (scene_store_ram(s))
            return;
        if (scene_cur() != s)
            tilt_scene_go(s, 1);
        return;
    }
    if (jl_oct_held(B_OCTDN)) {                         /* OCT- and a scene key: cleared (CLEAR C? CANCEL first) */
        scene_clear_ui(s, 0);
        return;
    }
    if (chain_busy())
        jl_drop_to_live();
    jm_chain = 0;
    tilt_scene_go(s, 1);
}
static void jl_chain_done(void)                         /* F#5 let go */
{
    uint32_t i;
    chain_config_t c;
    jw.held = 0;
    if (!jw.n) {                                        /* no taps: back to the chain, if there is one */
        if (chain_config.count && !chain_busy() && !tu_classic()) {
            jm_chain = 1;
            jl_rechain = 1;
        }
        return;
    }
    chain_defaults(&c);
    for (i = 0; i < jw.n; i++) {
        if (c.count && c.row[c.count - 1u].slot[0] == jw.sc[i] && c.row[c.count - 1u].repeat < 16u) {
            c.row[c.count - 1u].repeat++;
        } else if (c.count < CHAIN_ROWS) {
            c.row[c.count] = chain_row_of(jw.sc[i], 1);
            c.count++;
        }
    }
    if (chain_busy()) {
        ui_message("STOP TO EDIT");
        return;
    }
    jw_undo = chain_config;                             /* one undo brings the old one back */
    jw_undo_ok = 1;
    chain_config = c;
    jm_chain = 1;
    ui_message("CHAIN SET");
    ui.force = 1;
}
/* JAM's keys (tu_screen_input, every frame): `pressed` the keys that went down now */
static void jl_input(uint32_t pressed, uint32_t oct)
{
    uint32_t i;
    int h5 = jl_held(10), fill = jl_held(9);
    jam_silent = (uint8_t)jl_on();
    if (!jl_on())
        return;
    if (h5 && !jw.held) {
        jw.held = 1;
        jw.n = 0;
        ui.force = 1;
    } else if (!h5 && jw.held) {
        jl_chain_done();
        ui.force = 1;
    }
    if ((uint8_t)fill != jl_fill_prev) {                /* D#5 held: FILL (the steps' FILL / !FILL conditions) */
        jl_fill_prev = (uint8_t)fill;
        seq_fill = (uint8_t)fill;
        ui.force = 1;
    }
    for (i = 0; i < NSCN; i++) {
        int32_t k = jl_key(i);
        if (k >= 0 && ((pressed >> k) & 1u))
            jl_scene_key(i);
    }
    if (oct & 1u) {                                     /* OCT-: with F#5 the last tap goes; alone the chain before comes back */
        if (jw.held) {
            if (jw.n)
                jw.n--;
            ui.force = 1;
        } else if (jw_undo_ok && !jl_oct_held(B_OCTUP)) {
            chain_config_t c = chain_config;
            if (!chain_busy()) {
                chain_config = jw_undo;
                jw_undo = c;
                ui_message("CHAIN UNDONE");
                ui.force = 1;
            }
        }
    }
}
/* every main loop pass (tilt_scene_poll): the log, and a chain asked for from live */
static void jl_poll(void)
{
    const track_t *t = &trk[jl_clock()];
    {   /* the loop a cued scene waits for: the longest track's (seq.c scene_tick) */
        uint32_t k, c = 0, best = 0;
        for (k = 0; k < NTRK; k++) {
            uint32_t l = len48_of(trk[k].p[P_SLEN], trk[k].p[P_SDIV]);
            if (l > best) {
                best = l;
                c = k;
            }
        }
        scene_clk = (uint8_t)c;
    }
    uint32_t idx = t->seq_idx;
    if (song.playing && !jl_playing)
        jl_push(chain.running ? chain.config.row[chain.row].slot[0] : scene_cur());
    jl_playing = (uint8_t)song.playing;
    if (song.playing) {
        if (chain.running && chain.row != jl_row) {
            if (jl_row != 0xFFu || !jl_n)
                jl_push(chain.config.row[chain.row].slot[0]);
            jl_row = chain.row;
        } else if (!chain.running) {
            jl_row = 0xFF;
        }
        if (idx < jl_prev_idx && jl_n && jl[jl_n - 1u].loops < 255u)
            jl[jl_n - 1u].loops++;                      /* the clock track wrapped: a loop played */
        jl_prev_idx = (uint8_t)idx;
    } else {
        jl_row = 0xFF;
        jl_prev_idx = 0;
    }
    if (jl_rechain && !chain_busy()) {
        if (song.playing || transport_req == 1u)
            transport_req = 2;                          /* (stop the live music first) */
        else if (!transport_busy()) {
            jl_rechain = 0;
            chain_play_ui();
        }
    }
}
static int jl_play_chain(void) { return jl_on() && jm_chain && chain_config.count && !chain_busy(); }
/* the key LEDs on JAM: the scene playing solid, the queued one breathing, a used one at the mid level, D#5 and F#5 dim */
static uint32_t jl_leds(uint32_t *br, uint32_t *mid)
{
    uint32_t s, lit = 0, b = 0, m = 0, cur = scene_cur();
    if (chain.running && chain.config.row[chain.row].slot[0] < NSCN)
        cur = chain.config.row[chain.row].slot[0];
    for (s = 0; s < NSCN; s++) {
        int32_t k = jl_key(s);
        if (k < 0)
            continue;
        if (s == cur)
            lit |= 1u << k;
        else if (s == tsc.cue)
            b |= 1u << k;
        else if (tu_scene_used(s))
            m |= 1u << k;
    }
    if (jl_key(9) >= 0) m |= 1u << jl_key(9);
    if (jl_key(10) >= 0) m |= 1u << jl_key(10);
    if (br) *br = b;
    if (mid) *mid = m;
    return lit;
}

/* ================================================================== PERFORM (hold ARP) ============================ */
/* The layer of the ARP button held (LAYER_SCENE's slot; tap stays the arpeggiator page): the white keys ROLL (the held note
 * repeats at K1's rate: seq.c roll_tick; REC on, recorded as ratchets), F#3 G#3 A#3 C#4 mute T1-T4, D#4 held is FILL, F#4
 * CAPTURE (the last 1, 2 or 4 bars played on the selected track, REC off too, written into its pattern, K2 the length), G#4
 * MUTATE (the selected track's pattern and / or sound re-rolled in key, K3 the mode) */
static uint8_t prf_rate, prf_cap, prf_mut;           /* K1 0..4: 1/8 1/8T 1/16 1/16T 1/32; K2 0..2: 1 2 4 bars; K3 0..2: PATTERN SOUND BOTH */
static int prf_on(void) { return ui.layer == LAYER_SCENE && !tu_classic() && tu_on() && !ui.menu; }
static int prf_fill_key(void) { return (prf_on() && jl_held(4)) || jl_fill_prev; }
static void prf_mute(uint32_t i)
{
    track_t *t = &trk[i % NTRK];
    t->p[P_MUTE] = t->p[P_MUTE] ? 0 : 1;
    motion_capture(t, P_MUTE, t->p[P_MUTE]);
    ui.force = 1;
}
static void prf_capture(void)
{
    track_t *t = TSEL;
    uint32_t k = song.sel, win = (1u << prf_cap) * div_samples(2) * 16u, sp = div_samples((uint32_t)t->p[P_SDIV]), len = tq_len(t);
    uint32_t i, j, base = 0xFFFFFFFFu, cur = song.playing ? t->seq_idx % len : 0u, nclr, n = 0;
    if (chain_busy()) {
        ui_message("STOP TO EDIT");
        return;
    }
    for (i = 0; i < CAP_N; i++) {
        const cap_ev_t *e = &cap_ev[k][i];
        if (e->vel && cap_clock - e->t_on <= win && e->t_on < base) {
            base = e->t_on;
            n++;
        }
    }
    if (!n) {
        ui_message("NOTHING PLAYED");
        return;
    }
    tl_undo_take(t);
    nclr = win / sp < len ? win / sp : len;
    for (j = 0; j < nclr; j++) {                        /* the bars it replaces */
        uint32_t at = song.playing ? (cur + len * 4u - j % len) % len : j;
        step_clear(&t->step[at]);
        (void)motion_clear_locks(t, at);
    }
    for (i = 0; i < CAP_N; i++) {
        const cap_ev_t *e = &cap_ev[k][(uint32_t)(cap_w[k] + i) % CAP_N];
        uint32_t pos, ln, q;
        step_t *st;
        if (!e->vel || cap_clock - e->t_on > win)
            continue;
        if (song.playing) {
            uint32_t back = (cap_clock - e->t_on + sp / 2u) / sp;
            pos = (cur + len * 8u - back % len) % len;
        } else {
            pos = ((e->t_on - base + sp / 2u) / sp) % len;
        }
        ln = (e->len == CAP_OPEN ? cap_clock - e->t_on : (uint32_t)e->len << 6) + sp / 2u;
        ln = ln / sp ? ln / sp : 1u;
        st = &t->step[pos];
        if (st->time != ST_NOTE) {
            step_clear(st);
            st->time = ST_NOTE;
            st->vel = e->vel;
        }
        if (drum_track(t)) {
            st->hit |= (uint8_t)(1u << drum_lane(e->note));
        } else if (st->n < 4u) {
            uint32_t d;
            for (d = 0; d < st->n && st->note[d] != e->note; d++)
                ;
            if (d == st->n)
                st->note[st->n++] = e->note;
        }
        for (q = 1; q < ln && q < len && !drum_track(t); q++) {
            step_t *tie = &t->step[(pos + q) % len];
            if (tie->time == ST_NOTE && tie->n)
                break;
            step_clear(tie);
            tie->time = ST_TIE;
        }
    }
    ui_message(prf_cap == 0u ? "CAPTURED 1 BAR" : prf_cap == 1u ? "CAPTURED 2 BARS" : "CAPTURED 4 BARS");
    ui.force = 1;
}
static void prf_mutate(void)
{
    track_t *t = TSEL;
    uint32_t i = tfm_ins_of(t);
    if (chain_busy()) {
        ui_message("STOP TO EDIT");
        return;
    }
    if (prf_mut != 1u)
        tools_key(TL_RANDOM);                           /* the pattern re-rolled in the track's key (one undo: SAVE held) */
    if (prf_mut != 0u && i != TI_NONE)
        tfm_load(t, i, (fm1_ms / 7u + cap_clock / 97u) % TFM_INS[i].nsnd);
    ui_message(prf_mut == 0u ? "MUTATED: PATTERN" : prf_mut == 1u ? "MUTATED: SOUND" : "MUTATED: BOTH");
    ui.force = 1;
}
static void prf_key(uint32_t k, uint32_t p)            /* a black key under the layer */
{
    uint32_t i;
    (void)p;
    if (!key_black(k))
        return;
    for (i = 0; i < 11u; i++)
        if (jl_key(i) == (int32_t)k)
            break;
    if (i < 4u)
        prf_mute(i);
    else if (i == 5u)
        prf_capture();
    else if (i == 6u)
        prf_mutate();
}
static void prf_knob(uint32_t k, int32_t s)
{
    int32_t d = s > 0 ? 1 : -1;
    if (k == 0u)
        prf_rate = (uint8_t)clamp((int32_t)prf_rate + d, 0, 4);
    else if (k == 1u)
        prf_cap = (uint8_t)clamp((int32_t)prf_cap + d, 0, 2);
    else if (k == 2u)
        prf_mut = (uint8_t)clamp((int32_t)prf_mut + d, 0, 2);
    ui.force = 1;
}
static void prf_poll(void)                             /* every pass: the roll on while the layer is up */
{
    perf_roll_on = (uint8_t)prf_on();
    perf_roll_rate = prf_rate;
}
static uint32_t prf_lit(void)                          /* the key lights: the tracks playing, FILL held */
{
    uint32_t i, m = 0;
    for (i = 0; i < 4u; i++)
        if (jl_key(i) >= 0 && !trk[i].p[P_MUTE])
            m |= 1u << jl_key(i);
    if (jl_held(4))
        m |= 1u << jl_key(4);
    return m;
}
static uint32_t prf_mid(void) { return (1u << jl_key(5)) | (1u << jl_key(6)); }
