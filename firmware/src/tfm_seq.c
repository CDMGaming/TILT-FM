/* TFM-UI phase 5 (tfm-spec/TFM-UI-SPEC.md 3 STEP, NOTES, DRUMS): the sequencer's three screens. They are views of
 * Felucca's pattern (step_t, the cursor ui.cursor, the lane ui.lane, the track's LEN) and keep its entry: the keys write
 * at the cursor (seq_entry), a drum track's white keys toggle steps and its black keys pick the lane (grid_keys).
 * SEQ: STEP, again NOTES (a drum track: DRUMS), again STEP. G5, the last key, is REST in STEP and NOTES (silent).
 * Hold OCT+ on STEP: the step sheet (tfm_sheet.c); hold SEQ: STEP TOOLS (Felucca's layer). Included from tilt.c. */
#define TQ_REST_KEY 26u                                 /* G5: the 27th key */
static uint8_t tq_bar;                                  /* NOTES, DRUMS: the bar shown (16 steps) */
static uint8_t tq_grab;                                 /* NOTES: the note at the cursor is GRABbed (OCT+ tap): K1 moves it in time, K2 its pitch */
static uint8_t tq_pan;                                  /* NOTES: SELECT scrolled the roll (the window stays while the cursor does) */
static uint32_t tq_pan_cur, tq_oct_t0;
static int8_t tq_base;                                  /* NOTES: the top row's note */
static uint8_t tq_zoom;                                 /* NOTES: 0 one bar, 1 two bars, 2 the whole pattern (PRESETS) */
static uint8_t tq_extra;                                /* REC on, stopped, a key held: the steps K1 added to the note being entered */

static int tq_has_note(const track_t *t, const step_t *s) { return drum_track(t) ? s->hit != 0u : s->time == ST_NOTE && s->n; }
static uint32_t tq_len(const track_t *t) { return (uint32_t)clamp(t->p[P_SLEN], 1, NSTEP); }
static void tq_len_text(char *b, uint32_t len)          /* "2 BARS", "2 BARS + 3", "7 STEPS" */
{
    uint32_t bar = tfm_bar(), n = len / bar;
    if (!n) {
        fmt_int(b, (int32_t)len);
        str_cpy(b + str_len(b), len == 1u ? " STEP" : " STEPS", 8);
        return;
    }
    fmt_int(b, (int32_t)n);
    str_cpy(b + str_len(b), n == 1u ? " BAR" : " BARS", 6);
    if (len % bar) {
        str_cpy(b + str_len(b), " + ", 4);
        fmt_int(b + str_len(b), (int32_t)(len % bar));
    }
}
/* round 3: REC decides what the keys do on STEP, NOTES, DRUMS: off = they only play; on and stopped = they write at the
 * cursor; on and playing = they record live. The header says which, in words (red when REC is on) */
static int tq_rec_on(void) { return (int)((song.rec >> song.sel) & 1u); }
static const char *tq_mode(uint16_t *c)
{
    if (!tq_rec_on()) {
        *c = TC_QUIET;
        return "PLAY ONLY";
    }
    *c = TC_REC;
    return song.playing || transport_req == 1u || seq_counting() ? "REC LIVE" : "WRITE STEPS";
}
static uint32_t tq_mode_sig(void) { return (uint32_t)tq_rec_on() * 3u + (song.playing ? 5u : 0u); }
static void tq_head(const char *what)
{
    char l[24];
    uint16_t c;
    const char *m = tq_mode(&c);
    str_cpy(l, what, 12);
    str_cpy(l + str_len(l), " \xB7 ", 5);
    str_cpy(l + str_len(l), tu_inst_name(TSEL), 12);
    tu_head(song.sel, l, 0, "", 0);
    tu_text_r(231, 10, &AF_TM, m, c, TC_BG);
}
/* ================================================================== STEP ==================================== */
static uint32_t tq_pitch_lo, tq_pitch_hi;
static void tq_range(const track_t *t)                  /* the pattern's lowest and highest note (for the cells' ticks) */
{
    uint32_t i, lo = 127, hi = 0;
    for (i = 0; i < NSTEP; i++)
        if (t->step[i].time == ST_NOTE && t->step[i].n) {
            if (t->step[i].note[0] < lo) lo = t->step[i].note[0];
            if (t->step[i].note[0] > hi) hi = t->step[i].note[0];
        }
    tq_pitch_lo = lo <= hi ? lo : 48u;
    tq_pitch_hi = lo <= hi ? hi : 72u;
}
/* ================================================================== BARS ==================================== */
/* round 4 item 6 (tfm-spec/screens/Bars.dc.html): one row per track, each bar a block with a picture of its notes; bars past a
 * track's length are dashed repeats up to the scene's length. K1 TRACK, K2 LENGTH in bars (1, 2, 4; 8 when the bar is 8 steps:
 * the pattern holds 64 steps), K3 BAR, K4 SHIFT (the whole track a step, wrapping). OCT+ tap DOUBLE, EDIT clears the bar, hold OCT+
 * COPY (K3 the target, OCT+ pastes, OCT- cancels). Every change is one undo (SAVE held) */
