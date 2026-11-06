/* TFM: KNOB k as track k's jam control (TFM's JAM screen, tfm_ui.c): its instrument's first macro (tfm_instr.c; a track
 * with no instrument: its LEVEL). REC records the moves as automation (tfm_macro_set and the LEVEL's motion_capture, as
 * any knob). (Round 11: Felucca's old HOME > JAM view, MENU > DISPLAY > HOME JAM, is gone.) Included from tilt.c after
 * tfm_instr.c */
/* (the ring of a knob: tfm_faces.c draws it) */
#define JAM_RING_N 28u
static const int16_t JAM_RING[JAM_RING_N][2] = {        /* sin, -cos x 1024 from -135 to +135 degrees (0 = up) */
    {-724, 724}, {-839, 587}, {-928, 433}, {-989, 265}, {-1020, 89}, {-1020, -89}, {-989, -265}, {-928, -433},
    {-839, -587}, {-724, -724}, {-587, -839}, {-433, -928}, {-265, -989}, {-89, -1020}, {89, -1020}, {265, -989},
    {433, -928}, {587, -839}, {724, -724}, {839, -587}, {928, -433}, {989, -265}, {1020, -89}, {1020, 89},
    {989, 265}, {928, 433}, {839, 587}, {724, 724}};
#define JAM_FLASH 5u                                    /* frames a note's flash stays (the graph: every other frame) */

static int jam_macro(const track_t *t) { return tfm_ins_of(t) != TI_NONE; }
/* the knob's place 0..127: macro 1, or the LEVEL */
static int32_t jam_pos(const track_t *t)
{
    const param_desc_t *d;
    if (jam_macro(t))
        return tfm_macro_get(t, tfm_jam_m(t));
    d = track_desc(t, P_LEVEL);
    return d->max > d->min ? (t->p[P_LEVEL] - d->min) * 127 / (d->max - d->min) : 0;
}
static void jam_turn(uint32_t k, int32_t s)
{
    track_t *t = &trk[k & 3u];
    if ((k & 3u) != song.sel && ((song.rec >> (k & 3u)) & 1u))   /* REC armed there: the move is recorded on that track */
        track_select(k & 3u);                           /* (motion_capture records the selected track) */
    if (jam_macro(t)) {
        tfm_macro_turn_k(t, tfm_jam_m(t), k, s);
    } else {
        const param_desc_t *d = track_desc(t, P_LEVEL);
        t->p[P_LEVEL] = (int16_t)param_turn(d, t->p[P_LEVEL], accel(EN_K1 + (k & 3u), s, d->max - d->min));
        motion_capture(t, P_LEVEL, t->p[P_LEVEL]);
        ml_knob(t, P_LEVEL);
    }
}
