/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 CDM Gaming (TILT-FM) */
/* TILT-FM round 13 item 2 (tfm-spec/ROUND-13.md): the fifteen drum kits, one per preset of THUMP LAB, KITBOX and GARAGE.
 * Through the UI test's whole machinery (ui_test.c without its main: the instrument presets load by tfm_load, the track
 * plays through mix_block, so the levels, macros and insert are each preset's own):
 *   1. every kit's eight lanes as one-shots (1 s apart, the cymbal 3 s), then a bar of a simple beat, to
 *      tfm-spec/built/drums/<instrument>-<preset>.wav (mono, 44.1 kHz, 16 bit; the preset's room and echo on) for the ear
 *   2. per lane at the preset's settings (the room and the echo off, the lane alone): the peak, the decay to -40 dB, the
 *      spectral centroid of the first 93 ms
 *   3. per kit: round 6's feature distance (ui_test_r6.h: brightness, its change, attack, sustain, motion, level of the
 *      audit phrase) to the other four kits of its instrument and to the old kit its presets used before (THUMP LAB 9
 *      DEEP, KITBOX 5 "10", GARAGE 8 "77") at the same preset settings; the old presets' own distances (the five presets
 *      over the one old kit) for the comparison
 *   It fails unless
 *     - each instrument's five kits are all further apart (pairwise) than the old presets were (their mean)
 *     - no KITBOX lane but the cymbal rings past 150 ms (-40 dB)
 *     - no lane is more than 6 dB louder (peak) than the loudest other lane of its kit, at the preset's levels
 *     - adjacent keys show no level jump: the pitched lane up the scale (G4..G5, the eight degrees) within round 9's
 *       R9_KEY_LU (K-weighted loudness, the lane's note alone), every kit
 *   DKCAL=1 prints the table of peaks as the engine-level gain calibration wants it; DKQUICK=1 skips the WAV files.
 * Built and run by tests/run_tests.sh. */
#define UI_TEST_MAIN_NAME ui_test_main_unused
#include "ui_test.c"
#include <sys/stat.h>

#define DK_FS 44100u
/* the five presets of each instrument over its one old kit, as c808a83 had them (before round 13: tests/ui_test_r6.h's features on the
 * old tables, measured there): the ten pairwise distances in the order 0-1 0-2 0-3 0-4 1-2 1-3 1-4 2-3 2-4 3-4 */
static const double DK_OLDD[3][10] = {
    {0.99, 0.94, 1.51, 1.94, 0.89, 1.28, 1.79, 1.50, 1.64, 2.03},   /* THUMP LAB: min 0.89, mean 1.45, max 2.03 */
    {3.75, 3.06, 3.34, 3.43, 4.79, 4.99, 3.93, 5.20, 2.65, 6.05},   /* KITBOX: min 2.65, mean 4.12, max 6.05 */
    {1.92, 1.59, 1.96, 1.74, 1.13, 1.94, 0.61, 1.05, 0.75, 1.44}};  /* GARAGE: min 0.61, mean 1.41, max 1.96 */
static const uint8_t DK_INS[3] = {TI_THUMP, TI_KITBOX, TI_GARAGE};
static const uint8_t DK_OLD[3] = {9, 5, 8};              /* the kit each instrument's presets played before round 13 */
static const char *const DK_LN[8] = {"KICK", "SNARE", "CLAP", "HAT CL", "HAT OP", "TOM", "RIM", "BELL"};
#define DK_LNOTE(l) DRUM_LANE_NOTE[l]

static float dk_x[DK_FS * 6u];                           /* a lane's mono render */
static uint32_t dk_n;

/* DKSET="i.p.key=v;..." (a tuning aid, not part of the test): after a preset loads, instrument i (0 THUMP LAB, 1 KITBOX,
 * 2 GARAGE) preset p gets macro m1..m7 at v percent, or a value (E2 DLY REV LEVEL LN0..LN7 E0..E7) set */
