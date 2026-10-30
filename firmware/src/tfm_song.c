/* TFM-UI phase 6 and round 2 (tfm-spec/TFM-UI-SPEC.md 3 SCENES, SONG, SONGS): the song's screens. The scenes A..H are the
 * eight scenes of the song the music is in (flash, stored on a switch), the song is the chain; SONGS lists the three songs.
 * Included from tilt.c. */
#define TS_N NSCN                                       /* the scenes there are */
static uint8_t tsg_sel;                                 /* SONGS: the slot picked */
static void tsg_text_len(char *b, uint32_t bars)
{
    fmt_int(b, (int32_t)bars);
    str_cpy(b + str_len(b), bars == 1u ? " BAR" : " BARS", 6);
}
/* a scene's length (bars) and which tracks have a part: the live music for the scene now, else the stored slot */
static uint32_t tsg_info(uint32_t s, uint32_t *parts)
{
    uint32_t k, i, len = 0, bar = tfm_bar(), p = 0;
    if (s >= TS_N)
        return 0;
    if (s == tu_scene_cur()) {
        for (k = 0; k < NTRK; k++) {
            uint32_t l = tq_len(&trk[k]);
            for (i = 0; i < l; i++)
                if (tq_has_note(&trk[k], &trk[k].step[i])) {
                    p |= 1u << k;
                    break;
                }
            if (l > len) len = l;
        }
    } else if (project_used(s)) {
        proj_slot_info(s, &len, &p);
    }
    if (parts)
        *parts = p;
    return (len + bar - 1u) / bar;
}
static uint32_t tsg_row_bars(const chain_row_t *r)
{
    uint32_t k, best = 0;
    for (k = 0; k < NTRK; k++)
        if (r->slot[k] < TS_N) {
            uint32_t b = tsg_info(r->slot[k], 0);
            if (b > best) best = b;
        }
    return (best ? best : 1u) * r->repeat;              /* (round 4: every lane silent or blank: 1 bar) */
}
static uint32_t tsg_song_bars(void)
{
    uint32_t i, n = 0;
    for (i = 0; i < chain_config.count; i++)
        n += tsg_row_bars(&chain_config.row[i]);
    return n;
}
static void tsg_time_text(char *b, uint32_t bars)       /* bars -> "1:17" at the tempo (4 beats a bar) */
{
    uint32_t sec = bars * 240u / (uint32_t)clamp(song.g[G_BPM], 40, 300), n;
    fmt_int(b, (int32_t)(sec / 60u));
    n = str_len(b);
    b[n] = ':';
    b[n + 1u] = (char)('0' + (sec % 60u) / 10u);
    b[n + 2u] = (char)('0' + sec % 10u);
    b[n + 3u] = 0;
}
/* ================================================================== SCENES ================================== */
static uint32_t tsg_used_sig(void)                      /* which scenes are stored and which song this is (the zones' signatures) */
{
    return tu_used_mask() * 3u + (uint32_t)sng_cur * 17u;
}
static void tsg_chain_chips(int32_t y)
{
    uint32_t i, cur = chain.running ? chain.row : 0xFFu;
    int32_t x = 16, row = 0;
    char l[2] = {0, 0}, xs[4];
    for (i = 0; i < chain_config.count; i++) {
        const chain_row_t *r = &chain_config.row[i];
        uint8_t same = 1;
        uint32_t k;
        int32_t w;
        for (k = 1; k < NTRK; k++)
            same &= r->slot[k] == r->slot[0];
        l[0] = same && r->slot[0] < TS_N ? (char)('A' + r->slot[0]) : '*';
        xs[0] = 0;
        if (r->repeat > 1u) {
            str_cpy(xs, "\xD7", 3);
            fmt_int(xs + 1, r->repeat);
        }
        w = 12 + text_w(&AF_TL, l) + (xs[0] ? 3 + text_w(&AF_TS, xs) : 0);
        if (x + w > 232) {
            x = 16;
            row++;
        }
        tu_rrect(x, y + row * 25, w, 20, 4, i == cur ? TC_AMBER : TC_SURF, TC_BG);
        {
            int32_t tx = tu_text(x + 6, y + row * 25 + 10, &AF_TL, l, i == cur ? TC_BG : TC_TEXT, i == cur ? TC_AMBER : TC_SURF);
            if (xs[0])
                tu_text(tx + 3, y + row * 25 + 10, &AF_TS, xs, i == cur ? TC_BG : TC_MID, i == cur ? TC_AMBER : TC_SURF);
        }
        x += w + 4;
    }
}
static void tsg_scenes_body(void)                       /* y 22 .. 215 */
{
    uint32_t s, k, cur = tu_scene_cur();
    char b[12], t[12];
    tu_rect(0, 22, TU_W, 193, TC_BG);
    for (s = 0; s < TU_NSCENE; s++) {
        int32_t x = 8 + (int32_t)(s % 4u) * 57, y = 29 + (int32_t)(s / 4u) * 46;
        uint32_t parts = 0, bars = s < TS_N ? tsg_info(s, &parts) : 0u;
        int is_cur = s == cur, used = s < TS_N && (is_cur || project_used(s));
        char l[2] = {(char)('A' + s), 0};
        tu_rrect(x, y, 53, 42, 6, is_cur ? TC_AMBER : used ? TC_SURF : TC_SURF, TC_BG);
        tu_rrect(x + 1, y + 1, 51, 40, 5, is_cur ? RGB(58, 42, 16) : used ? TC_SURF : TC_BG, is_cur ? TC_AMBER : TC_SURF);
        tu_text(x + 6, y + 12, &AF_TX, l, used ? TC_TEXT : TC_DIM, is_cur ? RGB(58, 42, 16) : used ? TC_SURF : TC_BG);
        if (used && bars) {
            tsg_text_len(b, bars);
            tu_text_r(x + 49, y + 9, &AF_TS, b, TC_QUIET, is_cur ? RGB(58, 42, 16) : TC_SURF);
        }
        for (k = 0; k < NTRK; k++) {
            int has = (parts >> k) & 1u;
            tu_rrect(x + 6, y + 22 + (int32_t)k * 5, has ? 41 : 16, 3, 1, has ? TU_TC[k] : RGB(46, 37, 24),
                     is_cur ? RGB(58, 42, 16) : used ? TC_SURF : TC_BG);
        }
        (void)t;
    }
    tu_text(16, 129 + 4, &AF_TM, "SONG", TC_AMBER, TC_BG);
    tu_text(16 + text_w(&AF_TM, "SONG") + 6, 129 + 4, &AF_TS, "GLO edits it", TC_QUIET, TC_BG);
    {
        uint32_t bars = tsg_song_bars();
        tsg_text_len(b, bars);
        str_cpy(b + str_len(b), " \xB7 ", 5);
        tsg_time_text(t, bars);
        str_cpy(b + str_len(b), t, 8);
        tu_text_r(224, 133, &AF_TS, b, TC_QUIET, TC_BG);
    }
    tsg_chain_chips(148);
}
static void tsg_scenes_head(void)
{
    char t[8];
    tu_rect(0, 0, TU_W, TU_HY, TC_BG);
    tu_rect(0, TU_HY - 1, TU_W, 1, TC_SURF);
    tu_rec_dots(TC_BG);
    tu_text(TU_DX, 10, &AF_TM, "SCENES", TC_TEXT, TC_BG);
    {
        char sn[8] = "S1";
        sn[1] = (char)('1' + sng_cur % NSONG);
        tu_text(TU_DX + text_w(&AF_TM, "SCENES") + 5, 12, &AF_TS, sn, TC_QUIET, TC_BG);
    }
    tu_timer_text(t);
    if (t[0])
        tu_text_c(120, 10, &AF_PS, t, TC_AMBER, TC_BG);
    {
        char b[8];
        int32_t w;
        fmt_int(b, song.g[G_BPM]);
        w = text_w(&AF_TM, b);
        tu_text(231 - w, 10, &AF_TM, b, TC_TEXT, TC_BG);
        tu_play_icon(231 - w - 11, 10, song.playing, TC_TEXT);
    }
}
static void tu_scenes_draw(void)
{
    uint32_t sig = scene_cur() * 7u + tsg_used_sig() +
                   chain_config.count * 31u + (chain.running ? chain.row + 1u : 0u) * 977u + (uint32_t)song.g[G_BPM] * 131u +
                   (tm.on ? tm.left : 0u) + steps_hash(&trk[0]) + steps_hash(&trk[1]) * 3u + steps_hash(&trk[2]) * 5u + steps_hash(&trk[3]) * 7u;
    uint32_t i;
    for (i = 0; i < chain_config.count; i++)
        sig = sig * 31u + chain_config.row[i].repeat + chain_config.row[i].slot[0] * 7u + chain_config.row[i].slot[1] * 13u;
    tu_zone(0, 0, TU_HY, TC_BG, sig + tu_rec_sig() * 6007u, tsg_scenes_head);
    tu_zone(2, 22, 100, TC_BG, sig, tsg_scenes_body);
    tu_zone(3, 122, 93, TC_BG, sig, tsg_scenes_body);
    tu_foot(3, "KEY", "GO", "KEY+KEY", "COPY", "HOLD G5", "CHAIN", 0, 0, 0, 0);
    ui.force = 0;
}
/* ================================================================== SONG ==================================== */
static void tsg_song_rows(void)                         /* y 22 .. 215 */
{
    uint32_t n = chain_config.count, top, r, k;
    int32_t y;
    char b[16], blank;
    tu_rect(0, 22, TU_W, 193, TC_BG);
    top = ui.song_row >= 4u ? ui.song_row - 3u : 0u;
    for (r = top; r < top + 4u; r++) {
        int sel = r == ui.song_row;
        y = 30 + (int32_t)(r - top) * 38;
        tu_rrect(8, y, 224, 34, 7, sel ? RGB(58, 42, 16) : TC_SURF, TC_BG);
        if (sel)
            tu_rrect(8, y, 224, 34, 7, TC_AMBER, TC_BG), tu_rrect(9, y + 1, 222, 32, 6, RGB(58, 42, 16), TC_AMBER);
        if (r >= n) {
            tu_text_c(120, y + 17, &AF_TM, r == n ? "+ ADD SECTION  [OCT+]" : "", TC_QUIET, sel ? RGB(58, 42, 16) : TC_SURF);
            continue;
        }
        fmt_int(b, (int32_t)r + 1);
        tu_text(16, y + 17, &AF_TX, b, chain.running && chain.row == r ? TC_AMBER : TC_TEXT, sel ? RGB(58, 42, 16) : TC_SURF);
        blank = 0;
        for (k = 0; k < NTRK; k++) {
            uint8_t sl = chain_config.row[r].slot[k];
            int dim = sl < TS_N && !project_used(sl);   /* (round 4: a blank scene plays silence: dim, EMPTY) */
            char l[2] = {sl < TS_N ? (char)('A' + sl) : '-', 0};
            int32_t x = 46 + (int32_t)k * 34;
            uint16_t cbg = sl < TS_N && !dim ? TU_TC[k] : RGB(46, 37, 24);
            tu_rrect(x, y + 5, 30, 24, 5, cbg, sel ? RGB(58, 42, 16) : TC_SURF);
            tu_text_c(x + 15, y + 17, &AF_TL, l, sl < TS_N && !dim ? TC_BG : TC_QUIET, cbg);
            if (dim && !blank)
                blank = l[0];
        }
        str_cpy(b, "\xD7", 3);
        fmt_int(b + 1, chain_config.row[r].repeat);
        tu_text(186, y + 12, &AF_TL, b, TC_TEXT, sel ? RGB(58, 42, 16) : TC_SURF);
        if (blank) {
            b[0] = blank;
            str_cpy(b + 1, " EMPTY", 8);
        } else {
            tsg_text_len(b, tsg_row_bars(&chain_config.row[r]));
        }
        tu_text_r(226, y + 25, &AF_TS, b, TC_QUIET, sel ? RGB(58, 42, 16) : TC_SURF);
    }
    tu_text(10, 190, &AF_TS, "SOUNDS", TC_QUIET, TC_BG);
    tu_text(10 + text_w(&AF_TS, "SOUNDS") + 4, 190, &AF_TM, (ui_prefs3 & PREF_SONG_SCENE) ? "SCENE" : "CURRENT", TC_AMBER, TC_BG);
    tsg_text_len(b, tsg_song_bars());
    tu_text_r(230, 190, &AF_TS, b, TC_QUIET, TC_BG);
}
static void tsg_song_head(void)
{
    char t[8];
    tu_rect(0, 0, TU_W, TU_HY, TC_BG);
    tu_rect(0, TU_HY - 1, TU_W, 1, TC_SURF);
    tu_rec_dots(TC_BG);
    tu_text(TU_DX, 10, &AF_TM, "SONG", TC_TEXT, TC_BG);
    {
        char sn[4] = " 1";
        sn[1] = (char)('1' + sng_cur % NSONG);
        tu_text(TU_DX + text_w(&AF_TM, "SONG"), 10, &AF_TM, sn, TC_TEXT, TC_BG);
    }
    tu_timer_text(t);
    if (t[0])
        tu_text_c(120, 10, &AF_PS, t, TC_AMBER, TC_BG);
    tu_text_r(231, 10, &AF_TS, chain.running ? "PLAYING" : "PRESETS reps", chain.running ? TC_GOOD : TC_QUIET, TC_BG);
}
static void tu_song_draw(void)
{
    uint32_t i, sig = ui.song_row * 7u + chain_config.count * 31u + (chain.running ? chain.row + 1u : 0u) * 977u + ((ui_prefs3 & PREF_SONG_SCENE) ? 5u : 0u) +
                      tsg_used_sig();
    for (i = 0; i < chain_config.count; i++)
        sig = sig * 31u + chain_config.row[i].repeat + chain_config.row[i].slot[0] * 7u + chain_config.row[i].slot[1] * 13u +
              chain_config.row[i].slot[2] * 17u + chain_config.row[i].slot[3] * 19u;
    tu_zone(0, 0, TU_HY, TC_BG, sig + tu_rec_sig() * 6007u, tsg_song_head);
    tu_zone(2, 22, 100, TC_BG, sig, tsg_song_rows);
    tu_zone(3, 122, 93, TC_BG, sig, tsg_song_rows);
    tu_foot(3, "K1-K4", "SCENES", "OCT+", "ADD", "EDIT", "DELETE", 0, 0, 0, 0);
    ui.force = 0;
}
static void tu_song_input(uint32_t oct)
{
    uint32_t k;
    int32_t s;
    chain_row_t *r;
    if ((s = panel_enc(EN_SELECT)) != 0)
        ui.song_row = (uint8_t)clamp((int32_t)ui.song_row + (s > 0 ? 1 : -1), 0, chain_config.count ? (int32_t)chain_config.count - 1 : 0);
    if (ui.song_row >= chain_config.count && chain_config.count)
        ui.song_row = (uint8_t)(chain_config.count - 1u);
    r = chain_config.count ? &chain_config.row[ui.song_row] : 0;
    for (k = 0; k < 4u; k++)
        if ((s = panel_enc(EN_K1 + k)) != 0 && r) {
            if (chain_busy()) { ui_message("STOP TO EDIT"); continue; }
            {                                           /* (round 4: the knob runs "-" A B .. H: "-" first, no wrap) */
                int32_t p = r->slot[k] >= TS_N ? 0 : (int32_t)r->slot[k] + 1;
                p = clamp(p + (s > 0 ? 1 : -1), 0, (int32_t)TS_N);
                r->slot[k] = p ? (uint8_t)(p - 1) : (uint8_t)CHAIN_SILENT;
            }
        }
    if ((s = panel_enc(EN_PRESET)) != 0 && r && !chain_busy())
        r->repeat = (uint8_t)clamp((int32_t)r->repeat + (s > 0 ? 1 : -1), 1, 16);
    if (oct & 2u) {
        if (chain_busy()) {
            ui_message("STOP TO EDIT");
        } else if (chain_config.count >= CHAIN_ROWS) {
            ui_message("SONG FULL");
        } else {
            song_add();
        }
    }
}
static void tu_song_delete(void)
{
    uint32_t i;
    if (chain_busy()) {
        ui_message("STOP TO EDIT");
        return;
    }
    if (chain_config.count <= 1u || ui.song_row >= chain_config.count) {
        ui_message("A SONG KEEPS ONE SECTION");
        return;
    }
    for (i = ui.song_row; i + 1u < chain_config.count; i++)
        chain_config.row[i] = chain_config.row[i + 1u];
    chain_config.count--;
    if (ui.song_row >= chain_config.count)
        ui.song_row = (uint8_t)(chain_config.count - 1u);
    ui_message("SECTION DELETED");
}
static void tu_song_sounds(void)                        /* GLO again: SOUNDS SCENE / CURRENT (the song's sounds from each scene, or the current ones) */
{
    if (chain_busy()) {
        ui_message("STOP TO EDIT");
        return;
    }
    ui_prefs3 ^= PREF_SONG_SCENE;
    settings_save();
    ui_message(ui_prefs3 & PREF_SONG_SCENE ? "SOUNDS: SCENE" : "SOUNDS: CURRENT");
    ui.force = 1;
}
/* ================================================================== SONGS =================================== */
/* The three songs (round 2): the slot picked with SELECT; OCT+ LOADs it (the song now is stored first), SAVE stores the
 * song now into it (the NAME screen; a dialog first when it holds another song), EDIT clears it, GLO then GLO copies it onto
 * another slot. Every dialog has NO picked (OCT- / HOME cancel). The scenes of a song are 8 flash objects each (project.c). */
