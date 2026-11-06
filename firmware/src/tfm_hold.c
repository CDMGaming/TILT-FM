/* TFM-UI round 11 (tfm-spec/screens HoldFx / HoldGlo / HoldScl / HoldSeq .dc.html, drawn 2x): the four hold screens in TFM's
 * look. Only the drawing: the keys and knobs are ui_layer.c's (layer_key, layer_knob), CLASSIC keeps Felucca's drawing.
 * Each in the scene sheet's grammar: the header "HOLD FX . PUNCH-IN" (the sub-name in the layer's accent), the DMD box with
 * the layer's word in dots and three status lines, an accent-outlined panel with what the keys do, the black-key strip
 * where the black keys mean something (else a line saying so), the footer. Included from tilt.c after tfm_live.c */
#define TH_PBG RGB(28, 22, 13)                          /* the panel's ground */
#define TH_CELL RGB(46, 36, 22)                         /* a key's cell */
static int tu_hold_on(void)
{
    uint32_t l = ui.layer;
    return !tu_classic() && tu_on() && (l == LAYER_FX || l == LAYER_GLO || l == LAYER_SCL || l == LAYER_SEQ || l == LAYER_TILT);
}
static uint16_t th_acc(uint32_t l)
{
    return l == LAYER_FX ? RGB(255, 112, 80) : l == LAYER_GLO ? RGB(170, 236, 90) : l == LAYER_SCL ? RGB(80, 214, 236) : TC_AMBER;
}
#define TH_ENV RGB(255, 140, 60)                        /* (round 12: hold ENV's orange) */
static const char *th_scr_name(void)                    /* the screen under hold ENV (its header names it) */
{
    static const char *const N[] = {"JAM", "MIX", "SOUND", "BARS", "NOTES", "DRUMS", "SONG", "SONGS", "FX", "CHAR", "SPACE",
                                    "ARP", "CLOCK", "CLOCK", "MOTION", "DEEP"};
    return tu.scr < NELEM(N) ? N[tu.scr] : "";
}
static void th_head(void)
{
    uint32_t l = ui.layer;
    if (l == LAYER_TILT) {                              /* round 12: the track and the screen behind, its name in orange */
        char b[24] = "T1 ", t[8];
        int32_t x, w;
        b[1] = (char)('1' + song.sel % NTRK);
        str_cpy(b + 3, tu_inst_name(TSEL), sizeof b - 3);
        tu_rect(0, 0, TU_W, TU_HY, TC_BG);
        tu_rect(0, TU_HY - 1, TU_W, 1, TC_SURF);
        tu_rec_dots(TC_BG);
        x = tu_text(TU_DX, 10, &AF_TM, b, TC_TEXT, TC_BG) + 4;
        tu_text(x, 11, &AF_TS, "\xB7 ", TH_ENV, TC_BG);
        tu_text(x + text_w(&AF_TS, "\xB7 "), 11, &AF_TS, th_scr_name(), TH_ENV, TC_BG);
        fmt_int(t, song.g[G_BPM]);
        w = text_w(&AF_TM, t);
        tu_text(231 - w, 10, &AF_TM, t, TC_TEXT, TC_BG);
        tu_play_icon(231 - w - 11, 10, song.playing, TC_TEXT);
        return;
    }
    static const char *const W[4] = {"HOLD FX", "HOLD GLO", "HOLD SCL", "HOLD SEQ"};
    static const char *const S[4] = {"\xB7 PUNCH-IN", "\xB7 LIVE", "\xB7 ROOT", "\xB7 STEP TOOLS"};
    uint32_t i = l == LAYER_FX ? 0u : l == LAYER_GLO ? 1u : l == LAYER_SCL ? 2u : 3u;
    char b[8];
    int32_t x, w;
    tu_rect(0, 0, TU_W, TU_HY, TC_BG);
    tu_rect(0, TU_HY - 1, TU_W, 1, TC_SURF);
    tu_rec_dots(TC_BG);
    x = tu_text(TU_DX, 10, &AF_TM, W[i], TC_TEXT, TC_BG) + 4;
    tu_text(x, 11, &AF_TS, S[i], th_acc(l), TC_BG);
    fmt_int(b, song.g[G_BPM]);
    w = text_w(&AF_TM, b);
    tu_text(231 - w, 10, &AF_TM, b, TC_TEXT, TC_BG);
    tu_play_icon(231 - w - 11, 10, song.playing, TC_TEXT);
    tsl_draw(TC_BG);
}
/* the DMD box (y 25 .. 77): the word in dots, three lines (the first in the accent, the last amber) */
static void th_dmd(const char *word, int32_t pitch, const char *l1, const char *l2, const char *l3, uint16_t acc)
{
    uint16_t in = RGB(10, 7, 4);
    tu_rrect(5, 25, 230, 52, 6, RGB(42, 32, 18), TC_BG);
    tu_rrect(7, 27, 226, 48, 5, in, RGB(42, 32, 18));
    dmd_str(14, 51 - 7 * pitch / 2, pitch, word, acc, tu_mix(in, acc, 16));
    tu_text_fit(100, 37, &AF_TS, l1, acc, in, 130);
    tu_text_fit(100, 51, &AF_TS, l2, TC_TEXT, in, 130);
    tu_text_fit(100, 65, &AF_TS, l3, TC_AMBER, in, 130);
}
static void th_panel(uint16_t acc, const char *title)  /* y 80 .. 171, the title on its top line */
{
    tu_rrect(5, 80, 230, 91, 6, acc, TC_BG);
    tu_rrect(7, 82, 226, 87, 5, TH_PBG, acc);
    if (title)
        tu_text(13, 90, &AF_TS, title, TC_QUIET, TH_PBG);
}
static void th_lines(const char *a, const char *b)     /* the two lines under the panel (no strip there) */
{
    if (a)
        tu_text_c(120, 189, &AF_TS, a, TC_TEXT, TC_BG);
    if (b)
        tu_text_c(120, 203, &AF_TS, b, TC_QUIET, TC_BG);
}
static void th_cat(char *b, const char *s, uint32_t n) { str_cpy(b + str_len(b), s, n - str_len(b)); }

