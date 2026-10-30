/* TFM-UI phase 7 (tfm-spec/TFM-UI-SPEC.md 3 FX, SPACE, ARP, CLOCK, MOTION): one template, a big readout on top (its caption, its
 * name large in Silkscreen, a row of chips) and four coloured dials, K1-K4 (tfm-spec/screens/Fx.dc.html). FX: the track's
 * sends and insert (REVERB, DELAY, CHARACTER, AMOUNT); FX again SPACE: the shared reverb and delay; ARP: MODE RATE OCTAVES
 * GATE; SCL: CLOCK (TEMPO SWING KEY SCALE), again its page 2 (TIME SIG); LFO: MOTION (RATE DEPTH DESTINATION SHAPE).
 * Hold FX is Felucca's PUNCH-IN layer, hold SCL its root-on-keys layer, hold LFO MOMENTARY. Included from tilt.c. */
static struct {
    const char *cap, *chip[6];
    char big[16], hl[24], hr[24], kl[4][10], kv[4][12];
    uint16_t col;
    uint16_t frac[4];
    uint8_t nchip, sel, trk, hot;                       /* hot: the knob shown highlighted (0xFF: none) */
} ro;
static uint8_t clock_hot;                               /* CLOCK opens on K1 TEMPO; the knob last turned stays lit */
static void ro_knob(uint32_t k, const char *label, const param_desc_t *d, int32_t v, const char *over)
{
    char val[12];
    const char *unit;
    str_cpy(ro.kl[k], label, 10);
    if (over) {
        str_cpy(ro.kv[k], over, 12);
    } else if (d->fmt == F_PCT || d->fmt == F_INT) {
        param_format(d, v, val, &unit);
        str_cpy(ro.kv[k], val, 12);
    } else {
        param_format(d, v, val, &unit);
        str_cpy(ro.kv[k], val, 12);
    }
    ro.frac[k] = (uint16_t)(d->max > d->min ? (clamp(v, d->min, d->max) - d->min) * 1000 / (d->max - d->min) : 0);
}
static void ro_pct(uint32_t k, const char *label, int32_t v, int32_t lo, int32_t hi)   /* a value as a percent of its range */
{
    char b[8];
    str_cpy(ro.kl[k], label, 10);
    fmt_int(b, (clamp(v, lo, hi) - lo) * 100 / (hi - lo));
    str_cpy(ro.kv[k], b, 12);
    ro.frac[k] = (uint16_t)((clamp(v, lo, hi) - lo) * 1000 / (hi - lo));
}
static void ro_body(void)                               /* y 22 .. 215 */
{
    uint32_t i;
    int32_t x;
    tu_rect(0, 22, TU_W, 193, TC_BG);
    tu_rrect(8, 30, 224, 75, 8, TC_DEEP, TC_BG);
    tu_text(18, 41, &AF_TS, ro.cap, TC_QUIET, TC_DEEP);
    tu_text(18, 58, &AF_PM, ro.big, ro.col, TC_DEEP);
    {   /* the chips: padded to fit the card (six of them need less) */
        int32_t sum = 0, pad = 10;
        for (i = 0; i < ro.nchip; i++)
            sum += text_w(&AF_TS, ro.chip[i]);
        if (ro.nchip && sum + (int32_t)ro.nchip * pad + (int32_t)(ro.nchip - 1u) * 3 > 204)
            pad = (204 - sum - (int32_t)(ro.nchip - 1u) * 3) / (int32_t)ro.nchip;
        if (pad < 4)
            pad = 4;
        x = 18;
        for (i = 0; i < ro.nchip; i++) {
            int32_t w = text_w(&AF_TS, ro.chip[i]) + pad;
            tu_rrect(x, 78, w, 15, 3, i == ro.sel ? ro.col : TC_SURF, TC_DEEP);
            tu_text_c(x + w / 2, 85, &AF_TS, ro.chip[i], i == ro.sel ? TC_BG : TC_QUIET, i == ro.sel ? ro.col : TC_SURF);
            x += w + 3;
        }
    }
    for (i = 0; i < 12u; i++) {                         /* the bars at the right: a still picture of the character */
        int32_t h = 7 + (int32_t)((uint32_t)tu_sin(i * 11u) * 17u / 1024u);
        if (h < 7) h = 7;
        tu_rect(190 + (int32_t)i * 3, 56 - h / 2, 2, h, tu_mix(TC_DEEP, ro.col, 40u + 5u * (i % 6u)));
    }
    for (i = 0; i < 4u; i++) {
        int32_t cx = 34 + 57 * (int32_t)i + (i > 1u);
        char kb[3] = {'K', (char)('1' + i), 0};
        tu_dial(cx, 134, 20, 17, ro.frac[i], TU_TC[i], TC_TRACK);
        if (i == ro.hot)
            tu_ring(cx, 134, 25, 23, TU_TC[i]);
        tu_disc(cx, 134, 16, TC_BG);
        tu_text_c(cx, 134, str_len(ro.kv[i]) > 3u ? &AF_TM : &AF_TL, ro.kv[i], TC_TEXT, TC_BG);
        if (i == ro.hot) {                              /* the lit knob: its name in reverse */
            int32_t w = text_w(&AF_TM, ro.kl[i]) + 8;
            tu_rrect(cx - w / 2, 156, w, 13, 3, TU_TC[i], TC_BG);
            tu_text_c(cx, 162, &AF_TM, ro.kl[i], TC_BG, TU_TC[i]);
        } else {
            tu_text_c(cx, 162, &AF_TM, ro.kl[i], TU_TC[i], TC_BG);
        }
        tu_text_c(cx, 173, &AF_TS, kb, TC_QUIET, TC_BG);
    }
}
static void ro_head(void)
{
    char t[8];
    int32_t x = TU_DX;
    tu_rect(0, 0, TU_W, TU_HY, TC_BG);
    tu_rect(0, TU_HY - 1, TU_W, 1, TC_SURF);
    tu_rec_dots(TC_BG);
    tu_text(x, 10, &AF_TM, ro.hl, TC_TEXT, TC_BG);
    tu_timer_text(t);
    if (t[0])
        tu_text_c(150, 10, &AF_PS, t, TC_AMBER, TC_BG);
    tu_text_r(231, 10, &AF_TS, ro.hr, TC_QUIET, TC_BG);
}
static void ro_draw(const char *k0, const char *a0, const char *k1, const char *a1, const char *k2, const char *a2)
{
    uint32_t sig = fnv(2166136261u, &ro, sizeof ro) + (tm.on ? tm.left : 0u) + tu_rec_sig() * 6007u;
    tu_zone(0, 0, TU_HY, TC_BG, sig, ro_head);
    tu_zone(2, 22, 100, TC_BG, sig, ro_body);
    tu_zone(3, 122, 93, TC_BG, sig, ro_body);
    tu_foot(3, k0, a0, k1, a1, k2, a2, 0, 0, 0, 0);
    ui.force = 0;
}
static void ro_clear(void)
{
    memset(&ro, 0, sizeof ro);
    ro.trk = (uint8_t)song.sel;
    ro.hot = 0xFF;
    ro.col = TU_TC[song.sel & 3u];
}
/* ================================================================== FX, SPACE ============================== */
static const char *const FXC[6] = {"OFF", "DRIVE", "CRUSH", "PHASER", "FLANGE", "CHORUS"};
static const uint8_t FXI[6] = {0, 1, 5, 6, 7, 8};      /* the INSERT type each chip sets (SOFT for DRIVE) */
static uint32_t fx_chip(const track_t *t)               /* the chip of the track's insert (HARD / FOLD / FUZZ are DRIVE too) */
{
    uint32_t v = (uint32_t)t->p[P_ITYPE], i;
    if (v >= 1u && v <= 4u)
        return 1u;
    for (i = 0; i < 6u; i++)
        if (FXI[i] == v)
            return i;
    return 0u;
}
static void tu_fx_fill(void)
{
    track_t *t = TSEL;
    uint32_t i;
    ro_clear();
    if (tu.scr == TU_FX) {
        ro.cap = "CHARACTER";
        str_cpy(ro.big, fx_chip(t) == 1u && t->p[P_ITYPE] > 1 ? TP[P_ITYPE].names[t->p[P_ITYPE]] : FXC[fx_chip(t)], 16);
        for (i = 0; i < 6u; i++)
            ro.chip[i] = FXC[i];
        ro.nchip = 6;
        ro.sel = (uint8_t)fx_chip(t);
        ro_pct(0, "REVERB", t->p[P_REV], 0, 127);
        ro_pct(1, "DELAY", t->p[P_DLY], 0, 127);
        str_cpy(ro.kl[2], "CHARACTER", 10);
        str_cpy(ro.kv[2], FXC[fx_chip(t)], 12);
        ro.kv[2][4] = 0;
        ro.frac[2] = (uint16_t)(fx_chip(t) * 200u);
        ro_pct(3, "AMOUNT", t->p[P_IMIX], 0, 127);
        str_cpy(ro.hl, tu_inst_name(t), 12);
        str_cpy(ro.hl + str_len(ro.hl), " \xB7 FX", 8);
        str_cpy(ro.hr, "TRACK \xB7 SPACE", 16);
    } else {
        static const char *const RT[3] = {"ROOM", "SPRING", "HALL"};
        ro.trk = 0xFF;
        ro.cap = "REVERB";
        str_cpy(ro.big, RT[song.g[G_RTYPE] % 3], 16);
        for (i = 0; i < 3u; i++)
            ro.chip[i] = RT[i];
        ro.nchip = 3;
        ro.sel = (uint8_t)(song.g[G_RTYPE] % 3);
        ro.col = TC_CYAN;
        str_cpy(ro.kl[0], "REVERB", 10);
        str_cpy(ro.kv[0], RT[song.g[G_RTYPE] % 3], 12);
        ro.frac[0] = (uint16_t)(song.g[G_RTYPE] * 500);
        ro_pct(1, "SIZE", song.g[G_RSIZE], 0, 127);
        ro_pct(2, "DELAY", song.g[G_DTIME], 0, 127);
        ro_pct(3, "FEEDBACK", song.g[G_DFDBK], 0, 127);
        str_cpy(ro.hl, "SPACE", 8);
        str_cpy(ro.hr, "TRACK \xB7 SPACE", 16);
    }
}
static void tu_fx_input(uint32_t busy)
{
    track_t *t = TSEL;
    uint32_t k;
    int32_t s, d;
    if (busy)
        return;
    for (k = 0; k < 4u; k++) {
        if ((s = panel_enc(EN_K1 + k)) == 0)
            continue;
        d = s > 0 ? 1 : -1;
        if (tu.scr == TU_FX) {
            int16_t *vp = k == 0u ? &t->p[P_REV] : k == 1u ? &t->p[P_DLY] : &t->p[P_IMIX];
            if (k == 2u) {
                uint32_t c = (uint32_t)clamp((int32_t)fx_chip(t) + d, 0, 5);
                t->p[P_ITYPE] = FXI[c];
                motion_capture(t, P_ITYPE, t->p[P_ITYPE]);
            } else {
                const param_desc_t *pd = track_desc(t, k == 0u ? P_REV : k == 1u ? P_DLY : P_IMIX);
                *vp = (int16_t)param_turn(pd, *vp, accel(EN_K1 + k, s, 127));
                motion_capture(t, k == 0u ? P_REV : k == 1u ? P_DLY : P_IMIX, *vp);
            }
        } else {
            static const uint8_t G[4] = {G_RTYPE, G_RSIZE, G_DTIME, G_DFDBK};
            const param_desc_t *pd = &GP[G[k]];
            song.g[G[k]] = (int16_t)param_turn(pd, song.g[G[k]], k == 0u ? d : accel(EN_K1 + k, s, 127));
        }
        ui.hot_col = (uint8_t)k;
    }
}
/* ================================================================== ARP =================================== */
static void tu_arp_fill(void)
{
    track_t *t = TSEL;
    char b[8];
    ro_clear();
    ro.cap = "ARPEGGIATOR";
    str_cpy(ro.big, TP[P_AMODE].names[t->p[P_AMODE]], 16);
    str_cpy(ro.hl, tu_inst_name(t), 12);
    str_cpy(ro.hl + str_len(ro.hl), " \xB7 ARP", 8);
    str_cpy(ro.hr, t->p[P_AMODE] ? "ON \xB7 tap ARP: off" : "OFF \xB7 tap ARP: on", 24);
    ro_knob(0, "MODE", &TP[P_AMODE], t->p[P_AMODE], 0);
    str_cpy(ro.kv[0], TP[P_AMODE].names[t->p[P_AMODE]], 12);
    ro.kv[0][4] = 0;
    ro_knob(1, "RATE", &TP[P_ARATE], t->p[P_ARATE], TP[P_ARATE].names[t->p[P_ARATE]]);
    fmt_int(b, t->p[P_AOCT]);
    ro_knob(2, "OCTAVES", &TP[P_AOCT], t->p[P_AOCT], b);
    ro_pct(3, "GATE", t->p[P_AGATE], 1, 127);
}
static void tu_arp_input(uint32_t busy)
{
    track_t *t = TSEL;
    static const uint8_t ID[4] = {P_AMODE, P_ARATE, P_AOCT, P_AGATE};
    uint32_t k;
    int32_t s;
    if (busy)
        return;
    for (k = 0; k < 4u; k++)
        if ((s = panel_enc(EN_K1 + k)) != 0) {
            const param_desc_t *pd = &TP[ID[k]];
            t->p[ID[k]] = (int16_t)param_turn(pd, t->p[ID[k]], k == 3u ? accel(EN_K1 + k, s, 127) : (s > 0 ? 1 : -1));
            motion_capture(t, ID[k], t->p[ID[k]]);
            ui.hot_col = (uint8_t)k;
        }
}
/* ================================================================== CLOCK ================================= */
static void tu_clock_fill(void)
{
    char b[16];
    const char *u;
    ro_clear();
    ro.trk = 0xFF;
    ro.col = TC_AMBER;
    if (tu.scr == TU_CLOCK) {
        ro.cap = "TEMPO";
        ro.hot = clock_hot;
        fmt_int(ro.big, song.g[G_BPM]);
        str_cpy(ro.big + str_len(ro.big), " BPM", 6);
        ro_knob(0, "TEMPO", &GP[G_BPM], song.g[G_BPM], 0);
        ro_knob(1, "SWING", &GP[G_SWING], song.g[G_SWING], 0);
        param_format(&TP[P_ROOT], TSEL->p[P_ROOT], b, &u);
        ro_knob(2, "KEY", &TP[P_ROOT], TSEL->p[P_ROOT], b);
        ro_knob(3, "SCALE", &TP[P_SCALE], TSEL->p[P_SCALE], 0);
        str_cpy(ro.kv[3], tu_scale_full(TSEL->p[P_SCALE]), 12);
        ro.kv[3][4] = 0;
        ro.nchip = 0;
        str_cpy(ro.hl, "CLOCK", 8);
        str_cpy(ro.hr, "page 1 \xB7 SCL: page 2", 24);
    } else {
        static const char *const TS_[4] = {"4/4", "3/4", "6/8", "7/8"};
        uint32_t i;
        ro.cap = "TIME SIGNATURE";
        ro.hot = 0;
        str_cpy(ro.big, TS_[tfm_tsig()], 16);
        for (i = 0; i < 4u; i++)
            ro.chip[i] = TS_[i];
        ro.nchip = 4;
        ro.sel = (uint8_t)tfm_tsig();
        str_cpy(ro.kl[0], "TIME SIG", 10);
        str_cpy(ro.kv[0], TS_[tfm_tsig()], 12);
        ro.frac[0] = (uint16_t)(tfm_tsig() * 333u);
        str_cpy(ro.hl, "CLOCK", 8);
        str_cpy(ro.hr, "page 2 \xB7 SCL: page 1", 24);
    }
}
static void tu_clock_input(uint32_t busy)
{
    uint32_t k, i;
    int32_t s;
    if (busy)
        return;
    for (k = 0; k < 4u; k++) {
        if ((s = panel_enc(EN_K1 + k)) == 0)
            continue;
        clock_hot = (uint8_t)k;
        if (tu.scr == TU_CLOCK2) {
            if (k == 0u)
                ui_prefs3 = (uint8_t)((ui_prefs3 & ~(3u << PREF_TSIG_SH)) | (uint32_t)clamp((int32_t)tfm_tsig() + (s > 0 ? 1 : -1), 0, 3) << PREF_TSIG_SH);
            continue;
        }
        if (k == 0u) {
            song.g[G_BPM] = (int16_t)clamp(song.g[G_BPM] + accel(EN_K1, s, 200), GP[G_BPM].min, GP[G_BPM].max);
            ui.bpm_t = 40;
        } else if (k == 1u) {
            song.g[G_SWING] = (int16_t)param_turn(&GP[G_SWING], song.g[G_SWING], accel(EN_K1 + 1u, s, 100));
        } else if (k == 2u) {                           /* the key: every track's */
            int32_t v = (TSEL->p[P_ROOT] + (s > 0 ? 1 : 11)) % 12;
            for (i = 0; i < NTRK; i++)
                trk[i].p[P_ROOT] = (int16_t)v;
        } else {
            int32_t v = clamp(TSEL->p[P_SCALE] + (s > 0 ? 1 : -1), 0, TP[P_SCALE].max);
            for (i = 0; i < NTRK; i++)
                trk[i].p[P_SCALE] = (int16_t)v;
        }
        ui.hot_col = (uint8_t)k;
    }
}
/* ================================================================== MOTION ================================ */
static const uint8_t MO_DEST[4] = {P_LD_PIT, P_LD_FLT, P_LD_SHP, P_LD_AMP};
static const char *const MO_NAME[4] = {"PITCH", "FILTER", "SHAPE", "AMP"};
static uint32_t mo_dest(const track_t *t)               /* the destination the LFO has depth on (the first, else the filter) */
{
    uint32_t i;
    for (i = 0; i < 4u; i++)
        if (t->p[MO_DEST[i]])
            return i;
    return 1u;
}
static void tu_motion_fill(void)
{
    track_t *t = TSEL;
    uint32_t d = mo_dest(t), i;
    ro_clear();
    ro.cap = "LFO";
    str_cpy(ro.big, TP[P_LWAVE].names[t->p[P_LWAVE]], 16);
    for (i = 0; i < 4u; i++)
        ro.chip[i] = MO_NAME[i];
    ro.nchip = 4;
    ro.sel = (uint8_t)d;
    str_cpy(ro.hl, tu_inst_name(t), 12);
    str_cpy(ro.hl + str_len(ro.hl), " \xB7 MOTION", 12);
    str_cpy(ro.hr, "hold LFO: MOMENTARY", 24);
    ro_knob(0, "RATE", &TP[P_LRATE], t->p[P_LRATE], 0);
    ro_pct(1, "DEPTH", t->p[MO_DEST[d]], -64, 63);
    ro_knob(2, "DESTIN", &TP[P_LWAVE], d, MO_NAME[d]);
    ro.frac[2] = (uint16_t)(d * 333u);
    ro_knob(3, "SHAPE", &TP[P_LWAVE], t->p[P_LWAVE], TP[P_LWAVE].names[t->p[P_LWAVE]]);
}
static void tu_motion_input(uint32_t busy)
{
    track_t *t = TSEL;
    uint32_t k, d;
    int32_t s;
    if (busy)
        return;
    for (k = 0; k < 4u; k++) {
        if ((s = panel_enc(EN_K1 + k)) == 0)
            continue;
        d = mo_dest(t);
        if (k == 0u) {
            t->p[P_LRATE] = (int16_t)param_turn(&TP[P_LRATE], t->p[P_LRATE], accel(EN_K1, s, 127));
            motion_capture(t, P_LRATE, t->p[P_LRATE]);
        } else if (k == 1u) {
            const param_desc_t *pd = &TP[MO_DEST[d]];
            t->p[MO_DEST[d]] = (int16_t)param_turn(pd, t->p[MO_DEST[d]], accel(EN_K1 + 1u, s, 127));
            motion_capture(t, MO_DEST[d], t->p[MO_DEST[d]]);
        } else if (k == 2u) {                           /* the destination: the depth moves with it */
            uint32_t nd = (uint32_t)clamp((int32_t)d + (s > 0 ? 1 : -1), 0, 3);
            int16_t v = t->p[MO_DEST[d]];
            if (nd != d) {
                t->p[MO_DEST[d]] = 0;
                t->p[MO_DEST[nd]] = v ? v : 24;
            }
        } else {
            t->p[P_LWAVE] = (int16_t)param_turn(&TP[P_LWAVE], t->p[P_LWAVE], s > 0 ? 1 : -1);
        }
        ui.hot_col = (uint8_t)k;
    }
}
/* ---- the buttons and the draw ---- */
static void tu_pages_tap(uint32_t b)
{
    track_t *t = TSEL;
    if (b == B_FX)
        tu_goto(tu.scr == TU_FX ? TU_SPACE : TU_FX);
    else if (b == B_SCL) {
        if (tu.scr != TU_CLOCK && tu.scr != TU_CLOCK2)
            clock_hot = 0;                              /* (it opens on TEMPO) */
        tu_goto(tu.scr == TU_CLOCK ? TU_CLOCK2 : TU_CLOCK);
    } else if (b == B_LFO)
        tu_goto(TU_MOTION);
    else if (b == B_ARP) {
        if (tu.scr != TU_ARP) {                         /* on, and its page */
            if (!t->p[P_AMODE])
                t->p[P_AMODE] = 1;
            tu_goto(TU_ARP);
        } else {                                        /* again on its page: off */
            t->p[P_AMODE] = 0;
            ui.force = 1;
        }
    }
}
static void tu_pages_draw(void)
{
    switch (tu.scr) {
    case TU_FX: case TU_SPACE:
        tu_fx_fill();
        ro_draw("FX", tu.scr == TU_FX ? "SPACE" : "TRACK FX", "HOLD FX", "PUNCH-IN", "ALGO", "TRACK");
        break;
    case TU_ARP:
        tu_arp_fill();
        ro_draw("ARP", TSEL->p[P_AMODE] ? "OFF" : "ON", "HOLD ARP", "SCENES", "ALGO", "TRACK");
        break;
    case TU_CLOCK: case TU_CLOCK2:
        tu_clock_fill();
        ro_draw("SCL", "PAGE", "HOLD SCL", "ROOT ON KEYS", "HOME", "JAM");
        break;
    default:
        tu_motion_fill();
        ro_draw("LFO", "MOTION", "HOLD LFO", "MOMENTARY", "ALGO", "TRACK");
        break;
    }
}
static void tu_pages_input(uint32_t busy)
{
    switch (tu.scr) {
    case TU_FX: case TU_SPACE: tu_fx_input(busy); break;
    case TU_ARP: tu_arp_input(busy); break;
    case TU_CLOCK: case TU_CLOCK2: tu_clock_input(busy); break;
    default: tu_motion_input(busy); break;
    }
}
