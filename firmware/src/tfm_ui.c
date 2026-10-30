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

static const char *tu_scale_full(uint32_t v);           /* (tfm_dice.c) */
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
static void jl_input(uint32_t pressed, uint32_t oct);   /* (tfm_live.c) */
static struct { uint8_t held, n, sc[24]; } jw;          /* CHAIN (F#5 held): the taps */
static int jl_held(uint32_t idx);
static uint8_t prf_rate, prf_cap, prf_mut;              /* (tfm_live.c: PERFORM) */
static uint32_t jl_bars_left(void);
static void tu_jam_draw(void);
static void tb_draw(void);                              /* (tfm_seq.c: BARS) */
static void tb_input(uint32_t pressed, uint32_t oct, uint32_t busy);
static void tb_edit(void);
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
    uint8_t lvl[NTRK];                                  /* JAM: the level meter's LEDs lit (0..14) */
    int32_t prev_peak[NTRK];
    uint8_t mlv[NTRK];                                  /* round 6: the meters, 1/16 LED (0..224 = 14 LEDs, -48..0 dBFS) */
    uint8_t mpk[NTRK];                                  /* the peak dot: an LED 1..14, held about 1 s */
    uint32_t mpt[NTRK];
    int32_t raw[NTRK];                                  /* the track's peak this frame (Q15), read and reset once a frame */
    uint32_t mms;
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
static uint32_t tq_fit_sig;                             /* NOTES: the view the rows were last centred for (opening NOTES: none) */
static uint8_t tsg_copy = 0xFF;                         /* SONGS: GLO pressed on this slot: the next GLO copies it onto the one picked */
static void tu_goto(uint32_t scr)
{
    if (scr >= TU_N)
        return;
    if (scr != TU_SONGS)
        tsg_copy = 0xFF;
    tq_grab = 0;
    jam_silent = 0;
    tq_fit_sig = 0xFFFFFFFFu;
    if (tu.scr != scr)
        tu.prev = tu.scr;
    tu.scr = (uint8_t)scr;
    memset(tu.sig, 0xFF, sizeof tu.sig);
    ui.layer = 0;
    ui.menu = 0;
    ui.force = 1;
    song.seq_mode = scr == TU_NOTES || scr == TU_DRUMS;
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
/* round 4 item 3 (tfm-spec/screens/JamA.dc.html, the live meters of JamB): the scoreboard, the band as four cabinets, the scenes
 * as insert lamps. The display is a 5 x 7 dot matrix as TILT's timer and splash: lit dots amber, the unlit ones faint */
static const uint8_t DMD_G[41][7] = {
    {0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E}, {0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E}, {0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F},
    {0x1F, 0x02, 0x04, 0x02, 0x01, 0x11, 0x0E}, {0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02}, {0x1F, 0x10, 0x1E, 0x01, 0x01, 0x11, 0x0E},
    {0x06, 0x08, 0x10, 0x1E, 0x11, 0x11, 0x0E}, {0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08}, {0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E},
    {0x0E, 0x11, 0x11, 0x0F, 0x01, 0x02, 0x0C},                                                    /* 0..9 */
    {0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11}, {0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E}, {0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E},
    {0x1E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1E}, {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F}, {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10},
    {0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0F}, {0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11}, {0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E},
    {0x07, 0x02, 0x02, 0x02, 0x02, 0x12, 0x0C}, {0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11}, {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F},
    {0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11}, {0x11, 0x11, 0x19, 0x15, 0x13, 0x11, 0x11}, {0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E},
    {0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10}, {0x0E, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0D}, {0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11},
    {0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E}, {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04}, {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E},
    {0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04}, {0x11, 0x11, 0x11, 0x15, 0x15, 0x15, 0x0A}, {0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11},
    {0x11, 0x11, 0x0A, 0x04, 0x04, 0x04, 0x04}, {0x1F, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1F},                  /* A..Z */
    {0x00, 0x00, 0x10, 0x00, 0x10, 0x00, 0x00},                                                    /* : (one column) */
    {0x01, 0x01, 0x02, 0x04, 0x08, 0x10, 0x10}, {0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00},       /* / - */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10}, {0, 0, 0, 0, 0, 0, 0}};                           /* . and a space */
