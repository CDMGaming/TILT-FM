/* TFM-UI phase 4 (tfm-spec/TFM-UI-SPEC.md 3 SOUND, TFM-INSTRUMENTS.md): the instrument faces. EDIT opens the face of the
 * selected track's instrument: its toy, its name in Silkscreen in its colour, a line about it, the preset and
 * `PRESETS < n of m >`, four dials (K1-K4 = macros 1-4, then 5-8), EDIT steps MACROS 1 > MACROS 2 > DEEP (Felucca's
 * own pages for that track). Hold EDIT: the picker of the twelve instruments on the white keys. Each toy redraws its
 * own box, once a frame, and says in its comment what it reacts to. Included from tilt.c after tfm_ui.c. */

/* the face's palette: BG SURF ACC MUTE INK of the instrument (a track with none: TFM AMBER's) */
static struct { uint16_t bg, surf, acc, mute, ink; } fc;
static void tu_face_pal(const tfm_instr_t *in)
{
    if (in) {                                           /* (round 4: the material's colours; the accent is the instrument's own) */
        const tu_mat_t *m = &TU_MAT[(uint32_t)(in - TFM_INS) % TI_N];
        fc.bg = m->top; fc.surf = tu_mix(m->top, m->ink, 12); fc.acc = in->col[2]; fc.mute = tu_mix(m->top, m->ink, 55); fc.ink = m->ink;
    } else {
        fc.bg = TC_BG; fc.surf = TC_SURF; fc.acc = TC_AMBER; fc.mute = TC_QUIET; fc.ink = TC_TEXT;
    }
}
/* ---- what the toys see: per track, a frame at a time ---- */
static struct {
    int32_t prev;                                       /* the level last frame */
    uint8_t nh;                                         /* the notes held last frame */
    uint8_t on;                                         /* a note began this frame */
    uint8_t vel;                                        /* its loudness 0..127 (the jump in the level) */
    uint8_t note;                                       /* the last note that played */
    uint8_t flash;                                      /* frames since: 0 = now (counts down from 8) */
    uint8_t lane[8];                                    /* THUMP: a lane's chip, frames left */
    uint8_t lit[8];                                     /* the toy's own lights, frames left */
    uint16_t ang;                                       /* a spinner, 1/256 turns x 16 */
    int32_t env;                                        /* the level, smoothed */
    uint8_t acc;                                        /* the step now is accented */
    uint8_t amp;                                        /* PLUCK's string: 100 on a note, then dying with DAMP */
} fs[NTRK];
static int32_t tu_sin(uint32_t a256)                    /* sin, a 0..255 round, x 1024 */
{
    static const int16_t Q[65] = {0,25,50,75,100,125,150,175,200,224,249,273,297,321,345,369,392,415,438,460,483,505,526,548,569,590,610,630,650,669,688,706,724,742,759,775,792,807,822,837,851,865,878,891,903,915,926,936,946,955,964,972,980,987,993,999,1004,1009,1013,1016,1019,1021,1023,1024,1024};
    uint32_t q = (a256 >> 6) & 3u, i = a256 & 63u;
    int32_t v = (q & 1u) ? Q[64u - i] : Q[i];
    return q & 2u ? -v : v;
}
static int32_t tu_cos(uint32_t a256) { return tu_sin(a256 + 64u); }
/* each frame (tu_sound_draw, JAM): the notes a track began, its loudness */
static void tu_toy_poll(uint32_t k)
{
    track_t *t = &trk[k];
    int32_t p = t->peak;
    uint32_t l;
    fs[k].on = 0;
    if (t->nheld > fs[k].nh || p > fs[k].prev * 3 / 2 + 900) {
        fs[k].on = 1;
        fs[k].vel = (uint8_t)clamp(p / 90, 40, 127);
        fs[k].flash = 8;
        if (t->nheld)
            fs[k].note = t->held[t->nheld - 1u];
    }
    fs[k].nh = t->nheld;
    fs[k].prev = p;
    fs[k].env += (p - fs[k].env) / 4;
    if (fs[k].flash)
        fs[k].flash--;
    fs[k].acc = 0;
    if (tfm_ins_of(t) == TI_PLUCK) {
        int32_t damp = tfm_macro_get(t, 1);
        fs[k].amp = fs[k].on ? 100u : (uint8_t)((uint32_t)fs[k].amp * (uint32_t)(252 - damp * 36 / 127) / 256u);
    }
    if (song.playing) {                                 /* the step now: its accent, a drum's lanes */
        const step_t *s = &t->step[t->seq_idx % NSTEP];
        uint32_t cur = t->seq_idx;
        static uint16_t seen[NTRK];
        fs[k].acc = (uint8_t)((s->flags & SF_ACCENT) != 0u);
        if (cur != seen[k]) {
            seen[k] = (uint16_t)cur;
            for (l = 0; l < 8u; l++)
                if ((s->hit >> l) & 1u)
                    fs[k].lane[l] = 8;
            if (drum_track(t) && (s->hit & 1u))
                fs[k].on = 1;
        }
    }
    if (drum_track(t) && t->nheld && fs[k].on)           /* a key on a drum track: its note's lane */
        for (l = 0; l < 8u; l++)
            if (t->held[t->nheld - 1u] == DRUM_LANE_NOTE[l])
                fs[k].lane[l] = 8;
    for (l = 0; l < 8u; l++) {
        if (fs[k].lane[l])
            fs[k].lane[l]--;
        if (fs[k].lit[l])
            fs[k].lit[l]--;
    }
}
/* ---- shapes the toys share ---- */
static void tu_glow(int32_t cx, int32_t cy, int32_t r, uint16_t c, uint16_t under, int32_t pct)   /* a halo, then the disc */
{
    if (pct > 8) {
        tu_disc(cx, cy, r + 3, tu_mix(under, c, (uint32_t)pct * 18 / 100));
        tu_disc(cx, cy, r + 1, tu_mix(under, c, (uint32_t)pct * 40 / 100));
    }
}
static void tu_circle(int32_t cx, int32_t cy, int32_t r, uint16_t c) { tu_ring(cx, cy, r, r - 1, c); }
/* ---- round 4 item 5: each instrument is its own hardware (tfm-spec/screens/Sound*.dc.html, after TILT's panels). The drawn
 * controls ARE the K1-K4 macros (MACROS 2: the other four, in the same style) and move live; the live parts are said where
 * they are drawn. The materials are TU_MAT (tfm_ui.c), gradients are bands, wood and metal rules and hairlines ---- */
