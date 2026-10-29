/* TFM (tfm-next): HOME > JAM (MENU > DISPLAY > HOME JAM, ui.c HV_JAM). The four tracks as four bumpers in their
 * colours (ui.c TFM_TRK_COL: T1 blue, T2 yellow, T3 pink, T4 green), and KNOB k track k's jam control: its
 * instrument's first macro (tfm_instr.c; a track with no instrument: its LEVEL, as HOME LEVELS). A bumper's ring is
 * where its knob is (7 o'clock to 5), it flashes when the track plays a note (a jump in its level) and glows with
 * its level; a muted track is dark, its outline dashed. REC records the moves as automation (tfm_macro_set and the
 * LEVEL's motion_capture, as any knob). Included from tilt.c after tfm_instr.c */
#define JAM_RING_N 28u
static const int16_t JAM_RING[JAM_RING_N][2] = {        /* sin, -cos x 1024 from -135 to +135 degrees (0 = up) */
    {-724, 724}, {-839, 587}, {-928, 433}, {-989, 265}, {-1020, 89}, {-1020, -89}, {-989, -265}, {-928, -433},
    {-839, -587}, {-724, -724}, {-587, -839}, {-433, -928}, {-265, -989}, {-89, -1020}, {89, -1020}, {265, -989},
    {433, -928}, {587, -839}, {724, -724}, {839, -587}, {928, -433}, {989, -265}, {1020, -89}, {1020, 89},
    {989, 265}, {928, 433}, {839, 587}, {724, 724}};
#define JAM_FLASH 5u                                    /* frames a note's flash stays (the graph: every other frame) */
static struct {
    int32_t prev[NTRK];                                 /* the level last frame (a jump: a note) */
    uint8_t flash[NTRK];
} jam;

static int home_jam(void) { return ui.home && ui_home_view == HV_JAM; }

static int jam_macro(const track_t *t) { return tfm_ins_of(t) != TI_NONE; }
/* the knob's place 0..127: macro 1, or the LEVEL */
static int32_t jam_pos(const track_t *t)
{
    const param_desc_t *d;
    if (jam_macro(t))
        return tfm_macro_get(t, 0);
    d = track_desc(t, P_LEVEL);
    return d->max > d->min ? (t->p[P_LEVEL] - d->min) * 127 / (d->max - d->min) : 0;
}
static void jam_turn(uint32_t k, int32_t s)
{
    track_t *t = &trk[k & 3u];
    if ((k & 3u) != song.sel && ((song.rec >> (k & 3u)) & 1u))   /* REC armed there: the move is recorded on that track */
        track_select(k & 3u);                           /* (motion_capture records the selected track) */
    if (jam_macro(t)) {
        tfm_macro_turn_k(t, 0, k, s);
    } else {
        const param_desc_t *d = track_desc(t, P_LEVEL);
        t->p[P_LEVEL] = (int16_t)param_turn(d, t->p[P_LEVEL], accel(EN_K1 + (k & 3u), s, d->max - d->min));
        motion_capture(t, P_LEVEL, t->p[P_LEVEL]);
        ml_knob(t, P_LEVEL);
    }
}
/* the four cards: each track's jam control in its colour (muted: DIM) */
static void jam_cards(void)
{
    uint32_t c;
    for (c = 0; c < NTRK; c++) {
        const track_t *t = &trk[c];
        const tfm_macro_t *mc;
        char val[12];
        const char *unit = "";
        int32_t pos = jam_pos(t);
        uint16_t col = t->p[P_MUTE] ? T_DIM : c == ui.hot_col && ui.hot_t ? T_ACCENT : tfm_trk_col(c);
        if (!jam_macro(t)) {
            const param_desc_t *d = track_desc(t, P_LEVEL);
            param_format(d, t->p[P_LEVEL], val, &unit);
            draw_column(c, "LEVEL", val, unit, col, pos * 1000 / 127, trk_icon(c, 1));
            continue;
        }
        mc = &TFM_INS[tfm_ins_of(t)].mac[0];
        tfm_macro_text(t, 0, val, &unit);
        { char lb[6]; str_cpy(lb, mc->name, 6); draw_column(c, lb, val, unit, col, pos * 1000 / 127, trk_icon(c, 1)); }
    }
}
static void jam_disc(int32_t cx, int32_t cy, int32_t r, uint16_t c)
{
    int32_t dy, w;
    for (dy = -r; dy <= r; dy++) {
        for (w = r; w > 0 && w * w + dy * dy > r * r; w--)
            ;
        cv_rect(cx - w, cy + dy, 2 * w + 1, 1, c);
    }
}
/* the graph: four bumpers across the panel, the track's number and instrument under each */
static void graph_jam(void)
{
    uint32_t c, i;
    int32_t h = (int32_t)cv_h, r = h >= 100 ? 21 : h / 2 - 8, cy = h >= 100 ? 42 : h / 2;
    for (c = 0; c < NTRK; c++) {
        const track_t *t = &trk[c];
        int32_t cx = 30 + 60 * (int32_t)c, p = t->peak, pos = jam_pos(t), lit, glow;
        int mute = t->p[P_MUTE] != 0;
        uint16_t col = tfm_trk_col(c), body;
        if (p > jam.prev[c] * 3 / 2 + 900)              /* a jump in the level: a note */
            jam.flash[c] = JAM_FLASH;
        jam.prev[c] = p;
        glow = mute ? 0 : clamp(p * 100 / 14000, 0, 100);
        if (glow > 4)                                   /* the glow round it, with the level */
            jam_disc(cx, cy, r + 5, ux_mix(T_SURF, col, glow * 45 / 100));
        body = mute ? T_BG : jam.flash[c] ? ux_mix(col, T_TEXT, 45) : ux_mix(T_SURF, col, 40 + glow / 4);
        jam_disc(cx, cy, r, body);
        if (jam.flash[c])
            jam.flash[c]--;
        lit = pos ? (pos * (int32_t)JAM_RING_N + 126) / 127 : 0;
        for (i = 0; i < JAM_RING_N; i++) {              /* the ring: where the knob is (muted: dashed, DIM) */
            int32_t x = cx + JAM_RING[i][0] * (r + 3) / 1024, y = cy + JAM_RING[i][1] * (r + 3) / 1024;
            if (mute) {
                if (!(i & 1u))
                    cv_rect(x - 1, y - 1, 2, 2, T_DIM);
            } else {
                cv_rect(x - 1, y - 1, 3, 3, (int32_t)i < lit ? col : T_RAISE);
            }
        }
        if (c == song.sel)                              /* the selected track: a dot in the middle */
            jam_disc(cx, cy, 3, mute ? T_DIM : T_TEXT);
        if (h >= 100) {
            char b[4] = {'T', (char)('1' + c), 0, 0};
            GFX_HOOK_ALIGN(cx - 30, 0, cx + 30, 0, AL_H, "jam bumper number centred");
            cv_text_in(cx - 30, cy + r + 10, 60, &AF_S, b, mute ? T_DIM : col, T_SURF);
            GFX_HOOK_ALIGN(cx - 30, 0, cx + 30, 0, AL_H, "jam bumper instrument centred");
            cv_text_in(cx - 30, cy + r + 26, 60, &AF_S, jam_macro(t) ? tfm_ins_tag(t) : "--", mute ? T_DIM : T_MID, T_SURF);
        }
    }
}