/* ---- FX: PUNCH-IN (coral) ---- */
static uint32_t th_fx_held(void) { return perf_kill ? 0u : perf_held | (perf_latch_on ? perf_latched : 0u); }
static void th_fx(void)
{
    uint16_t acc = th_acc(LAYER_FX);
    uint32_t held = th_fx_held(), p, e, first = PF_NFX;
    int32_t m = perf_k[0];
    char l1[40], l2[40], l3[40], v[8];
    str_cpy(l1, "FILTER ", sizeof l1);
    if (m) {
        th_cat(l1, m < 0 ? "LP " : "HP ", sizeof l1);
        fmt_int(v, m < 0 ? -m : m);
        th_cat(l1, v, sizeof l1);
    } else {
        th_cat(l1, "OFF", sizeof l1);
    }
    th_cat(l1, " \xB7 CRUSH ", sizeof l1);
    fmt_int(v, perf_k[1]);
    th_cat(l1, perf_k[1] ? v : "OFF", sizeof l1);
    str_cpy(l2, "THROW ", sizeof l2);
    fmt_int(v, perf_k[2]);
    th_cat(l2, perf_k[2] ? v : "OFF", sizeof l2);
    th_cat(l2, perf_harm_on() ? " \xB7 SHIMR " : " \xB7 DEPTH ", sizeof l2);
    fmt_int(v, perf_harm_on() ? perf_k[3] : 100 - perf_k[3]);
    th_cat(l2, v, sizeof l2);
    for (p = 0; p < PF_KEYS && first == PF_NFX; p++)
        if (perf_map[p] < PF_NFX && ((held >> perf_map[p]) & 1u))
            first = perf_map[p];
    if (first < PF_NFX) {
        str_cpy(l3, "HELD: ", sizeof l3);
        th_cat(l3, PF_NAME[first], sizeof l3);
    } else {
        str_cpy(l3, perf_latch_on ? "LATCH ON" : "HOLD A WHITE KEY", sizeof l3);
    }
    th_dmd("FX", 3, l1, l2, l3, acc);
    th_panel(acc, "WHITE KEYS: HOLD AN EFFECT \xB7 LET GO: OFF");
    for (p = 0; p < PF_KEYS; p++) {                     /* F3 .. G5, 8 a row: the key and the effect it holds (perf_map) */
        int32_t x = 13 + (int32_t)(p % 8u) * 27, y = 99 + (int32_t)(p / 8u) * 34;
        char k[3] = {W_NOTE[p], (char)(p < 4u ? '3' : p < 11u ? '4' : '5'), 0};
        int on;
        uint16_t c;
        e = perf_map[p];
        on = e < PF_NFX && ((held >> e) & 1u);
        c = on ? acc : TH_CELL;
        tu_rrect(x, y, 25, 30, 4, c, TH_PBG);
        tu_text_c(x + 12, y + 9, &AF_TS, k, on ? TC_BG : TC_QUIET, c);
        tu_text_c(x + 12, y + 21, &AF_TS, e < PF_NFX ? PF_CELL[e] : "-", on ? TC_BG : e < PF_NFX ? TC_TEXT : TC_DIM, c);
    }
    th_lines(perf_harm_on() ? "K1 FILTER \xB7 K2 CRUSH \xB7 K3 THROW \xB7 K4 SHIMR" : "K1 FILTER \xB7 K2 CRUSH \xB7 K3 THROW \xB7 K4 DEPTH",
             "BLACK KEYS: NOTHING HERE");
}

