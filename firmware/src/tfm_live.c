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
static uint8_t jm_chain;                                /* jm_chain: PLAY plays the chain (a scene key makes it live) */
static uint8_t jl_fill_prev, jl_rechain, jw_did;      /* jw_did: F#5 held did an undo (its release then plays nothing) */

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
/* round 6: a scene key launches on its RELEASE (it waits for the loop's end anyway). Round 10: held past SH_HOLD_MS it
 * opens the SCENE SHEET instead (below); the old held combos (another scene key: copy; OCT-: clear) are gone, so two
 * quick taps never copy */
static struct { uint8_t held, used; uint32_t t0; } jh;
static void prf_capture(void);
static void jl_launch(uint32_t s)
{
    if (chain_busy())
        jl_drop_to_live();
    jm_chain = 0;
    tilt_scene_go(s, 1);
}
static void jl_scene_key(uint32_t s)                    /* a scene key went down */
{
    if (jw.held) {                                      /* CHAIN: a tap is a section */
        if (jw.n < sizeof jw.sc)
            jw.sc[jw.n++] = (uint8_t)s;
        ui.force = 1;
        return;
    }
    if (jh.held)                                        /* (another held: nothing; round 6's copy is gone) */
        return;
    jh.held = (uint8_t)(s + 1u);
    jh.used = 0;
    jh.t0 = fm1_ms;
}
/* ================================================================== the SCENE SHEET, round 10 ===================== */
/* A scene key held SH_HOLD_MS: its sheet over JAM (tfm_ui.c tu_sheet_panel: the letter, its bars, its tracks, the four
 * actions). While it is open no key plays (seq.c jam_sheet) and the white keys act:
 *   F3 DUP    the scene copied to the next empty scene (up from it, wrapping; NO EMPTY SCENE) and that one cued: the
 *             next edits make the variation (playing: at the bar's end, as a scene key)
 *   G3 CLEAR  asks (CF_SHEET_CLR, the words of CF_SCENE_CLR), then the scene's steps and automation go
 *   A3 x2     every track twice as long, its steps, notes and automation copied after themselves (NSTEP at most)
 *   B3 /2     every track half as long, the first half kept
 * C4 .. G5 nothing. Let go: the sheet closes, nothing launches. A stored scene changes now: stopped written as SAVE >
 * PROJECT writes it, playing to RAM only (project.c scn_store_ram, flash at the stop; STOP TO SAVE when three wait):
 * nothing erases or writes flash while playing. The scene playing changes at its loop's end (the header: AT LOOP END),
 * stopped at once. Hold SAVE right after: put back (project.c scene_ubuf, the one-level scene undo the SCENE layer has:
 * the stored scene's image, or the music's steps, lengths and automation; DUP: the copy goes again) */
