/* TFM (tfm-next): pinball toys on an instrument's MACROS page (graph_macro), a small loop reacting to the track's notes
 * and knobs. THUMP LAB: pop bumpers (KICK the big one, SNARE a slingshot, the hats a spinner, each lit as its lane
 * hits: the step playing, or a jump in the level); SQUELCH: a spinner, faster with the cutoff, lit on each note.
 * The other instruments: none yet. Included from tilt.c after tfm_jam.c (its jam_disc) */
static const int16_t TOY_ROT[16][2] = {                 /* sin, -cos x 1024 every 22.5 degrees from up */
    {0, -1024}, {392, -946}, {724, -724}, {946, -392}, {1024, 0}, {946, 392}, {724, 724}, {392, 946},
    {0, 1024}, {-392, 946}, {-724, 724}, {-946, 392}, {-1024, 0}, {-946, -392}, {-724, -724}, {-392, -946}};
#define TOY_LIT 6u                                      /* frames a hit stays lit */
static struct {
    uint8_t lit[3];                                     /* KICK, SNARE, HATS (SQUELCH: lit[0]) */
    uint16_t idx;                                       /* the step last seen playing */
    uint16_t ang;                                       /* the spinner, 1/16 turns x 16 */
    int32_t prev;                                       /* the level last frame */
} toy;
static int toy_has(const track_t *t) { uint32_t i = tfm_ins_of(t); return i == TI_THUMP || i == TI_SQUELCH; }
static void toy_spinner(int32_t cx, int32_t cy, int32_t r, uint32_t a, uint16_t c)
{
    const int16_t *v = TOY_ROT[(a >> 4) & 15u];
    cv_line_t(cx - v[0] * r / 1024, cy - v[1] * r / 1024, cx + v[0] * r / 1024, cy + v[1] * r / 1024, c, 3);
    jam_disc(cx, cy, 3, c);
}
/* the toy in y0 .. y0 + 44 */
static void graph_toy(const track_t *t, int32_t y0)
{
    uint32_t i, hit = 0, k;
    int32_t p = t->peak, cy = y0 + 22;
    int onset = p > toy.prev * 3 / 2 + 900;
    toy.prev = p;
    if (song.playing && t->seq_idx != toy.idx) {        /* a new step: its hits */
        const step_t *s = &t->step[t->seq_idx % NSTEP];
        toy.idx = t->seq_idx;
        hit = drum_track(t) ? s->hit : s->time == ST_NOTE && s->n ? 1u : 0u;
    } else if (!song.playing && onset) {
        hit = 1u;
    }
    if (tfm_ins_of(t) == TI_THUMP) {
        if (hit & 1u) toy.lit[0] = TOY_LIT;
        if (hit & 6u) toy.lit[1] = TOY_LIT;             /* SNARE, CLAP */
        if (hit & 24u) toy.lit[2] = TOY_LIT, toy.ang = (uint16_t)(toy.ang + 40u);   /* the hats spin it */
        jam_disc(70, cy, 20, toy.lit[0] ? T_ACCENT : T_RAISE);   /* KICK: the big bumper */
        jam_disc(70, cy, 12, toy.lit[0] ? T_TEXT : T_SURF);
        for (k = 0; k < 22u; k++) {                     /* SNARE: a slingshot, a triangle */
            int32_t w = (int32_t)k * 2 / 3;
            cv_rect(122 - w / 2 + 6, y0 + 4 + (int32_t)k * 2, w + 1, 2, toy.lit[1] ? T_ACCENT : T_RAISE);
        }
        cv_rect(126, y0 + 4, 3, 44, toy.lit[1] ? T_TEXT : T_MID);
        toy_spinner(178, cy, 18, toy.ang, toy.lit[2] ? T_ACCENT : T_MID);
    } else {
        uint32_t cut = (uint32_t)clamp(t->p[P_E0 + 4], 0, 127);   /* (ANALOG's CUT) */
        if (hit || onset) toy.lit[0] = TOY_LIT;
        if (t->peak > 300)                              /* sounding: it turns, faster with the cutoff */
            toy.ang = (uint16_t)(toy.ang + 2u + cut / 6u);
        jam_disc(120, cy, 22, toy.lit[0] ? ux_mix(T_SURF, T_ACCENT, 50) : T_RAISE);
        toy_spinner(120, cy, 20, toy.ang, toy.lit[0] ? T_TEXT : T_THEME);
    }
    for (i = 0; i < 3u; i++)
        if (toy.lit[i])
            toy.lit[i]--;
}
