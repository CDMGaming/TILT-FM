/* TFM-UI (tfm-ui): TFM's own screen system, tfm-spec/TFM-UI-SPEC.md. Included at the end of tilt.c.
 *
 * Felucca's engines, sequencer, storage, MIDI, USB and HAL stay. What is new is what the player sees and the buttons'
 * meaning: ui_input.c hands the frame to tu_input() and ui_draw.c to tu_draw() while tu_on() (a TFM screen is up: not
 * DEEP, not the MENU, not a modal screen of tilt.c: START, DICE, SETUP, TIME, NAME). The layers (a button held: ui_layer.c)
 * keep their key maps and their logic; only the TAP of a layer's button now opens a TFM screen (tu_tap).
 *
 * The drawings are 480 x 480 (2x): every size here is halved. A screen is zones (a header, the body, a footer), each
 * drawn in a canvas band of its own when its signature changes (tu_zone), so the animated parts redraw alone.
 * MENU > SYSTEM > UI: TFM / CLASSIC (CLASSIC: Felucca's own UI throughout; tests/ui_test.c runs the Felucca checks so). */

/* ---- colours: the drawings' (TFM AMBER); GREY and MONO take their luminance (no hue there) ---- */
#define TC_BG RGB(18, 14, 8)                            /* #120E08 */
#define TC_SURF RGB(42, 32, 18)                         /* #2A2012 */
#define TC_DEEP RGB(30, 24, 16)                         /* #1E1810 */
#define TC_RAISE RGB(58, 45, 28)                        /* #3A2D1C */
#define TC_TEXT RGB(246, 236, 216)                      /* #F6ECD8 */
#define TC_MID RGB(201, 188, 164)                       /* #C9BCA4 */
#define TC_QUIET RGB(169, 157, 136)                     /* #A99D88 */
#define TC_DIM RGB(90, 80, 69)                          /* #5A5045 */
#define TC_AMBER RGB(255, 168, 40)                      /* #FFA828 */
#define TC_CYAN RGB(96, 214, 230)                       /* #60D6E6 */
#define TC_GOOD RGB(75, 224, 143)                       /* #4BE08F */
#define TC_REC RGB(255, 90, 90)                         /* #FF5A5A */
#define TC_TRACK RGB(51, 41, 28)                        /* the ring's empty part: #33291C */
#define TC_PRESETS RGB(196, 155, 255)                   /* #C49BFF: the PRESETS knob */
static const uint16_t TU_TC[4] = {RGB(90, 169, 255), RGB(255, 210, 74), RGB(255, 107, 154), RGB(75, 224, 143)};   /* T1 / K1 .. T4 / K4 */

static uint16_t tu_c(uint16_t c)
{
    if (settings.palette == UI_GREY_INDEX || settings.palette == UI_BW_INDEX) {
        uint32_t r = (c >> 11) << 3, g = ((c >> 5) & 63u) << 2, b = (c & 31u) << 3, y = (r * 77u + g * 150u + b * 29u) >> 8;
        return ux_gray(y >> 3);
    }
    return c;
}
/* a colour mixed over another, pct % of c */
static uint16_t tu_mix(uint16_t over, uint16_t c, uint32_t pct)
{
    uint32_t r = (((over >> 11) & 31u) * (100u - pct) + ((c >> 11) & 31u) * pct) / 100u;
    uint32_t g = (((over >> 5) & 63u) * (100u - pct) + ((c >> 5) & 63u) * pct) / 100u;
    uint32_t b = ((over & 31u) * (100u - pct) + (c & 31u) * pct) / 100u;
    return (uint16_t)(r << 11 | g << 5 | b);
}

static void tu_pages_tap(uint32_t b);                   /* (tfm_pages.c) */
static void tu_pages_draw(void);
static void tu_pages_input(uint32_t busy);
static void tu_song_input(uint32_t oct);                /* (tfm_song.c) */
static void tu_songs_input(uint32_t oct);
static void tu_songs_save(void);
static void tu_songs_copy(void);
static void tu_songs_clear(void);
static void tu_song_draw(void);
static void tu_songs_draw(void);
static void tu_song_delete(void);
static void tu_song_sounds(void);
static uint8_t tq_bar;                                  /* (tfm_seq.c: the bar NOTES and DRUMS show) */
static void tq_input(uint32_t notes, uint32_t busy);
static void tq_oct(uint32_t pressed, uint32_t oct);
static void tq_step_draw(void);
static void tq_notes_draw(void);
static void tq_drums_draw(void);
static void tq_notes_delete(void);
static void tq_adapt(void);
static void tu_sound_step(int32_t s);                    /* (tfm_faces.c) */
static void tu_sound_input(uint32_t busy);
static void tu_sound_draw(void);
static void tu_deep(void);
/* ---- the screens ---- */
/* (the screens' numbers: ui.c) */
#define TU_ZONES 8u
static struct {
    uint8_t scr, prev;                                  /* the screen (TU_*), the one before (DEEP's way back) */
    uint8_t page;                                       /* SOUND: macros 1 / 2 */
    uint8_t cur;                                        /* a list's selected row */
    uint8_t flash[NTRK];                                /* JAM: a note's flash, frames */
    int32_t prev_peak[NTRK];
    uint32_t sig[TU_ZONES];
} tu;
static uint8_t tu_off = TU_OFF_DEFAULT;                 /* 1: CLASSIC (set by the menu, by the tests) */

