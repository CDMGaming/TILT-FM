/* TFM-UI phase 4 (tfm-spec/TFM-UI-SPEC.md 3 SOUND, TFM-INSTRUMENTS.md): the instrument faces. EDIT opens the face of the
 * selected track's instrument: its toy, its name in Silkscreen in its colour, a line about it, the preset and
 * `PRESETS < n of m >`, four dials (K1-K4 = macros 1-4, then 5-8), EDIT steps MACROS 1 > MACROS 2 > DEEP (Felucca's
 * own pages for that track). Hold EDIT: the picker of the twelve instruments on the white keys. Each toy redraws its
 * own box, once a frame, and says in its comment what it reacts to. Included from tilt.c after tfm_ui.c. */

/* the face's palette: BG SURF ACC MUTE INK of the instrument (a track with none: TFM AMBER's) */
static struct { uint16_t bg, surf, acc, mute, ink; } fc;
static void tu_face_pal(const tfm_instr_t *in)
{
    if (in) {
        fc.bg = in->col[0]; fc.surf = in->col[1]; fc.acc = in->col[2]; fc.mute = in->col[3]; fc.ink = in->col[4];
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
/* ---- the twelve toys, in a box of 70 x 70 at (x0, y0) ---- */
static void toy_spin(int32_t cx, int32_t cy, int32_t r, uint32_t a256, uint16_t c, int bars)
{
    uint32_t b;
    for (b = 0; b < (uint32_t)bars; b++) {
        uint32_t a = a256 + b * (128u / (uint32_t)bars);
        int32_t dx = tu_sin(a) * r / 1024, dy = -tu_cos(a) * r / 1024;
        cv_line_t(cx - dx, cy - dy, cx + dx, cy + dy, tu_c(c), 3);
    }
}
static void toy_squelch(uint32_t k, int32_t x0, int32_t y0)              /* a spinner: its speed follows the cutoff and the envelope; white on an accent */
{
    track_t *t = &trk[k];
    int32_t cx = x0 + 35, cy = y0 + 35;
    uint32_t cut = (uint32_t)clamp(t->p[P_E4], 0, 127);
    fs[k].ang = (uint16_t)(fs[k].ang + 6u + cut / 4u + (uint32_t)clamp(fs[k].env / 500, 0, 40));
    tu_disc(cx, cy, 30, tu_mix(fc.surf, fc.acc, 12));
    toy_spin(cx, cy, 28, fs[k].ang >> 4, fs[k].acc && fs[k].flash ? 0xFFFFu : fc.acc, 2);
    tu_disc(cx, cy, 6, fc.bg);
    tu_circle(cx, cy, 6, fc.acc);
}
static void toy_wobbler(uint32_t k, int32_t x0, int32_t y0)              /* a ball held by a magnet: pushed off centre by the LFO x WOB; the rings light on a note */
{
    track_t *t = &trk[k];
    int32_t cx = x0 + 35, cy = y0 + 35, depth = clamp(t->p[P_LD_FLT], 0, 63), off = (t->lfo_val >> 8) * depth / 63 * 20 / 128;
    int32_t j;
    for (j = 0; j < 3; j++)
        tu_circle(cx, cy, 29 - 9 * j, tu_mix(fc.surf, fc.acc, fs[k].flash > (uint32_t)j * 2u ? 100 : 35 + 30 * (uint32_t)j));
    tu_glow(cx + off, cy, 8, fc.acc, fc.surf, 60);
    tu_disc(cx + off, cy, 7, fc.ink);
    tu_rect(cx - 12, cy - 1, 8, 2, tu_mix(fc.surf, fc.acc, 50));
}
static void toy_chordette(uint32_t k, int32_t x0, int32_t y0)           /* drop targets, one per chord note: down while it sounds, flash when all are down */
{
    track_t *t = &trk[k];
    static const uint8_t SZ[10] = {1, 3, 4, 3, 3, 4, 4, 4, 3, 2};
    uint32_t n = SZ[t->p[P_CHRD] % 10], i, down = (t->nheld || fs[k].env > 400) ? n : 0u, all;
    all = down == n && n > 0u && (fm1_ms >> 7) & 1u;
    for (i = 0; i < 4u; i++) {
        int32_t x = x0 + 7 + (int32_t)i * 15;
        int on = i < n && i < down;
        uint16_t c = i < n ? (all ? fc.ink : fc.acc) : tu_mix(fc.surf, fc.acc, 20);
        if (on)
            tu_rrect(x, y0 + 38, 11, 9, 2, tu_mix(fc.surf, fc.acc, 40), fc.surf);
        else if (i < n)
            tu_rrect(x, y0 + 12, 11, 32, 2, c, fc.surf);
        else
            tu_rrect(x, y0 + 38, 11, 9, 2, c, fc.surf);
    }
    tu_rect(x0 + 5, y0 + 52, 60, 2, fc.mute);
}
static void toy_razor(uint32_t k, int32_t x0, int32_t y0)               /* a ramp, a ball whose height follows the pitch, chase lights faster with the filter envelope */
{
    track_t *t = &trk[k];
    uint32_t i, ph = fm1_ms >> (fs[k].env > 3000 ? 5 : fs[k].env > 800 ? 6 : 7);
    int32_t n = (int32_t)fs[k].note - 36, bx, by;
    n = clamp(n, 0, 48);
    for (i = 0; i < 6u; i++) {
        int32_t lx = x0 + 9 + (int32_t)i * 10, ly = y0 + 54 - (int32_t)i * 6;
        tu_disc(lx, ly, 2, (ph % 6u) == i ? fc.acc : tu_mix(fc.surf, fc.acc, 22));
    }
    cv_line_t(x0 + 4, y0 + 62, x0 + 66, y0 + 26, tu_c(fc.acc), 2);
    bx = x0 + 8 + n * 56 / 48;
    by = y0 + 56 - n * 36 / 48 - 10;
    tu_glow(bx, by, 6, fc.acc, fc.surf, t->nheld || fs[k].flash ? 70 : 20);
    tu_disc(bx, by, 5, fc.ink);
}
static void toy_haze(uint32_t k, int32_t x0, int32_t y0)                /* bulbs along a soft glow: brightness follows the envelope, the colour drifts with the LFO */
{
    track_t *t = &trk[k];
    uint32_t i, br = (uint32_t)clamp(fs[k].env / 90, 6, 100);
    uint16_t gc = tu_mix(fc.acc, TC_AMBER, (uint32_t)clamp(50 + (t->lfo_val >> 9), 0, 100) / 4u);
    for (i = 0; i < 6u; i++) {
        int32_t y = y0 + 20 + (int32_t)i * 6;
        tu_rect(x0 + 6, y, 58, 5, tu_mix(fc.surf, gc, br * (14u + 6u * (i / 2u)) / 100u));
    }
    for (i = 0; i < 5u; i++) {
        uint32_t b = br * (i == 2u ? 100u : i == 1u || i == 3u ? 80u : 50u) / 100u;
        tu_rrect(x0 + 6 + (int32_t)i * 12, y0 + 46, 8, 11, 3, tu_mix(fc.surf, TC_TEXT, b), fc.surf);
    }
}
static const uint8_t PIX_SPRITE[8] = {0x3C, 0x7E, 0xDB, 0xFF, 0xFF, 0x5A, 0xA5, 0x42};   /* an original sprite, 8 x 8 */
static void toy_pixel(uint32_t k, int32_t x0, int32_t y0)               /* a sprite on a dot-matrix grid: jumps on a note (height by the velocity), a coin on an accent or COIN */
{
    track_t *t = &trk[k];
    int32_t r, c, hop = fs[k].flash ? (int32_t)fs[k].flash * (int32_t)fs[k].vel / 64 : 0, gx, gy;
    for (gy = 0; gy < 8; gy++)
        for (gx = 0; gx < 11; gx++)
            tu_rect(x0 + 3 + gx * 6, y0 + 8 + gy * 6, 4, 4, tu_mix(fc.surf, fc.acc, 14));
    for (r = 0; r < 8; r++)
        for (c = 0; c < 8; c++)
            if ((PIX_SPRITE[r] >> (7 - c)) & 1u)
                tu_rect(x0 + 9 + c * 6, y0 + 8 + (r - (hop > 5 ? 5 : hop) / 3 * 0) * 6 - (hop > 6 ? 6 : hop), 4, 4, fc.acc);
    if ((t->p[P_ED_PIT] > 18 || fs[k].acc) && fs[k].flash > 3u)                /* the coin */
        for (r = 0; r < 3; r++)
            tu_rect(x0 + 57, y0 + 6 + r * 5, 5 - (r == 1 ? 0 : 2), 4, TC_AMBER);
}
static void toy_tine(uint32_t k, int32_t x0, int32_t y0)                /* five rollover lanes: a note lights the lane of its pitch class (note mod 5), fading with the decay */
{
    uint32_t i;
    if (fs[k].on)
        fs[k].lit[fs[k].note % 5u] = 24;
    for (i = 0; i < 5u; i++) {
        int32_t x = x0 + 6 + (int32_t)i * 13;
        tu_rrect(x, y0 + 6, 9, 58, 4, tu_mix(fc.bg, fc.mute, 10), fc.surf);
        tu_ring(x + 4, y0 + 35, 3, 2, tu_mix(fc.surf, fc.acc, 18));
        if (fs[k].lit[i])
            tu_disc(x + 4, y0 + 35, 3, tu_mix(fc.surf, fc.acc, 100u * fs[k].lit[i] / 24u));
    }
}
static void tu_star(int32_t cx, int32_t cy, int32_t r, uint16_t c)       /* a four-point star */
{
    int32_t i;
    tu_rect(cx - r, cy, 2 * r + 1, 1, c);
    tu_rect(cx, cy - r, 1, 2 * r + 1, c);
    for (i = 1; i < r; i++) {
        tu_rect(cx - (r - i) / 2, cy - i, 1 + (r - i), 1, c);
        tu_rect(cx - (r - i) / 2, cy + i, 1 + (r - i), 1, c);
    }
}
static void toy_glint(uint32_t k, int32_t x0, int32_t y0)               /* five star rollovers: how many light follows the velocity, the twinkle follows METAL */
{
    static const int8_t P[5][3] = {{14, 14, 8}, {50, 12, 5}, {34, 36, 10}, {12, 52, 5}, {52, 50, 8}};
    uint32_t i, n, metal = (uint32_t)clamp(trk[k].p[P_E2] + 64, 0, 127);
    if (fs[k].on) {
        n = 1u + (uint32_t)fs[k].vel * 4u / 128u;
        for (i = 0; i < 5u; i++)
            fs[k].lit[i] = i < n ? (uint8_t)(10 + metal / 16u) : 0u;
    }
    for (i = 0; i < 5u; i++) {
        int on = fs[k].lit[i] && (((fm1_ms >> (7 - (metal >> 6))) + i) & 1u);
        tu_star(x0 + P[i][0], y0 + P[i][1], P[i][2], on || fs[k].lit[i] > 6u ? fc.acc : tu_mix(fc.surf, fc.acc, 22));
    }
}
static void toy_pluck(uint32_t k, int32_t x0, int32_t y0)               /* four flippers: each note kicks the next one, harder notes higher; they settle with the damping */
{
    uint32_t i, dm = (uint32_t)clamp(trk[k].p[P_E3], 0, 127);
    static uint8_t nx[NTRK];
    if (fs[k].on) {
        uint32_t f = nx[k]++ & 3u;
        fs[k].lit[f] = (uint8_t)(6u + (uint32_t)fs[k].vel / 24u);
    }
    for (i = 0; i < 4u; i++) {
        int32_t bx = x0 + (i & 1u ? 38 : 6), by = y0 + (i >> 1) * 30 + 12, up = (int32_t)fs[k].lit[i] * (140 - (int32_t)dm) / 160;
        int32_t ex = bx + (i & 1u ? 0 : 26), ey = by + 6 + (i & 1u ? -(up - 8) : -(up - 8));
        cv_line_t(i & 1u ? bx + 26 : bx, by + 6, i & 1u ? bx : ex, ey > by + 14 ? by + 14 : ey, tu_c(fs[k].lit[i] ? fc.acc : fc.mute), 4);
        if (fs[k].lit[i] > 0u && fs[k].lit[i] < 3u)
            fs[k].lit[i]++;
    }
}
static void toy_vox(uint32_t k, int32_t x0, int32_t y0)                 /* a dot-matrix mouth: it opens with the envelope, its shape follows the vowel (A E I O U) */
{
    int32_t vw = clamp(trk[k].p[P_E0], 0, 127) * 5 / 128, open = clamp(fs[k].env / 350, 1, 7), gx, gy;
    int32_t w = vw == 2 ? 11 : vw == 3 ? 6 : vw == 4 ? 5 : vw == 1 ? 10 : 9;   /* I narrow? E wide */
    for (gy = 0; gy < 9; gy++)
        for (gx = 0; gx < 11; gx++)
            tu_rect(x0 + 3 + gx * 6, y0 + 6 + gy * 6, 4, 4, tu_mix(fc.surf, fc.acc, 12));
    for (gy = 0; gy < 9; gy++) {
        int32_t half = open - (gy < 4 ? 4 - gy : gy - 4) * (vw == 2 ? 3 : 1) - (vw >= 3 ? (gy < 4 ? 4 - gy : gy - 4) : 0);
        if (half < 0) half = 0;
        for (gx = 0; gx < 11; gx++) {
            int32_t dx = gx - 5, lim = w / 2;
            if ((dx < 0 ? -dx : dx) <= lim && (gy >= 4 - open / 2 && gy <= 4 + open / 2) && (dx * dx * 2 + (gy - 4) * (gy - 4) * 3) < w * w / 2 + open * 4)
                tu_rect(x0 + 3 + gx * 6, y0 + 6 + gy * 6, 4, 4, fc.acc);
        }
        (void)half;
    }
}
static void toy_wheel(uint32_t k, int32_t x0, int32_t y0)               /* a spinning disc: its speed follows the rotor with the rotor's inertia, brighter while keys are held */
{
    track_t *t = &trk[k];
    static int32_t sp[NTRK];
    int32_t want = t->p[P_E7] == 2 ? 28 : t->p[P_E7] == 1 ? 9 : 0, cx = x0 + 35, cy = y0 + 35, i;
    uint16_t on = tu_mix(fc.surf, fc.acc, t->nheld ? 100 : 62);
    sp[k] += (want * 16 - sp[k]) / 24;                  /* (the rotor's spin-up and spin-down) */
    fs[k].ang = (uint16_t)(fs[k].ang + (uint32_t)sp[k]);
    for (i = 0; i < 10; i++) {
        uint32_t a0 = (fs[k].ang >> 4) + (uint32_t)i * 25u;
        int32_t r;
        for (r = 12; r <= 30; r += 1) {
            int32_t px = cx + tu_sin(a0) * r / 1024, py = cy - tu_cos(a0) * r / 1024, px2 = cx + tu_sin(a0 + 6) * r / 1024,
                    py2 = cy - tu_cos(a0 + 6) * r / 1024;
            if (i & 1)
                continue;
            cv_line_t(px, py, px2, py2, tu_c(on), 1);
        }
    }
    tu_ring(cx, cy, 30, 28, fc.acc);
    tu_disc(cx, cy, 11, fc.bg);
    tu_ring(cx, cy, 11, 9, fc.acc);
    tu_text_c(cx, cy, &AF_TS, t->p[P_E7] == 2 ? "FAST" : t->p[P_E7] == 1 ? "SLOW" : "OFF", fc.acc, fc.bg);
}
/* THUMP LAB's panel is its own, as drawn (tfm-spec/screens/SoundThump.dc.html): bumpers, a hat spinner, eight lane chips.
 * Each bumper flashes on its hit with the velocity; the spinner advances a notch a hat; a chip lights while its lane hits */
static void tu_thump_panel(uint32_t k)
{
    track_t *t = &trk[k];
    static const char *const CH[8] = {"BD", "SD", "CP", "CH", "OH", "TM", "RM", "BL"};
    uint32_t i, hat;
    int32_t vk = fs[k].lane[0] ? 100 : 0, vs = fs[k].lane[1] ? 100 : 0, vc = fs[k].lane[2] ? 100 : 0;
    char b[24], v[8];
    tu_rrect(8, 28, 224, 98, 8, fc.surf, fc.bg);
    tu_text(16, 40, &AF_PN, "THUMP LAB", fc.acc, fc.surf);
    tfm_snd_label(t, b);
    tu_text(16, 54, &AF_TL, b, fc.ink, fc.surf);
    fmt_int(v, (int32_t)tfm_snd_of(t) + 1);
    {
        int32_t x = 16;
        str_cpy(b, " of ", 5);
        fmt_int(b + 4, (int32_t)TFM_INS[TI_THUMP].nsnd);
        tu_arrow(x, 66, 1, TC_PRESETS);
        x = tu_text(x + 6, 66, &AF_TS, "PRESETS", TC_PRESETS, fc.surf);
        x = tu_text(x + 3, 66, &AF_TS, v, TC_PRESETS, fc.surf);
        x = tu_text(x, 66, &AF_TS, b, TC_PRESETS, fc.surf);
        tu_arrow(x + 3, 66, 0, TC_PRESETS);
    }
    for (i = 0; i < 8u; i++) {
        int32_t x = 16 + (int32_t)i * 16;
        int on = fs[k].lane[i] != 0u;
        tu_rrect(x, 88, 13, 28, 3, on ? fc.acc : tu_mix(fc.surf, fc.acc, 22), fc.surf);
        tu_text_c(x + 6, 111, &AF_TS, CH[i], on ? fc.bg : fc.mute, on ? fc.acc : tu_mix(fc.surf, fc.acc, 22));
    }
    tu_disc(149, 62, 23 + 0, tu_mix(fc.surf, fc.acc, 22));
    tu_glow(149, 62, 20, fc.acc, fc.surf, vk);
    tu_disc(149, 62, 20, vk ? fc.acc : tu_mix(fc.surf, fc.acc, 12));
    tu_text_c(149, 62, &AF_TS, "KICK", vk ? fc.bg : fc.acc, vk ? fc.acc : tu_mix(fc.surf, fc.acc, 12));
    tu_disc(207, 58, 16, vs ? fc.acc : tu_mix(fc.surf, fc.acc, 14));
    tu_circle(207, 58, 16, fc.acc);
    tu_text_c(207, 58, &AF_TS, "SNARE", vs ? fc.bg : fc.acc, vs ? fc.acc : tu_mix(fc.surf, fc.acc, 14));
    tu_disc(172, 104, 14, vc ? fc.acc : tu_mix(fc.surf, fc.acc, 14));
    tu_circle(172, 104, 14, vc ? fc.ink : tu_mix(fc.surf, fc.acc, 50));
    tu_text_c(172, 104, &AF_TS, "CLAP", vc ? fc.bg : fc.mute, vc ? fc.acc : tu_mix(fc.surf, fc.acc, 14));
    hat = (fs[k].ang >> 4) & 1u;
    if ((fs[k].lane[3] == 8u || fs[k].lane[4] == 8u))
        fs[k].ang = (uint16_t)(fs[k].ang + 16u * 3u);
    tu_rrect(198, 87, 22, 32, 4, tu_mix(fc.surf, fc.ink, 55), fc.surf);
    tu_rrect(199, 88, 20, 30, 3, fc.surf, tu_mix(fc.surf, fc.ink, 55));
    tu_rect(204, 94 + (int32_t)hat * 3, 10, 3, fc.ink);
    tu_rect(204, 100 + (int32_t)((fs[k].ang >> 5) & 1u) * 3, 10, 3, tu_mix(fc.surf, fc.ink, 40));
    tu_text_c(209, 113, &AF_TS, "HATS", fc.ink, fc.surf);
}
static void tu_toy(uint32_t k, int32_t x0, int32_t y0)
{
    switch (tfm_ins_of(&trk[k])) {
    case TI_SQUELCH: toy_squelch(k, x0, y0); break;
    case TI_WOBBLER: toy_wobbler(k, x0, y0); break;
    case TI_CHORDETTE: toy_chordette(k, x0, y0); break;
    case TI_RAZOR: toy_razor(k, x0, y0); break;
    case TI_HAZE: toy_haze(k, x0, y0); break;
    case TI_PIXEL: toy_pixel(k, x0, y0); break;
    case TI_TINE: toy_tine(k, x0, y0); break;
    case TI_GLINT: toy_glint(k, x0, y0); break;
    case TI_PLUCK: toy_pluck(k, x0, y0); break;
    case TI_VOX: toy_vox(k, x0, y0); break;
    case TI_WHEEL: toy_wheel(k, x0, y0); break;
    default: break;
    }
}
/* ---- the face ---- */
static void tu_dial_draw(int32_t cx, int32_t cy, int32_t ro, uint32_t k, uint32_t m, const track_t *t, uint16_t bg)
{
    uint32_t i = tfm_ins_of(t);
    const tfm_macro_t *mc = &TFM_INS[i].mac[m & 7u];
    char val[12], kb[3] = {'K', (char)('1' + k), 0};
    const char *unit;
    int32_t pos = tfm_macro_get(t, m);
    tu_dial(cx, cy, ro, ro - 3, (uint32_t)(pos * 1000 / 127), TU_TC[k & 3u], tu_mix(fc.surf, fc.acc, 14));
    tu_disc(cx, cy, ro - 4, bg);
    tfm_macro_text(t, m, val, &unit);
    tu_text_c(cx, cy, str_len(val) > 3u ? &AF_TM : &AF_TL, val, fc.ink, bg);
    tu_text_c(cx, cy + ro + 8, &AF_TM, mc->name, TU_TC[k & 3u], bg);
    if (ro > 19)
        tu_text_c(cx, cy + ro + 19, &AF_TS, kb, fc.mute, bg);
}
static void tu_face_top(void)                           /* y 22 .. 125 */
{
    uint32_t k = song.sel, i = tfm_ins_of(&trk[k]);
    char b[24], v[8], w[16];
    int32_t x;
    tu_rect(0, 22, TU_W, 104, fc.bg);
    if (i == TI_THUMP) {
        tu_thump_panel(k);
        return;
    }
    tu_rrect(10, 30, 70, 70, 9, fc.acc, fc.bg);
    tu_rrect(11, 31, 68, 68, 8, fc.surf, fc.acc);
    if (i != TI_NONE)
        tu_toy(k, 10, 30);
    if (i == TI_NONE) {
        tu_text_fit(89, 46, &AF_PN, ENGINES[trk[k].eng_req % NENGINES]->name, fc.acc, fc.bg, 144);
        tu_text(89, 60, &AF_TS, "a sound without an instrument", fc.mute, fc.bg);
        tu_text(89, 74, &AF_TL, "EDIT: DEEP", fc.ink, fc.bg);
        return;
    }
    tu_text_fit(89, 46, str_len(TFM_INS[i].name) > 8u ? &AF_PN : &AF_PM, TFM_INS[i].name, fc.acc, fc.bg, 146);
    {
        char d[40];
        str_cpy(d, TFM_INS[i].desc, sizeof d);
        tu_text_fit(89, 59, &AF_TS, d, fc.mute, fc.bg, 141);
    }
    tfm_snd_label(&trk[k], b);
    tu_text_fit(89, 74, &AF_TL, b, fc.ink, fc.bg, 148);
    fmt_int(v, (int32_t)tfm_snd_of(&trk[k]) + 1);
    str_cpy(w, " of ", 5);
    fmt_int(w + 4, (int32_t)TFM_INS[i].nsnd);
    x = 89;
    tu_arrow(x, 86, 1, TC_PRESETS);
    x = tu_text(x + 6, 86, &AF_TS, "PRESETS", TC_PRESETS, fc.bg);
    x = tu_text(x + 3, 86, &AF_TS, v, TC_PRESETS, fc.bg);
    x = tu_text(x, 86, &AF_TS, w, TC_PRESETS, fc.bg);
    tu_arrow(x + 3, 86, 0, TC_PRESETS);
}
static void tu_face_dials(void)                         /* y 126 .. 215 */
{
    uint32_t k = song.sel, i = tfm_ins_of(&trk[k]), c;
    tu_rect(0, 126, TU_W, 89, fc.bg);
    if (i == TI_NONE)
        return;
    for (c = 0; c < 4u; c++) {
        int32_t cx = i == TI_THUMP ? 34 + 57 * (int32_t)c : 34 + 57 * (int32_t)c + (c > 1u);
        tu_dial_draw(cx, i == TI_THUMP ? 152 : 150, i == TI_THUMP ? 19 : 20, c, (uint32_t)tu.page * 4u + c, &trk[k], fc.bg);
    }
}
static void tu_head_sound(void)
{
    uint32_t k = song.sel;
    char t[8], b[4] = {'T', (char)('1' + k), 0, 0};
    int32_t x;
    tu_rect(0, 0, TU_W, TU_HY, fc.bg);
    tu_rect(0, TU_HY - 1, TU_W, 1, tu_mix(fc.bg, fc.acc, 14));
    tu_badge(9, 10, k, fc.bg);
    tu_text(26, 10, &AF_TM, b, fc.ink, fc.bg);
    tu_timer_text(t);
    if (t[0])
        tu_text_c(120, 10, &AF_PS, t, TC_AMBER, fc.bg);
    {   /* "MACROS 1 . 2 . DEEP": the page's number in the accent */
        static const char *const W[6] = {"MACROS", "1", "\xB7", "2", "\xB7", "DEEP"};
        uint32_t j;
        int32_t w = 5 * 4;
        for (j = 0; j < 6u; j++)
            w += text_w(&AF_TS, W[j]);
        x = 231 - w;
        for (j = 0; j < 6u; j++)
            x = tu_text(x, 10, &AF_TS, W[j], (j == 1u && tu.page == 0u) || (j == 3u && tu.page == 1u) ? fc.acc : fc.mute, fc.bg) + 4;
    }
}
static void tu_sound_draw(void)
{
    uint32_t k = song.sel, i = tfm_ins_of(&trk[k]), sig, m;
    tu_face_pal(i == TI_NONE ? 0 : &TFM_INS[i]);
    tu_toy_poll(k);
    sig = i * 977u + k * 31u + (uint32_t)tu.page * 7919u + tfm_snd_of(&trk[k]) * 131u + (tm.on ? tm.left : 0u);
    for (m = 0; m < 8u && i != TI_NONE; m++)
        sig = sig * 31u + (uint32_t)tfm_macro_get(&trk[k], m);
    tu_zone(0, 0, TU_HY, fc.bg, sig, tu_head_sound);
    tu_zone(2, 22, 104, fc.bg, sig + (i != TI_NONE ? (ui.frame >> 1) * 7u : 0u), tu_face_top);
    tu_zone(3, 126, 89, fc.bg, sig, tu_face_dials);
    tu_pbg = fc.bg;
    tu_pline = tu_mix(fc.bg, fc.acc, 14);
    tu_pq = fc.mute;
    if (i == TI_THUMP)
        tu_foot(3, "KEYS", "PLAY PARTS", "EDIT", "NEXT PAGE", "SEQ", "DRUM GRID", 0, 0, 0, 0);
    else
        tu_foot(3, "EDIT", "NEXT PAGE", "HOLD EDIT", "INSTRUMENT", "HOME", "JAM", 0, 0, 0, 0);
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
