/* TFM-UI round 2 (8 scenes x 3 songs): the screens' tests (included by ui_test.c after the round 1 tests; same helpers). Here
 * the scenes and songs are RAM (SCN_RAM 24, no flash); the flash side (the map, the migration, the switch while playing) is
 * tests/persistence_test.c. */
#define R2(c) do { if (!(c)) { ok = 0; fprintf(stderr, "R2 line %d ", __LINE__); } } while (0)
/* 3: SCENES: all eight tiles are scenes; a key per scene (C4..C5) goes there, stored on the way out */
static int test_r2_scenes(void)
{
    int bad = 0, ok = 1;
    uint32_t s;
    ui_power_on();
    tu_off = 0;
    sng_cur = 0;
    memset(proj_slot, 0, sizeof proj_slot);
    chain_defaults(&chain_config);
    scene_claim();
    tu_goto(TU_JAM);
    for (s = 0; s < NSCN; s++) {                        /* eight scenes, each with its own note */
        sc_note(0, 0, 40u + s);
        tilt_scene_go((s + 1u) % NSCN, 0);
        R2(scene_cur() == (s + 1u) % NSCN);
    }
    R2(tu_used_mask() == 0xFFu);                        /* (A.. H all stored, the last on the way back to A) */
    for (s = 0; s < NSCN; s++) {                        /* what the tiles draw: a length and the tracks with a part, for E..H as for A..D */
        uint32_t parts = 0, bars = tsg_info(s, &parts);
        R2(tu_scene_used(s) && bars == 1u && (parts & 1u));
    }
    for (s = 0; s < NSCN; s++) {                        /* the keys C4..C5 */
        uint32_t k = key_at(0, SK_SCENE + s);
        fm1_in.notes |= 1u << k; scene_key(k, SK_SCENE + s);
        fm1_in.notes &= ~(1u << k); scene_keys_poll();
        R2(scene_cur() == s && trk[0].step[0].note[0] == 40u + s);
        R2(!str_eq(ui.msg, "E-H: NO FLASH YET"));
    }
    ui.layer = LAYER_SCENE;
    frames(48);
    bad += check("TFM-UI round 2 SCENES: all eight scenes A..H store and are reached by C4..C5, none says no flash; the tiles are live", ok);
    return bad;
}
/* 3: the SONG chain uses A..H (K1..K4 step through them, then "-"), and plays them */
static int test_r2_song_chain(void)
{
    int bad = 0, ok = 1;
    uint32_t s, k;
    ui_power_on();
    tu_off = 0;
    sng_cur = 0;
    memset(proj_slot, 0, sizeof proj_slot);
    chain_defaults(&chain_config);
    scene_claim();
    for (s = 0; s < NSCN; s++) {                        /* the scenes, each a note of its own on every track */
        for (k = 0; k < NTRK; k++) {
            trk[k].p[P_SLEN] = 16; trk[k].p[P_SDIV] = 2; trk[k].p[P_SSWING] = 0;
            sc_note(k, 0, 40u + s * 4u + k);
        }
        tilt_scene_go((s + 1u) % NSCN, 0);
    }
    chain_config.count = 2;
    chain_config.row[0] = chain_row_of(0, 1);
    chain_config.row[1] = chain_row_of(0, 1);
    tu_goto(TU_SONG);
    ui.song_row = 1;
    frames(32);
    for (s = 1; s <= 9u; s++) {                         /* K1 up: B, C, .. H, then "-", and no further */
        turn(EN_K1, 1);
        R2(chain_config.row[1].slot[0] == (s < 8u ? s : CHAIN_SILENT));
    }
    R2(chain_valid(&chain_config));
    turn(EN_K1, -2);
    R2(chain_config.row[1].slot[0] == 7u);               /* back to H */
    chain_config.row[1].slot[1] = 4;                     /* T2 on E, T3 on F, T4 on G */
    chain_config.row[1].slot[2] = 5;
    chain_config.row[1].slot[3] = 6;
    frames(48);
    R2(chain_prepare() == 0 && chain.armed);
    events_block(32);
    R2(chain.running && chain.row == 0);
    R2(seq_steps(&trk[0])[0].note[0] == 40u && chain.lane[0] == 0u);
    {   /* the second section, from the pool for E..H */
        uint32_t n;
        for (n = 0; n < 8000u && chain.row < 1u; n++) events_block(32);
        R2(chain.row == 1u && chain.lane[0] == 7u && chain.lane[1] == 4u && chain.lane[3] == 6u);
        R2(seq_steps(&trk[0])[0].note[0] == 40u + 7u * 4u + 0u && seq_steps(&trk[1])[0].note[0] == 40u + 4u * 4u + 1u &&
           seq_steps(&trk[2])[0].note[0] == 40u + 5u * 4u + 2u && seq_steps(&trk[3])[0].note[0] == 40u + 6u * 4u + 3u);
    }
    seq_stop();
    chain_config.row[1].slot[0] = 8;
    chain_config.row[1].slot[2] = 9;
    R2(!chain_valid(&chain_config));                     /* (past "-": no section) */
    bad += check("TFM-UI round 2 SONG: a section's lanes name A..H or \"-\", the song plays scenes E..H (their patterns) in the right lane", ok);
    return bad;
}
/* 3: SONGS: three live slots (name, scenes, length), load, save, copy, clear (a dialog, NO picked) */
static int test_r2_songs(void)
{
    int bad = 0, ok = 1;
    uint32_t s;
    char nb[16];
    project_t pa, pb;
    ui_power_on();
    tu_off = 0;
    sng_cur = 0;
    memset(proj_slot, 0, sizeof proj_slot);
    memset(song_rec, 0, sizeof song_rec);
    chain_defaults(&chain_config);
    memset(proj_name, 0, sizeof proj_name);
    scene_claim();
    str_cpy(proj_name, "ONE", sizeof proj_name);
    song.g[G_BPM] = 100;
    for (s = 0; s < 3u; s++) {                          /* song 1: scenes A, B, C */
        sc_note(0, 0, 50u + s);
        tilt_scene_go((s + 1u) % 3u, 0);
    }
    chain_config.count = 2;
    chain_config.row[0] = chain_row_of(0, 2);
    chain_config.row[1] = chain_row_of(1, 1);
    R2(song_nscenes(0) == 3u && !song_stored(0));
    tu_goto(TU_SONGS);
    tsg_sel = 1;
    frames(48);
    R2(song_stored(0) == 0);
    /* SAVE on slot 2: the NAME screen, then the song is stored there and the music is song 2 */
    stap(B_SAVE);
    R2(nm.kind == NK_SONG_SAVE && nm.slot == 1u);
    str_cpy(nm.s, "TWO", sizeof nm.s); nm.len = nm.cur = 3;
    name_ok();
    R2(!nm.kind && sng_cur == 1u && song_stored(0) && song_stored(1) && song_nscenes(1) == 3u);
    R2(str_eq(song_name_of(0), "ONE") && str_eq(song_name_of(1), "TWO"));
    /* song 2 now: change its tempo and chain; song 1 keeps its own */
    song.g[G_BPM] = 130;
    chain_config.count = 1;
    chain_config.row[0] = chain_row_of(2, 3);
    sc_note(0, 1, 77);
    R2(song_load(0) == 0);
    R2(sng_cur == 0u && song.g[G_BPM] == 100 && chain_config.count == 2 && chain_config.row[0].repeat == 2 && str_eq(proj_name, "ONE"));
    R2(scene_cur() == 0u && trk[0].step[0].note[0] == 50u);
    R2(song_load(1) == 0);
    R2(sng_cur == 1u && song.g[G_BPM] == 130 && chain_config.count == 1 && chain_config.row[0].repeat == 3 && str_eq(proj_name, "TWO"));
    R2(scn_get(1 * NSCN + 0, &pa) && pa.t[0].step[1].note[0] == 77u);   /* (the scene edited was stored on the way out) */
    R2(song_load(2) == 1 && str_eq(ui.msg, "EMPTY SONG") && sng_cur == 1u);
    /* copy: song 1 onto slot 3 (GLO, SELECT, GLO); the song now cannot be copied over */
    tu_goto(TU_SONGS);
    tsg_sel = 0;
    stap(B_GLO);
    R2(tsg_copy == 0u && str_eq(ui.msg, "COPY TO: PICK, GLO"));
    turn(EN_SELECT, 1);
    stap(B_GLO);
    R2(tsg_copy == 0xFFu && str_eq(ui.msg, "NOT OVER THE SONG NOW") && !song_stored(2));   /* (slot 2 is the song now) */
    tsg_sel = 0;
    stap(B_GLO);
    turn(EN_SELECT, 1); turn(EN_SELECT, 1);
    R2(tsg_sel == 2u);
    stap(B_GLO);
    R2(song_stored(2) && str_eq(song_name_of(2), "ONE") && song_nscenes(2) == 3u && song_rec[2].chain.count == song_rec[0].chain.count);
    R2(scn_get(2 * NSCN + 1, &pa) && scn_get(0 * NSCN + 1, &pb) && pa.t[0].step[0].note[0] == pb.t[0].step[0].note[0]);
    /* copy over a stored one: the dialog, NO picked */
    tsg_sel = 0;
    stap(B_GLO);
    tsg_sel = 2;
    stap(B_GLO);
    R2(ui.confirm == CF_SONG_COPY);
    press(B_OCTDN);                                     /* OCT- cancels */
    R2(!ui.confirm && song_stored(2));
    /* clear: EDIT opens the dialog (NO picked); HOME and OCT- leave it; OCT+ clears the song and its scenes */
    tu_goto(TU_SONGS);
    tsg_sel = 2;
    stap(B_EDIT);
    R2(ui.confirm == CF_SONG_CLR && song_stored(2));
    stap(B_HOME);                                       /* one HOME tap out of the dialog */
    R2(!ui.confirm && song_stored(2) && song_nscenes(2) == 3u);
    stap(B_EDIT);
    press(B_OCTDN);
    R2(!ui.confirm && song_stored(2));
    stap(B_EDIT);
    press(B_OCTUP);
    R2(!ui.confirm && !song_stored(2) && song_nscenes(2) == 0u && sng_cur == 1u);
    tsg_sel = 2;
    stap(B_EDIT);
    R2(!ui.confirm && str_eq(ui.msg, "EMPTY SONG"));      /* (nothing to clear) */
    /* load from the screen: OCT+ */
    tsg_sel = 0;
    press(B_OCTUP);
    R2(sng_cur == 0u && str_eq(proj_name, "ONE"));
    /* clearing the song now blanks the music (a blank scene A, no chain, no name) */
    song_clear(0);
    R2(!song_stored(0) && song_nscenes(0) == 0u && chain_config.count == 0 && !proj_name[0] && scene_cur() == 0u);
    /* the screen draws all three rows (name, scenes, length) without a lint */
    tu_goto(TU_SONGS);
    frames(48);
    song_load(1);
    tu_goto(TU_SONGS);
    frames(48);
    tsg_song_name(1, nb);
    R2(str_eq(nb, "TWO"));
    bad += check("TFM-UI round 2 SONGS: save under a name into a slot, load, copy (GLO, pick, GLO), clear (a dialog, NO picked, HOME and OCT- leave it)", ok);
    return bad;
}
/* 3: autosave and the settings cover the current song and scene: a restore is in them again */
static int test_r2_remember(void)
{
    int bad = 0, ok = 1;
    ui_power_on();
    tu_off = 0;
    sng_cur = 0;
    memset(proj_slot, 0, sizeof proj_slot);
    memset(song_rec, 0, sizeof song_rec);
    chain_defaults(&chain_config);
    scene_claim();
    sc_note(0, 0, 61);
    tilt_scene_go(5, 0);
    R2(scene_cur() == 5u);
    song_save_named(2, "THREE");
    R2(sng_cur == 2u);
    tilt_scene_poll();
    R2((SCN_MEM & 3u) == 2u && (SCN_MEM & 4u) && (SCN_MEM >> 4) == 6u);
    sng_cur = 0; proj_cur = PROJ_NO_SLOT;                /* a power cycle's restore: the autosave is the music, the settings say where */
    scene_boot(1);
    R2(sng_cur == 2u && scene_cur() == 5u);
    SCN_MEM = 0;                                         /* settings of before round 2: the old scene bits (B) */
    ui_prefs3 = (uint8_t)((ui_prefs3 & ~(7u << PREF_SCENE_SH)) | 2u << PREF_SCENE_SH);
    sng_cur = 2; proj_cur = PROJ_NO_SLOT;
    scene_boot(1);
    R2(sng_cur == 0u && scene_cur() == 1u);
    bad += check("TFM-UI round 2: the settings remember the song and the scene; older settings (the scene bits only) restore into song 1", ok);
    return bad;
}