static int tu_classic(void) { return tu_off != 0u; }
static int tu_on(void)                                  /* a TFM screen is the UI now (ui_draw.c, ui_input.c) */
{
    return !tu_classic() && tu.scr != TU_DEEP && !ui.menu && !name_on() && !ui.uboot;
}
static int tu_deep_on(void) { return !tu_classic() && tu.scr == TU_DEEP && !ui.menu; }
static uint8_t tq_grab;                                 /* NOTES: a note is grabbed (tfm_seq.c) */
static uint8_t tsg_copy = 0xFF;                         /* SONGS: GLO pressed on this slot: the next GLO copies it onto the one picked */
static void tu_goto(uint32_t scr)
{
    if (scr >= TU_N)
        return;
    if (scr != TU_SONGS)
        tsg_copy = 0xFF;
    tq_grab = 0;
    if (tu.scr != scr)
        tu.prev = tu.scr;
    tu.scr = (uint8_t)scr;
    memset(tu.sig, 0xFF, sizeof tu.sig);
    ui.layer = 0;
    ui.menu = 0;
    ui.force = 1;
    song.seq_mode = scr == TU_STEP || scr == TU_NOTES || scr == TU_DRUMS;
    ui.home = !song.seq_mode;                           /* (the sequencer's own state: the cursor, the keys' entry) */
    if (song.seq_mode) {
        ui.page = (uint8_t)page_first(FAM_SEQ);
        tq_bar = (uint8_t)(ui.cursor / 16u);
    }
}

/* ---- text: vertically centred on a line, left / centre / right ---- */
static int32_t tu_cap_top(const aafont_t *f, int32_t *h)   /* 'H': the ink's top in the line, its height */
{
    const aag_t *g = &f->g[((uint32_t)'H' - f->first) << f->psh];
    *h = g->bh;
    return g->by;
}
static int32_t tu_ty(const aafont_t *f, int32_t ymid)  /* the line top that centres the capitals on ymid */
{
    int32_t h, t = tu_cap_top(f, &h);
    return ymid - t - h / 2;
}
static int32_t tu_text(int32_t x, int32_t ymid, const aafont_t *f, const char *s, uint16_t fg, uint16_t bg)
{
    return cv_text_on(x, tu_ty(f, ymid), f, s, tu_c(fg), tu_c(bg));
}
static int32_t tu_text_c(int32_t xc, int32_t ymid, const aafont_t *f, const char *s, uint16_t fg, uint16_t bg)
{
    int32_t b[4];
    text_ink(f, s, b);
    return cv_text_on(xc - (b[0] + b[2]) / 2, tu_ty(f, ymid), f, s, tu_c(fg), tu_c(bg));
}
static int32_t tu_text_r(int32_t xr, int32_t ymid, const aafont_t *f, const char *s, uint16_t fg, uint16_t bg)
{
    int32_t b[4];
    text_ink(f, s, b);
    return cv_text_on(xr - b[2], tu_ty(f, ymid), f, s, tu_c(fg), tu_c(bg));
}
/* a text that fits maxw (else ellipsised) */
static int32_t tu_text_fit(int32_t x, int32_t ymid, const aafont_t *f, const char *s, uint16_t fg, uint16_t bg, int32_t maxw)
{
    char b[40];
    text_fit(b, sizeof b, s, f, maxw);
    return tu_text(x, ymid, f, b, fg, bg);
}

/* ---- shapes ---- */
static uint32_t tu_isq(uint32_t n)                      /* floor(sqrt(n)) */
{
    uint32_t r = 0, b = 1u << 14;
    while (b > n)
        b >>= 2;
    for (; b; b >>= 2) {
        if (n >= r + b) {
            n -= r + b;
            r = (r >> 1) + b;
        } else {
            r >>= 1;
        }
    }
    return r;
}
static void tu_rect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t c) { cv_rect(x, y, w, h, tu_c(c)); }
static void tu_rrect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, uint16_t c, uint16_t under)
{
    cv_rrect(x, y, w, h, r, tu_c(c), tu_c(under));
}
static void tu_disc(int32_t cx, int32_t cy, int32_t r, uint16_t c)
{
    int32_t dy;
    c = tu_c(c);
    for (dy = -r; dy <= r; dy++) {
        int32_t w = (int32_t)tu_isq((uint32_t)(r * r - dy * dy));
        cv_rect(cx - w, cy + dy, 2 * w + 1, 1, c);
    }
}
static void tu_ring(int32_t cx, int32_t cy, int32_t ro, int32_t ri, uint16_t c)   /* an annulus, ri .. ro */
{
    int32_t dy;
    c = tu_c(c);
    for (dy = -ro; dy <= ro; dy++) {
        int32_t wo = (int32_t)tu_isq((uint32_t)(ro * ro - dy * dy));
        if (dy > -ri && dy < ri) {
            int32_t wi = (int32_t)tu_isq((uint32_t)(ri * ri - dy * dy));
            cv_rect(cx - wo, cy + dy, wo - wi, 1, c);
            cv_rect(cx + wi + 1, cy + dy, wo - wi, 1, c);
        } else {
            cv_rect(cx - wo, cy + dy, 2 * wo + 1, 1, c);
        }
    }
}
static const uint8_t TU_ATAN[65] = {0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13,
                                    13, 14, 14, 15, 15, 16, 16, 17, 17, 18, 18, 18, 19, 19, 20, 20, 21, 21, 21, 22, 22, 23,
                                    23, 23, 24, 24, 25, 25, 25, 26, 26, 26, 27, 27, 28, 28, 28};