static struct { uint8_t ii, a; char key[8]; int v; } dk_set[64];
static uint32_t dk_nset;
static int dk_pid(const char *k)
{
    static const struct { const char *n; int id; } T[] = {{"DLY", P_DLY}, {"REV", P_REV}, {"LEVEL", P_LEVEL}, {"IMIX", P_IMIX}, {"IA", P_IA}, {"IB", P_IB},
        {"IC", P_IC}, {"ITYPE", P_ITYPE}, {"E0", P_E0}, {"E1", P_E1}, {"E2", P_E2}, {"E3", P_E3}, {"E4", P_E4}, {"E5", P_E5}, {"E6", P_E6},
        {"E7", P_E7}, {"LN0", P_LN0}, {"LN1", P_LN1}, {"LN2", P_LN2}, {"LN3", P_LN3}, {"LN4", P_LN4}, {"LN5", P_LN5}, {"LN6", P_LN6},
        {"LN7", P_LN7}, {"DIST", P_DIST}};
    uint32_t i;
    for (i = 0; i < sizeof T / sizeof T[0]; i++)
        if (!strcmp(T[i].n, k)) return T[i].id;
    return -1;
}
static void dk_post(track_t *t, uint32_t ins, uint32_t pre)
{
    uint32_t i;
    for (i = 0; i < dk_nset; i++) {
        if (DK_INS[dk_set[i].ii] != ins || dk_set[i].a != pre) continue;
        if (dk_set[i].key[0] == 'm')
            tfm_macro_put(t, (uint32_t)(dk_set[i].key[1] - '0'), dk_set[i].v * 127 / 100, 0);
        else if (dk_pid(dk_set[i].key) >= 0)
            t->p[dk_pid(dk_set[i].key)] = (int16_t)dk_set[i].v;
    }
}
static void dk_parse(const char *e)
{
    unsigned ii, a, v;
    char key[8];
    while (e && *e && dk_nset < 64u) {
        if (sscanf(e, "%u.%u.%7[A-Za-z0-9]=%u", &ii, &a, key, &v) == 4) {
            dk_set[dk_nset].ii = (uint8_t)ii; dk_set[dk_nset].a = (uint8_t)a; strcpy(dk_set[dk_nset].key, key); dk_set[dk_nset].v = (int)v;
            dk_nset++;
        }
        e = strchr(e, ';');
        if (e) e++;
    }
}
/* kit k of instrument ii: the preset (ins, p) played with its own kit, or o (>= 0) instead */
static void dk_prep(uint32_t ins, uint32_t pre, int32_t kit_ov, int dry)
{
    track_t *t = &trk[0];
    int32_t o[2 * CTL];
    uint32_t f;
    r6l_setup(ins, pre);
    if (kit_ov >= 0)
        t->p[P_E0] = (int16_t)kit_ov;
    if (dry) {                                          /* the lane itself: no room, no echo */
        t->p[P_REV] = 0;
        t->p[P_DLY] = 0;
    }
    panic_req = (1u << NTRK) - 1u; mix_block(o, CTL);
    for (f = 0; f < 256u && (t->xf_on || t->eng_req != t->engine); f++) mix_block(o, CTL);
}

/* the lane's note alone for n samples (the pitched lane st semitones up when pit) into dk_x (mono), the peak (|L|, |R|) out;
 * kw: the K-weighted energy of the whole render (sum of both channels' squares) */