#define SH_HOLD_MS 400u
enum { SH_DUP = 1, SH_CLR, SH_X2, SH_H2 };
static struct {
    uint8_t open, s;                                    /* the sheet of scene s */
    uint8_t pend, ps;                                   /* an action on the scene playing, at its loop's end */
    uint8_t uk, us, ud, ulive;                          /* the undo: its action (0 none), the scene, DUP's copy, the music's */
    uint32_t ugen, pidx;
} sh;
static void tilt_scene_uncue(void);
static int sheet_open(void) { return sh.open; }
static int sheet_held(void) { return jh.held && !jw.held; }
static uint32_t sheet_scene(void) { return sh.s; }
static int sheet_pending(void) { return sh.pend != 0; }
static uint32_t sh_wkey(uint32_t p)                     /* the key of white key place p */
{
    uint32_t k;
    for (k = 0; k < 27u; k++)
        if (!key_black(k) && key_place(k) == p)
            return k;
    return 0;
}
static void sheet_do(uint32_t op);                      /* (project.c: the sheet's actions, with the scenes' storage) */
static void sheet_poll(void);
static void jl_chain_done(void)                         /* F#5 let go */
{
    uint32_t i;
    chain_config_t c;
    jw.held = 0;
    if (jw_did) {                                       /* (round 6: an undo while held: nothing more) */
        jw_did = 0;
        return;
    }
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
/* JAM's keys (tu_screen_input, every frame): `keys` the keys that went down now, `btns` the buttons */
static void jl_input(uint32_t keys, uint32_t btns)
{
    uint32_t i;
    int h5 = jl_held(10), fill = jl_held(9), dn = (int)((btns >> panel.btn[B_OCTDN]) & 1u);
    jam_silent = (uint8_t)jl_on();
    if (!jl_on()) {
        jam_sheet = 0;
        sh.open = 0;
        return;
    }
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
        if (k >= 0 && ((keys >> k) & 1u))
            jl_scene_key(i);
    }
    if (jl_key(8) >= 0 && ((keys >> jl_key(8)) & 1u) && !jw.held)   /* (round 8: C#5 CAPTURE, the length on SETUP) */
        prf_capture();
    if (dn && jw.held) {                                /* F#5 held + OCT-: the last tap goes; no taps: the chain before comes back */
        oct_as_mod(1u);                                 /* (the octave stays) */
        if (jw.n) {
            jw.n--;
        } else if (jw_undo_ok && !chain_busy()) {
            chain_config_t c = chain_config;
            chain_config = jw_undo;
            jw_undo = c;
            jw_did = 1;
            ui_message("CHAIN UNDONE");
        }
        ui.force = 1;
    }
    if (jh.held && !sh.open && !jh.used && !jw.held && jl_held(jh.held - 1u) && fm1_ms - jh.t0 >= SH_HOLD_MS) {
        sh.open = 1;                                    /* (round 10: held: the sheet) */
        sh.s = (uint8_t)(jh.held - 1u);
        jam_sheet = 1;
        ui.force = 1;
    }
    if (sh.open && !ui.confirm) {                       /* the sheet's white keys */
        static const uint8_t OP[4] = {SH_DUP, SH_CLR, SH_X2, SH_H2};
        for (i = 0; i < 4u; i++)
            if ((keys >> sh_wkey(i)) & 1u)
                sheet_do(OP[i]);
    }
    if (jh.held && !jl_held(jh.held - 1u)) {           /* the scene key let go: it launches, unless held to the sheet */
        uint32_t sc = jh.held - 1u;
        jh.held = 0;
        if (sh.open) {
            sh.open = 0;
            jam_sheet = 0;
            ui.force = 1;
        } else if (!jh.used && !jw.held && !ui.confirm) {
            jl_launch(sc);
        }
    }
}
/* every main loop pass (tilt_scene_poll): the log, and a chain asked for from live */
static void jl_poll(void)
{
    const track_t *t = &trk[jl_clock()];
    sheet_poll();                                       /* (round 10: the scene sheet's action at the loop's end) */
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

/* ================================================================== DJ (hold ARP), round 8; round 9 the master ===== */
/* The layer of the ARP button held (LAYER_SCENE's slot; a tap stays the arpeggiator page): four DJ moves on the whole mix
 * (round 9; round 8 had them on the selected track): K1 FILTER (left a low-pass closing, right a high-pass opening, the
 * middle off: perform.c's LPF | HPF), K2 ECHO (every track's delay send towards full and the delay's feedback up to 0.7:
 * ECHO THROW's share), K3 SPACE (the reverb sends: REV THROW's), K4 DIRT (a drive on the master, perform.c pf_drive).
 * They stay where they are left: the song's (perform.c dj_get / dj_put: two inert globals, saved with the song and its
 * scenes; no format change), recorded as motion on the selected track's lane (motion.c MO_DJ) with REC on while playing.
 * OCT- (no key rolling): all four to neutral, one undo (SAVE held). JAM's header says DJ while any is off neutral. The
 * white keys ROLL; with one held OCT- / OCT+ step the roll rate. ALGORITHM or F#3 G#3 A#3 C#4 pick the selected track (the
 * lane the moves record on). CAPTURE is JAM's C#5 (its length on SETUP), MUTATE hold ENV's A3 */
static uint8_t prf_rate, prf_cap, prf_mut;           /* the roll 0..4: 1/8 1/8T 1/16 1/16T 1/32; CAPTURE 0..2: 1 2 4 bars (SETUP); MUTATE 0..2 */
static int prf_on(void) { return ui.layer == LAYER_SCENE && !tu_classic() && tu_on() && !ui.menu; }
static int prf_fill_key(void) { return jl_fill_prev; }  /* (FILL is JAM's D#5) */
static int32_t dj_shown(uint32_t k, uint32_t knob) { (void)k; return dj_now(knob & 3u); }   /* (the screen) FILTER -63..63, the others 0..127 */
static int dj_any(void) { return dj_now(0) || dj_now(1) || dj_now(2) || dj_now(3); }
/* knob i to v: with REC on while playing a motion record on the selected track at the step it plays (as motion_capture), and
 * it sounds from now (until that lane's pass starts again or the transport stops: the song's value stays); else the song's */
static uint32_t mo_live_idx(const track_t *t)          /* the step a move recorded now lands on (as motion_capture) */
{
    uint32_t len = (uint32_t)clamp(t->p[P_SLEN], 1, NSTEP), period = div_samples((uint32_t)t->p[P_SDIV]), idx = t->seq_idx % len;
    if (t->seq_pos < 0x7FFFFFFFu && t->seq_pos > step_samples(t, period, idx) / 2u)
        idx = (idx + 1u) % len;
    return idx;
}
static void dj_set(uint32_t i, int32_t v)
{
    track_t *t = TSEL;
    if (song.playing && rec_on(t)) {
        uint32_t idx = mo_live_idx(t);
        dj_mv[i] = (int8_t)v;
        dj_mown[i] = (uint8_t)trk_index(t);
        dj_mb |= (uint8_t)(1u << i);
        (void)motion_set_event(t, idx, MO_DJ + i, (int16_t)v);
        return;
    }
    dj_put(i, v);
    if ((dj_mb >> i) & 1u)                              /* (a recorded value sounding: the turn heard now) */
        dj_mv[i] = (int8_t)v;
}
static void dj_reset(void)                              /* hold ARP + OCT-: all four neutral, one undo */
{
    uint32_t i;
    if (!dj_any())
        return;
    edit_undo_take(TSEL, 0, UNDO_DJ | UNDO_MOT);        /* (UNDO_MOT: no load joins this copy; the motion swaps as it is) */
    undo.dj[0] = song.g[G_DRLVL];
    undo.dj[1] = song.g[G_DRREV];
    for (i = 0; i < DJ_N; i++)
        dj_put(i, 0);
    dj_mb = 0;
    motion_undo_done(TSEL);
    ui_message("DJ RESET");
}
static uint32_t dj_white(void)                          /* the white keys held now (they ROLL) */
{
    uint32_t k, m = 0;
    for (k = 0; k < 27u; k++)
        if (!key_black(k))
            m |= 1u << k;
    return fm1_in.notes & m;
}
static void prf_oct(uint32_t taps)                      /* OCT- (bit 0) / OCT+ (bit 1) tapped with ARP held */
{
    if (dj_white()) {                                   /* rolling: the rate */
        if (taps & 1u)
            prf_rate = (uint8_t)(prf_rate ? prf_rate - 1u : 0u);
        if (taps & 2u)
            prf_rate = (uint8_t)(prf_rate < 4u ? prf_rate + 1u : 4u);
    } else if (taps & 1u) {                             /* OCT-: the four to neutral */
        dj_reset();
    }
    ui.force = 1;
}
static void prf_pick(uint32_t k)                        /* the track the knobs move */
{
    if (k < NTRK && k != song.sel) {
        track_select(k);
        tq_adapt();
    }
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
static void prf_key(uint32_t k, uint32_t p)            /* a black key under the layer: F#3 G#3 A#3 C#4 pick T1-T4 */
{
    uint32_t i;
    (void)p;
    if (!key_black(k))
        return;
    for (i = 0; i < 4u; i++)
        if (jl_key(i) == (int32_t)k)
            prf_pick(i);
}
static void prf_knob(uint32_t k, int32_t s)            /* K1 FILTER, K2 ECHO, K3 SPACE, K4 DIRT on the master */
{
    int32_t v = knob_sweep(k, s);
    k &= 3u;
    if (k == 0u)                                        /* (FILTER: half the speed, -63..63) */
        v = dj_now(0) + (v > 0 ? (v + 1) / 2 : v / 2 ? v / 2 : -1);
    else
        v = dj_now(k) + v;
    dj_set(k, clamp(v, DJ_DESC[k].min, DJ_DESC[k].max));
    ui.force = 1;
}
static void prf_poll(void)                             /* every pass: the roll while the layer is up */
{
    perf_roll_on = (uint8_t)prf_on();
    perf_roll_rate = prf_rate;
    prf_cap = (uint8_t)(CFG(CF_CAP) % 3u);
}
static uint32_t prf_lit(void)                          /* the key lights: the selected track's key */
{
    return jl_key(song.sel % NTRK) >= 0 ? 1u << jl_key(song.sel % NTRK) : 0u;
}
static uint32_t prf_mid(void)                          /* the other tracks' keys */
{
    uint32_t i, m = 0;
    for (i = 0; i < NTRK; i++)
        if (i != song.sel && jl_key(i) >= 0)
            m |= 1u << jl_key(i);
    return m;
}

/* ================================================================== SELECT, round 11 ============================== */
/* SELECT on JAM, MIX, SOUND and DRUMS turns what SETUP's SELECT row picks (tfm_dice.c SA_*; GLO + SELECT: its second row,
 * ui_input.c). FILTER ECHO SPACE are the DJ's on the master (hold ARP + K1 K2 K3: dj_set, so JAM's DJ marker shows them;
 * not the FX layer's FILTER, perform.c perf_k[0], a momentary macro of its own), MORPH the selected track's macro 1, LEVEL
 * its level, MOD its mod wheel and CC1 out (Felucca's MODW path, ui_input.c sel_play), SCENE the next / previous stored
 * scene cued as its key's tap cues it, SWING the song's, TEMPO the tempo. With REC on while playing the first six record
 * as motion as their knobs do (MOD as MO_MW); SCENE, SWING and TEMPO do not. The header shows the name and value ~1 s */
static void sa_say(uint32_t c, const char *v)
{
    tsl.n = SA_NAME[c % SA_N];
    str_cpy(tsl.v, v, sizeof tsl.v);
    tsl.t = fm1_ms;
    ui.force = 1;
}
static void sa_scene(int32_t d, char *v)
{
    uint32_t cur = scene_cur(), used = tu_used_mask(), base, i, s = 0;
    if (cur < NSCN)
        used |= 1u << cur;
    base = tsc.cue < NSCN ? tsc.cue : cur < NSCN ? cur : (d > 0 ? NSCN - 1u : 0u);
    for (i = 1; i <= NSCN; i++) {
        s = (base + NSCN + (d > 0 ? i : NSCN - i)) % NSCN;
        if ((used >> s) & 1u)
            break;
    }
    if (i > NSCN || s == base) {                        /* (no other scene stored) */
        str_cpy(v, used ? "ONLY ONE" : "NONE", 10);
        return;
    }
    if (s == cur && tsc.cue < NSCN)                     /* (back to the scene playing: the cue goes) */
        tilt_scene_uncue();
    else
        jl_launch(s);
    v[0] = (char)('A' + s);
    v[1] = 0;
}
static void sa_turn(uint32_t glo, int32_t s)
{
    uint32_t c = sa_get(glo);
    track_t *t = TSEL;
    const char *u = "";
    char v[12];
    v[0] = 0;
    switch (c) {
    case SA_FILTER: case SA_ECHO: case SA_SPACE: {
        uint32_t k = c - SA_FILTER;
        int32_t a = accel(EN_SELECT, s, 127);
        if (!k)                                         /* (FILTER: half the speed, -63..63, as hold ARP's K1) */
            a = a > 0 ? (a + 1) / 2 : a / 2 ? a / 2 : -1;
        dj_set(k, clamp(dj_now(k) + a, DJ_DESC[k].min, DJ_DESC[k].max));
        fmt_int(v, dj_now(k));
        if (!k && dj_now(0) > 0) {
            v[0] = '+';
            fmt_int(v + 1, dj_now(0));
        }
        break;
    }
    case SA_MORPH:
        if (tfm_ins_of(t) == TI_NONE) {
            str_cpy(v, "--", 4);
            break;
        }
        tfm_macro_turn_k(t, 0, 0, s);                   /* (macro 1 is MORPH on every instrument) */
        tfm_macro_text(t, 0, v, &u);
        break;
    case SA_LEVEL: {
        const param_desc_t *d = track_desc(t, P_LEVEL);
        t->p[P_LEVEL] = (int16_t)param_turn(d, t->p[P_LEVEL], accel(EN_SELECT, s, d->max - d->min));
        motion_capture(t, P_LEVEL, t->p[P_LEVEL]);
        ml_knob(t, P_LEVEL);
        param_format(d, t->p[P_LEVEL], v, &u);
        break;
    }
    case SA_MOD:
        t->mw = (uint8_t)clamp((int32_t)t->mw + accel(EN_SELECT, s, 127), 0, 127);
        fm1_irq_off();                                  /* (the out ring is the audio ISR's) */
        midi_out_event(0x0Bu | (0xB0u | trk_midi_ch(song.sel)) << 8 | 1u << 16 | (uint32_t)t->mw << 24);
        fm1_irq_on();
        if (song.playing && rec_on(t))
            (void)motion_set_event(t, mo_live_idx(t), MO_MW, (int16_t)t->mw);
        fmt_int(v, t->mw);
        break;
    case SA_SCENE:
        sa_scene(s, v);
        break;
    case SA_SWING:
        song.g[G_SWING] = (int16_t)clamp(song.g[G_SWING] + accel(EN_SELECT, s, 100), GP[G_SWING].min, GP[G_SWING].max);
        fmt_int(v, song.g[G_SWING]);
        str_cpy(v + str_len(v), "%", 2);
        break;
    case SA_TEMPO:
        song.g[G_BPM] = (int16_t)clamp(song.g[G_BPM] + accel(EN_SELECT, s, 200), GP[G_BPM].min, GP[G_BPM].max);
        ui.bpm_t = 40;
        fmt_int(v, song.g[G_BPM]);
        break;
    default:
        return;                                         /* (OFF: nothing) */
    }
    if (u[0] && str_len(v) + str_len(u) < sizeof v - 1u)
        str_cpy(v + str_len(v), u, sizeof v - str_len(v));
    sa_say(c, v);
}