static int32_t tf_y0 = 22;                              /* (the body is one drawing: both zones draw it, the canvas clips) */
static uint16_t tf_under(int32_t y)                     /* the material's colour at screen row y */
{
    const tu_mat_t *m = &TU_MAT[tfm_ins_of(&trk[song.sel]) % TI_N];
    return tu_mat_at(m, (uint32_t)(y < 22 ? 0 : y - 22), 193u);
}
static void tf_line(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint16_t c, int32_t w) { cv_line_t(x0, y0, x1, y1, tu_c(c), w); }
static void tf_knob(int32_t cx, int32_t cy, int32_t r, uint16_t disc, uint16_t ptr, int32_t pos)
{
    uint32_t a = (160u + (uint32_t)pos * 192u / 127u) & 255u;
    int32_t dx = tu_sin(a), dy = -tu_cos(a);
    tu_disc(cx, cy + 1, r + 1, tu_mix(tf_under(cy), TC_BG, 55));                        /* its shadow */
    tu_disc(cx, cy, r, disc);
    tu_disc(cx - r / 4, cy - r / 4, r * 2 / 3, tu_mix(disc, TC_TEXT, 9));               /* a highlight */
    tf_line(cx + dx * r * 28 / 102400, cy + dy * r * 28 / 102400, cx + dx * r * 88 / 102400, cy + dy * r * 88 / 102400, ptr, 2);
}
static void tf_fader(int32_t cx, int32_t top, int32_t h, uint16_t cap, uint16_t fill, int32_t pos, int glow)
{
    int32_t cy = top + h - h * pos / 127;
    uint16_t un = tf_under(top + h / 2);
    tu_rrect(cx - 2, top, 5, h, 2, tu_mix(un, TC_BG, 60), un);
    if (top + h - cy > 2)
        tu_rrect(cx - 2, cy, 5, top + h - cy, 2, fill, un);
    if (glow > 8)
        tu_rrect(cx - 11, cy - 6, 23, 13, 4, tu_mix(un, cap, glow * 40 / 100), un);
    tu_rrect(cx - 9, cy - 5, 19, 10, 3, cap, un);
    tu_rect(cx - 6, cy, 13, 1, tu_mix(cap, TC_BG, 35));
}
static void tf_label(uint32_t c, int32_t cx, int32_t y, uint32_t m, int showval)    /* "o NAME" in the ink, its value under it */
{
    track_t *t = &trk[song.sel];
    const tfm_macro_t *mc = &TFM_INS[tfm_ins_of(t)].mac[m & 7u];
    char val[12];
    const char *unit;
    int32_t w = text_w(&AF_TM, mc->name), x = cx - (w + 7) / 2;
    uint16_t un = tf_under(y);
    tu_disc(x + 2, y, 2, TU_TC[c & 3u]);
    tu_text(x + 7, y, &AF_TM, mc->name, fc.ink, un);
    if (showval) {
        tfm_macro_text(t, m, val, &unit);
        tu_text_c(cx, y + 11, &AF_TS, val, fc.mute, tf_under(y + 11));
    }
}
/* the four macros of the page as round knobs / faders in one row */
static void tf_row(int32_t cy, int32_t r, int fader, int32_t pitch, int32_t x0, uint16_t disc, uint16_t ptr, int32_t ly)
{
    track_t *t = &trk[song.sel];
    uint32_t c;
    for (c = 0; c < 4u; c++) {
        uint32_t m = (uint32_t)tu.page * 4u + c;
        int32_t cx = x0 + pitch * (int32_t)c, pos = tfm_macro_get(t, m);
        if (fader)
            tf_fader(cx, cy, r, disc, ptr, pos, (int32_t)fs[song.sel].env / 80);
        else
            tf_knob(cx, cy, r, disc, ptr, pos);
        tf_label(c, cx, ly, m, !fader);
    }
}
static void tf_preset(int32_t x, int32_t y, uint16_t c, uint16_t un)        /* "NAME < 1 of 5 >" */
{
    track_t *t = &trk[song.sel];
    uint32_t i = tfm_ins_of(t);
    char b[24], v[8], w[16];
    tfm_snd_label(t, b);
    x = tu_text(x, y, &AF_TM, b, fc.ink, un) + 6;
    fmt_int(v, (int32_t)tfm_snd_of(t) + 1);
    str_cpy(w, " of ", 5);
    fmt_int(w + 4, (int32_t)TFM_INS[i].nsnd);
    tu_arrow(x, y, 1, TC_PRESETS);
    x = tu_text(x + 5, y, &AF_TS, v, TC_PRESETS, un);
    x = tu_text(x, y, &AF_TS, w, TC_PRESETS, un);
    tu_arrow(x + 3, y, 0, TC_PRESETS);
}
static int32_t tf_sw(uint32_t m)                        /* a switch macro's place in its names */
{
    const track_t *t = &trk[song.sel];
    const tfm_macro_t *mc = &TFM_INS[tfm_ins_of(t)].mac[m & 7u];
    return mc->nsw ? (int32_t)tfm_sw_idx(mc, motion_base_value(t, mc->t[0].id)) : 0;
}
static void tf_tiles(int32_t x0, int32_t y, int32_t w, int32_t h, int32_t pitch, uint32_t m, uint16_t on, uint16_t off, uint16_t inkon, uint16_t inkoff)
{
    const tfm_macro_t *mc = &TFM_INS[tfm_ins_of(&trk[song.sel])].mac[m & 7u];
    uint32_t j;
    int32_t cur = tf_sw(m);
    for (j = 0; j < mc->nsw; j++) {
        char nb[6];
        str_cpy(nb, mc->swn ? mc->swn[j] : "-", 6);
        tu_rrect(x0 + (int32_t)j * pitch, y, w, h, 3, (int32_t)j == cur ? on : off, tf_under(y));
        tu_text_c(x0 + (int32_t)j * pitch + w / 2, y + h / 2, &AF_TS, nb, (int32_t)j == cur ? inkon : inkoff, (int32_t)j == cur ? on : off);
    }
}
/* the chord's real name and notes (D M7: "Dm7", D F A C) from the notes it makes */
static void tf_chord_name(uint32_t note, char *name, char *notes)
{
    track_t *t = &trk[song.sel];
    uint8_t ch[CHORD_MAX];
    int32_t r;
    uint16_t mask;
    uint32_t n = chord_make(t, note, ch, &r, &mask), j, set = 0, e = 0;
    char nb[8];
    name[0] = 0;
    notes[0] = 0;
    if (!n)
        return;
    for (j = 0; j < n; j++) {
        char *p = tu_note_name(ch[j], nb);
        uint32_t q = 0;
        set |= 1u << ((ch[j] + 120u - ch[0]) % 12u);
        while (p[q] && p[q] != '-' && !(p[q] >= '0' && p[q] <= '9'))
            q++;
        if (j) notes[e++] = ' ';
        memcpy(notes + e, p, q);
        e += q;
    }
    notes[e] = 0;
    {
        char *p = tu_note_name(ch[0], nb);
        uint32_t q = 0;
        const char *qual = "?";
        while (p[q] && !(p[q] >= '0' && p[q] <= '9') && p[q] != '-')
            q++;
        memcpy(name, p, q);
        name[q] = 0;
        if (set == (1u | 1u << 4 | 1u << 7)) qual = "";
        else if (set == (1u | 1u << 3 | 1u << 7)) qual = "m";
        else if (set == (1u | 1u << 4 | 1u << 7 | 1u << 10)) qual = "7";
        else if (set == (1u | 1u << 4 | 1u << 7 | 1u << 11)) qual = "M7";
        else if (set == (1u | 1u << 3 | 1u << 7 | 1u << 10)) qual = "m7";
        else if (set == (1u | 1u << 5 | 1u << 7)) qual = "sus4";
        else if (set == (1u | 1u << 2 | 1u << 7)) qual = "sus2";
        else if (set == (1u | 1u << 7)) qual = "5";
        else if (set == (1u | 1u << 3 | 1u << 6)) qual = "dim";
        str_cpy(name + q, qual, 6);
    }
}
static void tf_star(int32_t cx, int32_t cy, int32_t s, uint16_t c)
{
    tu_rect(cx, cy - s, 1, 2 * s + 1, c);
    tu_rect(cx - s, cy, 2 * s + 1, 1, c);
    if (s > 3) {
        tu_rect(cx - 1, cy - s / 2, 3, s, c);
        tu_rect(cx - s / 2, cy - 1, s, 3, c);
    }
}
static void tf_ell(int32_t cx, int32_t cy, int32_t rx, int32_t ry, uint16_t c)     /* a filled ellipse */
{
    int32_t dy;
    for (dy = -ry; dy <= ry; dy++) {
        int32_t w = (int32_t)tu_isq((uint32_t)(ry * ry - dy * dy)) * rx / (ry ? ry : 1);
        tu_rect(cx - w, cy + dy, 2 * w + 1, 1, c);
    }
}
static void tf_thump(void)
{
    track_t *t = &trk[song.sel];
    static const uint16_t PC[8] = {RGB(232, 100, 79), RGB(122, 85, 48), RGB(136, 140, 52), RGB(150, 214, 70),
                                   RGB(52, 128, 104), RGB(60, 120, 170), RGB(110, 96, 170), RGB(170, 90, 120)};
    uint32_t l;
    for (l = 0; l < 8u; l++) {
        int32_t x = 8 + (int32_t)(l & 3u) * 57, y = 28 + (int32_t)(l >> 2) * 52, lit = fs[song.sel].lane[l] ? 1 : 0;
        uint16_t pc = lit ? PC[l] : tu_mix(fc.bg, PC[l], 28);
        tu_rrect(x, y, 54, 47, 6, tu_mix(fc.bg, PC[l], 45), fc.bg);                         /* the pad's frame */
        tu_rrect(x + 3, y + 3, 48, 29, 4, pc, tu_mix(fc.bg, PC[l], 45));                  /* its rubber: lit on a hit */
        tu_text_c(x + 27, y + 39, &AF_TS, drum_lane_name(t, l), lit ? PC[l] : tu_mix(fc.bg, PC[l], 80), tu_mix(fc.bg, PC[l], 45));
    }
    tu_rrect(8, 132, 224, 80, 7, fc.surf, fc.bg);
    tu_text(14, 146, &AF_PS, "THUMP", fc.acc, fc.surf);
    tu_text(14, 159, &AF_PS, "LAB", fc.acc, fc.surf);
    {
        char pb[24];
        tfm_snd_label(t, pb);
        tu_text_fit(14, 176, &AF_TM, pb, fc.ink, fc.surf, 66);
    }
    {
        uint32_t c;
        for (c = 0; c < 4u; c++) {
            uint32_t m = (uint32_t)tu.page * 4u + c;
            int32_t cx = 101 + 37 * (int32_t)c;
            tf_knob(cx, 156, 12, RGB(26, 18, 16), TU_TC[c], tfm_macro_get(t, m));
            tf_label(c, cx, 177, m, 1);
        }
    }
}
static void tf_squelch(void)
{
    track_t *t = &trk[song.sel];
    int lit = fs[song.sel].acc && fs[song.sel].flash;
    tu_text(10, 40, &AF_PN, "SQUELCH", fc.ink, tf_under(40));
    tu_text(10, 57, &AF_TS, "ACID BASS \xB7 RUBBER", fc.mute, tf_under(57));
    tf_preset(10, 72, fc.ink, tf_under(72));
    if (lit)
        tu_disc(190, 40, 10, tu_mix(tf_under(40), RGB(255, 59, 47), 40));
    tu_disc(190, 40, 6, lit ? RGB(255, 59, 47) : RGB(120, 30, 28));                 /* the ACCENT lamp: lit on accented notes */
    tu_text(200, 40, &AF_TS, "ACCENT", fc.ink, tf_under(40));
    tf_row(97, 23, 0, 56, 36, RGB(26, 26, 28), RGB(236, 236, 236), 151);
    tu_text_c(120, 196, &AF_TS, "the plate glows brighter as the cutoff opens", fc.mute, tf_under(196));
    (void)t;
}
static void tf_wobbler(void)
{
    track_t *t = &trk[song.sel];
    uint32_t j, lit = (uint32_t)tfm_macro_get(t, 0) * JAM_RING_N / 128u, rate = (uint32_t)tf_sw(1);
    int32_t lfo = (t->lfo_val >> 8), br = clamp(70 + lfo * 28 / 128, 25, 100);
    tu_text(10, 38, &AF_PN, "WOBBLER", RGB(80, 255, 140), tf_under(38));
    tf_preset(10, 54, fc.ink, tf_under(54));
    if (tu.page == 0u) {
        for (j = 0; j < JAM_RING_N; j++) {                                           /* the LED ring = WOB; the lit ones pulse with the LFO */
            int32_t x = 120 + JAM_RING[j][0] * 52 / 1024, y = 92 + JAM_RING[j][1] * 52 / 1024;
            tu_disc(x, y, 2, j < lit ? tu_mix(RGB(20, 50, 30), RGB(80, 255, 140), (uint32_t)br) : RGB(32, 38, 34));
        }
        tf_knob(120, 92, 36, RGB(24, 26, 28), RGB(80, 255, 140), tfm_macro_get(t, 0));
        tu_text_c(120, 142, &AF_TM, "WOB", RGB(80, 255, 140), tf_under(142));
        for (j = 2; j < 4u; j++) {                                                    /* GROWL and CUT: small knobs */
            int32_t cy = j == 2u ? 52 : 118;
            tf_knob(205, cy, 16, RGB(24, 26, 28), TU_TC[j], tfm_macro_get(t, j));
            tu_disc(186, cy + 24, 2, TU_TC[j]);
            tu_text(190, cy + 24, &AF_TS, TFM_INS[TI_WOBBLER].mac[j].name, fc.ink, tf_under(cy + 24));
        }
        for (j = 0; j < 6u; j++) {                                                    /* the RATE tiles: the lit one is the rate */
            char nb[6];
            str_cpy(nb, TFM_INS[TI_WOBBLER].mac[1].swn ? TFM_INS[TI_WOBBLER].mac[1].swn[j] : "-", 6);
            tu_rrect(36 + (int32_t)j * 29, 166, 26, 17, 3, j == rate ? RGB(80, 255, 140) : RGB(26, 44, 34), tf_under(166));
            tu_text_c(49 + (int32_t)j * 29, 175, &AF_TS, nb, j == rate ? RGB(18, 28, 22) : RGB(120, 200, 150), j == rate ? RGB(80, 255, 140) : RGB(26, 44, 34));
        }
        tu_text_c(120, 192, &AF_TS, "K2 RATE", fc.mute, tf_under(192));
    } else {
        tf_row(100, 17, 0, 56, 36, RGB(24, 26, 28), RGB(80, 255, 140), 140);
    }
}
static void tf_chordette(void)
{
    track_t *t = &trk[song.sel];
    char name[10], notes[24], cn[12];
    const char *unit;
    uint32_t j, cur = tfm_snd_of(t), first;
    tu_rrect(8, 28, 224, 84, 4, RGB(90, 84, 72), tf_under(60));
    tu_rrect(11, 31, 218, 78, 3, RGB(14, 34, 24), RGB(90, 84, 72));                  /* the LCD */
    tu_text(18, 40, &AF_PS, "CHORDETTE", RGB(150, 236, 190), RGB(14, 34, 24));
    if (t->nheld || fs[song.sel].env > 400)
        tf_chord_name(fs[song.sel].note, name, notes);
    else
        tf_chord_name(60, name, notes), notes[0] = 0;
    tu_text(18, 68, &AF_PL, t->nheld || fs[song.sel].env > 400 ? name : "---", RGB(150, 236, 190), RGB(14, 34, 24));
    tfm_macro_text(t, 0, cn, &unit);
    {
        char b[36];
        str_cpy(b, cn, 12);
        if (notes[0]) {
            str_cpy(b + str_len(b), " \xB7 ", 5);
            str_cpy(b + str_len(b), notes, 16);
        }
        tu_text(18, 97, &AF_TS, b, RGB(110, 190, 150), RGB(14, 34, 24));
    }
    first = cur >= 4u ? cur - 3u : 0u;
    for (j = 0; j < 5u && first + j < TFM_INS[TI_CHORDETTE].nsnd; j++) {             /* the preset list; the one now lit */
        int32_t y = 44 + (int32_t)j * 13;
        int on = first + j == cur;
        if (on)
            tu_rect(124, y - 6, 100, 12, RGB(150, 236, 190));
        tu_text(128, y, &AF_TS, TFM_INS[TI_CHORDETTE].snd[first + j].name, on ? RGB(14, 34, 24) : RGB(110, 190, 150), on ? RGB(150, 236, 190) : RGB(14, 34, 24));
    }
    tf_row(142, 16, 0, 56, 36, RGB(28, 26, 24), RGB(236, 232, 224), 168);
}
static void tf_razor(void)
{
    tu_text(10, 36, &AF_PN, "RAZOR", RGB(230, 244, 255), tf_under(36));
    tu_text(88, 36, &AF_TM, "SYNC LEAD", RGB(110, 220, 255), tf_under(36));
    tf_preset(10, 52, fc.ink, tf_under(52));
    tf_row(73, 98, 1, 54, 33, RGB(150, 232, 255), RGB(70, 190, 230), 182);
}
static void tf_haze(void)
{
    track_t *t = &trk[song.sel];
    uint32_t j;
    for (j = 0; j < 4u; j++)                                                         /* a glass panel */
        tu_rect(11 + (int32_t)j, 29 + (int32_t)j, 218 - 2 * (int32_t)j, 1, tu_mix(tf_under(29), TC_TEXT, 24 - 5 * j));
    tu_rect(11, 29, 1, 160, tu_mix(tf_under(100), TC_TEXT, 22));
    tu_rect(228, 29, 1, 160, tu_mix(tf_under(100), TC_TEXT, 22));
    tu_rect(11, 188, 218, 1, tu_mix(tf_under(188), TC_TEXT, 22));
    tu_text(20, 46, &AF_PN, "haze", RGB(255, 255, 255), tf_under(46));
    tf_preset(120, 44, fc.ink, tf_under(44));
    tf_row(70, 90, 1, 50, 42, RGB(250, 250, 255), RGB(235, 235, 245), 172);
    (void)t;
}
static void tf_pixel(void)
{
    track_t *t = &trk[song.sel];
    uint32_t j, w = (uint32_t)tf_sw(1);
    int32_t x, y0 = 70, lastx = 22, lasty = 0;
    char b[24], v[12];
    const char *unit;
    tu_rrect(11, 29, 142, 98, 8, RGB(88, 88, 96), tf_under(70));
    tu_rrect(19, 34, 125, 76, 3, RGB(134, 168, 40), RGB(88, 88, 96));                 /* the screen: the WAVE, drawn */
    for (x = 0; x < 110; x++) {
        int32_t y;
        uint32_t ph = (uint32_t)x * 16u / 110u;
        if (w < 3u)
            y = (ph * 100u / 16u) % 100u < (w == 0u ? 50u : w == 1u ? 25u : 12u) ? 0 : 36;
        else if (w == 3u)
            y = ph < 8u ? (int32_t)ph * 36 / 8 : (int32_t)(16u - ph) * 36 / 8;
        else
            y = (int32_t)(((uint32_t)x * 2654435761u + (fm1_ms >> 7) * 40503u) >> 27) % 37;
        if (x && w >= 4u)
            tf_line(lastx, y0 - 18 + lasty, 27 + x, y0 - 18 + y, RGB(34, 52, 20), 2);
        else if (x)
            tf_line(lastx, y0 - 18 + lasty, 27 + x, y0 - 18 + y, RGB(34, 52, 20), 2);
        lastx = 27 + x;
        lasty = y;
    }
    tfm_snd_label(t, b);
    tfm_macro_text(t, 1, v, &unit);
    str_cpy(b + str_len(b), " \xB7 ", 5);
    str_cpy(b + str_len(b), v, 10);
    tu_text(22, 100, &AF_TS, b, RGB(34, 52, 20), RGB(134, 168, 40));
    tu_text(22, 118, &AF_PS, "PIXEL", fc.ink, RGB(88, 88, 96));
    for (j = 0; j < 5u; j++) {                                                         /* the WAVE buttons (K2): 50 25 12 TRI NZ */
        static const char *const WN[5] = {"50", "25", "12", "TRI", "NZ"};
        int32_t cx = j < 3u ? 169 + 25 * (int32_t)j : 181 + 25 * (int32_t)(j - 3u), cy = j < 3u ? 46 : 74;
        tu_disc(cx, cy, 10, j == w ? RGB(214, 60, 140) : RGB(130, 128, 150));
        tu_text_c(cx, cy, &AF_TS, WN[j], TC_TEXT, j == w ? RGB(214, 60, 140) : RGB(130, 128, 150));
    }
    tu_text(158, 100, &AF_TS, "K2 WAVE", RGB(214, 60, 140), tf_under(100));
    tf_row(152, 15, 0, 54, 36, RGB(52, 48, 78), RGB(236, 232, 240), 176);
}
static void tf_tine(void)
{
    track_t *t = &trk[song.sel];
    int32_t tp = tfm_macro_get(t, 2), trem = tp < 43 ? 0 : tp < 85 ? 1 : 2;       /* TREM: past 1/3 MONO, past 2/3 STEREO */
    uint32_t j;
    tu_text(10, 40, &AF_PN, "Tine", fc.ink, tf_under(40));
    tf_preset(70, 42, fc.ink, tf_under(42));
    tu_rrect(7, 66, 226, 122, 6, RGB(212, 210, 206), tf_under(120));
    tu_disc(18, 80, 3, TU_TC[2]);
    tu_text(26, 80, &AF_TS, "TREMOLO", RGB(34, 34, 34), RGB(212, 210, 206));
    for (j = 0; j < 3u; j++) {                                                         /* the lever = TREM: OFF / MONO / STER */
        static const char *const LN[3] = {"OFF", "MONO", "STER"};
        tu_rrect(14 + (int32_t)j * 34, 90, 31, 14, 3, (int32_t)j == trem ? RGB(190, 60, 40) : RGB(48, 48, 50), RGB(212, 210, 206));
        tu_text_c(29 + (int32_t)j * 34, 97, &AF_TS, LN[j], TC_TEXT, (int32_t)j == trem ? RGB(190, 60, 40) : RGB(48, 48, 50));
    }
    tu_text(122, 82, &AF_TS, "TREM: past 1/3 MONO,", RGB(60, 60, 60), RGB(212, 210, 206));
    tu_text(122, 94, &AF_TS, "past 2/3 STEREO", RGB(60, 60, 60), RGB(212, 210, 206));
    {
        uint16_t ink = fc.ink, mute = fc.mute;
        fc.ink = RGB(30, 30, 30);                                                     /* (the labels sit on the light panel) */
        fc.mute = RGB(96, 96, 96);
        tf_row(133, 16, 0, 52, 40, RGB(22, 22, 24), RGB(240, 240, 240), 158);
        fc.ink = ink;
        fc.mute = mute;
    }
    (void)t;
}
static void tf_glint(void)
{
    track_t *t = &trk[song.sel];
    uint32_t p = fm1_ms >> 8, flash = fs[song.sel].flash;
    uint16_t gold = RGB(232, 190, 100);
    tu_rect(10, 30, 219, 1, gold); tu_rect(10, 189, 219, 1, gold); tu_rect(10, 30, 1, 160, gold); tu_rect(228, 30, 1, 160, gold);
    tu_rect(14, 34, 211, 1, tu_mix(tf_under(40), gold, 50)); tu_rect(14, 185, 211, 1, tu_mix(tf_under(40), gold, 50));
    tu_rect(14, 34, 1, 152, tu_mix(tf_under(40), gold, 50)); tu_rect(224, 34, 1, 152, tu_mix(tf_under(40), gold, 50));
    tu_text_c(120, 50, &AF_PN, "Glint", gold, tf_under(50));
    {
        int32_t s0 = 3 + (int32_t)((p + 0u) & 3u) / 2 + (flash ? 3 : 0), s1 = 2 + (int32_t)((p + 1u) & 3u) / 2, s2 = 3 + (int32_t)((p + 2u) & 3u) / 2 + (flash ? 2 : 0), s3 = 2 + (int32_t)((p + 3u) & 3u) / 2;
        tf_star(33, 48, s0, gold);                                                   /* the stars twinkle, and flare on a note */
        tf_star(47, 68, s1, tu_mix(tf_under(68), gold, 70));
        tf_star(194, 72, s2, gold);
        tf_star(205, 49, s3, tu_mix(tf_under(49), gold, 70));
    }
    tf_preset(70, 66, gold, tf_under(66));
    tf_row(107, 17, 0, 51, 42, RGB(196, 150, 60), RGB(30, 20, 10), 133);
    (void)t;
}
static void tf_pluck(void)
{
    track_t *t = &trk[song.sel];
    int32_t x, amp = (int32_t)fs[song.sel].amp, lasty = 76, lastx = 24;
    tu_text(14, 40, &AF_PN, "Pluck", RGB(240, 214, 170), tf_under(40));
    tf_preset(84, 40, fc.ink, tf_under(40));
    tu_rrect(18, 58, 204, 38, 14, RGB(176, 130, 84), tf_under(76));
    for (x = 0; x < 192; x += 3) {                                                    /* the string: it vibrates on each note and dies with DAMP */
        int32_t y = 77 + (tu_sin((uint32_t)(x * 5 / 2) + (fm1_ms >> 3)) * amp * 14 / 102400) * (x > 90 ? (192 - x) : x) / 96;
        tf_line(lastx, lasty, 24 + x, y, RGB(130, 40, 24), 2);
        lastx = 24 + x;
        lasty = y;
    }
    tf_row(122, 15, 0, 51, 42, RGB(82, 50, 26), RGB(248, 236, 214), 148);
    (void)t;
}
static void tf_vox(void)
{
    track_t *t = &trk[song.sel];
    static const char VL[5] = {'U', 'O', 'A', 'E', 'I'};
    uint32_t j, v = (uint32_t)clamp(tfm_macro_get(t, 0) * 5 / 128, 0, 4), glow = fs[song.sel].flash || fs[song.sel].env > 900;
    int32_t open = clamp(fs[song.sel].env / 400, 2, 18), rx = v == 0u ? 8 : v == 1u ? 11 : v == 2u ? 14 : v == 3u ? 15 : 16;
    int32_t ry = v == 4u ? open / 3 + 1 : v == 3u ? open / 2 + 1 : open;
    tu_rrect(18, 32, 75, 83, 14, RGB(150, 168, 190), tf_under(70));
    tu_disc(37, 52, 6, glow ? RGB(120, 200, 255) : RGB(70, 110, 160));               /* the eyes glow on notes */
    tu_disc(74, 52, 6, glow ? RGB(120, 200, 255) : RGB(70, 110, 160));
    tf_ell(55, 90, rx, ry < 2 ? 2 : ry, RGB(24, 30, 40));                            /* the mouth: opens with the sound, shaped by VOWEL */
    tu_rrect(104, 32, 120, 83, 10, RGB(60, 72, 88), tf_under(70));
    for (j = 0; j < 20u; j++) {
        int32_t q;
        for (q = 0; q < 12; q++)
            tu_rect(110 + (int32_t)j * 5, 38 + q * 6, 2, 2, RGB(84, 96, 112));
    }
    for (j = 0; j < 5u; j++) {                                                        /* the vowels, the one in the voice lit */
        char l[2] = {VL[j], 0};
        tu_text_c(118 + (int32_t)j * 24, 76, &AF_TM, l, j == v ? RGB(255, 190, 60) : RGB(160, 172, 190), RGB(60, 72, 88));
    }
    tu_text(14, 128, &AF_PN, "VOX", RGB(120, 190, 240), tf_under(128));
    tf_preset(70, 128, fc.ink, tf_under(128));
    tf_row(152, 14, 0, 51, 40, RGB(196, 204, 214), RGB(30, 36, 46), 175);
}
static void tf_wheel(void)
{
    track_t *t = &trk[song.sel];
    static const uint8_t REG[10][9] = {{8, 8, 8, 8, 8, 8, 8, 8, 8}, {8, 8, 8, 0, 0, 0, 0, 0, 0}, {8, 8, 8, 6, 0, 0, 0, 0, 0}, {8, 0, 8, 0, 0, 0, 0, 0, 0},
                                       {8, 8, 8, 8, 0, 0, 0, 0, 0}, {0, 0, 8, 6, 4, 0, 0, 0, 0}, {8, 8, 8, 8, 8, 8, 0, 0, 0}, {8, 0, 8, 8, 8, 0, 0, 0, 0},
                                       {0, 0, 4, 4, 6, 6, 0, 0, 0}, {8, 6, 8, 6, 8, 6, 4, 4, 4}};
    uint32_t j, st = (uint32_t)tf_sw(0) % 10u;
    int32_t sub = tfm_macro_get(t, 4), body = tfm_macro_get(t, 5), top = tfm_macro_get(t, 6), rotor = tf_sw(2);
    char b[24];
    tu_rrect(6, 27, 228, 162, 8, RGB(78, 44, 24), tf_under(100));
    for (j = 0; j < 9u; j++) {                                                         /* the drawbars: the STOPS registration; SUB / BODY / TOP scale their groups */
        int32_t g = j < 2u ? sub : j < 6u ? body : top, len = (int32_t)REG[st][j] * 9 * (100 + (g - 64) * 55 / 64) / 100 + 6;
        uint16_t bc = j < 2u ? RGB(150, 70, 40) : (j == 2u || j == 4u || j == 6u || j == 8u) ? RGB(240, 232, 208) : RGB(30, 24, 20);
        tu_rect(14 + 15 * (int32_t)j, 31, 12, len, bc);
        tu_rect(14 + 15 * (int32_t)j, 31 + len - 1, 12, 1, tu_mix(bc, TC_BG, 40));
        tu_rect(18 + 15 * (int32_t)j, 28, 4, 4, RGB(60, 34, 18));
    }
    tu_text(150, 40, &AF_PS, "WHEEL", RGB(255, 170, 60), tf_under(40));
    tu_text(150, 53, &AF_PS, "HOUSE", RGB(255, 170, 60), tf_under(53));
    {
        char pb[24];
        tfm_snd_label(t, pb);
        tu_text_fit(150, 68, &AF_TM, pb, fc.ink, tf_under(68), 78);
    }
    tu_text(150, 84, &AF_TS, "ROTARY", fc.ink, tf_under(84));
    tf_tiles(150, 93, 25, 14, 27, 2, RGB(240, 232, 208), RGB(40, 24, 14), RGB(30, 24, 20), RGB(200, 180, 150));
    str_cpy(b, "STOPS: ", 8);
    str_cpy(b + str_len(b), TFM_INS[TI_WHEEL].snd[tfm_snd_of(t) % TFM_INS[TI_WHEEL].nsnd].name, 14);
    tu_disc(14, 126, 2, TU_TC[0]);
    tu_text_fit(20, 126, &AF_TS, b, fc.ink, tf_under(126), 200);
    if (tu.page == 0u) {
        tf_knob(49, 147, 13, RGB(206, 204, 198), RGB(30, 30, 30), tfm_macro_get(t, 1));
        tf_label(1, 49, 168, 1, 1);
        tf_knob(180, 147, 13, RGB(206, 204, 198), RGB(30, 30, 30), tfm_macro_get(t, 3));
        tf_label(3, 180, 168, 3, 1);
    } else {
        tf_row(147, 13, 0, 55, 49, RGB(206, 204, 198), RGB(30, 30, 30), 168);
    }
    (void)rotor;
}
static void tf_face(void)                               /* the whole body, y 22 .. 215 */
{
    uint32_t k = song.sel, i = tfm_ins_of(&trk[k]), y;
    const tu_mat_t *m = &TU_MAT[i % TI_N];
    for (y = 22; y < 215u; y += 4u)                                                   /* the material: bands, with its rules */
        tu_rect(0, (int32_t)y, TU_W, 4, tu_mat_at(m, y - 22u, 193u));
    if (m->tex == 1)
        for (y = 3; y < TU_W; y += 4u)
            tu_rect((int32_t)y, 22, 1, 193, tu_mix(tf_under(100), m->ink, 7));
    else if (m->tex == 2)
        for (y = 26; y < 215u; y += 8u)
            tu_rect(0, (int32_t)y, TU_W, 1, tu_mix(tf_under((int32_t)y), m->ink, 6));
    switch (i) {
    case TI_THUMP: tf_thump(); break;
    case TI_SQUELCH: tf_squelch(); break;
    case TI_WOBBLER: tf_wobbler(); break;
    case TI_CHORDETTE: tf_chordette(); break;
    case TI_RAZOR: tf_razor(); break;
    case TI_HAZE: tf_haze(); break;
    case TI_PIXEL: tf_pixel(); break;
    case TI_TINE: tf_tine(); break;
    case TI_GLINT: tf_glint(); break;
    case TI_PLUCK: tf_pluck(); break;
    case TI_VOX: tf_vox(); break;
    case TI_WHEEL: tf_wheel(); break;
    default:
        tu_text_fit(14, 46, &AF_PN, ENGINES[trk[k].eng_req % NENGINES]->name, fc.acc, fc.bg, 212);
        tu_text(14, 64, &AF_TS, "a sound without an instrument", fc.mute, fc.bg);
        tu_text(14, 80, &AF_TL, "EDIT: DEEP", fc.ink, fc.bg);
        break;
    }
}
static void tu_face_top(void) { tf_face(); }
static void tu_face_dials(void) { tf_face(); }
static void tu_head_sound(void)
{
    uint32_t k = song.sel;
    char t[8], b[4] = {'T', (char)('1' + k), 0, 0};
    int32_t x;
    uint16_t hb = tu_mix(fc.bg, TC_BG, 70);             /* (the header and footer dark over the material) */
    tu_rect(0, 0, TU_W, TU_HY, hb);
    tu_rect(0, TU_HY - 1, TU_W, 1, tu_mix(hb, fc.acc, 14));
    tu_rec_dots(hb);
    tu_text(TU_DX, 10, &AF_TM, b, TC_TEXT, hb);
    tu_timer_text(t);
    if (t[0])
        tu_text_c(120, 10, &AF_PS, t, TC_AMBER, hb);
    {   /* "MACROS 1 . 2 . DEEP": the page's number in the accent */
        static const char *const W[6] = {"MACROS", "1", "\xB7", "2", "\xB7", "DEEP"};
        uint32_t j;
        int32_t w = 5 * 4;
        for (j = 0; j < 6u; j++)
            w += text_w(&AF_TS, W[j]);
        x = 231 - w;
        for (j = 0; j < 6u; j++)
            x = tu_text(x, 10, &AF_TS, W[j], (j == 1u && tu.page == 0u) || (j == 3u && tu.page == 1u) ? fc.acc : TC_QUIET, hb) + 4;
    }
}
static void tu_sound_draw(void)
{
    uint32_t k = song.sel, i = tfm_ins_of(&trk[k]), sig, m;
    tu_face_pal(i == TI_NONE ? 0 : &TFM_INS[i]);
    tu_toy_poll(k);
    sig = i * 977u + k * 31u + (uint32_t)tu.page * 7919u + tfm_snd_of(&trk[k]) * 131u + (tm.on ? tm.left : 0u) + tu_rec_sig() * 6007u;
    for (m = 0; m < 8u && i != TI_NONE; m++)
        sig = sig * 31u + (uint32_t)tfm_macro_get(&trk[k], m);
    tu_zone(0, 0, TU_HY, fc.bg, sig, tu_head_sound);
    tu_zone(2, 22, 104, fc.bg, sig + (i != TI_NONE ? (ui.frame >> 1) * 7u : 0u), tu_face_top);
    tu_zone(3, 126, 89, fc.bg, sig + (i != TI_NONE ? (ui.frame >> 1) * 7u : 0u), tu_face_dials);
    tu_pbg = tu_mix(fc.bg, TC_BG, 70);
    tu_pline = tu_mix(fc.bg, fc.acc, 14);
    tu_pq = TC_QUIET;
    if (i == TI_THUMP)
        tu_foot(3, "KEYS", "PLAY PARTS", "EDIT", tu.page ? "PAGE 1" : "PAGE 2", "SEQ", "GRID", 0, 0, 0, 0);
    else
        tu_foot(3, "EDIT", tu.page ? "PAGE 1" : "PAGE 2", "PRESETS", "SOUND", "HOME", "JAM", 0, 0, 0, 0);
    tu_pbg = TC_BG;
    tu_pline = TC_SURF;
    tu_pq = TC_QUIET;
}
/* PRESETS: the track's next sound (its instrument's list: the named presets, then the older ones) */
static void tu_sound_step(int32_t s)
{
    track_t *t = &trk[song.sel];
    uint32_t i = tfm_ins_of(t);
    if (i == TI_NONE) {
        presets_turn(s);
        return;
    }
    tfm_load(t, i, (uint32_t)clamp((int32_t)tfm_snd_of(t) + (s > 0 ? 1 : -1), 0, (int32_t)TFM_INS[i].nsnd - 1));
    ui.force = 1;
}
static void tu_sound_input(uint32_t busy)
{
    track_t *t = &trk[song.sel];
    uint32_t k;
    int32_t s;
    if (busy)
        return;
    if ((s = panel_enc(EN_PRESET)) != 0)
        tu_sound_step(s);
    for (k = 0; k < 4u; k++)
        if ((s = panel_enc(EN_K1 + k)) != 0 && tfm_ins_of(t) != TI_NONE) {
            tfm_macro_turn_k(t, (uint32_t)tu.page * 4u + k, k, s);
            ui.hot_col = (uint8_t)k;
        }
}
/* hold EDIT: the picker, the twelve instruments on the white keys (ui_layer.c layer_keys, ui_draw.c) */
static void tu_picker_key(uint32_t p)
{
    if (p < TI_N && !chain_busy()) {
        tfm_load(TSEL, p, 0);
        ui_say("INSTRUMENT ", TFM_INS[p].name);
    } else if (chain_busy()) {
        ui_message("STOP TO EDIT");
    }
}
static void tu_picker_tile(int32_t x, int32_t y, uint32_t i, int sel)
{
    const tfm_instr_t *in = &TFM_INS[i];
    char key[3] = {W_NOTE[i % 7u], (char)(i < 4u ? '3' : i < 11u ? '4' : '5'), 0};
    tu_rrect(x, y, 71, 28, 6, sel ? in->col[2] : in->col[1], TC_BG);
    tu_text(x + 6, y + 8, &AF_TS, key, sel ? in->col[0] : in->col[3], sel ? in->col[2] : in->col[1]);
    tu_text_fit(x + 6, y + 19, &AF_TM, in->name, sel ? in->col[0] : in->col[4], sel ? in->col[2] : in->col[1], 62);
}
static void tu_picker_body(void)
{
    uint32_t i, cur = tfm_ins_of(TSEL);
    tu_rect(0, 22, TU_W, 193, TC_BG);
    for (i = 0; i < TI_N; i++)
        tu_picker_tile(8 + (int32_t)(i % 3u) * 75, 32 + (int32_t)(i / 3u) * 32, i, i == cur);
    tu_text_c(120, 166, &AF_TS, "a white key picks the instrument for this track", TC_QUIET, TC_BG);
}
static void tu_picker_draw(void)
{
    uint32_t sig = tfm_ins_of(TSEL) * 7u + song.sel * 977u;
    tu_head(song.sel, "INSTRUMENT", 0, 0, 0);
    tu_zone(2, 22, 97, TC_BG, sig, tu_picker_body);
    tu_zone(3, 119, 96, TC_BG, sig, tu_picker_body);
    tu_foot(3, "KEYS", "PICK", "OCT-", "BACK", "EDIT", "LET GO", 0, 0, 0, 0);
    ui.force = 0;
}