/* ---- GLO: LIVE (lime) ---- */
static void th_glo(void)
{
    static const char *const KR[4][2] = {{"C4", "ALL ON"}, {"D4", "MIDI LEARN"}, {"E4", "FILL (HOLD)"}, {"F4", "TAP TEMPO"}};
    static const char *const ML[4] = {"T1", "T2", "T3", "T4"};
    uint16_t acc = th_acc(LAYER_GLO);
    uint32_t c, solo = perf_solo, mute = 0, n;
    char l1[40], l2[40], l3[40], v[8];
    for (c = 0; c < NTRK; c++)
        mute |= trk[c].p[P_MUTE] ? 1u << c : 0u;
    l1[0] = l2[0] = 0;
    for (c = 0, n = 0; c < NTRK; c++)
        if ((solo >> c) & 1u) {
            th_cat(l1, n++ ? " " : "", sizeof l1);
            th_cat(l1, ML[c], sizeof l1);
        }
    th_cat(l1, n ? " SOLO" : "NO SOLO", sizeof l1);
    for (c = 0, n = 0; c < NTRK; c++)
        if ((mute >> c) & 1u) {
            th_cat(l2, n++ ? " " : "", sizeof l2);
            th_cat(l2, ML[c], sizeof l2);
        }
    th_cat(l2, n ? " MUTED" : "NONE MUTED", sizeof l2);
    if (tsl_on()) {                                     /* (round 11: GLO + SELECT turning: its choice and value) */
        str_cpy(l3, tsl.n, sizeof l3);
        th_cat(l3, " ", sizeof l3);
        th_cat(l3, tsl.v, sizeof l3);
    } else {
        str_cpy(l3, "TAP F4: ", sizeof l3);
        fmt_int(v, song.g[G_BPM]);
        th_cat(l3, v, sizeof l3);
    }
    th_dmd("LIVE", 3, l1, l2, l3, acc);
    th_panel(acc, 0);
    for (c = 0; c < NTRK; c++) {                        /* T1 .. T4: name, the level (K1 .. K4), SOLO / MUTED / it */
        int32_t cx = 27 + (int32_t)c * 34, lv = clamp(trk[c].p[P_LEVEL], TP[P_LEVEL].min, TP[P_LEVEL].max), h;
        int muted = (mute >> c) & 1u, so = (solo >> c) & 1u, quiet = muted || (solo && !so);
        uint16_t col = quiet ? tu_mix(TH_PBG, TU_TC[c], 40) : TU_TC[c];
        char k[3] = {W_NOTE[c], '3', 0};                /* (its SOLO key: the footer says WHITE SOLO) */
        const char *unit;
        tu_text_c(cx, 91, &AF_TS, ML[c], TU_TC[c], TH_PBG);
        tu_rrect(cx - 5, 99, 11, 43, 3, RGB(52, 40, 26), TH_PBG);
        h = TP[P_LEVEL].max > TP[P_LEVEL].min ? (lv - TP[P_LEVEL].min) * 43 / (TP[P_LEVEL].max - TP[P_LEVEL].min) : 0;
        if (h > 2)
            tu_rrect(cx - 5, 142 - h, 11, h, 3, col, RGB(52, 40, 26));
        if (so)
            str_cpy(v, "SOLO", sizeof v);
        else if (muted)
            str_cpy(v, "MUTED", sizeof v);
        else
            param_format(&TP[P_LEVEL], trk[c].p[P_LEVEL], v, &unit);
        tu_text_c(cx, 151, &AF_TS, v, so ? acc : muted ? TC_QUIET : TC_TEXT, TH_PBG);
        tu_text_c(cx, 162, &AF_TS, k, TC_QUIET, TH_PBG);
    }
    for (c = 0; c < 4u; c++) {
        int lit = (c == 1u && ui.ml.on) || (c == 2u && lys.fill);
        int32_t y = 96 + (int32_t)c * 19;
        tu_text(152, y, &AF_TS, KR[c][0], TC_AMBER, TH_PBG);
        tu_text(170, y, &AF_TS, KR[c][1], lit ? acc : TC_TEXT, TH_PBG);
    }
    {   /* the strip: T1 .. T4 mute as on MIX (lit while sounding) */
        static const char *const LB[4] = {"T1", "T2", "T3", "T4"};
        uint32_t lit = 0;
        for (c = 0; c < NTRK; c++)
            lit |= !((mute >> c) & 1u) ? 1u << c : 0u;
        tu_keystrip(4, LB, TU_TC, lit);
    }
}

