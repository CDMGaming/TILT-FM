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
static void tq_step_body(void)                          /* y 22 .. 160 */
{
    uint32_t k, i, b, c, len = tq_len(TSEL), cur = ui.cursor % NSTEP;
    char s[24];
    tu_rect(0, 22, TU_W, 138, TC_BG);
    tu_text(16, 31, &AF_TS, "ALL TRACKS", TC_QUIET, TC_BG);
    str_cpy(s, " \xB7 ", 5);
    tq_len_text(s + 3, 64u);
    tu_text(16 + text_w(&AF_TS, "ALL TRACKS") + 1, 31, &AF_TS, s, TC_QUIET, TC_BG);
    for (k = 0; k < NTRK; k++) {                        /* the thin overview: 64 steps of each track */
        const track_t *t = &trk[k];
        uint32_t tl = tq_len(t);
        int32_t y = 37 + (int32_t)k * 5;
        for (i = 0; i < 64u; i++) {
            const step_t *st = &t->step[i];
            int32_t x = 16 + (int32_t)(i * 208u / 64u);
            uint16_t col = i >= tl ? tu_mix(TC_BG, TC_SURF, 50) : tq_has_note(t, st) ? TU_TC[k] : TC_SURF;
            if (k != song.sel && col != TC_SURF && i < tl)
                col = tu_mix(TC_BG, col, 70);
            tu_rect(x, y, 2, 4, col);
        }
    }
    tq_range(TSEL);
    for (b = 0; b < 4u; b++) {                          /* the track's four rows of 16 */
        int32_t y = 62 + (int32_t)b * 24;
        char n[3] = {'1', 0, 0};
        uint32_t inlen = b * 16u < len;
        n[0] = (char)('1' + b);
        tu_text(8, y + 10, &AF_TS, "BAR", inlen ? TC_QUIET : TC_DIM, TC_BG);
        tu_text(8 + text_w(&AF_TS, "BAR") + 2, y + 10, &AF_TS, n, inlen ? TC_QUIET : TC_DIM, TC_BG);
        for (c = 0; c < 16u; c++) {
            uint32_t idx = b * 16u + c;
            const step_t *st = &TSEL->step[idx];
            int32_t x = 36 + (int32_t)c * 12;
            int has = tq_has_note(TSEL, st) && idx < len, playing = song.playing && TSEL->seq_idx == idx;
            uint16_t bg = playing ? TC_AMBER : has ? TU_TC[song.sel] : idx < len && st->time == ST_TIE ? tu_mix(TC_SURF, TU_TC[song.sel], 40) : TC_SURF;
            if (!inlen)
                bg = tu_mix(TC_BG, bg, 30);
            tu_rrect(x, y, 10, 21, 3, bg, TC_BG);
            if (has && !drum_track(TSEL)) {
                int32_t pad = 3 + (int32_t)(st->note[0] - (int32_t)tq_pitch_lo) * 12 / (int32_t)(tq_pitch_hi - tq_pitch_lo + 1u);
                tu_rect(x + 2, y + 17 - pad, 6, 2, TC_BG);
            }
            if (idx == cur) {
                tu_rect(x, y, 10, 1, TC_TEXT); tu_rect(x, y + 20, 10, 1, TC_TEXT);
                tu_rect(x, y, 1, 21, TC_TEXT); tu_rect(x + 9, y, 1, 21, TC_TEXT);
            }
        }
    }
}
static void tq_step_info(void)                          /* y 160 .. 215: the step, or the step sheet */
{
    const step_t *st = &TSEL->step[ui.cursor % NSTEP];
    char a[12], b[40], nn[6];
    tu_rect(0, 160, TU_W, 55, TC_BG);
    if (sheet_on()) {
        static const char *const L[4] = {"MOVE", "COPY", "NUDGE", "SLIDE"};
        uint32_t k;
        for (k = 0; k < 4u; k++) {
            char v[8], kb[3] = {'K', (char)('1' + k), 0};
            int32_t x = 8 + (int32_t)k * 57;
            if (k == 2u)
                fmt_int(v, step_nudge(st));
            else if (k == 3u)
                str_cpy(v, drum_track(TSEL) ? "RATCH" : (st->flags & SF_SLIDE) ? "ON" : "OFF", 8);
            else
                fmt_int(v, (int32_t)ui.cursor + 1);
            tu_rrect(x, 164, 54, 26, 5, TC_DEEP, TC_BG);
            tu_text(x + 5, 172, &AF_TS, kb, TU_TC[k], TC_DEEP);
            tu_text(x + 18, 172, &AF_TS, L[k], TU_TC[k], TC_DEEP);
            tu_text_c(x + 27, 183, &AF_TL, k < 2u && 0 ? "" : v, TC_TEXT, TC_DEEP);
        }
        return;
    }
    str_cpy(a, "STEP ", 8);
    fmt_int(a + 5, (int32_t)ui.cursor % NSTEP + 1);
    tu_text(16, 168, &AF_TM, a, TC_TEXT, TC_BG);
    if (drum_track(TSEL)) {
        str_cpy(b, st->hit ? "hit" : "empty", 8);
    } else if (st->time == ST_NOTE && st->n) {
        str_cpy(b, tu_note_name(st->note[0], nn), 6);
        if (st->flags & SF_ACCENT)
            str_cpy(b + str_len(b), " \xB7 accent", 12);
        if (st->flags & SF_SLIDE)
            str_cpy(b + str_len(b), " \xB7 slide", 12);
    } else {
        str_cpy(b, st->time == ST_TIE ? "tie" : "rest", 8);
    }
    tu_text(16 + text_w(&AF_TM, a) + 6, 168, &AF_TL, b, TC_MID, TC_BG);
    {
        static const char *const K[2] = {"K1", "SELECT"}, *const A[2] = {"STEP", "16 STEPS"};
        tu_hint_line(198, TC_BG, TC_QUIET, 2, K, A);
    }
}
static void tq_zero(void) { }
static void tq_step_draw(void)
{
    uint32_t k, i, sig = ui.cursor * 31u + song.sel * 7u + tq_len(TSEL) * 977u + (sheet_on() ? 99u : 0u);
    for (k = 0; k < NTRK; k++)
        sig = sig * 31u + steps_hash(&trk[k]) + (uint32_t)clamp(trk[k].p[P_SLEN], 1, NSTEP);
    if (song.playing)
        sig += TSEL->seq_idx * 131071u;
    (void)i;
    tq_head("STEP");
    tu_zone(2, 22, 70, TC_BG, sig, tq_step_body);
    tu_zone(3, 92, 68, TC_BG, sig, tq_step_body);
    tu_zone(4, 160, 55, TC_BG, sig, tq_step_info);
    tu_foot(3, "KEYS", "WRITE", "G5", "REST", "HOLD OCT+", "SHEET", 0, 0, 0, 0);
    (void)tq_zero;
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
        else if (!chain_busy())
            seq_entry(notes);
        else if (notes)
            ui_message("STOP TO EDIT");
    }
}
/* ================================================================== NOTES =================================== */
static int32_t tq_next_in_scale(const track_t *t, int32_t n, int32_t d) { return tl_neigh(t, n, d); }
static void tq_note_rows(const track_t *t, int32_t *top)   /* the top row's note: the window keeps the selected note */
{
    const step_t *s = &t->step[ui.cursor % NSTEP];
    int32_t n = s->time == ST_NOTE && s->n ? s->note[0] : 57, k, hi = tq_base;
    if (tq_pan && tq_pan_cur != ui.cursor)
        tq_pan = 0;                                     /* (the cursor moved: the window follows the note again) */
    if (hi < 12 || hi > 127)
        hi = n + 5;
    for (k = 0; k < 6; k++)                             /* (the in-scale note at or above) */
        if (tl_in_scale(t, hi))
            break;
        else
            hi++;
    if (!tq_pan) {
        int32_t bot = hi, j;
        for (j = 0; j < 10; j++)
            bot = tq_next_in_scale(t, bot, -1);
        if (n > hi) {
            while (n > hi)
                hi = tq_next_in_scale(t, hi, 1);
        } else if (n < bot) {
            while (n < bot) {
                hi = tq_next_in_scale(t, hi, -1);
                bot = tq_next_in_scale(t, bot, -1);
            }
        }
    }
    tq_base = (int8_t)hi;
    *top = hi;
}
static void tq_notes_body(void)                         /* y 22 .. 215 */
{
    track_t *t = TSEL;
    uint32_t i, r, len = tq_len(t), root = (uint32_t)t->p[P_ROOT] % 12u, nn, selidx = 0, cnt = 0, here = 0;
    int32_t top, notes[11];
    char b[8], l2[40];
    tu_rect(0, 22, TU_W, 193, TC_BG);
    tq_note_rows(t, &top);
    if (!(t->step[ui.cursor % NSTEP].time == ST_NOTE && t->step[ui.cursor % NSTEP].n) && tq_grab)
        tq_grab = 0;                                    /* (the grabbed note is gone: ALGORITHM, a clear) */
    for (r = 0, notes[0] = top; r < 11u; r++) {
        int32_t y = 26 + (int32_t)r * 11, n = notes[r];
        int rootrow = (uint32_t)n % 12u == root;
        tu_rrect(22, y, 212, 10, 2, rootrow ? RGB(46, 36, 22) : RGB(26, 20, 12), TC_BG);
        tu_text_r(19, y + 5, &AF_TS, tu_note_name((uint32_t)n, b), rootrow ? TC_AMBER : TC_QUIET, TC_BG);
        if (r < 10u)
            notes[r + 1u] = tq_next_in_scale(t, n, -1);
    }
    for (i = 0; i < 4u; i++)
        tu_rect(22 + (int32_t)i * 53, 26, 1, 120, TC_RAISE);
    if (song.playing) {
        uint32_t px = t->seq_idx % len;
        if (px / 16u == tq_bar)
            tu_rect(22 + (int32_t)(px % 16u) * 13, 26, 13, 120, tu_mix(TC_BG, TC_AMBER, 14));
    }
    for (i = 0; i < len && i < NSTEP; i++) {            /* the notes: bars with their length */
        const step_t *s = &t->step[i];
        if (s->time != ST_NOTE || !s->n)
            continue;
        if (i == ui.cursor % NSTEP)
            selidx = cnt;
        cnt += i / 16u == tq_bar;
        if (i / 16u != tq_bar)
            continue;
        for (r = 0; r < 11u; r++)
            if (notes[r] == (int32_t)s->note[0]) {
                uint32_t l = 1;
                int32_t x = 22 + (int32_t)(i % 16u) * 13;
                while (i + l < len && t->step[i + l].time == ST_TIE && (i + l) / 16u == tq_bar)
                    l++;
                tu_rrect(x + 1, 27 + (int32_t)r * 11, (int32_t)l * 13 - 2, 9, 3,
                         tq_grab && i == ui.cursor % NSTEP ? tu_mix(TU_TC[song.sel], TC_TEXT, 55) : tu_mix(TC_BG, TU_TC[song.sel], 85), TC_BG);
                if (i == ui.cursor % NSTEP) {
                    tu_rect(x + 1, 27 + (int32_t)r * 11, (int32_t)l * 13 - 2, 1, TC_TEXT);
                    tu_rect(x + 1, 35 + (int32_t)r * 11, (int32_t)l * 13 - 2, 1, TC_TEXT);
                    tu_rect(x + 1, 27 + (int32_t)r * 11, 1, 9, TC_TEXT);
                    tu_rect(x + (int32_t)l * 13 - 2, 27 + (int32_t)r * 11, 1, 9, TC_TEXT);
                }
            }
    }
    (void)selidx; (void)here;
    {   /* the info line and the four knobs */
        const step_t *s = &t->step[ui.cursor % NSTEP];
        uint32_t k, m = 0, tot = 0;
        for (i = 0; i < len; i++)
            if (t->step[i].time == ST_NOTE && t->step[i].n) {
                tot++;
                if (i <= ui.cursor % NSTEP)
                    m = tot;
            }
        (void)m; (void)tot;
        str_cpy(b, "STEP ", 8);                         /* "STEP 7 \xB7 EMPTY" or "STEP 7 \xB7 D3 \xB7 1/16" */
        fmt_int(b + 5, (int32_t)(ui.cursor % NSTEP) + 1);
        if (s->time == ST_NOTE && s->n) {
            char nb[8];
            uint32_t ln = 1;
            while (ui.cursor % NSTEP + ln < len && t->step[ui.cursor % NSTEP + ln].time == ST_TIE)
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
        tu_text(9, 158, &AF_TM, b, TC_TEXT, TC_BG);
        tu_text(9 + text_w(&AF_TM, b) + 5, 158, &AF_TL, l2, TC_MID, TC_BG);
        for (k = 0; k < 4u; k++) {
            static const char *const LB[4] = {"K1 STEP", "K2 PITCH", "K3 LENGTH", "K4 VELO"};
            int has = s->time == ST_NOTE && s->n;
            uint16_t vc = has || k == 0u ? TC_TEXT : TC_DIM, lc = has || k == 0u ? TU_TC[k] : TC_DIM;
            char v[10];
            int32_t x = 7 + (int32_t)k * 58;
            uint32_t l = 1;
            while (ui.cursor % NSTEP + l < len && s->time == ST_NOTE && t->step[ui.cursor % NSTEP + l].time == ST_TIE)
                l++;
            if (k == 0u)
                fmt_int(v, (int32_t)(ui.cursor % NSTEP) + 1);
            else if (!has)
                str_cpy(v, "-", 2);
            else if (k == 1u)
                str_cpy(v, tu_note_name(s->note[0], b), 8);
            else if (k == 2u)
                fmt_int(v, (int32_t)l);
            else
                fmt_int(v, s->vel);
            tu_rrect(x, 172, 55, 26, 5, TC_DEEP, TC_BG);
            tu_text_c(x + 27, 179, &AF_TS, LB[k], lc, TC_DEEP);
            tu_text_c(x + 27, 191, &AF_TL, v, vc, TC_DEEP);
        }
        (void)nn;
    }
}
static void tq_notes_head(void)
{
    char l[24], r[16], b[3] = {0, 0, 0};
    int32_t x;
    uint32_t i;
    const char *u;
    tu_rect(0, 0, TU_W, TU_HY, TC_BG);
    tu_rect(0, TU_HY - 1, TU_W, 1, TC_SURF);
    tu_badge(9, 10, song.sel, TC_BG);
    str_cpy(l, tq_grab ? "MOVING \xB7 " : "NOTES \xB7 ", 12);
    str_cpy(l + str_len(l), tu_inst_name(TSEL), 12);
    tu_text(26, 10, &AF_TM, l, tq_grab ? TC_AMBER : TC_TEXT, TC_BG);
    x = 108;
    for (i = 0; i < 4u; i++) {                          /* the bar tabs */
        uint32_t used = i * 16u < tq_len(TSEL);
        b[0] = (char)('1' + i);
        tu_rrect(x, 5, 13, 11, 3, i == tq_bar ? TC_AMBER : used ? TC_SURF : TC_BG, TC_BG);
        tu_text_c(x + 6, 10, &AF_TS, b, i == tq_bar ? TC_BG : used ? TC_TEXT : TC_DIM, i == tq_bar ? TC_AMBER : used ? TC_SURF : TC_BG);
        x += 15;
    }
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
                   (uint32_t)TSEL->p[P_ROOT] * 17u + (uint32_t)TSEL->p[P_SCALE] * 19u + (uint32_t)tq_grab * 6151u + tq_mode_sig() * 40503u;
    if (song.playing)
        sig += TSEL->seq_idx * 131071u;
    tu_zone(0, 0, TU_HY, TC_BG, sig, tq_notes_head);
    tu_zone(2, 22, 100, TC_BG, sig, tq_notes_body);
    tu_zone(3, 122, 93, TC_BG, sig, tq_notes_body);
    if (tq_grab)
        tu_foot(3, "K1", "MOVE", "OCT+", "DROP", "K2", "PITCH", 0, 0, 0, 0);
    else
        tu_foot(3, "K1", "STEP", "OCT+", "GRAB", "EDIT", "DELETE", 0, 0, 0, 0);
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
    if (k == 0u) {                                      /* every step, empty ones too; the bar follows */
        cursor_set((uint32_t)clamp((int32_t)cur + s, 0, (int32_t)len - 1));
        tq_bar = (uint8_t)(ui.cursor / 16u);
        return;
    }
    if (chain_busy()) {
        ui_message("STOP TO EDIT");
        return;
    }
    if (!(st->time == ST_NOTE && st->n))
        return;
    if (k == 1u) {
        st->note[0] = (uint8_t)tq_next_in_scale(t, st->note[0], d);
        tq_pan = 0;
    } else if (k == 2u && tq_grab) {                    /* (GRABbed: the length stays) */
        return;
    } else if (k == 2u) {
        uint32_t l = 1, nl;
        while (cur + l < len && t->step[cur + l].time == ST_TIE)
            l++;
        nl = (uint32_t)clamp((int32_t)l + d, 1, (int32_t)(len - cur));
        for (i = 1; i < nl; i++)
            if (t->step[cur + i].time != ST_TIE) {      /* (a note in the way stops the growing) */
                if (t->step[cur + i].time == ST_NOTE && t->step[cur + i].n) { nl = i; break; }
                step_clear(&t->step[cur + i]);
                t->step[cur + i].time = ST_TIE;
            }
        for (i = nl; i < l; i++)                        /* (shorter: the freed steps are rests) */
            step_clear(&t->step[cur + i]);
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
            int32_t x = 7 + (int32_t)k * 58;
            if (k == 0u)
                fmt_int(v, (int32_t)cur + 1);
            else if (k == 1u)
                str_cpy(v, drum_lane_name(t, ui.lane & 7u), 8);
            else if (k == 2u)
                fmt_int(v, s->vel);
            else {
                fmt_int(v, s->probability && s->probability <= 100u ? (int32_t)s->probability : 100);
                str_cpy(v + str_len(v), "%", 2);
            }
            tu_rrect(x, 153, 55, 24, 5, TC_DEEP, TC_BG);
            tu_text_c(x + 27, 159, &AF_TS, LB[k], TU_TC[k], TC_DEEP);
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
    tu_badge(9, 10, song.sel, TC_BG);
    str_cpy(l, "DRUMS \xB7 ", 12);
    str_cpy(l + str_len(l), tu_inst_name(TSEL), 12);
    tu_text(26, 10, &AF_TM, l, TC_TEXT, TC_BG);
    {
        uint16_t mc;
        const char *mt = tq_mode(&mc);
        tu_text_r(231, 10, &AF_TM, mt, mc, TC_BG);
    }
    str_cpy(r, "BAR ", 5);
    fmt_int(r + 4, (int32_t)tq_bar + 1);
    str_cpy(r + str_len(r), "/", 2);
    fmt_int(b, (int32_t)((tq_len(TSEL) + 15u) / 16u));
    str_cpy(r + str_len(r), b, 4);
    tu_text_r(142, 12, &AF_TS, r, TC_QUIET, TC_BG);
}
static void tq_drums_draw(void)
{
    uint32_t sig = ui.cursor * 31u + song.sel * 7u + tq_len(TSEL) * 977u + steps_hash(TSEL) + tq_bar * 7919u + (ui.lane & 7u) * 131u +
                   tq_mode_sig() * 40503u;
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
                    else if (k == 0u)
                        cursor_set((uint32_t)clamp((int32_t)ui.cursor + s, 0, (int32_t)tq_len(TSEL) - 1));
                }
        }
        if ((s = panel_enc(EN_PRESET)) != 0 && (tu.scr == TU_NOTES || tu.scr == TU_DRUMS)) {
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
        if ((s = panel_enc(EN_PRESET)) != 0 && tu.scr == TU_STEP)
            tu_sound_step(s);
        if (sel != 0 && tu.scr == TU_STEP)
            cursor_set((uint32_t)clamp((int32_t)ui.cursor + (sel > 0 ? 1 : -1) * 16, 0, (int32_t)tq_len(TSEL) - 1));
    }
    tq_keys(notes);
    if (tu.scr == TU_NOTES || tu.scr == TU_STEP || tu.scr == TU_DRUMS)
        tq_bar = (uint8_t)(tq_bar > (tq_len(TSEL) - 1u) / 16u ? (tq_len(TSEL) - 1u) / 16u : tq_bar);
}