static uint8_t tb_bar;                                  /* the selected bar of the selected track */
static uint8_t tb_copy;                                 /* COPY: 1 + the bar being copied (0: not copying) */
static uint8_t tb_tgt;                                  /* COPY: the bar it goes to */
static uint8_t tb_used;                                 /* the OCT+ hold became COPY: its release is no tap */
static int16_t tb_shift;                                /* the net shift of the selected track (K4) */
static uint32_t tb_t0;
static uint32_t tb_nbars(const track_t *t) { uint32_t b = tfm_bar(); return (tq_len(t) + b - 1u) / b; }
static uint32_t tb_scene_bars(void)
{
    uint32_t k, n = 1;
    for (k = 0; k < NTRK; k++)
        if (tb_nbars(&trk[k]) > n)
            n = tb_nbars(&trk[k]);
    return n;
}
static uint32_t tb_max_bars(void)                       /* the longest a track can be: 8 bars, as many as 64 steps hold */
{
    uint32_t n = NSTEP / tfm_bar();
    return n > 8u ? 8u : n;
}
static void tb_len_set(track_t *t, uint32_t nb)         /* nb bars: growing repeats the bars there are, shrinking keeps the first nb */
{
    uint32_t b = tfm_bar(), old = tq_len(t), nl = nb * b, i;
    if (nl > old)
        for (i = old; i < nl && i < NSTEP; i++)
            t->step[i] = t->step[i % old];
    t->p[P_SLEN] = (int16_t)(nl > NSTEP ? NSTEP : nl);
}
static void tb_clear(track_t *t, uint32_t bar)
{
    uint32_t b = tfm_bar(), i, len = tq_len(t);
    for (i = bar * b; i < (bar + 1u) * b && i < len; i++) {
        step_clear(&t->step[i]);
        (void)motion_clear_locks(t, i);
    }
}
static void tb_paste(track_t *t, uint32_t src, uint32_t dst)
{
    uint32_t b = tfm_bar(), i, len = tq_len(t);
    if (src == dst)
        return;
    for (i = 0; i < b; i++)
        if (dst * b + i < len && src * b + i < len)
            t->step[dst * b + i] = t->step[src * b + i];
}
static void tb_rotate(track_t *t, int32_t d)            /* the whole track a step left (d < 0) or right, wrapping */
{
    uint32_t len = tq_len(t), i;
    step_t first, last;
    if (len < 2u)
        return;
    first = t->step[0];
    last = t->step[len - 1u];
    if (d < 0) {
        for (i = 0; i + 1u < len; i++)
            t->step[i] = t->step[i + 1u];
        t->step[len - 1u] = first;
    } else {
        for (i = len - 1u; i > 0; i--)
            t->step[i] = t->step[i - 1u];
        t->step[0] = last;
    }
}
static void tb_dot(int32_t x, int32_t y, int32_t w, uint16_t c) { tu_rect(x, y, w, 2, c); }
static void tb_block(const track_t *t, uint32_t k, uint32_t bar, int32_t x, int32_t y, int sel)
{
    uint32_t b = tfm_bar(), nb = tb_nbars(t), src = bar % nb, i, len = tq_len(t), lo = 127, hi = 0;
    int past = bar >= nb;
    uint16_t col = TU_TC[k], dim = tu_mix(TC_BG, col, past ? 55 : 100), edge = past ? tu_mix(TC_BG, col, 55) : RGB(42, 32, 18);
    if (past) {
        int32_t q;
        for (q = 0; q < 41; q += 4) {                   /* the dashed outline of a repeat */
            tu_rect(x + q, y, 2, 1, edge);
            tu_rect(x + q, y + 25, 2, 1, edge);
        }
        for (q = 0; q < 26; q += 4) {
            tu_rect(x, y + q, 1, 2, edge);
            tu_rect(x + 40, y + q, 1, 2, edge);
        }
    } else {
        tu_cut(x, y, 41, 26, k == song.sel ? RGB(40, 30, 18) : RGB(26, 20, 12));
    }
    if (sel) {                                          /* the selected bar: outlined white */
        tu_rect(x - 1, y - 1, 43, 1, TC_TEXT); tu_rect(x - 1, y + 26, 43, 1, TC_TEXT);
        tu_rect(x - 1, y - 1, 1, 28, TC_TEXT); tu_rect(x + 41, y - 1, 1, 28, TC_TEXT);
    }
    for (i = 0; i < len; i++)
        if (!drum_track(t) && t->step[i].time == ST_NOTE && t->step[i].n) {
            if (t->step[i].note[0] < lo) lo = t->step[i].note[0];
            if (t->step[i].note[0] > hi) hi = t->step[i].note[0];
        }
    for (i = 0; i < b && src * b + i < len; i++) {      /* the picture: a dot a note (a bar for a long one), a drum lane a row */
        const step_t *s = &t->step[src * b + i];
        int32_t dx = x + 3 + (int32_t)(i * 35u / b);
        if (drum_track(t)) {
            uint32_t l;
            for (l = 0; l < 8u; l++)
                if ((s->hit >> l) & 1u)
                    tu_rect(dx, y + 3 + (int32_t)l * 2, 2, 1, dim);
        } else if (s->time == ST_NOTE && s->n) {
            uint32_t l = 1;
            while (src * b + i + l < len && (src * b + i + l) < (src + 1u) * b && t->step[src * b + i + l].time == ST_TIE)
                l++;
            tb_dot(dx, y + 3 + (int32_t)((hi - s->note[0]) * 17u / (hi - lo + 1u)), l > 1u ? (int32_t)(l * 35u / b) : 2, dim);
        }
    }
    if (past) {                                         /* the arrow of a repeat */
        int32_t cx = x + 20, cy = y + 13;
        tu_ring(cx, cy, 5, 4, tu_mix(TC_BG, col, 80));
        tu_rect(cx + 1, cy - 6, 4, 3, RGB(18, 14, 8));
        tu_rect(cx + 3, cy - 6, 1, 3, tu_mix(TC_BG, col, 80));
        tu_rect(cx + 2, cy - 5, 3, 1, tu_mix(TC_BG, col, 80));
    }
}
static void tb_body(void)                               /* y 22 .. 215 */
{
    uint32_t k, bar, nb, sb = tb_scene_bars();
    char b[28], n[12];
    tu_rect(0, 22, TU_W, 193, TC_BG);
    for (bar = 0; bar < sb && bar < 4u; bar++) {
        char bn[8] = "BAR ";
        fmt_int(bn + 4, (int32_t)bar + 1);
        tu_text_c(85 + 44 * (int32_t)bar, 30, &AF_TS, bn, tb_copy && bar == tb_tgt ? TC_AMBER : TC_QUIET, TC_BG);
    }
    for (k = 0; k < NTRK; k++) {
        const track_t *t = &trk[k];
        int32_t y = 37 + (int32_t)k * 33, w = 0;
        nb = tb_nbars(t);
        if (k == song.sel)
            tu_rect(0, y - 3, TU_W, 32, RGB(30, 22, 12));
        tu_disc(14, y + 7, 6, TU_TC[k]);
        b[0] = (char)('1' + k); b[1] = 0;
        tu_text_c(14, y + 7, &AF_TS, b, TC_BG, TU_TC[k]);
        {
            const char *nm = tfm_ins_of(t) != TI_NONE ? tfm_ins_name(t) : tu_inst_name(t);
            if (text_w(&AF_TS, nm) > 38 && tfm_ins_of(t) != TI_NONE)
                nm = tfm_ins_tag(t);
            tu_text_fit(24, y + 5, &AF_TS, nm, TC_TEXT, k == song.sel ? RGB(30, 22, 12) : TC_BG, 38);
        }
        fmt_int(n, (int32_t)nb);
        str_cpy(n + str_len(n), nb == 1u ? " BAR" : " BARS", 6);
        if (nb < sb) {
            str_cpy(n + str_len(n), " \xD7", 3);
            fmt_int(n + str_len(n), (int32_t)((sb + nb - 1u) / nb));
        }
        tu_text(24, y + 18, &AF_TS, n, TU_TC[k], k == song.sel ? RGB(30, 22, 12) : TC_BG);
        (void)w;
        for (bar = 0; bar < sb && bar < 4u; bar++)
            tb_block(t, k, bar, 64 + 44 * (int32_t)bar, y, k == song.sel && bar == (tb_copy ? tb_tgt : tb_bar));
    }
    nb = tb_nbars(TSEL);
    b[0] = 'T'; b[1] = (char)('1' + song.sel); b[2] = 0;
    str_cpy(b + 2, " ", 2);
    str_cpy(b + str_len(b), tfm_ins_of(TSEL) != TI_NONE ? tfm_ins_name(TSEL) : tu_inst_name(TSEL), 12);
    {
        int32_t x = tu_text(9, 177, &AF_TM, b, TC_TEXT, TC_BG);
        char c[40];
        str_cpy(c, "\xB7 bar ", 8);
        fmt_int(c + 6, (int32_t)tb_bar + 1);
        str_cpy(c + str_len(c), " of ", 5);
        fmt_int(c + str_len(c), (int32_t)nb);
        str_cpy(c + str_len(c), " \xB7 scene is ", 14);
        fmt_int(c + str_len(c), (int32_t)sb);
        str_cpy(c + str_len(c), sb == 1u ? " bar" : " bars", 6);
        tu_text(x + 5, 177, &AF_TS, c, TC_QUIET, TC_BG);
    }
    for (k = 0; k < 4u; k++) {
        static const char *const LB[4] = {"K1 TRACK", "K2 LENGTH", "K3 BAR", "K4 SHIFT"};
        char v[12];
        int32_t x = 7 + (int32_t)k * 58;
        if (k == 0u) { v[0] = 'T'; v[1] = (char)('1' + song.sel); v[2] = 0; }
        else if (k == 1u) { fmt_int(v, (int32_t)nb); str_cpy(v + str_len(v), nb == 1u ? " BAR" : " BARS", 6); }
        else if (k == 2u) fmt_int(v, (int32_t)(tb_copy ? tb_tgt : tb_bar) + 1);
        else fmt_int(v, tb_shift);
        tu_rrect(x, 188, 55, 26, 5, TC_DEEP, TC_BG);
        tu_text_c(x + 27, 195, &AF_TS, LB[k], TU_TC[k], TC_DEEP);
        tu_text_c(x + 27, 207, &AF_TL, v, TC_TEXT, TC_DEEP);
    }
}
static void tb_clamp(void);
static void tb_draw(void)
{
    uint32_t k, sig = song.sel * 31u + tb_bar * 7u + tb_copy * 977u + tb_tgt * 131u + (uint32_t)(tb_shift + 128) * 4001u + tu_rec_sig() * 6007u;
    char h[24] = "BARS \xB7 SCENE A";
    uint32_t cur = tu_scene_cur();
    tb_clamp();
    h[13] = cur < TU_NSCENE ? (char)('A' + cur) : '-';
    for (k = 0; k < NTRK; k++)
        sig = sig * 31u + steps_hash(&trk[k]) + (uint32_t)clamp(trk[k].p[P_SLEN], 1, NSTEP);
    tu_head(-1, h, 0, "SEQ NOTES", 0);
    tu_zone(2, 22, 100, TC_BG, sig, tb_body);
    tu_zone(3, 122, 93, TC_BG, sig, tb_body);
    if (tb_copy)
        tu_foot(3, "K3", "TARGET", "OCT+", "PASTE", "OCT-", "CANCEL", 0, 0, 0, 0);
    else
        tu_foot(3, "OCT+", "DOUBLE", "EDIT", "CLEAR BAR", "HOLD OCT+", "COPY", 0, 0, 0, 0);
    ui.force = 0;
}
static void tb_clamp(void)
{
    uint32_t nb = tb_nbars(TSEL);
    if (tb_bar >= nb)
        tb_bar = (uint8_t)(nb - 1u);
    if (tb_tgt >= nb)
        tb_tgt = (uint8_t)(nb - 1u);
}
static int tb_busy(void)
{
    if (chain_busy()) {
        ui_message("STOP TO EDIT");
        return 1;
    }
    return 0;
}
static void tb_input(uint32_t pressed, uint32_t oct, uint32_t busy)
{
    track_t *t = TSEL;
    int32_t s;
    uint32_t nb = tb_nbars(t), held = (fm1_in.buttons >> panel.btn[B_OCTUP]) & 1u;
    if (busy)
        return;
    if ((pressed >> panel.btn[B_OCTUP]) & 1u) {
        tb_t0 = fm1_ms;
        tb_used = 0;
    }
    if (held && !tb_copy && !tb_used && fm1_ms - tb_t0 > 450u) {        /* hold OCT+: COPY this bar; K3 picks where it goes */
        tb_copy = (uint8_t)(tb_bar + 1u);
        tb_tgt = (uint8_t)(tb_bar + 1u < nb ? tb_bar + 1u : tb_bar);
        tb_used = 1;
        ui.force = 1;
    }
    if ((s = panel_enc(EN_K1)) != 0) {                  /* TRACK */
        track_select((uint32_t)clamp((int32_t)song.sel + (s > 0 ? 1 : -1), 0, NTRK - 1));
        tb_shift = 0;
        tb_copy = 0;
        tb_clamp();
        ui.force = 1;
    }
    if ((s = panel_enc(EN_K2)) != 0 && !tb_busy()) {    /* LENGTH in bars: 1, 2, 4 (8) */
        uint32_t n = nb, mx = tb_max_bars();
        if (s > 0) {
            n = nb < 2u ? 2u : nb < 4u ? 4u : 8u;
            if (n > mx) n = nb;
        } else {
            n = nb > 4u ? 4u : nb > 2u ? 2u : 1u;
        }
        if (n != nb) {
            tl_undo_take(t);
            tb_len_set(t, n);
            tb_tgt = (uint8_t)(tb_tgt < n ? tb_tgt : n - 1u);
            tb_clamp();
            ui.force = 1;
        } else {
            ui_message(s > 0 ? "AS LONG AS IT GOES" : "ONE BAR");
        }
    }
    if ((s = panel_enc(EN_K3)) != 0) {                  /* BAR (COPY: the target) */
        uint32_t *p = tb_copy ? (uint32_t *)0 : 0;
        int32_t v = tb_copy ? (int32_t)tb_tgt : (int32_t)tb_bar;
        (void)p;
        v = clamp(v + (s > 0 ? 1 : -1), 0, (int32_t)nb - 1);
        if (tb_copy)
            tb_tgt = (uint8_t)v;
        else
            tb_bar = (uint8_t)v;
        ui.force = 1;
    }
    if ((s = panel_enc(EN_K4)) != 0 && !tb_busy()) {    /* SHIFT: the whole track a step */
        tl_undo_take(t);
        tb_rotate(t, s > 0 ? 1 : -1);
        tb_shift = (int16_t)clamp(tb_shift + (s > 0 ? 1 : -1), -99, 99);
        ui.force = 1;
    }
    if (oct & 1u && tb_copy) {                          /* OCT-: cancel */
        tb_copy = 0;
        ui_message("COPY CANCELLED");
        ui.force = 1;
    }
    if (oct & 2u) {
        if (tb_used) {
            tb_used = 0;                                /* (the release of the hold that began COPY) */
        } else if (tb_copy) {                           /* paste */
            if (!tb_busy()) {
                tl_undo_take(t);
                tb_paste(t, (uint32_t)tb_copy - 1u, tb_tgt);
                tb_bar = tb_tgt;
                ui_message("PASTED");
            }
            tb_copy = 0;
            ui.force = 1;
        } else if (fm1_ms - tb_t0 <= 450u && !tb_busy()) {        /* tap: DOUBLE */
            if (nb * 2u > tb_max_bars()) {
                ui_message("AS LONG AS IT GOES");
            } else {
                tl_undo_take(t);
                tb_len_set(t, nb * 2u);
                ui_message("DOUBLED");
            }
            ui.force = 1;
        }
    }
}
static void tb_edit(void)                               /* EDIT: clear the selected bar */
{
    if (tb_busy())
        return;
    tl_undo_take(TSEL);
    tb_clear(TSEL, tb_bar);
    ui_message("BAR CLEARED");
    ui.force = 1;
}

