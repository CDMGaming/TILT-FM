/* TFM-UI round 4: the tests of the round's items (included by ui_test.c after round 3; same helpers) */
#define R4(c) do { if (!(c)) { ok = 0; fprintf(stderr, "R4 line %d ", __LINE__); } } while (0)
/* 1: blank scenes play silence */
static int test_r4_blank(void)
{
    int bad = 0, ok = 1;
    uint32_t n, k, pl;
    ui_power_on();
    tu_off = 0;
    r3_two_scenes();                                    /* song 1: A (note 55), B (note 66); C..H blank */
    R4(project_used(0) && project_used(1) && !project_used(2));
    chain_config.count = 4;
    chain_config.row[0] = chain_row_of(0, 1);
    chain_config.row[1] = chain_row_of(1, 1);
    chain_config.row[2] = chain_row_of(2, 2);           /* C: blank, two repeats */
    chain_config.row[3] = chain_row_of(0, 1);
    R4(chain_prepare() == 0 && chain.armed);
    R4(chain.config.row[2].slot[0] == CHAIN_SILENT && chain.config.row[2].slot[3] == CHAIN_SILENT && chain.config.row[1].slot[0] == 1u);
    R4(chain_config.row[2].slot[0] == 2u);              /* the song as written is untouched */
    events_block(32);
    R4(chain.running && chain.row == 0u && trk[0].seq_notes[0] == 55);
    pl = 16u * div_samples(2);                          /* LEN 16 of 1/16: a 4/4 bar */
    n = 1u + lanes_until(0, 4000);
    R4(chain.row == 1u && lanes_near(n - 1u, pl) && trk[0].seq_notes[0] == 66);
    n = 1u + lanes_until(1, 4000);
    R4(chain.row == 2u);
    {
        int quiet = 1;
        n = 0;
        while (n < 8000u && chain.running && chain.row == 2u) {
            events_block(32);
            n++;
            for (k = 0; k < NTRK; k++) quiet &= chain.row != 2u || (!trk[k].seq_n && seq_steps(&trk[k]) == chain_silent);
        }
        R4(quiet);
        R4(chain.row == 3u && lanes_near(n, 2u * pl));  /* an all-blank section: 1 bar x its 2 repeats */
    }
    R4(trk[0].seq_notes[0] == 55);                      /* and A plays on after it */
    seq_stop(); transport_req = 0; events_block(32);
    R4(tsg_row_bars(&chain_config.row[2]) == 2u && tsg_row_bars(&chain_config.row[0]) >= 1u);
    /* no blank / empty message in the quick chain */
    chain_defaults(&chain_config);
    ui.msg[0] = 0;
    quick_chain(2);
    R4(chain_config.count == 1u && chain_config.row[0].slot[0] == 2u && !str_eq(ui.msg, "SCENE EMPTY"));
    /* SONG's knob: "-" A B .. H, "-" first, no wrap */
    chain_defaults(&chain_config);
    chain_config.count = 1;
    chain_config.row[0] = (chain_row_t){{CHAIN_SILENT, 0, 7, 3}, 1};
    tu_goto(TU_SONG);
    ui.song_row = 0;
    frames(8);
    turn(EN_K1, -1);                                    /* "-" stays "-" */
    R4(chain_config.row[0].slot[0] == CHAIN_SILENT);
    turn(EN_K1, 1);
    R4(chain_config.row[0].slot[0] == 0u);
    turn(EN_K1, 1);
    R4(chain_config.row[0].slot[0] == 1u);
    for (k = 0; k < 8u; k++) turn(EN_K1, 1);
    R4(chain_config.row[0].slot[0] == 7u);              /* H, and no wrap */
    turn(EN_K1, 1);
    R4(chain_config.row[0].slot[0] == 7u);
    turn(EN_K2, -1);                                    /* A -> "-" (left of A) */
    R4(chain_config.row[0].slot[1] == CHAIN_SILENT);
    turn(EN_K3, 1);
    R4(chain_config.row[0].slot[2] == 7u);
    turn(EN_K4, -1);
    R4(chain_config.row[0].slot[3] == 2u);
    /* an old chain keeps its meaning: stored A..D 0..3, "-" 4, E..H 5..8 */
    R4(chain_dec_b(0) == 0u && chain_dec_b(3) == 3u && chain_dec_b(4) == CHAIN_SILENT && chain_dec_b(8) == 7u);
    bad += check("round 4 item 1: blank scenes play silence (section lengths, no error, knob order \"-\" A..H)", ok);
    return bad;
}