static uint32_t tu_ang(int32_t dx, int32_t dy)          /* 0..255 round, clockwise from the top (dy down) */
{
    int32_t x = dx, y = -dy, ax = x < 0 ? -x : x, ay = y < 0 ? -y : y;
    uint32_t a;
    if (!ax && !ay)
        return 0;
    a = ax > ay ? 64u - TU_ATAN[ay * 64 / ax] : TU_ATAN[ax * 64 / ay];
    return x >= 0 ? (y >= 0 ? a : 128u - a) : (y < 0 ? 128u + a : (256u - a) & 255u);
}
/* the drawings' dial: a ring that starts at 7:30 and runs 270 degrees clockwise, filled to frac (0..1000) in c, the
 * rest of the 270 in track, the last quarter open */
static void tu_dial(int32_t cx, int32_t cy, int32_t ro, int32_t ri, uint32_t frac, uint16_t c, uint16_t track)
{
    int32_t x, y, fill = (int32_t)(frac > 1000u ? 1000u : frac) * 192 / 1000;
    c = tu_c(c);
    track = tu_c(track);
    for (y = -ro; y <= ro; y++) {
        int32_t wo = (int32_t)tu_isq((uint32_t)(ro * ro - y * y));
        for (x = -wo; x <= wo; x++) {
            uint32_t d2 = (uint32_t)(x * x + y * y);
            int32_t a;
            if (d2 < (uint32_t)(ri * ri))
                continue;
            a = (int32_t)((tu_ang(x, y) - 160u) & 255u);
            if (a < 192)
                cv_rect(cx + x, cy + y, 1, 1, a < fill || (!a && fill) ? c : track);
        }
    }
}
static void tu_play_icon(int32_t x, int32_t ymid, int playing, uint16_t c)   /* ▶ / ■, 7 px */
{
    int32_t i;
    c = tu_c(c);
    if (!playing) {
        cv_rect(x, ymid - 3, 6, 6, c);
        return;
    }
    for (i = 0; i < 4; i++)
        cv_rect(x + i, ymid - 3 + i, 1, 7 - 2 * i, c);
}
static void tu_arrow(int32_t x, int32_t ymid, int left, uint16_t c)   /* ◂ / ▸, 3 x 5 */
{
    int32_t i;
    c = tu_c(c);
    for (i = 0; i < 3; i++)
        cv_rect(left ? x + 2 - i : x + i, ymid - 2 + i, 1, 5 - 2 * i, c);
}