/* the keys of the screens: a drum track's grid, else the cursor step (STEP's entry), G5 a REST */
static void tq_rest(void)
{
    track_t *t = TSEL;
    if (chain_busy()) {
        ui_message("STOP TO EDIT");
        return;
    }
    step_clear(&t->step[ui.cursor % NSTEP]);
    (void)motion_clear_locks(t, ui.cursor % NSTEP);
    cursor_set(ui.cursor + 1u);
    ui_message("REST");
}
static void tq_keys(uint32_t notes)
{
    if (notes & (1u << TQ_REST_KEY) && !drum_track(TSEL)) {
        tq_rest();
        notes &= ~(1u << TQ_REST_KEY);
    }
    if (fm1_in.notes & (1u << TQ_REST_KEY))             /* (G5 is the screen's: silent while held; never set otherwise: a layer button would see a combo) */
        kb_layer |= 1u << TQ_REST_KEY;
    song.grid = (uint8_t)keys_mode();
    if (!tq_rec_on() && song.grid != 3u) {              /* REC off: the keys only play (stopped or playing), never write */
        song.grid = 0;
        ui.entry_open = 0;
        return;
    }
    lock_keys();
    if (song.grid == 3u)
        song_keys(notes);
    else if (song.grid)
        grid_keys(notes);
    else if (song.seq_mode && cur_page()->graph == GR_ROLL) {
        if (live_rec_sel() || song.playing)             /* (REC on, playing: the keys record live; */
            ui.entry_open = 0;                          /* REC on, stopped: they write at the cursor) */
        else if (!chain_busy()) {
            uint32_t c0 = ui.cursor;
            seq_entry(notes);
            if (tq_extra && ui.cursor != c0 && tu.scr == TU_NOTES) {   /* (a key let go: the cursor moves past the length K1 gave the note) */
                cursor_set(ui.cursor + tq_extra);
                tq_extra = 0;
            }
            if (!ui.entry_open)
                tq_extra = 0;
        } else if (notes)
            ui_message("STOP TO EDIT");
    }
}
/* ================================================================== NOTES =================================== */
/* round 4 item 4 (tfm-spec/screens/NotesV2.dc.html): a strip of every bar of the pattern, the song's scale notes only (folded) in
 * about two octaves of rows, a violet frame round the cursor's column, the note at the cursor outlined with its name over it, the
 * playhead, a held key marked at the left edge. PRESETS = ZOOM (1 bar, 2 bars, the whole pattern); the rows centre on the bar
 * shown whenever it changes, and keep the cursor's note in view */