/* ---- SCL: ROOT (cyan) ---- */
static void th_scl(void)
{
    static const char *const WN[7] = {"C", "D", "E", "F", "G", "A", "B"};
    static const uint8_t WS[7] = {0, 2, 4, 5, 7, 9, 11};
    static const char *const BN[5] = {"C#", "D#", "F#", "G#", "A#"};
    static const uint8_t BS[5] = {1, 3, 6, 8, 10}, BX[5] = {0, 1, 3, 4, 5};
    uint16_t acc = th_acc(LAYER_SCL), tint = tu_mix(RGB(40, 52, 56), acc, 30);
    track_t *t = TSEL;
    uint32_t root = (uint32_t)clamp(t->p[P_ROOT], 0, 11), mask = scale_mask(t), i;
    char l1[40], l2[40], l3[40], v[16], big[3];
    const char *u;
    param_format(&TP[P_ROOT], t->p[P_ROOT], v, &u);
    big[0] = v[0];
    big[1] = 0;
    str_cpy(l1, "ROOT ", sizeof l1);
    th_cat(l1, v, sizeof l1);
    th_cat(l1, " \xB7 ", sizeof l1);
    th_cat(l1, tu_scale_full((uint32_t)t->p[P_SCALE]), sizeof l1);
    str_cpy(l2, "CHORD ", sizeof l2);
    param_format(track_desc(t, P_CHRD), t->p[P_CHRD], v, &u);
    th_cat(l2, v, sizeof l2);
    th_cat(l2, " \xB7 VOICING ", sizeof l2);
    param_format(track_desc(t, P_VOIC), t->p[P_VOIC], v, &u);
    th_cat(l2, v, sizeof l2);
    str_cpy(l3, "T1 ", sizeof l3);
    l3[1] = (char)('1' + song.sel % NTRK);
    th_cat(l3, tu_inst_name(t), sizeof l3);
    th_dmd(big, 4, l1, l2, l3, acc);
    {   /* (a sharp root: its '#' beside the dots, which have no '#') */
        char r[16];
        param_format(&TP[P_ROOT], t->p[P_ROOT], r, &u);
        if (r[1] == '#')
            tu_text(14 + 6 * 4 + 2, 33, &AF_TM, "#", acc, RGB(10, 7, 4));
    }
    th_panel(acc, "ANY KEY: ITS NOTE IS THE ROOT");
    for (i = 0; i < 7u; i++) {                          /* one octave: the root lit, the scale's notes tinted */
        uint32_t d = (WS[i] + 12u - root) % 12u;
        int r = WS[i] == root, in = (mask >> d) & 1u;
        int32_t x = 17 + (int32_t)i * 30;
        uint16_t c = r ? acc : in ? tint : TH_CELL;
        tu_rect(x, 98, 27, 52, c);                     /* (a plain key: the black ones sit over two) */
        tu_text_c(x + 13, 141, &AF_TS, WN[i], r ? TC_BG : in ? TC_TEXT : TC_DIM, c);
    }
    for (i = 0; i < 5u; i++) {
        uint32_t d = (BS[i] + 12u - root) % 12u;
        int r = BS[i] == root, in = (mask >> d) & 1u;
        int32_t x = 17 + (int32_t)BX[i] * 30 + 18;
        uint16_t c = r ? acc : in ? tu_mix(RGB(14, 10, 6), acc, 45) : RGB(14, 10, 6);
        tu_rrect(x, 96, 20, 32, 3, c, TH_PBG);
        tu_text_c(x + 10, 119, &AF_TS, BN[i], r ? TC_BG : TC_QUIET, c);
    }
    tu_text(13, 160, &AF_TS, "LIT: THE ROOT \xB7 TINTED: THE SCALE'S NOTES", TC_QUIET, TH_PBG);
    th_lines("K1 ROOT K2 SCALE K3 CHORD K4 VOICING", 0);
}