static double dk_lane(uint32_t ins, uint32_t pre, uint32_t lane, int32_t st, uint32_t n, int dry, double *kw)
{
    track_t *t = &trk[0];
    int32_t o[2 * CTL], pk = 0;
    uint32_t i, k, c;
    r6bq_t sh[2], hp[2];
    double e = 0;
    dk_prep(ins, pre, -1, dry);
    for (c = 0; c < 2u; c++) r6_kw(&sh[c], &hp[c]);
    drum_ppl[0] = st ? (uint8_t)(lane + 1u) : 0;
    drum_pst[0] = (int8_t)st;
    trk_note_on(t, DK_LNOTE(lane), 100);
    for (k = 0; k < n; k += CTL) {
        if (k >= DK_FS / 10u && k < DK_FS / 10u + CTL) trk_note_off(t, DK_LNOTE(lane));
        mix_block(o, CTL);
        for (i = 0; i < CTL && k + i < n; i++) {
            int32_t l = o[2 * i], r = o[2 * i + 1];
            pk = l > pk ? l : -l > pk ? -l : pk;
            pk = r > pk ? r : -r > pk ? -r : pk;
            dk_x[k + i] = (float)(l + r) / 65536.0f;
            if (kw)
                for (c = 0; c < 2u; c++) {
                    double y = r6bq(&hp[c], r6bq(&sh[c], (c ? r : l) / 32768.0));
                    e += y * y;
                }
        }
    }
    drum_ppl[0] = 0;
    dk_n = n;
    if (kw) *kw = e;
    return 20.0 * log10(pk / 32768.0 + 1e-9);
}

static double dk_rms_db(uint32_t a, uint32_t b)
{
    double s = 0;
    uint32_t i;
    if (b > dk_n) b = dk_n;
    for (i = a; i < b; i++) s += (double)dk_x[i] * dk_x[i];
    return 10.0 * log10(s / (b - a) + 1e-20);
}
/* the time (s) of the last 2.5 ms window within 40 dB of the loudest */
static double dk_t40(void)
{
    uint32_t w = DK_FS / 400u, f, nf = dk_n / w, last = 0;
    double mx = -999;
    for (f = 0; f < nf; f++) { double l = dk_rms_db(f * w, f * w + w); if (l > mx) mx = l; }
    for (f = 0; f < nf; f++) if (dk_rms_db(f * w, f * w + w) >= mx - 40.0) last = f;
    return (last + 1u) * (double)w / DK_FS;
}
static void dk_fft(double *re, double *im, uint32_t n)
{
    uint32_t i, j = 0, len, k;
    for (i = 1; i < n; i++) {
        uint32_t bit = n >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j ^= bit;
        if (i < j) { double t = re[i]; re[i] = re[j]; re[j] = t; t = im[i]; im[i] = im[j]; im[j] = t; }
    }
    for (len = 2; len <= n; len <<= 1) {
        double a = -2 * 3.14159265358979 / len;
        for (i = 0; i < n; i += len)
            for (k = 0; k < len / 2u; k++) {
                double wr = cos(a * k), wi = sin(a * k), ur = re[i + k], ui = im[i + k];
                double vr = re[i + k + len / 2] * wr - im[i + k + len / 2] * wi;
                double vi = re[i + k + len / 2] * wi + im[i + k + len / 2] * wr;
                re[i + k] = ur + vr, im[i + k] = ui + vi;
                re[i + k + len / 2] = ur - vr, im[i + k + len / 2] = ui - vi;
            }
    }
}
static double dk_centroid(void)                         /* the first 93 ms, magnitude-weighted */
{
    static double re[4096], im[4096];
    uint32_t i;
    double sm = 0, sf = 0;
    for (i = 0; i < 4096u; i++) re[i] = dk_x[i], im[i] = 0;
    dk_fft(re, im, 4096);
    for (i = 1; i < 2048u; i++) { double m = sqrt(re[i] * re[i] + im[i] * im[i]); sm += m; sf += m * i * (double)DK_FS / 4096.0; }
    return sf / (sm + 1e-20);
}