static uint32_t tsg_song_bars_n(uint32_t n)             /* song n's length in bars */
{
    return n == sng_cur ? tsg_song_bars() : song_bars_n(n, tfm_bar());
}
static void tsg_song_name(uint32_t i, char *nm)         /* the name shown for slot i ("" none) */
{
    if (i == sng_cur)
        project_cur_name(nm);
    else
        memcpy(nm, song_name_of(i), 16);
}
static void tsg_songs_rows(void)                        /* y 22 .. 215 */
{
    uint32_t i;
    char b[24], nm[16];
    tu_rect(0, 22, TU_W, 193, TC_BG);
    for (i = 0; i < NSONG; i++) {
        int32_t y = 30 + (int32_t)i * 50;
        int sel = i == tsg_sel, live = i == sng_cur, has = live || song_stored(i);
        uint16_t bg = sel ? TC_AMBER : TC_SURF;
        tu_rrect(12, y, 216, 44, 8, bg, TC_BG);
        if (has) {
            uint32_t n = song_nscenes(i);
            tsg_song_name(i, nm);
            tu_text(24, y + 14, &AF_TL, nm[0] ? nm : "UNTITLED", sel ? TC_BG : TC_TEXT, bg);
            fmt_int(b, (int32_t)n);
            str_cpy(b + str_len(b), n == 1u ? " scene \xB7 " : " scenes \xB7 ", 14);
            tsg_text_len(b + str_len(b), tsg_song_bars_n(i));
            tu_text(24, y + 31, &AF_TS, b, sel ? TC_BG : TC_QUIET, bg);
        } else {
            tu_text(24, y + 14, &AF_TL, "EMPTY", sel ? TC_BG : TC_DIM, bg);
            tu_text(24, y + 31, &AF_TS, "SAVE puts the song now here", sel ? TC_BG : TC_DIM, bg);
        }
        tu_text_r(220, y + 14, &AF_TS, tsg_copy == i ? "COPY FROM HERE" : live ? "THE SONG NOW" : "", sel ? TC_BG : TC_AMBER, bg);
        b[0] = (char)('1' + i);
        b[1] = 0;
        tu_text_r(220, y + 31, &AF_TS, b, sel ? TC_BG : TC_QUIET, bg);
    }
    {
        static const char *const K0[3] = {"EDIT", "GLO", "OCT-"}, *const A0[3] = {"CLEAR", "COPY", "BACK"};
        static const char *const K1[3] = {"GLO", "SELECT", "OCT-"}, *const A1[3] = {"COPY HERE", "PICK", "CANCEL"};
        static const char *const KJ[1] = {"SEQ"}, *const AJ[1] = {"SAVE JAM HERE"};   /* (round 6) */
        if (tsg_copy == 0xFFu) {
            tu_hint_line(184, TC_BG, TC_QUIET, 1, KJ, AJ);
            tu_hint_line(202, TC_BG, TC_QUIET, 3, K0, A0);
        } else
            tu_hint_line(198, TC_BG, TC_QUIET, 3, K1, A1);
    }
}
static void tsg_songs_head(void)
{
    char t[8];
    tu_rect(0, 0, TU_W, TU_HY, TC_BG);
    tu_rect(0, TU_HY - 1, TU_W, 1, TC_SURF);
    tu_rec_dots(TC_BG);
    tu_text(TU_DX, 10, &AF_TM, tsg_new ? "REPLACE WHICH?" : "SONGS", tsg_new ? TC_AMBER : TC_TEXT, TC_BG);
    tu_timer_text(t);
    if (t[0])
        tu_text_c(120, 10, &AF_PS, t, TC_AMBER, TC_BG);
    tu_text_r(231, 10, &AF_TS, "autosaved", TC_QUIET, TC_BG);
}
static void tu_songs_draw(void)
{
    uint32_t i, sig = tsg_sel * 7u + (uint32_t)tsg_new * 53u + (uint32_t)tsg_copy * 29u + tsg_used_sig() + chain_config.count * 31u;
    char nm[16];
    for (i = 0; i < NSONG; i++) {
        tsg_song_name(i, nm);
        sig = sig * 31u + (uint32_t)song_stored(i) * 5u + str_hash(3u, nm) + song_nscenes(i) * 977u + tsg_song_bars_n(i) * 131u;
    }
    tu_zone(0, 0, TU_HY, TC_BG, sig + tu_rec_sig() * 6007u, tsg_songs_head);
    tu_zone(2, 22, 100, TC_BG, sig, tsg_songs_rows);
    tu_zone(3, 122, 93, TC_BG, sig, tsg_songs_rows);
    if (tsg_new)
        tu_foot(3, "SELECT", "PICK", "OCT+", "REPLACE", "OCT-", "CANCEL", 0, 0, 0, 0);
    else
        tu_foot(3, "SELECT", "PICK", "OCT+", "LOAD", "SAVE", "SAVE", 0, 0, 0, 0);
    ui.force = 0;
}
static void tu_songs_load(void)
{
    tsg_copy = 0xFF;
    if (tsg_sel == sng_cur) {
        ui_message("THE SONG NOW");
    } else if (!song_stored(tsg_sel)) {
        ui_message("EMPTY SONG");
    } else if (transport_busy()) {
        ui_message("STOP TO LOAD");
    } else {
        (void)song_load(tsg_sel);
    }
    ui.force = 1;
}
static void tu_songs_save(void)                         /* SAVE on SONGS: the song now, under a name, into the slot picked */
{
    tsg_copy = 0xFF;
    if (transport_busy())
        ui_message("STOP TO SAVE");
    else if (tsg_sel != sng_cur && song_stored(tsg_sel))
        confirm_open(CF_SONG_OVR, tsg_sel);
    else
        name_open(NK_SONG_SAVE, tsg_sel);
    ui.force = 1;
}
/* round 6: SEQ on SONGS: SAVE JAM HERE: the jam played becomes the song's chain (as NEXT BALL), then saved into the slot
 * picked as SAVE does (another song there: asked first, Cancel the default; then the NAME screen) */
