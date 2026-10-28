/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 CDM Gaming (TILT-FM) */
/* TFM: TILT's setup codes (tfm_rng.c says what they carry; tfm_instr.c maps the instruments) */
/* ------------------------------------------------------------ the codes --- */
static const char TFM_B32[33] = "0123456789ABCDEFGHJKMNPQRSTVWXYZ";   /* Crockford: no I L O U */
#define TFM_CODE_MAX 40u                                /* chars of a code, dashes in */
static uint32_t tilt_scale_of(uint32_t felucca)         /* N_SCALE -> TILT's number (one TILT lacks: minor) */
{
    uint32_t i;
    for (i = 0; i < sizeof TILT_SCALE; i++)
        if (TILT_SCALE[i] == felucca)
            return i;
    return 1u;
}
/* the setup as TILT's code (format 1 when every slot fits a nibble, else 2) into b (TFM_CODE_MAX + 1) */
static void tfm_encode(const tfm_setup_t *s, char *b)
{
    uint8_t by[5 + 2 * NTRK];
    uint32_t n = 0, i, v1 = 1, bits = 0, nb = 0, o = 0, chars = 0;
    for (i = 0; i < NTRK; i++)
        v1 &= s->pre[i] < 16u && tfm_kind(s->ins[i]) < 16u;
    by[n++] = (uint8_t)(v1 ? 1u : 2u);
    by[n++] = (uint8_t)((s->key & 15u) | tilt_scale_of(s->scale) << 4);
    by[n++] = (uint8_t)clamp(s->bpm - 40, 0, 255);
    by[n++] = (uint8_t)clamp(s->swing, 0, 100);
    by[n++] = NTRK;
    for (i = 0; i < NTRK; i++) {
        if (v1) {
            by[n++] = (uint8_t)(tfm_kind(s->ins[i]) << 4 | (s->pre[i] & 15u));
        } else {
            by[n++] = (uint8_t)tfm_kind(s->ins[i]);
            by[n++] = s->pre[i];
        }
    }
    for (i = 0; i < n; i++) {                           /* 8-bit bytes -> 5-bit symbols (the last padded with 0s) */
        bits = bits << 8 | by[i];
        nb += 8u;
        while (nb >= 5u) {
            nb -= 5u;
            if (chars && !(chars & 3u)) b[o++] = '-';
            b[o++] = TFM_B32[(bits >> nb) & 31u];
            chars++;
        }
    }
    if (nb) {
        if (chars && !(chars & 3u)) b[o++] = '-';
        b[o++] = TFM_B32[(bits << (5u - nb)) & 31u];
    }
    b[o] = 0;
}
/* a code -> the setup (dice.ts decodeSetup, then onto TFM's four tracks). 0 a code TILT would refuse */
static int tfm_decode(const char *code, tfm_setup_t *s)
{
    uint8_t by[64];
    uint32_t n = 0, bits = 0, nb = 0, at, i, slots, hash = 2166136261u;
    tfm_rng_t r;
    for (; *code && n < sizeof by; code++) {
        uint32_t c = (uint8_t)*code, v;
        if (c >= 'a' && c <= 'z') c -= 32u;
        if (c == 'O') c = '0';
        if (c == 'I' || c == 'L') c = '1';
        if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z')))
            continue;                                   /* (dashes, spaces) */
        for (v = 0; v < 32u && (uint8_t)TFM_B32[v] != c; v++)
            ;
        if (v == 32u)
            return 0;                                   /* (U) */
        hash = (hash ^ v) * 16777619u;
        bits = bits << 5 | v;
        nb += 5u;
        if (nb >= 8u) {
            nb -= 8u;
            by[n++] = (uint8_t)(bits >> nb);
        }
    }
    if (n < 5u || (by[0] != 1u && by[0] != 2u) || by[4] < 1u || by[4] > 8u || (by[1] >> 4) >= sizeof TILT_SCALE)
        return 0;
    s->key = by[1] & 15u;
    if (s->key > 11u)
        return 0;
    s->scale = TILT_SCALE[by[1] >> 4];
    s->bpm = (int16_t)clamp(by[2] + 40, GP[G_BPM].min, GP[G_BPM].max);
    s->swing = (uint8_t)clamp(by[3], 0, 100);
    slots = by[4];
    for (i = 0, at = 5; i < slots; i++) {
        uint32_t kind, pre;
        if (at >= n)
            return 0;
        if (by[0] == 1u) {
            kind = by[at] >> 4;
            pre = by[at++] & 15u;
        } else {
            kind = by[at] & 0x7Fu;
            if (by[at++] & 0x80u) {                     /* a generated sound: its seed picks the preset here */
                if (at + 1u >= n)
                    return 0;
                pre = (uint32_t)by[at] << 8 | by[at + 1u];
                at += 2u;
            } else {
                if (at >= n)
                    return 0;
                pre = by[at++];
            }
        }
        if (kind >= TILT_NKIND)
            return 0;
        if (i < NTRK) {
            s->ins[i] = (uint8_t)TILT_TO_TFM[kind];
            s->pre[i] = (uint8_t)(pre % tfm_nsounds(s->ins[i]));
        }
    }
    r.s = hash | 1u;                                    /* fewer than four: the rest from the code itself */
    for (i = slots; i < NTRK; i++)
        tfm_band_track(&r, s, i);
    return 1;
}
