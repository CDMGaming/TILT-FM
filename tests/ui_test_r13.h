/* TILT-FM round 13 (tfm-spec/ROUND-13.md): included by ui_test.c after ui_test_r12.h */
#define R13(c) do { if (!(c)) { ok = 0; printf("  R13 line %d\n", __LINE__); } } while (0)

/* item 1: START's defaults, a TIMER of 15 minutes and four RANDOM rules */
static int test_r13_start(void)
{
    int bad = 0, ok = 1;
    uint32_t i, k, n, cnt[TI_N], kits2 = 0, dup = 0, lo = 1000, hi = 0;
    char b[28];
    ui_power_on();
    tu_off = 0;
    for (i = 0; i < 16u; i++)
        CFG(i) = 0;                                     /* (a fresh setup: every byte 0) */
    R13(TM_LIST[cfg_timer_idx()] == 15u);
    for (i = 0; i < NTRK; i++)
        R13(cfg_opt(i) == 0u);
    tu_su_row_text(7, b);
    R13(str_eq(b, "15 MIN"));
    for (i = 0; i < NTRK; i++) {
        tu_su_row_text(i, b);
        R13(str_eq(b, "RANDOM"));
    }
    tilt_ns_show(); frames(2);
    R13(tilt_timer_min == 15u);                         /* (DICE opens with SETUP's TIMER when none runs) */
    memset(cnt, 0, sizeof cnt);
    for (n = 0; n < 200u; n++) {                        /* 200 rolls */
        uint32_t nk = 0;
        ns.hold = 0;
        tu_dice_nudge();
        for (i = 0; i < NTRK; i++) {
            cnt[ns.s.ins[i] % TI_N]++;
            nk += ins_is_kit(ns.s.ins[i]);
            for (k = 0; k < i; k++)
                dup += ns.s.ins[k] == ns.s.ins[i] && ns.s.pre[k] == ns.s.pre[i];
        }
        kits2 += nk > 1u;
    }
    printf("  round 13 START: 200 rolls, 800 tracks:");
    for (i = 0; i < TI_N; i++) {
        printf(" %s %u%s", TFM_INS[i].name, cnt[i], i + 1u < TI_N ? "," : "\n");
        lo = cnt[i] < lo ? cnt[i] : lo;
        hi = cnt[i] > hi ? cnt[i] : hi;
    }
    R13(lo > 0u && hi <= 2u * 800u / TI_N && !kits2 && !dup);
    bad += check("round 13 START: a fresh setup rolls 15 min and four RANDOM rules; 200 rolls: every instrument, none over twice its share, never two kits, never one sound twice", ok);
    {   /* a setting the user changed is kept */
        ok = 1;
        CFG(CF_TIMER) = 2;                              /* 10 MIN, chosen */
        CFG(CF_RULE) = 1 + 1;                           /* T1: TYPE DRUMS, chosen */
        R13(TM_LIST[cfg_timer_idx()] == 10u && cfg_opt(0) == 1u && cfg_opt(1) == 0u);
        bad += check("round 13 START: changed settings kept (TIMER 10, T1 DRUMS)", ok);
    }
    for (i = 0; i < 16u; i++)
        CFG(i) = 0;
    ui_power_on();
    return bad;
}

/* item 2: the fifteen drum kits, one per drum preset (tests/drum_kits_test.c measures them and writes the WAV files; tests/drum_test.c
 * holds the old kits' renders). Every preset sets its own P_E0, in the order the enum appends the kits; the older sounds keep their
 * instrument's old kit; the kits' lanes are the runs the lane names say; a project saved before this round plays its kit */