#define TQ_ROWS 15
#define TQ_GX 22                                        /* the grid's left, and its width */
#define TQ_GW 208
#define TQ_GY 49                                        /* its top; a row is 8 px (7 and a gap) */
static int32_t tq_next_in_scale(const track_t *t, int32_t n, int32_t d) { return tl_neigh(t, n, d); }
static int32_t tq_floor(const track_t *t, int32_t n) { return tl_in_scale(t, n) ? n : tq_next_in_scale(t, n, -1); }   /* the row a note sits on */
static int32_t tq_up(const track_t *t, int32_t n, int32_t rows)          /* rows up (down: negative) the scale from n */
{
    for (; rows > 0; rows--)
        n = tq_next_in_scale(t, n, 1);
    for (; rows < 0; rows++)
        n = tq_next_in_scale(t, n, -1);
    return n;
}
static int32_t tq_dist(const track_t *t, int32_t lo, int32_t hi)         /* rows from lo up to hi */
{
    int32_t d = 0;
    lo = tq_floor(t, lo);
    hi = tq_floor(t, hi);
    while (lo < hi && d < 130) {
        lo = tq_next_in_scale(t, lo, 1);
        d++;
    }
    return d;
}
static void tq_view(uint32_t *v0, uint32_t *span)       /* the steps shown: the bar, two bars, or the whole pattern */
{
    uint32_t cur = ui.cursor % NSTEP, len = tq_len(TSEL);
    if (tq_zoom == 0u) {
        *v0 = cur / 16u * 16u;
        *span = 16u;
    } else if (tq_zoom == 1u) {
        *v0 = cur / 32u * 32u;
        *span = 32u;
    } else {
        *v0 = 0;
        *span = len;
    }
}
static int tq_has(const step_t *s) { return s->time == ST_NOTE && s->n; }
/* centre the rows on the notes of the steps shown; more rows than fit: the cursor's (or the nearest) note stays in view */
static void tq_fit(void)
{
    track_t *t = TSEL;
    uint32_t v0, span, i, j, cur = ui.cursor % NSTEP, len = tq_len(t), best = NSTEP;
    int32_t lo = 127, hi = -1, top;
    tq_view(&v0, &span);
    for (i = v0; i < v0 + span && i < len; i++)
        if (tq_has(&t->step[i])) {
            for (j = 0; j < t->step[i].n; j++) {
                if (t->step[i].note[j] < lo) lo = t->step[i].note[j];
                if (t->step[i].note[j] > hi) hi = t->step[i].note[j];
            }
            if (best == NSTEP || (i > cur ? i - cur : cur - i) < (best > cur ? best - cur : cur - best))
                best = i;
        }
    if (hi < 0) {                                       /* nothing there: round the last note played (or the middle C) */
        top = tq_up(t, tq_floor(t, last_note ? (int32_t)last_note : 60), TQ_ROWS / 2);
    } else {
        int32_t rows = tq_dist(t, lo, hi) + 1;
        if (rows <= TQ_ROWS)
            top = tq_up(t, tq_floor(t, hi), (TQ_ROWS - rows) / 2);
        else
            top = tq_up(t, tq_floor(t, t->step[best].note[0]), TQ_ROWS / 2);
    }
    tq_base = (int8_t)clamp(top, 0, 127);
    tq_pan = 0;
}
static void tq_note_rows(const track_t *t, int32_t *top)   /* the top row's note: centred when the view changes, the cursor's note kept in */
{
    const step_t *s = &t->step[ui.cursor % NSTEP];
    uint32_t v0, span, sig;
    tq_view(&v0, &span);
    sig = v0 * 4099u + (uint32_t)tq_zoom * 257u + song.sel * 31u + scene_cur() * 7u + tq_len(t) + 1u;
    if (sig != tq_fit_sig) {
        tq_fit_sig = sig;
        tq_fit();
    }
    if (tq_pan && tq_pan_cur != ui.cursor)
        tq_pan = 0;                                     /* (the cursor moved: the window follows the note again) */
    if (!tq_pan && tq_has(s)) {
        int32_t n = tq_floor(t, s->note[0]), hi = tq_base, bot = tq_up(t, hi, 1 - TQ_ROWS);
        while (n > hi) {
            hi = tq_next_in_scale(t, hi, 1);
            bot = tq_next_in_scale(t, bot, 1);
        }
        while (n < bot) {
            hi = tq_next_in_scale(t, hi, -1);
            bot = tq_next_in_scale(t, bot, -1);
        }
        tq_base = (int8_t)clamp(hi, 0, 127);
    }
    *top = tq_base;
}
static int32_t tq_sx(uint32_t i, uint32_t v0, uint32_t span) { return TQ_GX + (int32_t)((i - v0) * (uint32_t)TQ_GW / span); }
static void tq_strip(uint32_t v0, uint32_t span)        /* y 26 .. 43: every bar of the pattern, the bars shown outlined */
{
    track_t *t = TSEL;
    uint32_t len = tq_len(t), nb = (len + 15u) / 16u, b, i;
    tq_range(t);
    for (b = 0; b < nb && b < 4u; b++) {
        int32_t x0 = TQ_GX + (int32_t)b * 52;
        int shown = b * 16u >= v0 && b * 16u < v0 + span;
        char n[2] = {(char)('1' + b), 0};
        tu_rrect(x0, 26, 49, 17, 3, shown ? TC_AMBER : RGB(42, 32, 18), TC_BG);
        tu_rrect(x0 + 1, 27, 47, 15, 2, shown ? RGB(58, 42, 16) : RGB(30, 24, 16), shown ? TC_AMBER : RGB(42, 32, 18));
        for (i = b * 16u; i < b * 16u + 16u && i < len; i++) {
            const step_t *s = &t->step[i];
            uint32_t l = 1;
            if (!tq_has(s))
                continue;
            while (i + l < len && t->step[i + l].time == ST_TIE && (i + l) < b * 16u + 16u)
                l++;
            tu_rect(x0 + 3 + (int32_t)((i - b * 16u) * 30u / 16u), 28 + (int32_t)((tq_pitch_hi - s->note[0]) * 11u / (tq_pitch_hi - tq_pitch_lo + 1u)),
                    (int32_t)(l * 30u / 16u) > 1 ? (int32_t)(l * 30u / 16u) : 2, 2, tu_mix(TC_BG, TU_TC[song.sel], 85));
        }
        tu_text_r(x0 + 45, 34, &AF_TS, n, shown ? TC_AMBER : TC_QUIET, shown ? RGB(58, 42, 16) : RGB(30, 24, 16));
    }
}
static void tq_notes_body(void)                         /* y 22 .. 215 */
{
    track_t *t = TSEL;
    uint32_t i, j, r, len = tq_len(t), cur = ui.cursor % NSTEP, root = (uint32_t)t->p[P_ROOT] % 12u, v0, span, above = 0, below = 0, k;
    int32_t top, notes[TQ_ROWS];
    char b[8], l2[40];
    tu_rect(0, 22, TU_W, 193, TC_BG);
    tq_note_rows(t, &top);
    if (!tq_has(&t->step[cur]) && tq_grab)
        tq_grab = 0;                                    /* (the grabbed note is gone: ALGORITHM, a clear) */
    tq_view(&v0, &span);
    tq_strip(v0, span);
    for (r = 0, notes[0] = top; r < TQ_ROWS; r++) {     /* the rows: the scale's notes, root rows tinted and named */
        int32_t y = TQ_GY + (int32_t)r * 8, n = notes[r], lx = tq_sx(len < v0 + span ? len : v0 + span, v0, span);
        int rootrow = (uint32_t)n % 12u == root;
        uint16_t rc = rootrow ? RGB(46, 36, 22) : RGB(26, 20, 12);
        tu_rect(TQ_GX, y, lx - TQ_GX, 7, rc);
        if (lx < TQ_GX + TQ_GW)
            tu_rect(lx, y, TQ_GX + TQ_GW - lx, 7, tu_mix(TC_BG, rc, 45));
        if (rootrow)
            tu_text_r(20, y + 3, &AF_TS, tu_note_name((uint32_t)n, b), TC_AMBER, TC_BG);
        if (r + 1u < TQ_ROWS)
            notes[r + 1u] = tq_next_in_scale(t, n, -1);
    }
    for (i = v0 + 4u; i < v0 + span && i < len; i += 4u)                         /* the beats */
        tu_rect(tq_sx(i, v0, span), TQ_GY, 1, TQ_ROWS * 8 - 1, TC_RAISE);
    for (i = v0; i < v0 + span && i < len; i++) {       /* the notes: bars with their length */
        const step_t *s = &t->step[i];
        uint32_t l = 1;
        int32_t x, w;
        if (!tq_has(s))
            continue;
        while (i + l < len && i + l < v0 + span && t->step[i + l].time == ST_TIE)
            l++;
        x = tq_sx(i, v0, span);
        w = tq_sx(i + l, v0, span) - x - 1;
        if (w < 2) w = 2;
        for (j = 0; j < s->n; j++) {
            int32_t fl = tq_floor(t, s->note[j]);
            if (fl > top) { above++; continue; }
            if (fl < notes[TQ_ROWS - 1]) { below++; continue; }
            for (r = 0; r < TQ_ROWS; r++)
                if (notes[r] == fl)
                    break;
            if (r == TQ_ROWS)
                continue;
            tu_cut(x, TQ_GY + (int32_t)r * 8, w, 7, i == cur && tq_grab ? tu_mix(TU_TC[song.sel], TC_TEXT, 55) : tu_mix(TC_BG, TU_TC[song.sel], 90));
            if (i == cur && j == 0u) {                  /* the note at the cursor: outlined white, its name over it */
                int32_t y = TQ_GY + (int32_t)r * 8;
                tu_rect(x, y, w, 1, TC_TEXT); tu_rect(x, y + 6, w, 1, TC_TEXT);
                tu_rect(x, y, 1, 7, TC_TEXT); tu_rect(x + w - 1, y, 1, 7, TC_TEXT);
                tu_text(x, r ? y - 6 : y + 13, &AF_TS, tu_note_name(s->note[0], b), TC_TEXT, r ? RGB(26, 20, 12) : RGB(26, 20, 12));
            }
        }
    }
    if (song.playing && TSEL->seq_idx % len >= v0 && TSEL->seq_idx % len < v0 + span)             /* the playhead */
        tu_rect(tq_sx(TSEL->seq_idx % len, v0, span) + (int32_t)(TQ_GW / span / 2u), TQ_GY, 2, TQ_ROWS * 8 - 1, TC_AMBER);
    if (cur >= v0 && cur < v0 + span) {                 /* the cursor's column: a violet frame */
        int32_t x = tq_sx(cur, v0, span), w = tq_sx(cur + 1u, v0, span) - x;
        tu_rect(x, TQ_GY - 2, w, 1, TC_PRESETS); tu_rect(x, TQ_GY + TQ_ROWS * 8 + 1, w, 1, TC_PRESETS);
        tu_rect(x, TQ_GY - 2, 1, TQ_ROWS * 8 + 4, TC_PRESETS); tu_rect(x + w - 1, TQ_GY - 2, 1, TQ_ROWS * 8 + 4, TC_PRESETS);
    }
    for (k = 0; k < 27u; k++)                           /* a key held: its row marked at the left edge */
        if ((fm1_in.notes >> k) & 1u && !((kb_layer >> k) & 1u)) {
            uint32_t note = kb_map(t, k);
            int32_t fl;
            if (note == KB_SILENT)
                continue;
            fl = tq_floor(t, (int32_t)note);
            for (r = 0; r < TQ_ROWS; r++)
                if (notes[r] == fl)
                    tu_rect(17, TQ_GY + (int32_t)r * 8 + 1, 4, 5, TC_GOOD);
        }
    if (above || below) {                               /* notes out of view: ▲n at the top, ▼n at the bottom */
        for (k = 0; k < 2u; k++) {
            uint32_t n = k ? below : above;
            char nb[6];
            int32_t y = k ? TQ_GY + TQ_ROWS * 8 - 11 : TQ_GY + 1, x = 212, q;
            if (!n)
                continue;
            for (q = 0; q < 4; q++)
                tu_rect(x + (k ? q : 3 - q), y + 3 + q, 2 * (k ? 4 - q : q + 1) - 1, 1, TC_AMBER);
            fmt_int(nb, (int32_t)n);
            tu_text(x + 8, y + 5, &AF_TS, nb, TC_AMBER, RGB(26, 20, 12));
        }
    }
    {   /* the info line and the four knobs */
        const step_t *s = &t->step[cur];
        uint32_t m = 0;
        for (i = 0; i < len; i++)
            m += tq_has(&t->step[i]);
        (void)m;
        str_cpy(b, "STEP ", 8);                         /* "STEP 7 \xB7 EMPTY" or "STEP 7 \xB7 D3 \xB7 1/16" */
        fmt_int(b + 5, (int32_t)cur + 1);
        if (tq_has(s)) {
            char nb[8];
            uint32_t ln = 1;
            while (cur + ln < len && t->step[cur + ln].time == ST_TIE)
                ln++;
            str_cpy(l2, "\xB7 ", 4);
            str_cpy(l2 + str_len(l2), tu_note_name(s->note[0], nb), 8);
            str_cpy(l2 + str_len(l2), " \xB7 ", 5);
            if (ln == 1u) str_cpy(l2 + str_len(l2), "1/16", 6);
            else if (ln == 2u) str_cpy(l2 + str_len(l2), "1/8", 6);
            else if (ln == 3u) str_cpy(l2 + str_len(l2), "1/8.", 6);
            else if (ln == 4u) str_cpy(l2 + str_len(l2), "1/4", 6);
            else { fmt_int(l2 + str_len(l2), (int32_t)ln); str_cpy(l2 + str_len(l2), "/16", 4); }
            if (s->flags & SF_SLIDE)
                str_cpy(l2 + str_len(l2), " \xB7 slide", 12);
        } else {
            str_cpy(l2, s->time == ST_TIE ? "\xB7 TIE" : "\xB7 EMPTY", 10);
        }
        tu_text(9, 179, &AF_TM, b, TC_TEXT, TC_BG);
        {
            int32_t x = tu_text(9 + text_w(&AF_TM, b) + 5, 179, &AF_TM, l2, TC_MID, TC_BG);
            char bb[16];
            str_cpy(bb, "\xB7 bar ", 8);
            fmt_int(bb + 6, (int32_t)(cur / 16u) + 1);
            str_cpy(bb + str_len(bb), " of ", 5);
            fmt_int(bb + str_len(bb), (int32_t)((len + 15u) / 16u));
            tu_text(x + 5, 179, &AF_TS, bb, TC_QUIET, TC_BG);
        }
        for (k = 0; k < 4u; k++) {
            static const char *const LB[4] = {"K1 STEP", "K2 PITCH", "K3 LENGTH", "K4 VELO"};
            int has = tq_has(s);
            uint16_t vc = has || k == 0u || (k == 1u && !tq_grab && s->time != ST_TIE) ? TC_TEXT : TC_DIM, lc = vc == TC_TEXT ? TU_TC[k] : TC_DIM;
            char v[10];
            int32_t x = 7 + (int32_t)k * 58;
            uint32_t l = 1;
            while (cur + l < len && has && t->step[cur + l].time == ST_TIE)
                l++;
            if (k == 0u)
                fmt_int(v, (int32_t)cur + 1);
            else if (!has)
                str_cpy(v, k == 1u && s->time != ST_TIE ? "ADD" : "-", 4);
            else if (k == 1u)
                str_cpy(v, tu_note_name(s->note[0], b), 8);
            else if (k == 2u)
                fmt_int(v, (int32_t)l);
            else
                fmt_int(v, s->vel);
            tu_rrect(x, 188, 55, 26, 5, TC_DEEP, TC_BG);
            tu_text_c(x + 27, 195, &AF_TS, LB[k], lc, TC_DEEP);
            tu_text_c(x + 27, 207, &AF_TL, v, vc, TC_DEEP);
        }
    }
}
static void tq_notes_head(void)
{
    char l[24], r[16];
    const char *u;
    tu_rect(0, 0, TU_W, TU_HY, TC_BG);
    tu_rect(0, TU_HY - 1, TU_W, 1, TC_SURF);
    tu_rec_dots(TC_BG);
    str_cpy(l, tq_grab ? "MOVING \xB7 " : "NOTES \xB7 ", 12);
    str_cpy(l + str_len(l), tu_inst_name(TSEL), 12);
    tu_text(TU_DX, 10, &AF_TM, l, tq_grab ? TC_AMBER : TC_TEXT, TC_BG);
    {
        uint16_t mc;
        const char *mt = tq_mode(&mc);
        (void)r; (void)u;
        tu_text_r(231, 10, &AF_TS, mt, mc, TC_BG);
    }
}
static void tq_notes_draw(void)
{
    uint32_t sig = ui.cursor * 31u + song.sel * 7u + tq_len(TSEL) * 977u + steps_hash(TSEL) + tq_bar * 7919u + (uint32_t)(uint8_t)tq_base * 131u +
                   (uint32_t)TSEL->p[P_ROOT] * 17u + (uint32_t)TSEL->p[P_SCALE] * 19u + (uint32_t)tq_grab * 6151u + tq_mode_sig() * 40503u + tu_rec_sig() * 6007u + (uint32_t)tq_zoom * 1009u +
                   (fm1_in.notes & ~kb_layer) * 7727u;
    if (song.playing)
        sig += TSEL->seq_idx * 131071u;
    tu_zone(0, 0, TU_HY, TC_BG, sig, tq_notes_head);
    tu_zone(2, 22, 100, TC_BG, sig, tq_notes_body);
    tu_zone(3, 122, 93, TC_BG, sig, tq_notes_body);
    if (tq_grab)
        tu_foot(3, "K1", "MOVE", "OCT+", "DROP", "K2", "PITCH", 0, 0, 0, 0);
    else
        tu_foot(3, "OCT+", "GRAB", "EDIT", "DELETE", "PRESETS", "ZOOM", 0, 0, 0, 0);
}
/* NOTES: OCT+ tapped (a hold is the step sheet) GRABs the note at the cursor, tapped again DROPs it */
static void tq_oct(uint32_t pressed, uint32_t oct)
{
    const step_t *s = &TSEL->step[ui.cursor % NSTEP];
    if ((pressed >> panel.btn[B_OCTUP]) & 1u)
        tq_oct_t0 = fm1_ms;
    if (!(oct & 2u) || fm1_ms - tq_oct_t0 > 450u)
        return;
    if (tq_grab) {
        tq_grab = 0;
        ui_message("DROPPED");
    } else if (chain_busy()) {
        ui_message("STOP TO EDIT");
    } else if (s->time == ST_NOTE && s->n) {
        tq_grab = 1;
        ui_message("MOVING");
    } else {
        ui_message("NO NOTE HERE");
    }
    ui.force = 1;
}
static int tq_occupied(const step_t *s) { return s->time == ST_TIE || (s->time == ST_NOTE && s->n); }
/* a grabbed note (and its ties) one step in time: not past another note or the pattern's end. 1 moved */
static int tq_move(int32_t d)
{
    track_t *t = TSEL;
    uint32_t len = tq_len(t), cur = ui.cursor % NSTEP, l = 1, i;
    step_t buf[NSTEP];
    while (cur + l < len && t->step[cur + l].time == ST_TIE)
        l++;
    if (d > 0 ? cur + l >= len || tq_occupied(&t->step[cur + l]) : cur == 0u || tq_occupied(&t->step[cur - 1u]))
        return 0;
    for (i = 0; i < l; i++)
        buf[i] = t->step[cur + i];
    for (i = 0; i < l; i++)
        step_clear(&t->step[cur + i]);
    for (i = 0; i < l; i++)
        t->step[(uint32_t)((int32_t)cur + d) + i] = buf[i];
    cursor_set((uint32_t)((int32_t)cur + d));
    tq_bar = (uint8_t)(ui.cursor / 16u);
    return 1;
}
static uint32_t tq_note_len(const track_t *t, uint32_t cur, uint32_t len)
{
    uint32_t l = 1;
    while (cur + l < len && t->step[cur + l].time == ST_TIE)
        l++;
    return l;
}
static void tq_set_len(track_t *t, uint32_t cur, uint32_t len, uint32_t nl)    /* the note at cur, nl steps long (ties) */
{
    uint32_t l = tq_note_len(t, cur, len), i;
    (void)len;
    for (i = 1; i < nl; i++)
        if (t->step[cur + i].time != ST_TIE) {          /* (a note in the way stops the growing) */
            if (t->step[cur + i].time == ST_NOTE && t->step[cur + i].n) { nl = i; break; }
            step_clear(&t->step[cur + i]);
            t->step[cur + i].time = ST_TIE;
        }
    for (i = nl; i < l; i++)                            /* (shorter: the freed steps are rests) */
        step_clear(&t->step[cur + i]);
}
/* K1 the next note (a bar flips), K2 its pitch in the scale, K3 its length (ties), K4 its velocity */
static void tq_notes_knob(uint32_t k, int32_t s)
{
    track_t *t = TSEL;
    uint32_t len = tq_len(t), i, cur = ui.cursor % NSTEP;
    step_t *st = &t->step[cur];
    int32_t d = s > 0 ? 1 : -1;
    if (k == 0u && tq_grab) {                           /* GRABbed: the note moves in time */
        if (chain_busy())
            ui_message("STOP TO EDIT");
        else {
            int32_t n;
            for (n = 0; n < (s > 0 ? s : -s); n++)
                if (!tq_move(d)) {
                    ui_message(d > 0 ? "NO ROOM AHEAD" : "NO ROOM BEHIND");
                    break;
                }
        }
        ui.force = 1;
        return;
    }
    if (k == 0u && ui.entry_open && tq_rec_on() && !song.playing && !chain_busy() && tq_has(st)) {   /* REC on, stopped, a key held: */
        uint32_t l = tq_note_len(t, cur, len), nl = (uint32_t)clamp((int32_t)l + s, 1, (int32_t)(len - cur));   /* K1 sets the new note's length */
        tq_set_len(t, cur, len, nl);
        tq_extra = (uint8_t)(nl - 1u);
        ui.force = 1;
        return;
    }
    if (k == 0u) {                                      /* every step, empty ones too; the bar follows */
        cursor_set((uint32_t)clamp((int32_t)cur + s, 0, (int32_t)len - 1));
        tq_bar = (uint8_t)(ui.cursor / 16u);
        return;
    }
    if (chain_busy()) {
        ui_message("STOP TO EDIT");
        return;
    }
    if (k == 1u && !tq_has(st) && st->time != ST_TIE && !tq_grab) {     /* an empty step: K2 puts the last note used there (an edit: REC off too) */
        st->n = 1;
        st->time = ST_NOTE;
        st->note[0] = (uint8_t)tq_floor(t, last_note ? (int32_t)last_note : 60);
        st->vel = 100;
        ui.force = 1;
        return;
    }
    if (!tq_has(st))
        return;
    if (k == 1u) {
        st->note[0] = (uint8_t)tq_next_in_scale(t, st->note[0], d);
        tq_pan = 0;
    } else if (k == 2u && tq_grab) {                    /* (GRABbed: the length stays) */
        return;
    } else if (k == 2u) {
        uint32_t l = tq_note_len(t, cur, len);
        tq_set_len(t, cur, len, (uint32_t)clamp((int32_t)l + d, 1, (int32_t)(len - cur)));
    } else {
        st->vel = (uint8_t)clamp((int32_t)st->vel + d * 4, 1, 127);
    }
    ui.force = 1;
}
static void tq_notes_delete(void)
{
    if (chain_busy()) {
        ui_message("STOP TO EDIT");
        return;
    }
    step_clear(&TSEL->step[ui.cursor % NSTEP]);
    (void)motion_clear_locks(TSEL, ui.cursor % NSTEP);
    ui_message("NOTE DELETED");
}
/* ================================================================== DRUMS =================================== */
/* round 8: the cursor's step on the pitched lane with a hit there (and no notes): K2 is its PITCH */
static int tq_pitch_here(const track_t *t, const step_t *s)
{
    uint32_t pl = drum_plane_fn ? drum_plane_fn(t) : 0xFFu;
    return pl < NLANE && (ui.lane & 7u) == pl && ((s->hit >> pl) & 1u) && !s->n;
}
static void tq_drums_body(void)                         /* y 22 .. 215 */
{
    track_t *t = TSEL;
    uint32_t l, c, len = tq_len(t), cur = ui.cursor % NSTEP;
    char b[8];
    tu_rect(0, 22, TU_W, 193, TC_BG);
    for (l = 0; l < 8u; l++) {
        int32_t y = 27 + (int32_t)l * 15;
        int sel = l == (ui.lane & 7u);
        tu_text_r(26, y + 6, &AF_TS, drum_lane_name(t, l), sel ? TU_TC[song.sel] : TC_QUIET, TC_BG);
        for (c = 0; c < 16u; c++) {
            uint32_t idx = (uint32_t)tq_bar * 16u + c;
            const step_t *s = &t->step[idx % NSTEP];
            int32_t x = 29 + (int32_t)c * 13;
            int on = idx < len && ((s->hit >> l) & 1u), playing = song.playing && t->seq_idx == idx;
            uint16_t bg = on ? tu_mix(TC_BG, TU_TC[song.sel], (s->acc >> l) & 1u || s->vel > 110 ? 100 : 60 + (uint32_t)s->vel / 4u)
                             : c % 4u == 0u ? RGB(46, 36, 22) : TC_DEEP;
            if (idx >= len)
                bg = tu_mix(TC_BG, bg, 35);
            if (playing)
                bg = on ? TC_AMBER : tu_mix(bg, TC_AMBER, 30);
            tu_rrect(x, y, 11, 13, 3, bg, TC_BG);
            if (on && !s->n && s->note[0] && drum_plane_fn && drum_plane_fn(t) == l) {   /* (round 8: the pitch: a mark up or down) */
                int32_t p = clamp((int32_t)s->note[0] - 64, -7, 7);
                tu_rect(x + 2, y + 6 - p * 5 / 7, 7, 2, TC_TEXT);
            }
            if (idx == cur && sel) {
                tu_rect(x, y, 11, 1, TC_TEXT); tu_rect(x, y + 12, 11, 1, TC_TEXT);
                tu_rect(x, y, 1, 13, TC_TEXT); tu_rect(x + 10, y, 1, 13, TC_TEXT);
            }
        }
    }
    {   /* the four knobs */
        const step_t *s = &t->step[cur];
        uint32_t k;
        for (k = 0; k < 4u; k++) {
            static const char *const LB[4] = {"K1 STEP", "K2 LANE", "K3 VELO", "K4 CHANCE"};
            char v[10];
            int32_t x = 7 + (int32_t)k * 58, ph = k == 1u && tq_pitch_here(t, s);
            if (k == 0u)
                fmt_int(v, (int32_t)cur + 1);
            else if (ph) {                              /* (round 8: K2 PITCH on the pitched lane) */
                int32_t p = s->note[0] ? clamp((int32_t)s->note[0] - 64, -7, 7) : 0;
                v[0] = p > 0 ? '+' : 0;
                fmt_int(v + (p > 0), p);
            } else if (k == 1u)
                str_cpy(v, drum_lane_name(t, ui.lane & 7u), 8);
            else if (k == 2u)
                fmt_int(v, s->vel);
            else {
                fmt_int(v, s->probability && s->probability <= 100u ? (int32_t)s->probability : 100);
                str_cpy(v + str_len(v), "%", 2);
            }
            tu_rrect(x, 153, 55, 24, 5, TC_DEEP, TC_BG);
            tu_text_c(x + 27, 159, &AF_TS, ph ? "K2 PITCH" : LB[k], TU_TC[k], TC_DEEP);
            tu_text_c(x + 27, 170, &AF_TL, v, TC_TEXT, TC_DEEP);
        }
    }
    (void)b;
}
static void tq_drums_head(void)
{
    char l[24], r[16], b[8];
    tu_rect(0, 0, TU_W, TU_HY, TC_BG);
    tu_rect(0, TU_HY - 1, TU_W, 1, TC_SURF);
    tu_rec_dots(TC_BG);
    str_cpy(l, "DRUMS \xB7 ", 12);
    str_cpy(l + str_len(l), tu_inst_name(TSEL), 12);
    tu_text(TU_DX, 10, &AF_TM, l, TC_TEXT, TC_BG);
    {
        uint16_t mc;
        const char *mt = tq_mode(&mc);
        tu_text_r(231, 10, &AF_TM, mt, mc, TC_BG);
    }
    (void)r; (void)b;                                   /* (the bar is the grid's own 16 columns; no room beside the mode) */
}
static void tq_drums_draw(void)
{
    uint32_t sig = ui.cursor * 31u + song.sel * 7u + tq_len(TSEL) * 977u + steps_hash(TSEL) + tq_bar * 7919u + (ui.lane & 7u) * 131u +
                   tq_mode_sig() * 40503u + tu_rec_sig() * 6007u;
    if (song.playing)
        sig += TSEL->seq_idx * 131071u;
    tu_zone(0, 0, TU_HY, TC_BG, sig, tq_drums_head);
    tu_zone(2, 22, 100, TC_BG, sig, tq_drums_body);
    tu_zone(3, 122, 93, TC_BG, sig, tq_drums_body);
    tu_foot(3, "K1", "STEP", "KEYS", "HIT", "K2", "LANE", 0, 0, 0, 0);
}
static void tq_drums_knob(uint32_t k, int32_t s)
{
    track_t *t = TSEL;
    step_t *st = &t->step[ui.cursor % NSTEP];
    int32_t d = s > 0 ? 1 : -1;
    if (k == 0u) {
        cursor_set((uint32_t)clamp((int32_t)ui.cursor + d, 0, (int32_t)tq_len(t) - 1));
        tq_bar = (uint8_t)(ui.cursor / 16u);
    } else if (k == 1u && tq_pitch_here(t, st)) {     /* (round 8: the pitched lane, a hit there: K2 its pitch in the scale) */
        int32_t p = st->note[0] ? clamp((int32_t)st->note[0] - 64, -7, 7) : 0;
        if (chain_busy()) {
            ui_message("STOP TO EDIT");
            return;
        }
        p = clamp(p + d, -7, 7);
        st->note[0] = (uint8_t)(p ? 64 + p : 0);
    } else if (k == 1u) {
        ui.lane = (uint8_t)((ui.lane + (uint32_t)(d + 8)) & 7u);
    } else if (chain_busy()) {
        ui_message("STOP TO EDIT");
    } else if (k == 2u) {
        st->vel = (uint8_t)clamp((int32_t)st->vel + d * 4, 1, 127);
    } else {
        int32_t c = st->probability && st->probability <= 100u ? st->probability : 100;
        c = clamp(c + d * 5, 5, 100);
        st->probability = c >= 100 ? 0u : (uint8_t)c;
    }
    ui.force = 1;
}
/* ---- the three screens' input and the SEQ button ---- */
static void tq_adapt(void)                              /* ALGORITHM moved to a drum / melodic track: its screen */
{
    if (tu.scr == TU_NOTES && drum_track(TSEL))
        tu_goto(TU_DRUMS);
    else if (tu.scr == TU_DRUMS && !drum_track(TSEL))
        tu_goto(TU_NOTES);
}
static void tq_input(uint32_t notes, uint32_t busy)
{
    uint32_t k;
    int32_t s, sel;
    if (!busy) {
        if (sheet_on()) {                               /* hold OCT+: the step sheet (STEP, NOTES) */
            for (k = 0; k < 4u; k++)
                if ((s = panel_enc(EN_K1 + k)) != 0) {
                    oct_as_mod(2u);
                    sheet_turn(k, s);
                }
        } else {
            for (k = 0; k < 4u; k++)
                if ((s = panel_enc(EN_K1 + k)) != 0) {
                    if (tu.scr == TU_NOTES)
                        tq_notes_knob(k, s);
                    else if (tu.scr == TU_DRUMS)
                        tq_drums_knob(k, s);
                }
        }
        s = panel_enc(EN_PRESET);
        if (s != 0 && tu.scr == TU_NOTES) {                               /* ZOOM: 1 bar, 2 bars, the whole pattern; the cursor stays */
            static const char *const Z[3] = {"ZOOM: 1 BAR", "ZOOM: 2 BARS", "ZOOM: THE WHOLE PATTERN"};
            tq_zoom = (uint8_t)clamp((int32_t)tq_zoom + (s > 0 ? 1 : -1), 0, 2);
            ui_message(Z[tq_zoom]);
            ui.force = 1;
        }
        if (s != 0 && tu.scr == TU_DRUMS) {
            /* a bar on, and the cursor to the same step in it */
            uint32_t nb = (uint32_t)clamp((int32_t)tq_bar + (s > 0 ? 1 : -1), 0, (int32_t)(tq_len(TSEL) - 1u) / 16);
            uint32_t ni = nb * 16u + ui.cursor % 16u;
            cursor_set(ni < tq_len(TSEL) ? ni : tq_len(TSEL) - 1u);
            tq_bar = (uint8_t)(ui.cursor / 16u);
        }
        sel = panel_enc(EN_SELECT);
        if (sel != 0 && tu.scr == TU_NOTES) {           /* the roll up / down by pitch (rows), not steps */
            tq_note_rows(TSEL, &(int32_t){0});
            for (k = 0; k < (uint32_t)(sel > 0 ? sel : -sel); k++)
                tq_base = (int8_t)tq_next_in_scale(TSEL, tq_base, sel > 0 ? 1 : -1);
            tq_pan = 1; tq_pan_cur = ui.cursor;
            ui.force = 1;
        } else if (sel != 0 && tu.scr == TU_DRUMS) {
            cursor_set((uint32_t)clamp((int32_t)ui.cursor + (sel > 0 ? 1 : -1), 0, (int32_t)tq_len(TSEL) - 1));
            tq_bar = (uint8_t)(ui.cursor / 16u);
        }
    }
    tq_keys(notes);
    if (tu.scr == TU_NOTES || tu.scr == TU_DRUMS)
        tq_bar = (uint8_t)(tq_bar > (tq_len(TSEL) - 1u) / 16u ? (tq_len(TSEL) - 1u) / 16u : tq_bar);
}
