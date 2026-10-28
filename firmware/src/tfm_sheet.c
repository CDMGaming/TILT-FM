/* TFM (tfm-next): the STEP SHEET (after SLOOP's step sheet; written anew). On STEP (the piano roll or the DRUM grid,
 * not DETAIL), OCT+ held: the four cards are the cursor step's actions, KNOB 1 MOVE (the step one place left / right,
 * its locks and automation with it; the cursor follows), KNOB 2 COPY (the step onto the next / previous place, the
 * cursor follows: turn on to stamp it along), KNOB 3 NUDGE (its timing, as STEP's steps held + PRESETS), KNOB 4 SLIDE
 * (a DRUM track: RATCH x1..x4). Each is one undo (SAVE held). OCT+'s release after a turn is no tap (ui_input.c
 * oct_mod). DELETE is EDIT (the step a REST, its locks gone), FILL is DETAIL's condition. Included from tilt.c */
static int sheet_on(void)
{
    return song.seq_mode && !ui.home && !ui.menu && !ui.confirm && !ui.layer && cur_page()->graph == GR_ROLL &&
           !detail_on() && ((fm1_in.buttons >> panel.btn[B_OCTUP]) & 1u);
}
static void sheet_cards(void)
{
    const track_t *t = TSEL;
    const step_t *s = &t->step[ui.cursor % NSTEP];
    char v[12];
    const char *u = "";
    fmt_int(v, (int32_t)ui.cursor + 1);
    draw_column(0, "MOVE", v, "", VAL(0u), -1, ICON_AUTO);
    draw_column(1, "COPY", ">", "", VAL(1u), -1, ICON_AUTO);
    fmt_int(v, step_nudge(s));
    draw_column(2, "NUDGE", v, "", VAL(2u), (step_nudge(s) + 8) * 1000 / 15, ICON_AUTO);
    if (drum_track(t)) {
        v[0] = 'x'; v[1] = (char)('1' + ((s->flags & SF_RATCH) >> SF_RATCH_SH)); v[2] = 0;
        draw_column(3, "RATCH", v, u, VAL(3u), -1, ICON_AUTO);
    } else {
        draw_column(3, "SLIDE", s->flags & SF_SLIDE ? "ON" : "OFF", u, VAL(3u), -1, ICON_AUTO);
    }
}
static void sheet_turn(uint32_t k, int32_t d)
{
    track_t *t = TSEL;
    uint32_t len = tl_len(t), c = ui.cursor % len, to = (uint32_t)clamp((int32_t)c + (d > 0 ? 1 : -1), 0, (int32_t)len - 1);
    step_t *s = &t->step[c];
    tl_undo_take(t);
    if (k == 0u && to != c) {                           /* MOVE: c and its neighbour change places */
        uint8_t src[NSTEP];
        uint32_t i;
        for (i = 0; i < len; i++)
            src[i] = (uint8_t)i;
        src[c] = (uint8_t)to;
        src[to] = (uint8_t)c;
        tl_order(t, len, src);
        ui.cursor = (uint8_t)to;
    } else if (k == 1u && to != c) {                    /* COPY: onto the neighbour (its own locks stay) */
        t->step[to] = *s;
        ui.cursor = (uint8_t)to;
    } else if (k == 2u) {
        step_set_nudge(s, clamp(step_nudge(s) + d, -8, 7));
    } else if (k == 3u && drum_track(t)) {
        uint32_t r = (uint32_t)clamp((int32_t)((s->flags & SF_RATCH) >> SF_RATCH_SH) + (d > 0 ? 1 : -1), 0, 3);
        s->flags = (uint8_t)((s->flags & ~SF_RATCH) | r << SF_RATCH_SH);
    } else if (k == 3u) {
        s->flags = (uint8_t)(d > 0 ? s->flags | SF_SLIDE : s->flags & ~SF_SLIDE);
    }
    ui.force = 1;
}