/* ------------------------------------------------------------------------------------------ the WAV files --- */
static void dk_le(FILE *f, uint32_t v, int n) { while (n--) { fputc((int)(v & 255u), f); v >>= 8; } }
typedef struct { uint32_t at; uint8_t note, on; } dk_ev_t;
static void dk_wav(uint32_t ins, uint32_t pre, const char *path)
{
    static dk_ev_t ev[64];
    static int16_t pcm[DK_FS * 16u];
    track_t *t = &trk[0];
    int32_t o[2 * CTL];
    uint32_t ne = 0, k, e, n, total, base, l;
    FILE *f;
    static const struct { uint8_t note; uint16_t at; } BEAT[] = {   /* a bar at 120 BPM, 1/16 = 125 ms (units of 1/16) */
        {36, 0}, {42, 0}, {42, 2}, {38, 4}, {42, 4}, {42, 6}, {36, 6}, {36, 8}, {42, 8}, {42, 10}, {36, 11}, {38, 12}, {39, 12},
        {42, 12}, {46, 14}, {45, 15}};
    for (l = 0; l < 8u; l++) {                          /* the eight lanes, one by one (the cymbal rings longer) */
        base = l * DK_FS;
        ev[ne].at = base; ev[ne].note = DK_LNOTE(l); ev[ne++].on = 1;
        ev[ne].at = base + DK_FS / 8u; ev[ne].note = DK_LNOTE(l); ev[ne++].on = 0;
    }
    base = 10u * DK_FS + DK_FS / 2u;                    /* the beat: after the one-shots and the cymbal's ring (3 s) */
    for (k = 0; k < sizeof BEAT / sizeof BEAT[0]; k++) {
        ev[ne].at = base + BEAT[k].at * (DK_FS / 8u); ev[ne].note = BEAT[k].note; ev[ne++].on = 1;
        ev[ne].at = base + BEAT[k].at * (DK_FS / 8u) + DK_FS / 16u; ev[ne].note = BEAT[k].note; ev[ne++].on = 0;
    }
    total = base + 2u * DK_FS + DK_FS * 3u / 2u;
    dk_prep(ins, pre, -1, 0);
    for (n = 0; n < total; n += CTL) {
        for (e = 0; e < ne; e++)
            if (ev[e].at >= n && ev[e].at < n + CTL) {
                if (ev[e].on) trk_note_on(t, ev[e].note, 100); else trk_note_off(t, ev[e].note);
            }
        mix_block(o, CTL);
        for (k = 0; k < CTL && n + k < total; k++) {
            pcm[n + k] = (int16_t)clamp((o[2 * k] + o[2 * k + 1]) / 2, -32768, 32767);   /* (mono: the two channels' mean) */
        }
    }
    f = fopen(path, "wb");
    if (!f) { printf("drum_kits: cannot write %s\n", path); return; }
    fwrite("RIFF", 1, 4, f); dk_le(f, 36u + total * 2u, 4); fwrite("WAVEfmt ", 1, 8, f);
    dk_le(f, 16, 4); dk_le(f, 1, 2); dk_le(f, 1, 2); dk_le(f, DK_FS, 4); dk_le(f, DK_FS * 2u, 4); dk_le(f, 2, 2); dk_le(f, 16, 2);
    fwrite("data", 1, 4, f); dk_le(f, total * 2u, 4);
    for (k = 0; k < total; k++) dk_le(f, (uint16_t)pcm[k], 2);
    fclose(f);
}
static void dk_fname(uint32_t ii, uint32_t a, char *b, size_t n)
{
    char nm[64];
    size_t i, j = 0;
    snprintf(nm, sizeof nm, "%s-%s", TFM_INS[DK_INS[ii]].name, TFM_INS[DK_INS[ii]].snd[a].name);
    for (i = 0; nm[i] && j < sizeof nm - 1u; i++)
        nm[j++] = nm[i] == ' ' ? '-' : (char)(nm[i] >= 'A' && nm[i] <= 'Z' ? nm[i] + 32 : nm[i]);
    nm[j] = 0;
    snprintf(b, n, "tfm-spec/built/drums/%s.wav", nm);
}