static uint32_t dmd_ix(char c)
{
    if (c >= 'a' && c <= 'z')
        c = (char)(c - 32);
    if (c >= '0' && c <= '9')
        return (uint32_t)(c - '0');
    if (c >= 'A' && c <= 'Z')
        return 10u + (uint32_t)(c - 'A');
    return c == ':' ? 36u : c == '/' ? 37u : c == '.' ? 39u : c == ' ' ? 40u : 38u;
}
static uint32_t dmd_cols(uint32_t g) { return g == 36u || g == 39u ? 1u : g == 40u ? 3u : 5u; }
static int32_t dmd_w(const char *s, int32_t pitch)
{
    int32_t w = 0;
    for (; *s; s++)
        w += (int32_t)(dmd_cols(dmd_ix(*s)) + 1u) * pitch;
    return w ? w - pitch : 0;
}
/* a string in dots, its top left at x y: a dot is pitch - 1 square, pitch apart; the unlit ones in `dim` */
static int32_t dmd_str(int32_t x, int32_t y, int32_t pitch, const char *s, uint16_t lit, uint16_t dim)
{
    for (; *s; s++) {
        uint32_t g = dmd_ix(*s), cols = dmd_cols(g), c, r;
        for (c = 0; c < cols; c++)
            for (r = 0; r < 7u; r++) {
                uint32_t bit = g == 36u || g == 39u ? (DMD_G[g][r] >> 4) & 1u : (DMD_G[g][r] >> (4u - c)) & 1u;
                tu_rect(x + (int32_t)c * pitch, y + (int32_t)r * pitch, pitch - 1, pitch - 1, bit ? lit : dim);
            }
        x += (int32_t)(cols + 1u) * pitch;
    }
    return x;
}
#define DMD_DIM RGB(44, 31, 12)
#define DMD_ROW 30u
static void tu_jam_box(int32_t x, int32_t y, char l, int on, int played)        /* a chain box: its scene's letter */
{
    char b[2] = {l, 0};
    uint16_t c = on ? TC_AMBER : played ? RGB(150, 110, 40) : RGB(150, 110, 40);
    tu_rect(x, y, 14, 1, c); tu_rect(x, y + 13, 14, 1, c); tu_rect(x, y, 1, 14, c); tu_rect(x + 13, y, 1, 14, c);
    if (on)
        tu_rect(x + 1, y + 1, 12, 12, TC_AMBER);
    tu_text_c(x + 7, y + 7, &AF_TS, b, on ? RGB(10, 7, 4) : played ? RGB(255, 217, 160) : RGB(150, 110, 40), on ? TC_AMBER : RGB(10, 7, 4));
}
static void tu_jam_dmd(void)                            /* y 22 .. 79: the scoreboard */
{
    char t[8], b[28], r[12];
    const char *u;
    uint32_t bar, tot, shown = chain.running && chain.config.row[chain.row].slot[0] < NSCN ? chain.config.row[chain.row].slot[0] : tu_scene_cur();
    int32_t x = 134;
    int writing = jw.held;
    tu_rrect(5, 25, 230, 52, 6, RGB(42, 32, 18), TC_BG);
    tu_rrect(7, 27, 226, 48, 5, RGB(10, 7, 4), RGB(42, 32, 18));
    tu_timer_text(t);
    tu_bars(&bar, &tot);
    if (prf_on()) {                                     /* PERFORM: the display turns violet */
        static const char *const RN[5] = {"1/8", "1/8T", "1/16", "1/16T", "1/32"}, *const MN[3] = {"PATTERN", "SOUND", "BOTH"};
        dmd_str(14, 36, 3, "PERFORM", RGB(170, 130, 255), RGB(34, 26, 56));
        str_cpy(b, "ROLL ", 8);
        str_cpy(b + 5, RN[prf_rate % 5u], 8);
        tu_text(142, 36, &AF_TS, b, RGB(190, 160, 255), RGB(10, 7, 4));
        str_cpy(b, "CAPTURE ", 10);
        fmt_int(b + 8, 1 << prf_cap);
        str_cpy(b + str_len(b), prf_cap ? " BARS" : " BAR", 6);
        tu_text(142, 52, &AF_TS, b, RGB(110, 230, 150), RGB(10, 7, 4));
        str_cpy(b, "MUTATE ", 9);
        str_cpy(b + 7, MN[prf_mut % 3u], 9);
        tu_text(142, 68, &AF_TS, b, RGB(255, 170, 60), RGB(10, 7, 4));
        return;
    }
    if (writing) {                                      /* CHAIN held: the taps, spelled out */
        uint32_t i;
        dmd_str(14, 38, 4, "CHAIN", RGB(150, 50, 40), RGB(36, 20, 12));
        for (i = 0; i < 6u; i++) {
            uint32_t first = jw.n > 5u ? jw.n - 5u : 0u, j = first + i;
            if (j < jw.n)
                tu_jam_box(x + (int32_t)i * 16, 30, (char)('A' + jw.sc[j]), 0, 1);
            else if (j == jw.n)
                tu_jam_box(x + (int32_t)i * 16, 30, ' ', 0, 0);
        }
        tu_text(x, 55, &AF_TS, "LET GO: SET CHAIN", RGB(255, 110, 70), RGB(10, 7, 4));
        tu_text(x, 67, &AF_TS, "OCT- UNDO A TAP", RGB(230, 150, 60), RGB(10, 7, 4));
        return;
    }
    if (t[0]) {                                         /* the timer big (pitch 4) with the dots under it */
        uint32_t left = tm.left, total = (uint32_t)tilt_timer_min * 60u, lit, i;
        dmd_str(14, 32, 4, t, TC_AMBER, DMD_DIM);
        lit = total ? (left * DMD_ROW + total - 1u) / total : 0u;
        if (lit > DMD_ROW)
            lit = DMD_ROW;
        for (i = 0; i < DMD_ROW; i++)                   /* the dots under it drain with the time (red for the last minute) */
            tu_rect(14 + (int32_t)i * 4, 68, 3, 3, i < lit ? (left <= 60u ? TC_REC : TC_AMBER) : RGB(58, 42, 16));
    } else {                                            /* no TIMER: the song's name and its scene instead */
        char nm[16];
        project_cur_name(nm);
        nm[9] = 0;
        dmd_str(14, 31, 2, nm[0] ? nm : "UNTITLED", TC_AMBER, DMD_DIM);
        str_cpy(b, "SCENE ", 8);
        b[6] = shown < NSCN ? (char)('A' + shown) : '-';
        b[7] = 0;
        dmd_str(14, 49, 2, b, TC_AMBER, DMD_DIM);
    }
    if (chain.running) {                                /* the chain as boxes: the playing one lit, the played outlined */
        uint32_t i, n = 0, cur = 0, k, rep, first;
        char c[28];
        for (i = 0; i < chain.config.count; i++) {
            if (i == chain.row)
                cur = n + (uint32_t)chain.config.row[i].repeat - chain.remaining;
            n += chain.config.row[i].repeat;
        }
        first = cur > 2u ? cur - 2u : 0u;
        if (first + 6u > n)
            first = n > 6u ? n - 6u : 0u;
        for (k = 0; k < 6u && first + k < n; k++) {
            uint32_t idx = first + k, acc = 0;
            for (i = 0; i < chain.config.count; i++) {
                acc += chain.config.row[i].repeat;
                if (idx < acc)
                    break;
            }
            rep = i < chain.config.count ? chain.config.row[i].slot[0] : 0u;
            tu_jam_box(x + (int32_t)k * 16, 30, rep < NSCN ? (char)('A' + rep) : '-', idx == cur, idx < cur);
        }
        str_cpy(c, "CHAIN ", 8);
        fmt_int(c + 6, (int32_t)cur + 1);
        str_cpy(c + str_len(c), "/", 2);
        fmt_int(c + str_len(c), (int32_t)n);
        str_cpy(c + str_len(c), " \xB7 BAR ", 9);
        fmt_int(c + str_len(c), (int32_t)bar);
        str_cpy(c + str_len(c), "/", 2);
        fmt_int(c + str_len(c), (int32_t)tot);
        tu_text(x, 56, &AF_TS, c, RGB(255, 217, 160), RGB(10, 7, 4));
    } else {                                            /* live: SCENE B > C, and C IN 2 BARS . LIVE */
        char c[28] = "SCENE A";
        c[6] = shown < NSCN ? (char)('A' + shown) : '-';
        if (tsc.cue < NSCN) {
            str_cpy(c + 7, " > A", 5);
            c[10] = (char)('A' + tsc.cue);
        }
        tu_text(x, 36, &AF_TM, c, TC_AMBER, RGB(10, 7, 4));
        if (tsc.cue < NSCN) {
            c[0] = (char)('A' + tsc.cue);
            str_cpy(c + 1, " IN ", 5);
            fmt_int(c + 5, (int32_t)jl_bars_left());
            str_cpy(c + str_len(c), jl_bars_left() == 1u ? " BAR " : " BARS ", 8);
        } else {
            str_cpy(c, "BAR ", 6);
            fmt_int(c + 4, (int32_t)bar);
            str_cpy(c + str_len(c), "/", 2);
            fmt_int(c + str_len(c), (int32_t)tot);
            str_cpy(c + str_len(c), " ", 2);
        }
        str_cpy(c + str_len(c), "\xB7 LIVE", 8);
        tu_text(x, 54, &AF_TS, c, RGB(255, 217, 160), RGB(10, 7, 4));
    }
    param_format(&TP[P_ROOT], TSEL->p[P_ROOT], r, &u);
    str_cpy(b, r, 4);
    str_cpy(b + str_len(b), " ", 2);
    str_cpy(b + str_len(b), tu_scale_full(TSEL->p[P_SCALE]), 10);
    str_cpy(b + str_len(b), " ", 2);
    fmt_int(b + str_len(b), song.g[G_BPM]);
    tu_text(x, 68, &AF_TS, b, RGB(185, 138, 58), RGB(10, 7, 4));
}
/* ---- the cabinets: each instrument in its own material (tfm-spec/screens/Sound*.dc.html, item 5) ---- */
typedef struct { uint16_t top, bot, ink; uint8_t tex; } tu_mat_t;       /* the face's two tones, its ink, 1 hairlines / 2 horizontal rules */
static const tu_mat_t TU_MAT[TI_N] = {
    {RGB(29, 29, 32), RGB(29, 29, 32), RGB(238, 238, 238), 0},          /* THUMP LAB: dark steel */
    {RGB(217, 220, 223), RGB(169, 173, 178), RGB(20, 20, 20), 1},       /* SQUELCH: brushed aluminium */
    {RGB(27, 29, 31), RGB(20, 21, 23), RGB(230, 242, 234), 2},          /* WOBBLER: a checker plate */
    {RGB(185, 179, 167), RGB(164, 158, 146), RGB(42, 38, 32), 0},       /* CHORDETTE: cream plastic */
    {RGB(36, 80, 143), RGB(23, 58, 107), RGB(220, 232, 255), 1},        /* RAZOR: blue hammerite */
    {RGB(59, 42, 92), RGB(248, 177, 149), RGB(255, 255, 255), 0},       /* HAZE: a dusk gradient */
    {RGB(233, 229, 220), RGB(214, 209, 197), RGB(59, 53, 80), 0},       /* PIXEL: a grey handheld */
    {RGB(26, 26, 26), RGB(20, 20, 20), RGB(232, 226, 214), 2},          /* TINE: black vinyl */
    {RGB(31, 95, 92), RGB(13, 53, 51), RGB(232, 211, 155), 0},          /* GLINT: teal */
    {RGB(74, 50, 30), RGB(42, 29, 18), RGB(232, 214, 176), 1},          /* PLUCK: wood */
    {RGB(58, 68, 82), RGB(28, 34, 43), RGB(231, 236, 242), 0},          /* VOX: slate */
    {RGB(42, 28, 18), RGB(26, 17, 11), RGB(239, 230, 207), 0},          /* WHEELHOUSE: dark walnut */
};
static const tu_mat_t TU_MAT_NONE = {RGB(26, 22, 16), RGB(18, 14, 8), RGB(246, 236, 216), 0};
static uint16_t tu_mat_at(const tu_mat_t *m, uint32_t y, uint32_t h)    /* the body's colour y rows down */
{
    return tu_mix(m->top, m->bot, h ? y * 100u / h : 0u);
}
static void tu_cab_rec(int32_t cx, int32_t cy, uint32_t k, uint16_t under)     /* the REC dot: as the header's */
{
    int armed = (int)((song.rec >> k) & 1u);
    if (armed && song.playing) {
        tu_disc(cx, cy, 4, TC_REC);
    } else if (armed) {
        tu_disc(cx, cy, 4, TC_REC);
        tu_disc(cx, cy, 2, under);
    } else {
        tu_disc(cx, cy, 2, tu_mix(under, TC_REC, 30));
    }
}
static void tu_cut(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t c)       /* a block with its corners cut (no text cell for the lint) */
{
    tu_rect(x + 2, y, w - 4, 1, c);
    tu_rect(x + 1, y + 1, w - 2, 1, c);
    tu_rect(x, y + 2, w, h - 4, c);
    tu_rect(x + 1, y + h - 2, w - 2, 1, c);
    tu_rect(x + 2, y + h - 1, w - 4, 1, c);
}
#define CAB_X(c) (5 + 58 * (int32_t)(c))
#define CAB_Y 84
#define CAB_H 89
static void tu_cab_draw(uint32_t c)
{
    const track_t *t = &trk[c];
    int ins = jam_macro(t), mute = t->p[P_MUTE] != 0, sel = c == song.sel;
    const tu_mat_t *m = ins ? &TU_MAT[tfm_ins_of(t) % TI_N] : &TU_MAT_NONE;
    int32_t x0 = CAB_X(c), y0 = CAB_Y, cx = x0 + 32, cy = y0 + 38, pos = jam_pos(t), i, lvl = tu.lvl[c];
    uint16_t top = m->top, bot = m->bot, ink = m->ink, col = TU_TC[c], face, acc, mid;
    tu_mat_t mm;
    char nm[12], v[10], b[4] = {'T', (char)('1' + c), 0, 0};
    const char *unit = "", *name;
    if (mute) {                                         /* dimmed: the material a third as bright, the ink DIM */
        mm = *m;
        mm.top = tu_mix(TC_BG, top, 30);
        mm.bot = tu_mix(TC_BG, bot, 30);
        mm.ink = TC_DIM;
        m = &mm;
        top = mm.top; bot = mm.bot; ink = TC_DIM;
        col = TC_DIM;
    }
    if (sel)                                            /* the selected track (ALGORITHM): outlined white */
        tu_cut(x0 - 2, y0 - 2, 58, CAB_H + 4, TC_TEXT);
    tu_cut(x0, y0, 54, CAB_H, bot);
    tu_cut(x0, y0, 54, 40, top);
    for (i = 8; i < CAB_H - 8; i += 4)                  /* the gradient, in bands of four rows */
        tu_rect(x0, y0 + i, 54, 4, tu_mat_at(m, (uint32_t)i + 2u, CAB_H));
    if (m->tex == 1) {                                  /* hairlines: brushed metal */
        for (i = 3; i < 54; i += 4)
            tu_rect(x0 + i, y0 + 8, 1, CAB_H - 16, tu_mix(tu_mat_at(m, 40, CAB_H), ink, 9));
    } else if (m->tex == 2) {                           /* rules: a plate */
        for (i = 10; i < CAB_H - 8; i += 8)
            tu_rect(x0, y0 + i, 54, 1, tu_mix(tu_mat_at(m, (uint32_t)i, CAB_H), ink, 7));
    }
    tu_cut(x0, y0, 54, 4, col);                         /* the track colour strip */
    mid = tu_mat_at(m, 38, CAB_H);
    name = ins ? tfm_ins_name(t) : tu_inst_name(t);
    str_cpy(nm, name, 12);
    if (text_w(&AF_TS, nm) > 50)
        str_cpy(nm, ins ? tfm_ins_tag(t) : "--", 12);
    tu_text_c(x0 + 29, y0 + 12, &AF_TS, nm, ink, tu_mat_at(m, 12, CAB_H));
    for (i = 0; i < 14; i++)                            /* the level meter, the left edge: 14 LEDs, green to red */
        tu_rect(x0 + 3, y0 + 8 + (13 - i) * 5, 3, 3,
                i < lvl ? (i >= 13 ? TC_REC : i >= 10 ? TC_AMBER : TC_GOOD) :
                i + 1 == (int32_t)tu.mpk[c] ? (i >= 13 ? TC_REC : i >= 10 ? TC_AMBER : TC_GOOD) :      /* (the peak dot) */
                tu_mix(tu_mat_at(m, (uint32_t)(8 + (13 - i) * 5), CAB_H), ink, 12));
    if (!prf_on()) {                                    /* (PERFORM: the dial gives way to its ON / MUTED panel) */
    acc = ins ? TFM_INS[tfm_ins_of(t) % TI_N].col[2] : TC_AMBER;
    if (mute)
        acc = TC_DIM;
    tu_dial(cx, cy, 19, 16, (uint32_t)(pos * 1000 / 127), acc, tu_mix(mid, ink, 18));
    face = tu.flash[c] && !mute ? tu_mix(tu_mix(mid, TC_BG, 35), ink, 40) : tu_mix(mid, TC_BG, mute ? 10 : 35);
    tu_disc(cx, cy, 15, face);                          /* (a note flashes the dial) */
    if (mute) {
        tu_text_c(cx, cy, &AF_TS, "MUTED", TC_TEXT, face);
    } else if (ins) {
        tfm_macro_text(t, tfm_jam_m(t), v, &unit);
        tu_text_c(cx, cy, &AF_TM, v, TC_TEXT, face);
    } else {
        fmt_int(v, pos * 100 / 127);
        tu_text_c(cx, cy, &AF_TM, v, TC_TEXT, face);
    }
    str_cpy(nm, ins ? TFM_INS[tfm_ins_of(t)].mac[tfm_jam_m(t)].name : "LEVEL", 12);
    tu_text_c(x0 + 29, y0 + 64, &AF_TS, nm, ink, tu_mat_at(m, 64, CAB_H));
    tu_cut(x0 + 6, y0 + 72, 17, 12, col);                                              /* T1..T4 on a chip of the track colour */
    tu_text_c(x0 + 14, y0 + 78, &AF_TS, b, TC_BG, col);
    }
    tu_cab_rec(x0 + 45, y0 + 79, c, tu_mat_at(m, 79, CAB_H));
    if (prf_on()) {                                     /* PERFORM: an ON / MUTED panel over it (M1..M4 on the black keys) */
        uint16_t pc = mute ? TC_DIM : TU_TC[c], dk = mute ? RGB(12, 10, 8) : tu_mix(RGB(10, 8, 6), TU_TC[c], 6);
        tu_cut(x0 + 3, y0 + 8, 48, 76, pc);
        tu_cut(x0 + 5, y0 + 10, 44, 72, dk);
        tu_text_c(x0 + 27, y0 + 46, mute ? &AF_TM : &AF_TL, mute ? "MUTED" : "ON", mute ? RGB(150, 140, 120) : TC_TEXT, dk);
    }
}
static void tu_jam_body(void)                           /* y 79 .. 177: the four cabinets */
{
    uint32_t c;
    for (c = 0; c < NTRK; c++)
        tu_cab_draw(c);
}
/* ---- the scenes A..H as pinball insert lamps (the arrow shape) ---- */
static void tu_lamp(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t c)     /* the arrow, x y its top left: a point above a block */
{
    int32_t r, pt = h * 38 / 100;
    for (r = 0; r < h; r++) {
        int32_t hw = r < pt ? (w / 2) * (r + 1) / (pt + 1) : w / 2;
        tu_rect(x + w / 2 - hw, y + r, 2 * hw + (w & 1 ? 1 : 0) + (r >= pt ? 0 : 0), 1, c);
    }
}
static void tu_strip_draw(void)                         /* y 177 .. 215: the 11 black keys as lamps, grouped 3 2 3 2 1 */
{
    static const int16_t X[11] = {7, 26, 45, 69, 88, 110, 129, 148, 172, 192, 212};
    static const char *const KN[11] = {"F#3", "G#3", "A#3", "C#4", "D#4", "F#4", "G#4", "A#4", "C#5", "D#5", "F#5"};
    uint32_t s, cur = chain.running && chain.config.row[chain.row].slot[0] < NSCN ? chain.config.row[chain.row].slot[0] : tu_scene_cur();
    uint32_t cue = tsc.cue < NSCN ? tsc.cue : 0xFFu, blink = (ui.frame >> 3) & 1u;
    if (prf_on()) {                                     /* PERFORM: M1 M2 M3 M4 FILL CAP MUT on the keys */
        static const char *const PN[7] = {"M1", "M2", "M3", "M4", "FIL", "CAP", "MUT"};
        for (s = 0; s < 11u; s++) {
            int32_t x = X[s];
            tu_text_c(x + 9, 208, &AF_TS, KN[s], TC_QUIET, TC_BG);
            if (s < 7u) {
                uint16_t col = s < 4u ? TU_TC[s] : s == 4u ? RGB(120, 96, 200) : s == 5u ? RGB(80, 220, 140) : RGB(255, 170, 60);
                int lit = s < 4u ? !trk[s].p[P_MUTE] : s == 4u ? jl_held(4) : 0;
                tu_disc(x + 9, 190, 8, lit ? col : tu_mix(TC_BG, col, 30));
                tu_text_c(x + 9, 190, &AF_TS, PN[s], lit ? TC_BG : tu_mix(TC_BG, col, 85), lit ? col : tu_mix(TC_BG, col, 30));
            } else {
                tu_rect(x, 180, 18, 1, RGB(42, 32, 18)); tu_rect(x, 199, 18, 1, RGB(42, 32, 18));
                tu_rect(x, 180, 1, 20, RGB(42, 32, 18)); tu_rect(x + 17, 180, 1, 20, RGB(42, 32, 18));
            }
        }
        return;
    }
    for (s = 0; s < 11u; s++) {
        int32_t x = X[s], y = 178;
        tu_text_c(x + 9, 208, &AF_TS, KN[s], s < 8u && s == cur ? TC_AMBER : TC_QUIET, TC_BG);
        if (s < 8u) {
            int on = s == cur, used = tu_scene_used(s);
            char l[2] = {(char)('A' + s), 0};
            uint16_t body = on ? TC_AMBER : used ? RGB(122, 90, 34) : RGB(42, 32, 18);
            if (on)
                tu_lamp(x - 2, y - 2, 22, 24, RGB(110, 72, 14));
            tu_lamp(x, y, 18, 20, body);
            if (s == cue && blink) {                    /* queued: outlined, blinking */
                tu_lamp(x - 1, y - 1, 20, 22, RGB(255, 226, 168));
                tu_lamp(x + 2, y + 3, 14, 15, body);
            }
            tu_text_c(x + 9, y + 12, &AF_TM, l, on ? TC_BG : used ? TC_TEXT : TC_DIM, body);
        } else if (s == 8u) {
            tu_rect(x, y + 2, 18, 1, RGB(42, 32, 18)); tu_rect(x, y + 21, 18, 1, RGB(42, 32, 18));
            tu_rect(x, y + 2, 1, 20, RGB(42, 32, 18)); tu_rect(x + 17, y + 2, 1, 20, RGB(42, 32, 18));
        } else {
            int lit = s == 9u ? jl_held(9) : (jw.held || chain.running);
            uint16_t col = s == 9u ? RGB(120, 96, 200) : RGB(220, 80, 70);
            tu_cut(x, y + 2, 18, 20, lit ? col : tu_mix(TC_BG, col, 35));
            tu_text_c(x + 9, y + 12, &AF_TS, s == 9u ? "FIL" : "CHN", lit ? TC_BG : tu_mix(TC_BG, col, 80), lit ? col : tu_mix(TC_BG, col, 35));
        }
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
    uint32_t cur = tu_scene_cur(), bar, tot, i, sig = 0, dsig;
    char tn[24], nmb[16];
    project_cur_name(nmb);
    tu_bars(&bar, &tot);
    tn[0] = 'T';
    tn[1] = (char)('1' + song.sel % NTRK);
    tn[2] = 0;
    str_cpy(tn + 2, " \xB7 ", 6);
    str_cpy(tn + str_len(tn), tu_inst_name(&trk[song.sel % NTRK]), 14);
    {   /* the header: the selected track, "T3 \xB7 SQUELCH" */
        static char hl[24];
        str_cpy(hl, prf_on() ? "HOLD ARP \xB7 PERFORM" : jw.held ? "HOLD F#5 \xB7 CHAIN" : tn, sizeof hl);
        tu_head(-1, hl, 0, 0, 1);
    }
    for (i = 0; i < NTRK; i++)                          /* (the meters: tu_meter_poll, in dB) */
        tu.lvl[i] = (uint8_t)(tu.mlv[i] / 16u);
    dsig = (tm.on ? tm.left * 31u : 0u) + (uint32_t)tilt_timer_min * 977u + bar * 7u + tot * 131u + (uint32_t)song.g[G_BPM] * 4099u +
           (uint32_t)TSEL->p[P_ROOT] * 17u + (uint32_t)TSEL->p[P_SCALE] * 19u + cur * 6151u + (chain.running ? chain.row + 1u : 0u) * 53u +
           str_hash(5u, nmb) + (uint32_t)song.sel * 2999u + (uint32_t)jw.held * 977u + jw.n * 31u + (tsc.cue < NSCN ? 1u + tsc.cue * 13u + jl_bars_left() * 101u : 0u) +
           (chain.running ? 1u + chain.row * 17u + chain.remaining * 3u : 0u) + (prf_on() ? 11u + prf_rate * 7u + prf_cap * 31u + prf_mut * 131u : 0u);
    tu_zone(1, TU_HY, 57, TC_BG, dsig, tu_jam_dmd);
    for (i = 0; i < NTRK; i++) {
        sig = sig * 131u + (uint32_t)jam_pos(&trk[i]) * 7u + (uint32_t)(trk[i].p[P_MUTE] != 0) * 3u + trk[i].eng_req +
              (uint32_t)tfm_ins_of(&trk[i]) * 257u + (uint32_t)tu.flash[i] * 1031u + (uint32_t)tu.lvl[i] * 4001u + (uint32_t)tu.mpk[i] * 37u +
              (uint32_t)((song.rec >> i) & 1u) * 8209u + (uint32_t)prf_on() * 65537u;
    }
    sig = sig * 31u + song.sel * 6151u + (song.playing ? 977u : 0u);
    tu_zone(2, 79, 98, TC_BG, sig, tu_jam_body);
    tu_zone(3, 177, 38, TC_BG, cur * 977u + tu_used_mask() * 5u + (uint32_t)sng_cur * 131u + (tsc.cue < NSCN ? 1u + ((ui.frame >> 3) & 1u) + tsc.cue * 3u : 0u) +
            (chain.running ? 1u + chain.row * 17u : 0u) + (uint32_t)jw.held * 7919u + (uint32_t)jl_held(9) * 4001u + (prf_on() ? 3u + (trk[0].p[P_MUTE] | trk[1].p[P_MUTE] << 1 | trk[2].p[P_MUTE] << 2 | trk[3].p[P_MUTE] << 3) * 31u + (uint32_t)jl_held(4) * 977u : 0u), tu_strip_draw);
    if (prf_on())
        tu_foot(3, "WHITE", "ROLL", "K1", "RATE", "LET GO", "BACK", 0, 0, 0, 0);
    else if (jw.held)
        tu_foot(3, "BLACK", "ADD SCENE", "OCT-", "UNDO TAP", "LET GO", "DONE", 0, 0, 0, 0);
    else
        tu_foot(3, "BLACK", "SCENES", "HOLD F#5", "CHAIN", "HOLD ARP", "PERFORM", 0, 0, 0, 0);
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
        mh = mute ? 0 : h * (int32_t)tu.mlv[c] / 224;      /* (round 6: the dB meter, tu_meter_poll) */
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
              (uint32_t)tu.mlv[i] * 3u;
    if (sig == 0)
        sig = 1;
    tu_zone(2, TU_HY, 62, TC_BG, sig, tu_mix_body);     /* (one pass: the faders reach 215: the band is cut below) */
    tu_zone(3, TU_HY + 62, 62, TC_BG, sig, tu_mix_body);
    tu_zone(4, TU_HY + 124, 69, TC_BG, sig, tu_mix_body);
    tu_foot(3, "K1-K4", "LEVEL", "BLACK KEYS", "MUTE", "HOME", "JAM", 0, 0, 0, 0);
}

/* ================================================================== routing =================================== */
static void tu_draw_screen(void);
/* round 6: the level meters in dB (as Felucca's own, ui_graph.c meter_px): the track's peak read and reset once a frame,
 * 8 log2 of it over -48..0 dBFS on 14 LEDs (green to -12, yellow to -3, red above), up at once, down over about 300 ms, a peak
 * dot held about a second. JAM's cabinets, MIX and the dial's flash on a note all read it */
static uint32_t tu_db16(int32_t a)                       /* |sample| Q15 -> 1/16 LED: 0 at -48 dBFS, 224 at 0 dBFS */
{
    int32_t lg = 0, v;
    if (a < 130)
        return 0;
    while ((a >> lg) > 1)
        lg++;
    v = lg * 8 + (((a << 3) >> lg) & 7);                 /* 8 log2(a): 56 (-48 dB) .. 120 (0 dB) */
    return (uint32_t)clamp((v - 56) * 224 / 64, 0, 224);
}
static void tu_meter_poll(void)
{
    uint32_t i, dt = fm1_ms - tu.mms, fall;
    tu.mms = fm1_ms;
    fall = dt > 1000u ? 224u : dt * 224u / 300u;
    for (i = 0; i < NTRK; i++) {
        track_t *t = &trk[i];
        int32_t p = t->peak;
        uint32_t now;
        t->peak = 0;
        tu.raw[i] = p;
        now = t->p[P_MUTE] ? 0u : tu_db16(p);
        if (now > (uint32_t)tu.prev_peak[i] + 48u)      /* (a jump of three LEDs: a note: the dial flashes) */
            tu.flash[i] = 5;
        tu.prev_peak[i] = (int32_t)now;
        if (now >= tu.mlv[i])
            tu.mlv[i] = (uint8_t)now;
        else
            tu.mlv[i] = (uint8_t)(tu.mlv[i] > fall ? tu.mlv[i] - fall : 0u);
        if ((now + 15u) / 16u >= tu.mpk[i] || fm1_ms - tu.mpt[i] > 1000u) {
            tu.mpk[i] = (uint8_t)((now + 15u) / 16u);
            tu.mpt[i] = fm1_ms;
        }
    }
}
static void tu_draw(void)
{
    tu_meter_poll();
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
    return !tu_classic() && !ui.menu && (tu.scr == TU_SONG || tu.scr == TU_SONGS || tu.scr == TU_NOTES || tu.scr == TU_BARS || tu.scr == TU_JAM);
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
        jl_input(notes, oct);
        for (k = 0; k < 4u; k++)
            if ((s = panel_enc(EN_K1 + k)) != 0) {
                jam_turn(k, s);
                ui.force = 0;
            }
        break;
    case TU_SOUND:
        tu_sound_input(busy);
        break;
    case TU_BARS:
        tb_input(pressed, oct, busy);
        break;
    case TU_NOTES: case TU_DRUMS:
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
    prf_poll();                                         /* (round 5: PERFORM's roll on while its layer is up) */
    if (!busy && (s = panel_enc(EN_ALGO)) != 0) {       /* ALGORITHM: the track, everywhere */
        track_select((uint32_t)clamp((int32_t)song.sel + (s > 0 ? 1 : -1), 0, NTRK - 1));
        tq_adapt();
        tu_sel_until = fm1_ms + 1000u;                  /* (JAM: the track's name big for about a second) */
    }
    if (!busy && tu.scr != TU_SOUND && tu.scr != TU_NOTES && tu.scr != TU_DRUMS && tu.scr != TU_SONG && tu.scr != TU_BARS && (s = panel_enc(EN_PRESET)) != 0)
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
    case B_SEQ:                                         /* NOTES (a drum track: DRUMS); again BARS; again NOTES / DRUMS */
        if (tu.scr == TU_NOTES || tu.scr == TU_DRUMS)
            tu_goto(TU_BARS);
        else
            tu_goto(drum_track(TSEL) ? TU_DRUMS : TU_NOTES);
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
        } else if (tu.scr == TU_BARS) {
            tb_edit();
        } else if (tu.scr != TU_SOUND) {
            tu_goto(TU_SOUND);
            tu.page = 0;
        } else {                                        /* (round 6: EDIT toggles MACROS 1 / 2; DEEP is the MENU's DEEP EDIT) */
            tu.page = (uint8_t)(tu.page ? 0u : 1u);
            ui.force = 1;
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
    if (tu.scr == TU_DEEP) {                            /* DEEP: back to the SOUND face (round 6: always) */
        tu_deep_leave();
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
    case TU_BARS:
        tb_draw();
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