/* ---- SEQ: STEP TOOLS (amber) ---- */
static void th_seq(void)
{
    static const char *const TK[8][2] = {{"F3", "CLEAR"}, {"G3", "REVERSE"}, {"A3", "SHIFT <"}, {"B3", "SHIFT >"},
                                         {"C4", "RANDOM"}, {"D4", "COOK"}, {"E4", "BEAT"}, {"F4", "EUCLID"}};
    static const char *const LK[6][2] = {{"G4", "LANE CLEAR"}, {"A4", "LANE REV"}, {"B4", "LANE FILL"}, {"C5", "LANE RND"},
                                         {"D5", "DOUBLE"}, {"E5", "HALF"}};
    uint16_t acc = TC_AMBER;
    track_t *t = TSEL;
    int drum = drum_track(t);
    uint32_t i;
    char l1[48], l2[48], l3[40], v[16];
    const char *u;
    str_cpy(l1, "T1 ", sizeof l1);
    l1[1] = (char)('1' + song.sel % NTRK);
    th_cat(l1, tu_inst_name(t), sizeof l1);
    if (drum) {
        th_cat(l1, " \xB7 LANE ", sizeof l1);
        th_cat(l1, drum_lane_name(t, ui.lane), sizeof l1);
    }
    if (eu.on) {                                        /* EUCLID open: its numbers */
        str_cpy(l2, "HITS ", sizeof l2);
        fmt_int(v, eu.hits);
        th_cat(l2, v, sizeof l2);
        th_cat(l2, " \xB7 STEPS ", sizeof l2);
        fmt_int(v, eu.steps);
        th_cat(l2, v, sizeof l2);
        th_cat(l2, " \xB7 ROT ", sizeof l2);
        fmt_int(v, eu.rot);
        th_cat(l2, v, sizeof l2);
        str_cpy(l3, "LET GO: APPLIED", sizeof l3);
    } else {
        str_cpy(l2, "LEN ", sizeof l2);
        fmt_int(v, t->p[P_SLEN]);                       /* (the steps) */
        th_cat(l2, v, sizeof l2);
        th_cat(l2, " \xB7 ", sizeof l2);
        param_format(track_desc(t, P_SDIV), t->p[P_SDIV], v, &u);
        th_cat(l2, v, sizeof l2);
        th_cat(l2, " \xB7 SW ", sizeof l2);
        fmt_int(v, t->p[P_SSWING]);
        th_cat(l2, v, sizeof l2);
        th_cat(l2, " \xB7 G ", sizeof l2);
        fmt_int(v, t->p[P_SGATE]);
        th_cat(l2, v, sizeof l2);
        str_cpy(l3, "OCT- BACK AS OPENED", sizeof l3);
    }
    th_dmd(eu.on ? "EUC" : "SEQ", 3, l1, l2, l3, acc);
    if (eu.on) {                                        /* EUCLID: its cycle as cells, 16 a row, the hits lit (the frame's) */
        uint32_t n = eu.steps ? eu.steps : 1u;
        th_panel(acc, "EUCLID \xB7 K1 HITS K2 STEPS K3 ROT");
        for (i = 0; i < n && i < NSTEP; i++) {
            int32_t x = 13 + (int32_t)(i % 16u) * 13, y = 100 + (int32_t)(i / 16u) * 17;
            int hit = eu_hit(i);
            tu_rrect(x, y, 11, 14, 2, hit ? acc : TH_CELL, TH_PBG);
        }
    } else {
        th_panel(acc, 0);
        tu_text(13, 90, &AF_TS, "THE TRACK", TC_QUIET, TH_PBG);
        tu_text(152, 90, &AF_TS, drum ? "THE LANE \xB7 LENGTH" : "LENGTH", TC_QUIET, TH_PBG);
        for (i = 0; i < 8u; i++) {                      /* (11 px text: the track's eight tools in two columns of four) */
            int32_t x = 13 + (int32_t)(i / 4u) * 70, y = 104 + (int32_t)(i % 4u) * 15;
            tu_text(x, y, &AF_TS, TK[i][0], acc, TH_PBG);
            tu_text(x + 18, y, &AF_TS, TK[i][1], TC_TEXT, TH_PBG);
        }
        for (i = 0; i < 6u; i++) {                      /* the lane's tools dim on a melodic track (DOUBLE, HALF: any) */
            int32_t y = 104 + (int32_t)i * 11;
            int dim = !drum && i < 4u;
            tu_text(152, y, &AF_TS, LK[i][0], dim ? TC_DIM : acc, TH_PBG);
            tu_text(170, y, &AF_TS, LK[i][1], dim ? TC_DIM : TC_TEXT, TH_PBG);
        }
    }
    if (drum) {                                         /* the strip: the 8 lanes (the selected one lit) */
        static const char *const LN[8] = {"1", "2", "3", "4", "5", "6", "7", "8"};
        tu_keystrip(8, LN, TF_LC, 1u << (ui.lane & 7u));
    } else {
        th_lines(0, "BLACK KEYS: LANES ON A DRUM TRACK");
    }
}