static int test_r13_kits(void)
{
    static const uint8_t INS[3] = {TI_THUMP, TI_KITBOX, TI_GARAGE};
    static const uint8_t OLD[3] = {DK_DEEP, DK_10, DK_77};
    static const char *const MN[3] = {"BOOM", "BOX", "HANDS"};
    int bad = 0, ok = 1;
    uint32_t i, a, l, seen = 0;
    track_t *t = &trk[0];
    ui_power_on();
    tu_off = 0;
    song.sel = 0;
    R13(DK_WAREHOUSE == 12 && DK_TINSHED == 26 && DK_COUNT == 27 && ENG_DRUM.edit[0].max == 26);
    R13(DV_NKIT == 23 && DV_KTYPE(23, 7) < 256);
    for (i = 0; i < 3u; i++) {
        const tfm_instr_t *in = &TFM_INS[INS[i]];
        uint32_t pl = i == 0u ? 0u : 5u;
        for (a = 0; a < in->nsnd; a++) {
            tfm_load(t, INS[i], a);
            if (a < 5u) {                               /* a named preset: its own kit, in the order the kits were appended */
                uint32_t kit = (uint32_t)t->p[P_E0], run;
                R13(in->snd[a].ov[0].id == P_E0 && kit == DK_WAREHOUSE + 5u * i + a);
                R13(drum_kit_plays((int32_t)kit) == kit);       /* (not an alias) */
                seen |= 1u << (kit - DK_WAREHOUSE);
                for (l = 0; l < NLANE; l++) {           /* every lane plays through its run */
                    int32_t st;
                    uint32_t ty = drum_gm(t->p, DRUM_LANE_NOTE[l], &st);
                    R13(ty == DV_KTYPE(kit - DK_80 + 1u, l) && DV_KITOF(ty) == kit - DK_80 + 1u);
                    R13(dv_run_type(ty) == DV_KIT[kit - DK_80][l].run);
                }
                run = DV_KIT[kit - DK_80][pl].run;      /* the pitched lane: THUMP LAB's kick, the others' tom or conga */
                R13(drum_plane_fn(t) == pl && (i == 0u ? run == DVT_PUNCH || run == DVT_ROUND : run == DVT_TOM || run == DVT_CONGA));
                R13(DV_KIT[kit - DK_80][DV_HATC].run == DVT_HATC && DV_KIT[kit - DK_80][DV_HATO].run == DVT_HATO);
                R13(DV_KIT[kit - DK_80][DV_BELL].run == DVT_CYM || DV_KIT[kit - DK_80][DV_BELL].run == DVT_BELL);
                R13(drum_lane_name(t, DV_TOM)[0] != 0 && !str_eq(tf_lane(t, DV_BELL, 48), "") && !str_eq(tf_lane(t, DV_KICK, 48), ""));
                for (l = 0; l < 8u; l++)
                    R13(DV_KIT[kit - DK_80][l].gain > 0 && DV_KIT[kit - DK_80][l].tau0 > 0);
            } else {
                R13(t->p[P_E0] == OLD[i]);              /* the older sounds keep the old kit */
            }
        }
        R13(str_eq(in->mac[0].name, "MORPH") && in->mac[0].nsw == TFM_MSW);
        {
            uint32_t m, named = 0;
            for (m = 0; m < 8u; m++)
                named |= str_eq(in->mac[m].name, MN[i]);
            R13(named);                                 /* the toy's own word for a macro: BOOM, BOX, HANDS */
        }
    }
    R13(seen == (1u << 15) - 1u);
    /* a project saved before the fifteen kits: its KIT value is what the kit played from, whatever the kit types are now */
    {
        static project_t q, r;
        static project_store_t ps;
        static const uint8_t V[] = {0, 4, 5, 6, 7, 8, 9, 10, 11};
        uint32_t k;
        for (k = 0; k < sizeof V; k++) {
            int32_t st;
            tfm_load(t, TI_THUMP, 5);
            t->p[P_E0] = V[k];
            project_capture(&q);
            proj_pack(&ps, &q);
            R13(proj_unpack(&r, ps.raw, PROJ_STORE_SIZE) == 1 && r.t[0].p[P_E0] == V[k]);
            R13(drum_kit_plays(r.t[0].p[P_E0]) == V[k]);
            R13(drum_gm(r.t[0].p, DRUM_LANE_NOTE[DV_KICK], &st) == (V[k] < DK_80 ? (uint32_t)DVT_PUNCH : DV_KTYPE(V[k] - DK_80 + 1u, DV_KICK)));
        }
        for (k = 1; k < 4u; k++)                        /* (the retired HAND CYM H+CYM land where they did) */
            R13(param_fit(&ENG_DRUM.edit[0], (int32_t)k) == (k == 1u ? DK_66 : k == 2u ? DK_10 : DK_77));
    }
    bad += check("round 13 item 2: fifteen drum kits, one per preset (P_E0 12..26 in the order appended, the older sounds on their old kit, every lane through the run its name says, the pitched lane where it was, MORPH first, BOOM / BOX / HANDS), saved projects keep their KIT 0..11", ok);
    return bad;
}

/* item 2: a preset change moves P_E0 and is ONE undo step with the sound (UNDO_SOUND); PRESETS' brush guard and card (round 10) apply */
static int test_r13_kit_undo(void)
{
    int bad = 0, ok = 1;
    track_t *t = &trk[0];
    uint8_t p0;
    int16_t e0;
    uint32_t i, j;
    for (j = 0; j < 3u; j++) {
        static const uint8_t INS[3] = {TI_THUMP, TI_KITBOX, TI_GARAGE};
        ui_power_on();
        tu_off = 0;
        song.sel = 0;
        tfm_load(t, INS[j], 0);
        tu_goto(TU_SOUND);
        frames(8);
        p0 = t->preset;
        e0 = t->p[P_E0];
        R13(e0 == DK_WAREHOUSE + 5 * (int)j);
        turn(EN_PRESET, 1);                             /* a brush: the card only, the kit stays */
        R13(t->preset == p0 && t->p[P_E0] == e0 && tu_card_on());
        for (i = 0; i < 160u && tu_card_on(); i++) frame();
        R13(!tu_card_on() && t->preset == p0 && t->p[P_E0] == e0);
        turn(EN_PRESET, 1); turn(EN_PRESET, 1);        /* two within 1 s: the next sound, with its own kit */
        R13(tfm_snd_of(t) == 1u && t->p[P_E0] == e0 + 1);
        turn(EN_PRESET, 1);                             /* and on: the third sound (one browse) */
        R13(tfm_snd_of(t) == 2u && t->p[P_E0] == e0 + 2);
        undo_swap();                                    /* one undo: the sound it started from, its kit with it */
        R13(t->preset == p0 && t->p[P_E0] == e0);
        undo_swap();                                    /* (again: the other way) */
        R13(tfm_snd_of(t) == 2u && t->p[P_E0] == e0 + 2);
        undo_swap();
        R13(t->preset == p0 && t->p[P_E0] == e0);
        for (i = 0; i < 160u && tu_card_on(); i++) frame();
        turn(EN_PRESET, 1); turn(EN_PRESET, 1); turn(EN_PRESET, 1);   /* the held SAVE does the same */
        R13(tfm_snd_of(t) == 2u && t->p[P_E0] == e0 + 2);
        hold(B_SAVE); frames(4);
        R13(t->preset == p0 && t->p[P_E0] == e0);
        for (i = 0; i < 160u && tu_card_on(); i++) frame();
    }
    tu_goto(TU_JAM);
    bad += check("round 13 item 2: a preset change moves the KIT with the sound and is one undo step (UNDO_SOUND: undo_swap and the held SAVE put the start back, KIT too); PRESETS' brush shows the card only", ok);
    return bad;
}