static void tu_songs_jam(void)
{
    tsg_copy = 0xFF;
    if (transport_busy()) {
        ui_message("STOP TO SAVE");
    } else if (!jl_keep()) {
        ui_message("NO JAM PLAYED YET");
    } else {
        tu_songs_save();
        return;
    }
    ui.force = 1;
}
static void tu_songs_copy(void)                         /* GLO: the slot picked is the source; GLO again on another: the target */
{
    uint32_t from;
    if (transport_busy()) {
        tsg_copy = 0xFF;
        ui_message("STOP TO COPY");
    } else if (tsg_copy == 0xFFu) {
        if (!song_stored(tsg_sel) && tsg_sel != sng_cur)
            ui_message("EMPTY SONG");
        else {
            tsg_copy = tsg_sel;
            ui_message("COPY TO: PICK, GLO");
        }
    } else if (tsg_copy == tsg_sel) {
        tsg_copy = 0xFF;
        ui_message("COPY OFF");
    } else if (tsg_sel == sng_cur) {
        tsg_copy = 0xFF;
        ui_message("NOT OVER THE SONG NOW");
    } else {
        from = tsg_copy;
        tsg_copy = 0xFF;
        if (song_stored(tsg_sel))
            confirm_open(CF_SONG_COPY, from << 2 | tsg_sel);
        else
            (void)song_copy(from, tsg_sel);
    }
    ui.force = 1;
}
static void tu_songs_clear(void)                        /* EDIT: the slot picked and its scenes (a dialog, NO picked) */
{
    tsg_copy = 0xFF;
    if (transport_busy())
        ui_message("STOP TO ERASE");
    else if (!song_stored(tsg_sel) && tsg_sel != sng_cur)
        ui_message("EMPTY SONG");
    else
        confirm_open(CF_SONG_CLR, tsg_sel);
    ui.force = 1;
}
static void tu_songs_input(uint32_t oct)
{
    int32_t s;
    if ((s = panel_enc(EN_SELECT)) != 0)
        tsg_sel = (uint8_t)clamp((int32_t)tsg_sel + (s > 0 ? 1 : -1), 0, (int32_t)NSONG - 1);
    if (tsg_new) {                                      /* round 3: which song does the new one replace? OCT+ that one, OCT- / HOME: DICE again */
        if (oct & 2u) {
            tsg_new = 0;
            tfm_new_slot = tsg_sel;
            ns_start();
        } else if (oct & 1u) {
            tsg_new = 0;
            tilt_ns_show();
        }
        return;
    }
    if (oct & 2u) {
        tu_songs_load();
    } else if (oct & 1u) {
        tsg_copy = 0xFF;
        tu_goto(TU_JAM);
    }
}
