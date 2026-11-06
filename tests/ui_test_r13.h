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