/* ------------------------------------------------------------------------------------------------ main --- */
int main(void)
{
    static double pk[3][5][8], t40[3][5][8], cen[3][5][8];
    static double fn[3][5][R6_NF], fo[3][5][R6_NF];
    int fail = 0, quick = getenv("DKQUICK") != 0, cal = getenv("DKCAL") != 0;
    uint32_t ii, a, b, l, d;
    setvbuf(stdout, NULL, _IONBF, 0);
    mkdir("tfm-spec", 0777); mkdir("tfm-spec/built", 0777); mkdir("tfm-spec/built/drums", 0777);
    if (getenv("DKSET")) { dk_parse(getenv("DKSET")); r6_post = dk_post; }
    if (getenv("DKDUMP")) {                             /* DKDUMP=i.a.l: the lane's envelope, 10 ms windows, dB under its loudest */
        unsigned di, da, dl;
        if (sscanf(getenv("DKDUMP"), "%u.%u.%u", &di, &da, &dl) == 3) {
            double mx = -999, e[64];
            uint32_t w;
            dk_lane(DK_INS[di], da, dl, 0, DK_FS * 3u, 1, 0);
            for (w = 0; w < 64u; w++) { e[w] = dk_rms_db(w * 441u, w * 441u + 441u); mx = e[w] > mx ? e[w] : mx; }
            for (w = 0; w < 64u; w++) printf("%d ", (int)(e[w] - mx));
            printf("\n");
        }
        return 0;
    }
    if (getenv("DKFAST")) {                             /* a tuning aid: only the feature distances, one line each */
        for (ii = 0; ii < 3u; ii++) {
            double f1[5][R6_NF], mn = 1e9, sm = 0;
            for (a = 0; a < 5u; a++) { r6_kit = -1; r6_render(DK_INS[ii], a); r6_features(f1[a]); }
            printf("DKD %u", ii);
            for (a = 0; a < 5u; a++)
                for (b = a + 1u; b < 5u; b++) {
                    double dd = r6_dist(f1[a], f1[b]);
                    printf(" %.3f", dd); mn = dd < mn ? dd : mn; sm += dd;
                }
            printf("  min %.3f mean %.3f\n", mn, sm / 10.0);
            if (getenv("DKX")) {                        /* the longest non-cymbal decay (ms) and the pitched lane's largest key step (LU) of each preset */
                uint32_t pl = ii == 0u ? 0u : 5u;
                printf("DKX %u", ii);
                for (a = 0; a < 5u; a++) {
                    double mt = 0, prev = 0, w = 0;
                    for (l = 0; l < 7u; l++) {
                        double t;
                        dk_lane(DK_INS[ii], a, l, 0, DK_FS * 2u, 1, 0);
                        t = dk_t40();
                        mt = t > mt ? t : mt;
                    }
                    for (d = 0; d < 8u; d++) {
                        double kw, cv;
                        dk_lane(DK_INS[ii], a, pl, drum_deg_st(&trk[0], (int32_t)d), R9_KN, 0, &kw);
                        cv = -0.691 + 10.0 * log10(kw / R9_KN + 1e-20);
                        if (d && fabs(cv - prev) > w) w = fabs(cv - prev);
                        prev = cv;
                    }
                    printf(" %.0f/%.2f", mt * 1000.0, w);
                }
                printf("\n");
            }
        }
        return 0;
    }
    printf("drum_kits: round 13 item 2: fifteen drum kits (one-shots at the preset's settings, room and echo off; peak dBFS, decay to -40 dB, centroid of the first 93 ms)\n");
    for (ii = 0; ii < 3u; ii++) {
        const tfm_instr_t *in = &TFM_INS[DK_INS[ii]];
        double worst_all = 0, kit_t40_max = 0;
        for (a = 0; a < 5u; a++) {
            double hi = -999, hi2 = -999, worst;
            uint32_t hl = 0;
            char fl[160];
            printf("drum_kits: %s %s (KIT %d, %s)\n", in->name, in->snd[a].name, in->snd[a].ov[0].v, N_DRUM_KIT[in->snd[a].ov[0].v]);
            for (l = 0; l < 8u; l++) {
                track_t *t;
                pk[ii][a][l] = dk_lane(DK_INS[ii], a, l, 0, DK_FS * (l == 7u ? 5u : 3u), 1, 0);
                t = &trk[0];
                t40[ii][a][l] = dk_t40();
                cen[ii][a][l] = dk_centroid();
                printf("drum_kits:   %-7s %-6s peak %6.1f dBFS  -40 dB at %5.0f ms  centroid %5.0f Hz\n", DK_LN[l], drum_lane_name(t, l), pk[ii][a][l], t40[ii][a][l] * 1000.0, cen[ii][a][l]);
                if (pk[ii][a][l] > hi) { hi2 = hi; hi = pk[ii][a][l]; hl = l; } else if (pk[ii][a][l] > hi2) hi2 = pk[ii][a][l];
                if (DK_INS[ii] == TI_KITBOX && l != 7u && t40[ii][a][l] > kit_t40_max) kit_t40_max = t40[ii][a][l];
                if (DK_INS[ii] == TI_KITBOX && l != 7u && t40[ii][a][l] > 0.150) {
                    fail = 1;
                    printf("drum_kits:   FAIL: KITBOX %s %s rings %.0f ms (-40 dB), over 150 ms\n", in->snd[a].name, DK_LN[l], t40[ii][a][l] * 1000.0);
                }
            }
            worst = hi - hi2;                           /* the loudest lane over the next loudest */
            worst_all = worst > worst_all ? worst : worst_all;
            printf("drum_kits:   loudest lane %s is %.1f dB over the next\n", DK_LN[hl], worst);
            if (worst > 6.0) { fail = 1; printf("drum_kits:   FAIL: %s %s: a lane %.1f dB louder than any other (6 dB at most)\n", in->name, in->snd[a].name, worst); }
            if (cal) {
                printf("DKCAL %u %u", ii, a);
                for (l = 0; l < 8u; l++) printf(" %.2f", pk[ii][a][l]);
                printf("\n");
            }
            (void)fl;
        }
        printf("drum_kits: %s: widest lane-level spread %.1f dB (top two lanes), longest non-cymbal decay %.0f ms\n", in->name, worst_all, kit_t40_max * 1000.0);
    }
    if (cal)
        return 0;                                       /* (DKCAL: the calibration table only) */
    if (getenv("DKONLY")) r6l_only = (uint8_t)atoi(getenv("DKONLY"));
    if (getenv("DKLOUD")) {                             /* the audit phrase\x27s integrated loudness (LUFS) of each preset: round 6 item 10 wants 1.5 LU of the median, -19 .. -15.5 */
        for (ii = 0; ii < 3u; ii++) {
            double l[5], s2, srt[5];
            printf("DKLOUD %-10s", TFM_INS[DK_INS[ii]].name);
            for (a = 0; a < 5u; a++) { l[a] = srt[a] = r6_loud(DK_INS[ii], a, 0, &s2); printf(" %6.1f (pk %5.1f, lvl %3d)", l[a], 20.0 * log10(r6l_peak / 32768.0 + 1e-9), trk[0].p[P_LEVEL]); }
            qsort(srt, 5, sizeof srt[0], r6_cmpd);
            printf("   median %.1f\n", srt[2]);
        }
        return 0;
    }
    /* the features of the audit phrase: each preset with its own new kit, and with the old kit of its instrument */
    for (ii = 0; ii < 3u; ii++)
        for (a = 0; a < 5u; a++) {
            r6_kit = -1;
            r6_render(DK_INS[ii], a); r6_features(fn[ii][a]);
            r6_kit = DK_OLD[ii];
            r6_render(DK_INS[ii], a); r6_features(fo[ii][a]);
            r6_kit = -1;
        }
    if (getenv("DKFEAT"))
        for (ii = 0; ii < 3u; ii++)
            for (a = 0; a < 5u; a++) {
                printf("DKFEAT %-10s %-12s new", TFM_INS[DK_INS[ii]].name, TFM_INS[DK_INS[ii]].snd[a].name);
                for (b = 0; b < R6_NF; b++) printf(" %6.2f", fn[ii][a][b]);
                printf("   old");
                for (b = 0; b < R6_NF; b++) printf(" %6.2f", fo[ii][a][b]);
                printf("\n");
            }
    for (ii = 0; ii < 3u; ii++) {
        const tfm_instr_t *in = &TFM_INS[DK_INS[ii]];
        double nmin = 1e9, nsum = 0, omin = 1e9, osum = 0, omax = 0, nmax = 0;
        uint32_t q = 0;
        for (q = 0; q < 10u; q++) {
            omin = DK_OLDD[ii][q] < omin ? DK_OLDD[ii][q] : omin;
            omax = DK_OLDD[ii][q] > omax ? DK_OLDD[ii][q] : omax;
            osum += DK_OLDD[ii][q];
        }
        printf("drum_kits: %s: feature distances (round 6: brightness, its change, attack, sustain, motion, level)\n", in->name);
        printf("drum_kits:   %-12s", "new kits");
        for (b = 0; b < 5u; b++) printf(" %9.9s", in->snd[b].name);
        printf("   old kit\n");
        for (a = 0; a < 5u; a++) {
            printf("drum_kits:   %-12s", in->snd[a].name);
            for (b = 0; b < 5u; b++) {
                double dd = r6_dist(fn[ii][a], fn[ii][b]);
                if (b < a) printf(" %9.2f", dd);
                else printf(" %9s", b == a ? "-" : "");
                if (b > a) { nmin = dd < nmin ? dd : nmin; nmax = dd > nmax ? dd : nmax; nsum += dd; }
            }
            printf("   %.2f\n", r6_dist(fn[ii][a], fo[ii][a]));
        }
        printf("drum_kits:   old presets over the old kit (c808a83): min %.2f, mean %.2f, max %.2f; new kits: min %.2f, mean %.2f, max %.2f\n", omin, osum / 10.0, omax, nmin, nsum / 10.0, nmax);
        for (a = 0; a < 5u; a++)
            for (b = a + 1u; b < 5u; b++)
                if (r6_dist(fn[ii][a], fn[ii][b]) <= omin) {
                    fail = 1;
                    printf("drum_kits:   FAIL: %s: %s / %s are %.2f apart, not further than the old presets' nearest pair %.2f\n", in->name, in->snd[a].name, in->snd[b].name, r6_dist(fn[ii][a], fn[ii][b]), omin);
                }
    }
    for (ii = 0; ii < 3u; ii++) {                       /* (and as a set: the mean of the ten distances beyond the old presets' mean) */
        double om = 0, nm = 0;
        for (a = 0; a < 10u; a++) om += DK_OLDD[ii][a] / 10.0;
        for (a = 0; a < 5u; a++)
            for (b = a + 1u; b < 5u; b++) nm += r6_dist(fn[ii][a], fn[ii][b]) / 10.0;
        if (nm <= om) { fail = 1; printf("drum_kits:   FAIL: %s: mean distance %.2f not beyond the old presets' %.2f\n", TFM_INS[DK_INS[ii]].name, nm, om); }
    }
    /* adjacent keys: the pitched lane up the scale, G4..G5 (the degrees 0..7), K-weighted loudness of the lane's note alone */
    for (ii = 0; ii < 3u; ii++) {
        const tfm_instr_t *in = &TFM_INS[DK_INS[ii]];
        uint32_t pl = DK_INS[ii] == TI_THUMP ? 0u : 5u;
        double worst = 0;
        for (a = 0; a < 5u; a++) {
            double prev = 0, w = 0, cv[8];
            for (d = 0; d < 8u; d++) {
                double kw;
                dk_lane(DK_INS[ii], a, pl, drum_deg_st(&trk[0], (int32_t)d), R9_KN, 0, &kw);
                cv[d] = -0.691 + 10.0 * log10(kw / R9_KN + 1e-20);
                if (d && fabs(cv[d] - prev) > w) w = fabs(cv[d] - prev);
                prev = cv[d];
            }
            if (getenv("DKKEYS") || w > R9_KEY_LU) {
                printf("drum_kits:   keys (LUFS)");
                for (d = 0; d < 8u; d++) printf(" %.1f", cv[d]);
                printf("\n");
            }
            worst = w > worst ? w : worst;
            printf("drum_kits: %s %s: the pitched lane up the scale, largest step between adjacent keys %.2f LU\n", in->name, in->snd[a].name, w);
            if (w > R9_KEY_LU) { fail = 1; printf("drum_kits:   FAIL: %s %s: adjacent keys %.2f LU apart (round 9: %.1f)\n", in->name, in->snd[a].name, w, R9_KEY_LU); }
        }
        printf("drum_kits: %s: the largest step between adjacent keys of the pitched lane: %.2f LU\n", in->name, worst);
    }
    /* the macros: every one moves the sound clearly on all five kits (the audit's distance from 0 to 100 %, R8_MACRO_MIN as
     * round 8 item 3 asks of the first preset), and MORPH keeps the level within 3 LU over its turn on every kit */
    for (ii = 0; ii < 3u; ii++) {
        const tfm_instr_t *in = &TFM_INS[DK_INS[ii]];
        uint32_t m;
        printf("drum_kits: %s: macros, the distance 0 -> 100 %% on each kit (at least %.2f)\n", in->name, R8_MACRO_MIN);
        printf("drum_kits:   %-8s", "");
        for (a = 0; a < 5u; a++) printf(" %9.9s", in->snd[a].name);
        printf("\n");
        for (m = 0; m < 8u; m++) {
            printf("drum_kits:   %-8s", in->mac[m].name);
            for (a = 0; a < 5u; a++) {
                double d, f0[R6_NF], f1[R6_NF];
                r6_mac = (int32_t)m; r6_macv = 0;
                r6_render(DK_INS[ii], a); r6_features(f0);
                r6_macv = 127;
                r6_render(DK_INS[ii], a); r6_features(f1);
                r6_mac = -1;
                d = r6_dist(f0, f1);
                printf(" %9.2f", d);
                if (in->mac[m].nsw == 0u && d < R8_MACRO_MIN) { fail = 1; printf("!"); }
            }
            printf("\n");
        }
        printf("drum_kits:   MORPH level over the turn (loudest - quietest, LU; 3 at most):");
        for (a = 0; a < 5u; a++) {
            double l[3], lo = 999, hi = -999, s2;
            uint32_t q;
            static const int32_t ps[3] = {0, 64, 127};
            for (q = 0; q < 3u; q++) {
                r6l_setup(DK_INS[ii], a);
                tfm_macro_put(&trk[0], 0, ps[q], 0);
                r6l_play(DK_INS[ii], 0);
                r6_lufs(&l[q], &s2);
                lo = l[q] < lo ? l[q] : lo; hi = l[q] > hi ? l[q] : hi;
            }
            printf(" %.1f (%.1f %.1f %.1f)", hi - lo, l[0], l[1], l[2]);
            if (hi - lo > 3.0) { fail = 1; printf("!"); }
        }
        printf("\n");
    }
    if (!quick) {
        for (ii = 0; ii < 3u; ii++)
            for (a = 0; a < 5u; a++) {
                char fn2[128];
                dk_fname(ii, a, fn2, sizeof fn2);
                dk_wav(DK_INS[ii], a, fn2);
            }
        printf("drum_kits: WAV files in tfm-spec/built/drums/ (the eight lanes one by one, then a bar of a beat; mono)\n");
    }
    printf(fail ? "drum_kits: FAILED\n" : "drum_kits: all passed\n");
    return fail;
}