/* ---- ENV (round 12, HoldEnv.dc.html): a small sheet, the screen behind dimmed; F3 MUTATE (its mode chips, K2), G3 STUCK ---- */
static void th_env(void)
{
    static const char *const MN[3] = {"PATTERN", "SOUND", "BOTH"};
    uint32_t i, m = prf_mut_now();
    int busy = chain_busy();
    char q[24] = "QUICK HELP FOR T1";
    q[16] = (char)('1' + song.sel % NTRK);
    tu_rrect(5, 25, 230, 48, 6, RGB(32, 25, 15), TC_BG);   /* (the screen behind, dimmed to its frame) */
    tu_rrect(7, 27, 226, 44, 5, RGB(16, 12, 7), RGB(32, 25, 15));
    tu_rrect(5, 80, 230, 91, 6, TH_ENV, TC_BG);
    tu_rrect(7, 82, 226, 87, 5, TH_PBG, TH_ENV);
    tu_text(15, 93, &AF_TM, "HOLD ENV", TH_ENV, TH_PBG);
    tu_text(78, 94, &AF_TS, q, TC_QUIET, TH_PBG);
    tu_text(15, 115, &AF_TM, "F3", TC_AMBER, TH_PBG);
    tu_text(35, 115, &AF_TM, "MUTATE", busy ? TC_DIM : TC_TEXT, TH_PBG);
    tu_text(90, 116, &AF_TS, busy ? "STOP TO EDIT" : "CHANGE IT A LITTLE", TC_QUIET, TH_PBG);
    for (i = 0; i < 3u; i++) {                          /* the mode chips: K2 */
        int32_t x = 35 + (int32_t)i * 48;
        uint16_t c = i == m ? TH_ENV : RGB(44, 32, 20);
        tu_rrect(x, 126, 44, 14, 3, c, TH_PBG);
        tu_text_c(x + 22, 133, &AF_TS, MN[i], i == m ? TC_BG : TC_MID, c);
    }
    tu_text(186, 133, &AF_TS, "K2 MODE", TC_QUIET, TH_PBG);
    tu_text(15, 155, &AF_TM, "G3", TC_AMBER, TH_PBG);
    tu_text(35, 155, &AF_TM, "STUCK", busy ? TC_DIM : TC_TEXT, TH_PBG);
    tu_text(90, 156, &AF_TS, "COOK IT, OR START ONE", TC_QUIET, TH_PBG);
    tu_text_c(120, 184, &AF_TS, "NEW SONG: TAP ENV \xB7 TIMER, MUTATE MODE: ON SETUP", TC_QUIET, TC_BG);
}
static void th_body(void)
{
    tu_rect(0, TU_HY, TU_W, TU_FY - TU_HY, TC_BG);
    switch (ui.layer) {
    case LAYER_FX: th_fx(); break;
    case LAYER_GLO: th_glo(); break;
    case LAYER_SCL: th_scl(); break;
    case LAYER_TILT: th_env(); break;
    default: th_seq(); break;
    }
}
static uint32_t th_sig(void)
{
    uint32_t l = ui.layer, s = l * 7919u + song.sel * 131u + (uint32_t)song.g[G_BPM] * 977u + tsl_sig() * 19u + (uint32_t)song.playing;
    track_t *t = TSEL;
    uint32_t i;
    if (l == LAYER_FX) {
        s += th_fx_held() * 31u + (uint32_t)perf_latch_on * 11u + (uint32_t)perf_harm_on() * 3u;
        for (i = 0; i < 4u; i++)
            s = s * 33u + (uint32_t)(perf_k[i] + 128);
        for (i = 0; i < PF_KEYS; i++)
            s = s * 33u + perf_map[i];
    } else if (l == LAYER_GLO) {
        s += perf_solo * 7u + (uint32_t)ui.ml.on * 13u + (lys.fill ? 17u : 0u);
        for (i = 0; i < NTRK; i++)
            s = s * 31u + (uint32_t)trk[i].p[P_LEVEL] + (trk[i].p[P_MUTE] ? 101u : 0u);
    } else if (l == LAYER_TILT) {
        s += prf_mut_now() * 7u + (uint32_t)chain_busy() * 3u + (uint32_t)tu.scr * 101u;
    } else if (l == LAYER_SCL) {
        s += (uint32_t)t->p[P_ROOT] * 3u + (uint32_t)t->p[P_SCALE] * 37u + (uint32_t)t->p[P_CHRD] * 401u + (uint32_t)t->p[P_VOIC] * 4001u;
    } else {
        s += (uint32_t)t->p[P_SLEN] * 3u + (uint32_t)t->p[P_SDIV] * 37u + (uint32_t)t->p[P_SSWING] * 401u + (uint32_t)t->p[P_SGATE] * 4001u +
             (ui.lane & 7u) * 13u + (eu.on ? 1u + eu.hits * 7u + eu.steps * 131u + eu.rot * 1009u : 0u);
    }
    return s;
}
static void tu_hold_draw(void)
{
    uint32_t sig = th_sig(), l = ui.layer;
    tu_zone(0, 0, TU_HY, TC_BG, sig + tu_rec_sig() * 6007u, th_head);
    tu_zone(2, TU_HY, 100, TC_BG, sig, th_body);
    tu_zone(3, TU_HY + 100, TU_FY - TU_HY - 100, TC_BG, sig, th_body);
    if (l == LAYER_FX)
        tu_foot(3, "WHITE", "EFFECT", "PRESETS", "SWAP IT", "K1-K4", "SHAPE", 0, 0, 0, 0);
    else if (l == LAYER_GLO)
        tu_foot(3, "BLACK", "MUTE", "WHITE", "SOLO", "K1-K4", "LEVELS", 0, 0, 0, 0);
    else if (l == LAYER_SCL)
        tu_foot(3, "KEYS", "ROOT", "K2", "SCALE", "LET GO", "DONE", 0, 0, 0, 0);
    else if (l == LAYER_TILT)
        tu_foot(3, "F3", "MUTATE", "G3", "STUCK", "K2", "MODE", 0, 0, 0, 0);
    else
        tu_foot(3, "WHITE", "TOOL", "BLACK", "LANE", "K1-K4", "PATTERN", 0, 0, 0, 0);
    ui.force = 0;
}