/* ---- the layout grammar: header 22, footer 25 (an instrument's face has its own page colours: tfm_faces.c) ---- */
static uint16_t tu_pbg = TC_BG, tu_pline = TC_SURF, tu_pq = TC_QUIET;   /* the page, its rules, the hints' words */
#define TU_HY 22
#define TU_FY 215
#define TU_W 240
static void tu_zone(uint32_t id, int32_t y0, int32_t h, uint16_t bg, uint32_t sig, void (*fn)(void))
{
    if (!ui.force && tu.sig[id] == sig)
        return;
    tu.sig[id] = sig;
    cv_begin(TU_W, (uint32_t)h, tu_c(bg));
    cv_oy = -y0;
    fn();
    cv_oy = 0;
    cv_blit(0, (uint32_t)y0);
}
static const char *tu_note_name(uint32_t n, char *b)    /* "A2" */
{
    static const char *const N[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    uint32_t o = n / 12u;
    str_cpy(b, N[n % 12u], 4);
    fmt_int(b + str_len(b), (int32_t)o - 1);
    return b;
}
static const char *tu_inst_name(const track_t *t)       /* the instrument, or the engine's name for a sound that has none */
{
    return tfm_ins_of(t) != TI_NONE ? tfm_ins_name(t) : ENGINES[t->eng_req % NENGINES]->name;
}
/* a track's badge: its colour, the number on it (r 12 at 2x: 12 px wide at 1x) */
static void tu_badge(int32_t x, int32_t ymid, uint32_t k, uint16_t bg)
{
    char b[2] = {(char)('1' + k % NTRK), 0};
    tu_disc(x + 6, ymid, 6, TU_TC[k % NTRK]);
    tu_text_c(x + 6, ymid, &AF_TS, b, TC_BG, TU_TC[k % NTRK]);
    (void)bg;
}
/* round 4 item 2: REC state on every header but START, DICE and TIME: T1..T4 as four dots in their track colours, the selected
 * track's larger; a red ring round the dot = armed (song.rec), a filled red disc = recording now (armed and playing).
 * The dots take the header's left (TU_DX is where its title starts) */
#define TU_DX 54
static uint32_t tu_rec_sig(void) { return (song.rec & 15u) * 7919u + (song.sel & 3u) * 131u + (song.playing ? 977u : 0u) + 1u; }
static void tu_rec_dots(uint16_t bg)
{
    uint32_t k;
    for (k = 0; k < NTRK; k++) {
        int32_t cx = 14 + 10 * (int32_t)k, rb = k == song.sel ? 3 : 2;
        int armed = (int)((song.rec >> k) & 1u);
        if (armed && song.playing) {
            tu_disc(cx, 11, rb + 2, TC_REC);
            continue;
        }
        if (armed) {
            tu_disc(cx, 11, rb + 2, TC_REC);
            tu_disc(cx, 11, rb + 1, bg);
        }
        tu_disc(cx, 11, rb, TU_TC[k]);
    }
}
/* the timer's text, "7:42" (empty: no TIMER running) */
static void tu_timer_text(char *b)
{
    b[0] = 0;
    if (tm.on)
        tm_text(b);
}
/* the header: left (what, where), centre (the TIMER), right (the tempo or the page) */
static struct { const char *l, *l2, *r; int badge; int play; uint32_t sig; } tu_hd;
static void tu_head_draw(void)
{
    char t[8];
    int32_t x = TU_DX;
    tu_rect(0, 0, TU_W, TU_HY, tu_pbg);
    tu_rect(0, TU_HY - 1, TU_W, 1, tu_pline);
    tu_rec_dots(tu_pbg);
    x = tu_text(x, 10, &AF_TM, tu_hd.l ? tu_hd.l : "", TC_TEXT, TC_BG) + 3;
    if (tu_hd.l2)
        tu_text(x, 10, &AF_TS, tu_hd.l2, TC_QUIET, TC_BG);
    tu_timer_text(t);
    if (t[0])
        tu_text_c(120, 10, &AF_PS, t, TC_AMBER, TC_BG);
    if (tu_hd.play >= 0) {
        char b[8];
        int32_t w;
        fmt_int(b, song.g[G_BPM]);
        w = text_w(&AF_TM, b);
        tu_text(231 - w, 10, &AF_TM, b, TC_TEXT, TC_BG);
        tu_play_icon(231 - w - 11, 10, tu_hd.play, TC_TEXT);
    } else if (tu_hd.r) {
        tu_text_r(231, 10, &AF_TS, tu_hd.r, TC_QUIET, TC_BG);
    }
}
static void tu_head(int badge, const char *l, const char *l2, const char *r, int play)
{
    char t[8];
    tu_hd.badge = badge;
    tu_hd.l = l;
    tu_hd.l2 = l2;
    tu_hd.r = r;
    tu_hd.play = play ? (int)song.playing : -1;
    tu_timer_text(t);
    tu_hd.sig = str_hash(1u, l ? l : "") * 31u + str_hash(2u, l2 ? l2 : "") + str_hash(3u, r ? r : "") * 7u +
                str_hash(4u, t) * 13u + (uint32_t)(badge + 1) * 977u + (uint32_t)(song.g[G_BPM]) * 4099u +
                (uint32_t)(tu_hd.play + 1) * 65537u + (tm.on ? tm_level() * 131071u : 0u) + tu_rec_sig() * 40503u;
    tu_zone(0, 0, TU_HY, tu_pbg, tu_hd.sig + tu_pbg * 7u, tu_head_draw);
}
/* the footer: `KEY ACTION` hints centred, the key amber; a message (ui.msg) replaces them while it shows */
static struct { const char *k[5], *a[5]; uint32_t n; } tu_ft;
/* `KEY ACTION` hints centred on a line at y: the key amber, the action in qc (the footer's, and the lines of hints a screen
 * carries in a band of its own) */
static void tu_hint_line(int32_t y, uint16_t bg, uint16_t qc, uint32_t n, const char *const *k, const char *const *a)
{
    int32_t w = 0, x, i;
    for (i = 0; i < (int32_t)n; i++)
        w += text_w(&AF_TS, k[i]) + 3 + text_w(&AF_TS, a[i]) + (i ? 8 : 0);
    x = (TU_W - w) / 2;
    for (i = 0; i < (int32_t)n; i++) {
        x = tu_text(x, y, &AF_TS, k[i], TC_AMBER, bg) + 3;
        x = tu_text(x, y, &AF_TS, a[i], qc, bg) + 8;
    }
}
static void tu_foot_draw(void)
{
    tu_rect(0, TU_FY, TU_W, 25, tu_pbg);
    tu_rect(0, TU_FY, TU_W, 1, tu_pline);
    if (ui.msg_t && ui.msg[0]) {
        tu_text_c(120, TU_FY + 13, &AF_TL, ui.msg, TC_AMBER, tu_pbg);
        return;
    }
    tu_hint_line(TU_FY + 13, tu_pbg, tu_pq, tu_ft.n, tu_ft.k, tu_ft.a);
}
static void tu_foot(uint32_t n, const char *k0, const char *a0, const char *k1, const char *a1, const char *k2, const char *a2,
                    const char *k3, const char *a3, const char *k4, const char *a4)
{
    uint32_t i, sig = n * 7u;
    tu_ft.n = n;
    tu_ft.k[0] = k0; tu_ft.a[0] = a0; tu_ft.k[1] = k1; tu_ft.a[1] = a1; tu_ft.k[2] = k2; tu_ft.a[2] = a2;
    tu_ft.k[3] = k3; tu_ft.a[3] = a3; tu_ft.k[4] = k4; tu_ft.a[4] = a4;
    for (i = 0; i < n; i++)
        sig = sig * 31u + str_hash(1u, tu_ft.k[i]) + str_hash(2u, tu_ft.a[i]) * 3u;
    sig += ui.msg_t ? str_hash(5u, ui.msg) * 131u + 1u : 0u;
    tu_zone(1, TU_FY, 25, tu_pbg, sig + tu_pbg * 7u + tu_pq * 3u, tu_foot_draw);
}

/* ---- the scene letters: A..H (round 2: all eight are stored scenes) ---- */
#define TU_NSCENE NSCN
static uint32_t tu_scene_cur(void) { uint32_t c = scene_cur(); return c < NSCN ? c : 0xFFu; }
static int tu_scene_used(uint32_t s) { return s < NSCN && project_used(s); }
static uint32_t tu_used_mask(void)                      /* the scenes stored, one bit each (the zones' signatures) */
{
    uint32_t s, m = 0;
    for (s = 0; s < NSCN; s++)
        m |= (uint32_t)(project_used(s) != 0) << s;
    return m;
}

/* ---- bars: the longest pattern, where it plays ---- */
static void tu_bars(uint32_t *cur, uint32_t *tot)
{
    uint32_t i, best = 0, len = 1, bar = tfm_bar();
    for (i = 0; i < NTRK; i++) {
        uint32_t l = (uint32_t)clamp(trk[i].p[P_SLEN], 1, NSTEP);
        if (l > len) {
            len = l;
            best = i;
        }
    }
    *tot = (len + bar - 1u) / bar;
    *cur = song.playing ? (uint32_t)(trk[best].seq_idx % len) / bar + 1u : 1u;
}

/* ================================================================== JAM (home) ================================ */
static void tu_jam_body(void)                           /* y 22 .. 126: four bumpers */
{
    uint32_t c;
    for (c = 0; c < NTRK; c++) {
        const track_t *t = &trk[c];
        int32_t cx = 35 + 56 * (int32_t)c + (c > 1u ? 1 : 0), cy = 73, pos = jam_pos(t), ang;
        int mute = t->p[P_MUTE] != 0, ins = jam_macro(t);
        uint16_t col = TU_TC[c], edge = mute ? TC_DIM : col, fillc;
        char b[24], v[8];
        int32_t p = t->peak;
        if (p > tu.prev_peak[c] * 3 / 2 + 900)
            tu.flash[c] = 5;
        tu.prev_peak[c] = p;
        b[0] = 'T';
        b[1] = (char)('1' + c);
        b[2] = 0;
        tu_text_c(cx, 40, &AF_TS, b, col, TC_BG);
        ang = pos * 192 / 127;                           /* the ring: 270 degrees, 0..192 */
        tu_dial(cx, cy, 24, 21, (uint32_t)(pos * 1000 / 127), mute ? TC_DIM : col, TC_TRACK);
        tu_disc(cx, cy, 21, TC_BG);
        fillc = mute ? TC_BG : tu.flash[c] ? tu_mix(col, TC_TEXT, 45) : tu_mix(TC_BG, col, 25 + (uint32_t)clamp(p / 140, 0, 60));
        if (!mute && tu.flash[c] == 0 && p < 300)
            fillc = tu_mix(TC_BG, col, 25);
        tu_disc(cx, cy, 15, edge);
        tu_disc(cx, cy, 14, fillc);
        tu_ring(cx, cy, 5, 4, edge);
        if (c == song.sel)                               /* the selected track: a bright outer ring */
            tu_ring(cx, cy, 28, 26, tu_mix(col, TC_TEXT, 55));
        (void)ang;
        if (tu.flash[c])
            tu.flash[c]--;
        if (c == song.sel) {                             /* (its name and macro in reverse: filled in the track colour) */
            tu_rrect(cx - 27, 98, 54, 28, 4, col, TC_BG);
            tu_text_c(cx, 105, &AF_TS, ins ? tfm_ins_name(t) : tu_inst_name(t), TC_BG, col);
        } else {
            tu_text_c(cx, 105, &AF_TS, ins ? tfm_ins_name(t) : tu_inst_name(t), TC_TEXT, TC_BG);
        }
        if (ins) {
            str_cpy(b, TFM_INS[tfm_ins_of(t)].mac[tfm_jam_m(t)].name, 12);
        } else {
            str_cpy(b, "LEVEL", 12);
        }
        fmt_int(v, pos * 100 / 127);
        str_cpy(b + str_len(b), " ", 2);
        str_cpy(b + str_len(b), v, 6);
        if (c == song.sel)
            tu_text_c(cx, 118, &AF_TS, b, TC_BG, col);
        else
            tu_text_c(cx, 118, &AF_TS, b, mute ? TC_DIM : col, TC_BG);
    }
}
static void tu_strip_draw(void)                         /* y 126 .. 170: the scene strip A..H */
{
    uint32_t s, cur = tu_scene_cur();
    int32_t x;
    tu_text(8, 138, &AF_TS, "SCENES", TC_QUIET, TC_BG);
    x = 8 + text_w(&AF_TS, "SCENES") + 3;
    tu_text(x, 138, &AF_TS, "- hold ARP to switch", TC_QUIET, TC_BG);
    for (s = 0; s < TU_NSCENE; s++) {
        int32_t tx = 8 + (int32_t)s * 28;
        int is_cur = s == cur, used = tu_scene_used(s);
        char b[2] = {(char)('A' + s), 0};
        uint16_t edge = is_cur ? TC_AMBER : used ? RGB(138, 124, 102) : RGB(51, 41, 28);
        tu_rrect(tx, 148, 25, 18, 4, edge, TC_BG);
        tu_rrect(tx + 1, 149, 23, 16, 3, is_cur ? TC_AMBER : TC_BG, edge);
        tu_text_c(tx + 12, 157, &AF_TL, b, is_cur ? TC_BG : used ? TC_TEXT : TC_DIM, is_cur ? TC_AMBER : TC_BG);
    }
}
static uint32_t tu_sel_until;                           /* ALGORITHM turned on JAM: the track's name shows big until then (fm1_ms) */
static int tu_sel_big(void) { return tu_sel_until && (int32_t)(tu_sel_until - fm1_ms) > 0; }
static void tu_gap_draw(void)
{
    tu_rect(0, 170, TU_W, 45, TC_BG);
    if (!tu_sel_big()) {                                /* the band under the scenes: the buttons the footer has no room for */
        static const char *const K1[2] = {"REC", "EDIT"}, *const A1[2] = {"RECORD MOVES", "SOUND"};
        static const char *const K2[3] = {"SEQ", "GLO", "SAVE"}, *const A2[3] = {"STEPS", "SONG", "SONGS"};
        tu_hint_line(186, TC_BG, TC_QUIET, 2, K1, A1);
        tu_hint_line(203, TC_BG, TC_QUIET, 3, K2, A2);
    }
    if (tu_sel_big()) {
        char b[24];
        const track_t *t = &trk[song.sel % NTRK];
        b[0] = 'T';
        b[1] = (char)('1' + song.sel % NTRK);
        b[2] = 0;
        str_cpy(b + 2, " \xB7 ", 6);
        str_cpy(b + str_len(b), tu_inst_name(t), 14);
        tu_text_c(120, 192, &AF_PM, b, TU_TC[song.sel % NTRK], TC_BG);
    }
}
static void tu_jam_draw(void)
{
    char l[16], l2[16];
    uint32_t cur = tu_scene_cur(), bar, tot, i, sig = 0;
    str_cpy(l, "SCENE ", 8);
    l[6] = cur < TU_NSCENE ? (char)('A' + cur) : '-';
    l[7] = 0;
    tu_bars(&bar, &tot);
    str_cpy(l2, ". bar ", 8);
    fmt_int(l2 + 6, (int32_t)bar);
    str_cpy(l2 + str_len(l2), " of ", 5);
    fmt_int(l2 + str_len(l2), (int32_t)tot);
    l2[0] = '-';
    {   /* the header: the selected track, "T2 \xB7 SQUELCH", then where the song is */
        char tn[24];
        tn[0] = 'T';
        tn[1] = (char)('1' + song.sel % NTRK);
        tn[2] = 0;
        str_cpy(tn + 2, " \xB7 ", 6);
        str_cpy(tn + str_len(tn), tu_inst_name(&trk[song.sel % NTRK]), 14);
        {
            static char hl[24], hl2[28];
            str_cpy(hl, tn, sizeof hl);
            str_cpy(hl2, l, 8);
            str_cpy(hl2 + str_len(hl2), " ", 2);
            str_cpy(hl2 + str_len(hl2), l2, 16);
            tu_head(-1, hl, hl2, 0, 1);
        }
    }
    sig = song.sel * 6151u + (uint32_t)tu_sel_big() * 3571u;
    for (i = 0; i < NTRK; i++)
        sig = sig * 131u + (uint32_t)jam_pos(&trk[i]) * 7u + (uint32_t)(trk[i].p[P_MUTE] != 0) * 3u + trk[i].eng_req +
              (uint32_t)tfm_ins_of(&trk[i]) * 257u + (uint32_t)tu.flash[i] * 1031u;
    for (i = 0; i < NTRK; i++)                          /* (the level moves the glow: a frame in two while it sounds) */
        if (trk[i].peak > 200 || tu.flash[i])
            sig += ui.frame >> 1;
    tu_zone(2, TU_HY, 104, TC_BG, sig, tu_jam_body);
    tu_zone(3, 126, 44, TC_BG, cur * 977u + tu_used_mask() * 5u + (uint32_t)sng_cur * 131u, tu_strip_draw);
    tu_zone(4, 170, 45, TC_BG, 1u + song.sel * 7u + (uint32_t)tu_sel_big() * 101u, tu_gap_draw);
    tu_foot(3, "K1-K4", "JAM", "SCL", "TEMPO", "ENV", "DICE", 0, 0, 0, 0);
}

/* ================================================================== MIX ====================================== */
static int32_t tu_lvl_pct(const track_t *t)               /* the LEVEL as 0..100 of its own range */
{
    const param_desc_t *d = track_desc(t, P_LEVEL);
    return d->max > d->min ? (clamp(t->p[P_LEVEL], d->min, d->max) - d->min) * 100 / (d->max - d->min) : 0;
}
static void tu_mix_body(void)                           /* y 22 .. 126 and 126 .. 215 drawn as one: the faders */
{
    uint32_t c;
    for (c = 0; c < NTRK; c++) {
        const track_t *t = &trk[c];
        int32_t cx = 36 + 56 * (int32_t)c, top = 70, h = 115, lvl = tu_lvl_pct(t), fh, mh;
        int mute = t->p[P_MUTE] != 0;
        char b[8];
        uint16_t col = mute ? RGB(58, 50, 38) : TU_TC[c];
        b[0] = 'T';
        b[1] = (char)('1' + c);
        b[2] = 0;
        tu_text_c(cx, 37, &AF_TS, b, TU_TC[c], TC_BG);
        tu_text_c(cx, 49, &AF_TS, tu_inst_name(t), TC_MID, TC_BG);
        tu_rrect(cx - 11, top, 23, h, 6, TC_DEEP, TC_BG);
        fh = h * lvl / 100;
        if (fh > 3)
            tu_rrect(cx - 11, top + h - fh, 23, fh, 6, col, TC_DEEP);
        mh = mute ? 0 : h * clamp(t->peak * 100 / 12000, 0, 100) / 100;
        if (mh > 0)
            tu_rect(cx - 8, top + h - mh - 1, 17, 2, TC_TEXT);
        fmt_int(b, lvl);
        tu_text_c(cx, 198, &AF_TL, b, mute ? RGB(122, 110, 92) : TC_TEXT, TC_BG);
        if (mute)
            tu_text_c(cx, 210, &AF_TS, "MUTED", TC_QUIET, TC_BG);
    }
}
static void tu_mix_draw(void)
{
    uint32_t i, sig = 0;
    tu_head(-1, "MIX", ". JAM page 2", 0, 1);
    for (i = 0; i < NTRK; i++)
        sig = sig * 131u + (uint32_t)(trk[i].p[P_LEVEL] + 200) + (uint32_t)(trk[i].p[P_MUTE] != 0) * 977u +
              (uint32_t)(trk[i].peak / 400) * 3u;
    if (sig == 0)
        sig = 1;
    tu_zone(2, TU_HY, 62, TC_BG, sig, tu_mix_body);     /* (one pass: the faders reach 215: the band is cut below) */
    tu_zone(3, TU_HY + 62, 62, TC_BG, sig, tu_mix_body);
    tu_zone(4, TU_HY + 124, 69, TC_BG, sig, tu_mix_body);
    tu_foot(3, "K1-K4", "LEVEL", "BLACK KEYS", "MUTE", "HOME", "JAM", 0, 0, 0, 0);
}

/* ================================================================== routing =================================== */
static void tu_draw_screen(void);
static void tu_draw(void)
{
    tu_draw_screen();
    ui.force = 0;
}
static void tu_black_mute(uint32_t notes)               /* MIX: the black keys, in order, mute T1..T4 */
{
    uint32_t k, n = 0;
    for (k = 0; k < 27u; k++) {
        if (!key_black(k))
            continue;
        if (((notes >> k) & 1u) && n < NTRK) {
            track_t *t = &trk[n];
            t->p[P_MUTE] = t->p[P_MUTE] ? 0 : 1;
            motion_capture(t, P_MUTE, t->p[P_MUTE]);
            ui.force = 1;
        }
        n++;
    }
}
static int tu_song_on(void) { return tu.scr == TU_SONG; }
static int tu_oct_ok(void)                              /* OCT+ is OK and OCT- BACK here (no octave) */
{
    return !tu_classic() && !ui.menu && (tu.scr == TU_SONG || tu.scr == TU_SONGS || tu.scr == TU_NOTES);
}
static void tu_deep(void)                               /* SOUND > DEEP: Felucca's own pages for this track */
{
    tu.prev = TU_SOUND;
    tu.scr = TU_DEEP;
    ui.layer = 0;
    ui.home = 0;
    song.seq_mode = 0;
    ui.page = (uint8_t)page_first(FAM_EDIT);
    page_entered();
    ui.force = 1;
}
static void tu_deep_leave(void)
{
    tu.scr = TU_SOUND;
    tu_goto(TU_SOUND);
}
static void tu_screen_input(uint32_t pressed, uint32_t notes, uint32_t oct, uint32_t busy)
{
    int32_t s;
    uint32_t k;
    if (busy)                                           /* (a layer's button is down: the layer has the knobs) */
        return;
    switch (tu.scr) {
    case TU_JAM:
        for (k = 0; k < 4u; k++)
            if ((s = panel_enc(EN_K1 + k)) != 0) {
                jam_turn(k, s);
                ui.force = 0;
            }
        break;
    case TU_SOUND:
        tu_sound_input(busy);
        break;
    case TU_STEP: case TU_NOTES: case TU_DRUMS:
        tq_input(notes, busy);
        if (tu.scr == TU_NOTES)
            tq_oct(pressed, oct);
        break;
    case TU_FX: case TU_SPACE: case TU_ARP: case TU_CLOCK: case TU_CLOCK2: case TU_MOTION:
        tu_pages_input(busy);
        break;
    case TU_SONG:
        tu_song_input(oct);
        break;
    case TU_SONGS:
        tu_songs_input(oct);
        break;
    case TU_MIX:
        for (k = 0; k < 4u; k++)
            if ((s = panel_enc(EN_K1 + k)) != 0) {
                track_t *t = &trk[k];
                const param_desc_t *d = track_desc(t, P_LEVEL);
                if (k != song.sel && ((song.rec >> k) & 1u))
                    track_select(k);
                t->p[P_LEVEL] = (int16_t)param_turn(d, t->p[P_LEVEL], accel(EN_K1 + k, s, d->max - d->min));
                motion_capture(t, P_LEVEL, t->p[P_LEVEL]);
                ml_knob(t, P_LEVEL);
            }
        tu_black_mute(notes);
        break;
    default:
        break;
    }
    (void)pressed;
    (void)oct;
}
/* every frame while a TFM screen is up (ui_input.c, after the layers, the dialogs and the buttons that are not screens'):
 * the knobs every screen shares, then the screen's */
static void tu_input(uint32_t pressed, uint32_t notes, uint32_t oct, uint32_t busy)
{
    int32_t s;
    if (!busy && (s = panel_enc(EN_ALGO)) != 0) {       /* ALGORITHM: the track, everywhere */
        track_select((uint32_t)clamp((int32_t)song.sel + (s > 0 ? 1 : -1), 0, NTRK - 1));
        tq_adapt();
        tu_sel_until = fm1_ms + 1000u;                  /* (JAM: the track's name big for about a second) */
    }
    if (!busy && tu.scr != TU_SOUND && tu.scr != TU_NOTES && tu.scr != TU_DRUMS && tu.scr != TU_SONG && (s = panel_enc(EN_PRESET)) != 0)
        tu_sound_step(s);                               /* PRESETS: the track's sound, everywhere it is not a list */
    tu_screen_input(pressed, notes, oct, busy);
    enc_drop();                                         /* (the knobs nobody took) */
    ui_notices();
}
/* a screen not built yet: Felucca's page for the button, in DEEP (HOME back) */
static void tu_fallback(uint32_t b)
{
    tu.prev = tu.scr;
    tu.scr = TU_DEEP;
    ui.layer = 0;
    ui.force = 1;
    page_tap(b);
}
/* a button's tap (ui_input.c page_tap): the TFM screen it opens (TFM-UI-SPEC 2) */
static void tu_tap(uint32_t b)
{
    switch (b) {
    case B_HOME:
        if (tu.scr == TU_SONGS && tsg_new) {            /* (round 3: picking the song a new one replaces: HOME cancels, DICE again) */
            tsg_new = 0;
            tilt_ns_show();
            break;
        }
        tu_goto(tu.scr == TU_JAM ? TU_MIX : TU_JAM);
        break;
    case B_ENV:                                         /* DICE (a modal screen of tilt.c) */
        tilt_ns_show();
        break;
    case B_SEQ:                                         /* STEP; again NOTES (a drum track: DRUMS); again STEP */
        if (tu.scr == TU_STEP)
            tu_goto(drum_track(TSEL) ? TU_DRUMS : TU_NOTES);
        else if (tu.scr == TU_NOTES || tu.scr == TU_DRUMS)
            tu_goto(TU_STEP);
        else
            tu_goto(TU_STEP);
        break;
    case B_FX: case B_SCL: case B_LFO: case B_ARP:      /* FX (again SPACE), CLOCK (again page 2), MOTION, ARP */
        tu_pages_tap(b);
        break;
    case B_GLO:                                         /* SONG; again: SOUNDS SCENE / CURRENT (SONGS: copy the song picked) */
        if (tu.scr == TU_SONG)
            tu_song_sounds();
        else if (tu.scr == TU_SONGS)
            tu_songs_copy();
        else
            tu_goto(TU_SONG);
        break;
    case B_SAVE:                                        /* SONGS; there: save the song now (the NAME screen) into the slot picked */
        if (tu.scr == TU_SONGS)
            tu_songs_save();
        else
            tu_goto(TU_SONGS);
        break;
    case B_EDIT:                                        /* SOUND: the face; again: macros 2; again: DEEP (NOTES: delete; SONG: delete the section) */
        if (tu.scr == TU_SONG) {
            tu_song_delete();
        } else if (tu.scr == TU_SONGS) {
            tu_songs_clear();
        } else if (tu.scr == TU_NOTES) {
            tq_notes_delete();
        } else if (tu.scr != TU_SOUND) {
            tu_goto(TU_SOUND);
            tu.page = 0;
        } else if (tu.page == 0u) {
            tu.page = 1;
            ui.force = 1;
        } else {
            tu_deep();
        }
        break;
    default:
        tu_fallback(b);
        break;
    }
}
static void tu_home_reset(void)                        /* go_home (ui.c): every way out of a page ends on JAM */
{
    if (!tu_classic() && tu.scr != TU_JAM && !(ui.menu && tu.scr == TU_DEEP))
        tu_goto(TU_JAM);
}
static void tu_home_tap(void)
{
    if (tu.scr == TU_DEEP) {                            /* DEEP: back to the SOUND face, or JAM when it was not from there */
        if (tu.prev == TU_SOUND)
            tu_deep_leave();
        else
            tu_goto(TU_JAM);
        return;
    }
    tu_tap(B_HOME);
}
static void tu_draw_screen(void)
{
    tu_pbg = TC_BG;
    tu_pline = TC_SURF;
    tu_pq = TC_QUIET;
    switch (tu.scr) {
    case TU_MIX:
        tu_mix_draw();
        break;
    case TU_SOUND:
        tu_sound_draw();
        break;
    case TU_STEP:
        tq_step_draw();
        break;
    case TU_NOTES:
        tq_notes_draw();
        break;
    case TU_DRUMS:
        tq_drums_draw();
        break;
    case TU_SONG:
        tu_song_draw();
        break;
    case TU_FX: case TU_SPACE: case TU_ARP: case TU_CLOCK: case TU_CLOCK2: case TU_MOTION:
        tu_pages_draw();
        break;
    case TU_SONGS:
        tu_songs_draw();
        break;
    default:
        tu_jam_draw();
        break;
    }
}
