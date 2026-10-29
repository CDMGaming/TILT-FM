/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* Firmware save flows against NOR flash: deferred settings, retries, and failed-save rollback. */
#define main hostsim_main
#include "hostsim.c"
#undef main

static int32_t fm1_enc_take(uint32_t e) { (void)e; return 0; }
static uint32_t irq_start_race, irq_races;
static void fm1_irq_off(void)
{
    if (irq_start_race) {                         /* audio serviced PLAY immediately before main masks it */
        irq_start_race = 0;
        events_block(CTL);
        irq_races++;
    }
}
static void fm1_irq_on(void) {}
static void lcd_sync(void) {}
static void lcd_power(uint32_t s) { (void)s; }   /* (MENU > SCREEN OFF: lcd.c) */
static void lcd_wake_now(void) {}
static void lcd_blit(uint32_t x, uint32_t y, uint32_t w, uint32_t h, const uint16_t *p)
{ (void)x; (void)y; (void)w; (void)h; (void)p; }
#define FELUCCA_FLASH 1
#include "../firmware/src/gfx.c"
#include "../firmware/src/panel.c"
#include "../firmware/src/ui.c"
#include "../firmware/src/menu_items.c"
static void panel_setup(void) {}

static uint8_t nor[0x100000], flash_ok = 1;
static int erase_error, fail_after = -1;
static uint32_t erases, as_erases;               /* (as_erases: the autosave's two sectors, 0xE5000 / 0xE6000) */
/* round 2: a power cut after cut_ops more erase / program calls (-1: none): the call that meets it applies half of a
 * program and fails, then the part is dead (every call fails, nothing changes) until the test powers it on again */
static int cut_ops = -1, cut_dead;
static uint32_t progs, off_min = 0xFFFFFFFFu, off_max, reads;
static int st_read(uint32_t off, void *dst, uint32_t n) { reads++; memcpy(dst, nor + off, n); return 0; }
static int st_erase(uint32_t off)
{
    erases++;
    as_erases += off >= 0xE5000u && off < 0xE7000u;
    if (off < off_min) off_min = off;
    if (off + 4096u > off_max) off_max = off + 4096u;
    if (cut_dead) return -9;
    if (cut_ops == 0) { cut_dead = 1; memset(nor + off, 0x00, 2048); return -9; }   /* (a torn erase: half the sector left at 0) */
    if (cut_ops > 0) cut_ops--;
    if (erase_error) return -8;
    memset(nor + off, 0xFF, 4096);
    return 0;
}
static int st_prog(uint32_t off, const void *src, uint32_t n)
{
    const uint8_t *s = src;
    uint32_t i;
    progs++;
    if (off < off_min) off_min = off;
    if (off + n > off_max) off_max = off + n;
    if (cut_dead) return -9;
    if (cut_ops == 0) {
        cut_dead = 1;
        for (i = 0; i < n / 2u; i++) nor[off + i] &= s[i];
        return -9;
    }
    if (cut_ops > 0) cut_ops--;
    if (fail_after == 0) return -9;
    if (fail_after > 0) fail_after--;
    for (i = 0; i < n; i++) nor[off + i] &= s[i];
    return 0;
}
static uint32_t irq_save(void) { return 0; }
static uint32_t wdt_feeds;
static void fm1_wdt_feed(void) { wdt_feeds++; }
static void irq_restore(uint32_t f) { (void)f; }
static uint32_t fl_jedec_ram(void) { return 0; }
static void fl_plain_window_init(void) {}
#define FL_FAR(fn) (fn)
#include "../firmware/src/storage.c"
#include "../firmware/src/upreset.c"
#include "../firmware/src/project.c"

enum { ED_UI_STATE = 34, ED_UI_SET, ED_UI_PALETTES, ED_FAV_GET, ED_FAV_SET };
static uint8_t ed_rep[64];                        /* the reply's bytes (MENU_SET) */
static uint32_t ed_repn;
static void ed_b(uint32_t n) { if (ed_repn < sizeof ed_rep) ed_rep[ed_repn++] = (uint8_t)(n & 127u); }
static void ed_v(int32_t n) { ed_b((uint32_t)(n + 8192)); ed_b((uint32_t)(n + 8192) >> 7); }
static int32_t ed_rv(const uint8_t *a) { return (int32_t)(a[0] | (uint32_t)a[1] << 7) - 8192; }
static void ed_str(const char *s, uint32_t n) { (void)s; (void)n; }
#include "../firmware/src/editor_preferences.c"
#include "../firmware/src/editor_menu.c"

static int check(const char *what, int ok)
{
    printf("persist: %-66s %s\n", what, ok ? "ok" : "FAIL");
    return ok ? 0 : 1;
}

/* round 2: a scene lives in flash (project.c scn_raw loads it into proj_slot[0]); these read it as a stored image and
 * rescan which scenes there are after a test wrote or damaged flash directly */
static project_store_t slot_img(uint32_t s)
{
    project_store_t z;
    const project_store_t *r = scn_raw(s);
    if (r) return *r;
    memset(&z, 0, sizeof z);
    return z;
}
static void scn_rescan(void)
{
    uint32_t g;
    scn_used = 0;
    for (g = 0; g < SCN_N; g++)
        if (scn_flash_ok(g)) scn_used |= 1u << g;
}

static void reset(void)
{
    memset(nor, 0xFF, sizeof nor);
    scn_used = 0; sng_cur = 0; memset(scn_pend_g, 0xFF, sizeof scn_pend_g); memset(scn_inf, 0, sizeof scn_inf);
    memset(&song, 0, sizeof song);
    memset(trk, 0, sizeof trk);
    memset(&chain, 0, sizeof chain);
    chain_defaults(&chain_config);
    memset(proj_slot, 0, sizeof proj_slot);
    memset(up_bank, 0, sizeof up_bank);
    memset(&persist_saved, 0, sizeof persist_saved);
    memset(&settings, 0, sizeof settings);
    memset(&ui, 0, sizeof ui);
    memset(&favorites, 0, sizeof favorites);
    panel = PANEL_DEFAULT;
    host_tracks_init();
    fm1_ms = 0;
    transport_req = 0;
    persist_pending = 0;
    persist_retry_ms = 0;
    flash_ok = 1;
    erase_error = 0;
    fail_after = -1;
    cut_ops = -1; cut_dead = 0; progs = 0; off_min = 0xFFFFFFFFu; off_max = 0; reads = 0;
    up_gen = erases = 0;
    irq_start_race = irq_races = 0;
}

/* ---- 1.0.3: the retired FM6 bank's patches move into the user presets on the first boot (up_fm6.c) ---- */
static uint8_t mig_pre[0x100000];
static fm6_bank_t mig_bank;

static void mig_rec(up_bank_t *b, uint32_t i, uint32_t engine, int16_t slot, char tag)
{
    up_rec_t *r = &b->r[i];
    uint32_t k;
    memset(r, 0, sizeof *r);
    r->used = UP_USED; r->ver = UP_VER; r->engine = (uint8_t)engine; r->np = P_COUNT;
    r->name[0] = tag;
    for (k = 0; k < P_COUNT; k++) up_set_value(r, k, param_desc_of(engine, k)->def);
    up_set_value(r, P_E7, slot);
    up_set_value(r, P_E0, (int16_t)(tag & 7));          /* (each record another sound) */
}

/* a 1.0.2 flash: user presets on B3 (slot 1), F2 (2), B5 empty (3), DRUM (4), B1 (18); the bank written twice */
static void mig_setup(void)
{
    static up_bank_t b[2];
    memset(nor, 0xFF, sizeof nor);
    memset(b, 0, sizeof b);
    for (uint32_t i = 0; i < 2u; i++) { b[i].magic = UP_BANK_MAGIC; b[i].rsize = sizeof(up_rec_t); b[i].nslot = UP_PER_BANK; }
    mig_rec(&b[0], 0, ENGI_FM6, FM6_NFACTORY + 2, 'A');
    mig_rec(&b[0], 1, ENGI_FM6, 1, 'B');
    mig_rec(&b[0], 2, ENGI_FM6, FM6_NFACTORY + 4, 'C');
    mig_rec(&b[0], 3, ENGI_DRUM, FM6_NFACTORY + 2, 'D');
    mig_rec(&b[1], 1, ENGI_FM6, FM6_NFACTORY + 0, 'E');
    st_save(OBJ_UPRESET0, &b[0], sizeof b[0]);
    st_save(OBJ_UPRESET0 + 1, &b[1], sizeof b[1]);
    memset(&mig_bank, 0, sizeof mig_bank);
    mig_bank.magic = FM6_BANK_MAGIC; mig_bank.ver = 1; mig_bank.nslot = FM6_BANK_N; mig_bank.used = 1u | 1u << 2;
    memcpy(mig_bank.v[2], FM6_FACTORY[4], FM6_PACKED); memcpy(mig_bank.v[2] + 118, "OLD BANK 1", 10);
    st_save(OBJ_FM6BANK, &mig_bank, sizeof mig_bank);   /* A: an older bank */
    memcpy(mig_bank.v[0], FM6_FACTORY[6], FM6_PACKED); memcpy(mig_bank.v[0] + 118, "NEW B1    ", 10);
    memcpy(mig_bank.v[2] + 118, "NEW B3    ", 10);
    st_save(OBJ_FM6BANK, &mig_bank, sizeof mig_bank);   /* B: the newest */
}

static void mig_boot(void)                               /* a power-on: RAM lost, the stores from flash */
{
    memset(up_bank, 0, sizeof up_bank);
    memset(&upf, 0, sizeof upf);
    up_boot();
}

/* the presets play what 1.0.2 played: slot 1 the newest B3, 18 B1, 2 F2, 3 init (an empty B5), DRUM none */
static int mig_ok(void)
{
    uint8_t pk[FM6_PACKED];
    return !upf_get(0, pk) && !memcmp(pk + 118, "NEW B3    ", 10) && !upf_get(17, pk) &&
           !memcmp(pk + 118, "NEW B1    ", 10) && upf_get(1, pk) && upf_get(2, pk) && upf_get(3, pk);
}

static int mig_test(void)
{
    int bad = 0, ok = 1;
    uint32_t cut, before, cuts = 0;
    uint8_t pk[FM6_PACKED];
    upf_t got;
    reset();
    mig_setup();
    memcpy(mig_pre, nor, sizeof nor);
    mig_boot();
    bad += check("FM6 bank -> user presets: B slots get the bank's newest patches", mig_ok());
    bad += check("  .. written into A (the bank's older copy); the bank's newest copy (B) still whole",
                 st_load(OBJ_UPFM6, &got, sizeof got) == (int)sizeof got && !memcmp(&got, &upf, sizeof got) &&
                     ((st_hdr_t *)(nor + 0x9F000))->type == OBJ_UPFM6 && st_load(OBJ_FM6BANK, &mig_bank, sizeof mig_bank) > 0 &&
                     !memcmp(mig_bank.v[0] + 118, "NEW B1", 6));
    up_load(0);
    bad += check("  .. UP_LOAD 1 plays the bank's B3, SLOT OWN", !memcmp(fm6_patch[song.sel] + FP_NAME, "NEW B3    ", 10) &&
                 TSEL->p[P_E7] == FM6_OWN);
    up_load(1);
    bad += check("  .. UP_LOAD 2 (F2) plays F2, SLOT F2", !memcmp(fm6_patch[song.sel] + FP_NAME, FM6_FACTORY[1] + 118, 10) &&
                 TSEL->p[P_E7] == 1);
    up_load(2);
    bad += check("  .. UP_LOAD 3 (an empty B5) plays the init voice, as before",
                 !memcmp(fm6_patch[song.sel] + FP_NAME, "INIT VOICE", 10) && TSEL->p[P_E7] == FM6_OWN);
    before = erases;
    mig_boot();
    bad += check("  .. the next boot: done (no write), the same patches", mig_ok() && erases == before);
    /* a power cut at every write step of the migration: the next boot ends with the same result */
    for (cut = 0; cut < 64u && ok; cut++) {
        memcpy(nor, mig_pre, sizeof nor);
        fail_after = (int)cut;
        mig_boot();                                      /* (writes stop at the cut: the rest fails) */
        if (fail_after > 0) break;                       /* (this boot finished before the cut) */
        fail_after = -1;
        cuts++;
        mig_boot();
        ok &= mig_ok() && st_load(OBJ_UPFM6, &got, sizeof got) == (int)sizeof got;
    }
    memcpy(nor, mig_pre, sizeof nor);
    erase_error = 1;
    mig_boot();                                          /* a cut before the erase */
    erase_error = 0;
    mig_boot();
    ok &= mig_ok();
    bad += check("a power cut at any step of the migration loses nothing (the next boot redoes it)", ok && cuts >= 10u);
    /* after it: a preset save writes the other sector (the bank's last copy); a cut there keeps the first copy */
    memcpy(nor, mig_pre, sizeof nor);
    fail_after = -1;
    mig_boot();
    trk[song.sel].eng_req = ENGI_FM6;
    fm6_load_slot(song.sel, 3);
    fail_after = (int)((sizeof(up_bank_t) + 255u) / 256u + 1u + 3u);   /* the preset bank's write passes, the patches' fails */
    up_store(20, "CUT");
    fail_after = -1;
    mig_boot();
    bad += check("a cut while a preset's patch is written: the others kept, that preset without one",
                 mig_ok() && upf_get(20, pk));
    up_store(20, "SAVED");
    mig_boot();
    bad += check("saved again: its patch, and the bank's sectors now both hold the patches object",
                 mig_ok() && !upf_get(20, pk) && !memcmp(pk + 118, FM6_FACTORY[3] + 118, 10) &&
                     ((st_hdr_t *)(nor + 0x9F000))->type == OBJ_UPFM6 && ((st_hdr_t *)(nor + 0xFE000))->type == OBJ_UPFM6);
    memset(nor, 0xFF, sizeof nor);                        /* a device that never had a bank: nothing written */
    before = erases;
    mig_boot();
    bad += check("no bank: nothing to move, nothing written", erases == before && upf_valid(&upf));
    return bad;
}

/* ---- 1.2, Discussion #130: the autosave (project.c): written only when the music changed, stopped and idle 10 s, at
 * most one a minute; restored at power-on (RESTORE LAST ON, a clean boot); a damaged one ignored ---- */
static void as_run(uint32_t ms)                          /* the main loop for ms (autosave_poll checks every 250 ms) */
{
    uint32_t end = fm1_ms + ms;
    while ((int32_t)(end - fm1_ms) > 0) {
        fm1_ms += 50u;
        autosave_poll();
    }
}
static void as_power_on(void)                            /* RAM lost: the defaults, then main.c's restore */
{
    uint32_t k;
    memset(trk, 0, sizeof trk);
    memset(&motion, 0, sizeof motion);
    memset(motion_active, 0, sizeof motion_active);
    memset(&as, 0, sizeof as);
    memset(&ui, 0, sizeof ui);
    proj_name[0] = 0;
    chain_defaults(&chain_config);
    host_tracks_init();
    for (k = 0; k < NTRK; k++)
        trk[k].eng_req = trk[k].engine = (uint8_t)TRK_DEF[k][0];
}
static void as_edit(uint32_t i)                          /* a change a project holds: step i's note (always another) */
{
    static uint32_t n;
    track_t *t = &trk[i % NTRK];
    step_t *s = &t->step[i % 16u];
    s->time = ST_NOTE;
    s->n = 1;
    s->note[0] = (uint8_t)(20u + (s->note[0] + 1u + n++ % 50u) % 100u);
    s->vel = 100;
}
static int autosave_test(void)
{
    int bad = 0, ok;
    uint32_t w, k, b0, minute;
    step_t st[NSTEP];
    project_store_t raw;
    reset();
    as_power_on();
    as_erases = 0;
    ok = !autosave_boot(1) && !as.writes;
    as_run(120000u);
    bad += check("autosave: power-on with none: nothing restored; 2 min unchanged: no write", ok && !as.writes && !as_erases);
    as_edit(0);
    as_run(9000u);
    ok = !as.writes;
    as_run(1500u);
    bad += check("autosave: a change: no write within 10 s, one once idle 10 s (one erase)",
                 ok && as.writes == 1u && as_erases == 1u && st_load(OBJ_AUTOSAVE, &raw, sizeof raw) == (int)sizeof raw);
    as_run(600000u);
    bad += check("autosave: 10 min unchanged after it: no more writes", as.writes == 1u && as_erases == 1u);
    song.g[G_SLOT] = 3;                                  /* (the PROJECT page's pick, the selected track: not the music) */
    song.sel = 2;
    as_run(120000u);
    bad += check("autosave: the selected track / PROJECT's slot pick alone: no write", as.writes == 1u);
    song.g[G_BPM] = 133;
    song.playing = 1;
    as_run(300000u);
    ok = as.writes == 1u;
    song.playing = 0;
    as_run(9000u);
    ok &= as.writes == 1u;
    as_run(1500u);
    bad += check("autosave: changed while playing: no write until stopped and idle 10 s, then one", ok && as.writes == 2u);
    as_edit(1);
    fm1_in.notes = 1u << 7;                              /* a key held (playing the sound, stopped) */
    as_run(60000u);
    ok = as.writes == 2u;
    fm1_in.notes = 0;
    trk[1].v[0].active = 1;                              /* .. its release still sounding */
    as_run(20000u);
    ok &= as.writes == 2u;
    trk[1].v[0].active = 0;
    as_run(9000u);
    ok &= as.writes == 2u;
    as_run(1500u);
    bad += check("autosave: a key held, a voice sounding: no write (an erase stops the audio); after, idle 10 s: one",
                 ok && as.writes == 3u);
    as_run(60000u);
    as_edit(2);                                          /* written 10 s on; then edits 15 s apart: a minute's gap */
    as_run(11000u);
    w = as.writes;
    as_edit(3);
    as_run(15000u);
    ok = as.writes == w;
    as_edit(4);
    as_run(15000u);
    ok &= as.writes == w;
    as_run(40000u);
    bad += check("autosave: two writes at least 60 s apart", ok && as.writes == w + 1u);
    as_run(60000u);
    w = as.writes;
    as_edit(5);
    for (k = 0; k < 120u; k++) {                         /* an editor backup going on: holds it */
        as_run(500u);
        autosave_hold();
    }
    ok = as.writes == w;
    as_run(10500u);
    bad += check("autosave: none during an editor transfer (autosave_hold), one after", ok && as.writes == w + 1u);
    /* power-on: restored */
    as_edit(6);
    song.g[G_BPM] = 97;
    str_cpy(proj_name, "MY TUNE", sizeof proj_name);
    as_run(70000u);
    memcpy(st, trk[0].step, sizeof st);
    b0 = as_erases;
    as_power_on();
    ok = memcmp(st, trk[0].step, sizeof st) != 0 && song.g[G_BPM] != 97;
    ok &= autosave_boot(1) == 1 && !memcmp(st, trk[0].step, sizeof st) && song.g[G_BPM] == 97 && str_eq(proj_name, "MY TUNE") &&
          ui.msg_t && str_eq(ui.msg, "RESTORED");
    as_run(300000u);
    bad += check("autosave: power-on restores it (steps, tempo, name; RESTORED); no write after (unchanged)",
                 ok && !as.writes && as_erases == b0);
    as_power_on();
    ok = autosave_boot(0) == 0 && song.g[G_BPM] != 97;
    as_run(60000u);
    bad += check("autosave: after a crash / hang / failed boot: not restored, nothing written", ok && as_erases == b0);
    as_power_on();
    ui_prefs |= PREF_RESTORE_OFF;
    ok = autosave_boot(1) == 0 && song.g[G_BPM] != 97;
    as_edit(7);
    as_run(300000u);
    bad += check("autosave: RESTORE LAST OFF: none restored, none written", ok && !as.writes && as_erases == b0);
    ui_prefs &= (uint8_t)~PREF_RESTORE_OFF;
    {   /* a damaged copy: the other one (A/B); both damaged: none */
        st_hdr_t h;
        int cur, other;
        as_power_on();
        autosave_boot(1);
        song.g[G_BPM] = 111;                             /* a newer copy into the other sector */
        as_run(11000u);
        cur = st_current(OBJ_AUTOSAVE, &h);
        nor[st_sector(OBJ_AUTOSAVE, (uint32_t)cur) + ST_PAYLOAD_OFF + 100u] ^= 0x10u;   /* its payload: the CRC fails */
        other = st_current(OBJ_AUTOSAVE, &h);
        as_power_on();
        ok = cur >= 0 && other == !cur && autosave_boot(1) == 1 && song.g[G_BPM] == 97;
        bad += check("autosave: the newest copy damaged: the copy before it is restored", ok);
        nor[st_sector(OBJ_AUTOSAVE, (uint32_t)other) + 12u] ^= 0x01u;   /* the other one's header too */
        as_power_on();
        ok = autosave_boot(1) == 0 && song.g[G_BPM] != 97 && song.g[G_BPM] != 111;
        bad += check("autosave: both copies damaged: ignored, power-on as new", ok);
        memset(nor + 0xE5000u, 0xFF, 0x2000u);
        st_save(OBJ_AUTOSAVE, "NOT A PROJECT", 13u);     /* a valid record that is no project */
        as_power_on();
        bad += check("autosave: a record that is no project: ignored", autosave_boot(1) == 0 && song.g[G_BPM] != 97);
    }
    /* a power cut in the middle of a write: the copy before stays in charge */
    memset(nor + 0xE5000u, 0xFF, 0x2000u);
    as_power_on();
    autosave_boot(1);
    as_edit(8);
    as_run(11000u);                                      /* (written: copy A) */
    song.g[G_BPM] = 150;
    fail_after = 3;                                      /* the next write stops after 3 programs */
    as_run(70000u);
    fail_after = -1;
    ok = as.err;
    as_power_on();
    ok &= autosave_boot(1) == 1 && song.g[G_BPM] != 150 && trk[0].step[8].n == 1u;
    bad += check("autosave: a write cut short (power off): the copy before is restored", ok);
    /* wear: a 4-hour session: 3 h of editing (a change every 3 s), pauses and playing it back, then an hour away */
    as_power_on();
    autosave_boot(1);
    as_erases = 0;
    for (minute = 0; minute < 240u; minute++) {
        uint32_t s;
        if (minute >= 180u) {
            as_run(60000u);
            continue;
        }
        song.playing = minute % 10u >= 7u;               /* 3 minutes in 10 playing */
        for (s = 0; s < 60u; s += 3u) {
            if (minute % 5u != 4u)                       /* (every fifth minute a pause, no edits) */
                as_edit(minute * 20u + s);
            as_run(3000u);
        }
    }
    song.playing = 0;
    as_run(20000u);
    printf("persist:   a 4-hour session (3 h editing and playing, 1 h idle): %u autosave writes\n", (unsigned)as_erases);
    bad += check("autosave wear: a session's writes stay far below one a minute", as_erases > 0u && as_erases <= 60u);
    {   /* worst case: a change, then just enough idle, over and over, for 8 hours */
        uint32_t i;
        as_erases = 0;
        for (i = 0; i < 8u * 3600u / 11u; i++) {
            as_edit(i);
            as_run(11000u);
        }
        printf("persist:   worst case, 8 h of a change every 11 s: %u writes (%u per sector)\n", (unsigned)as_erases,
               (unsigned)(as_erases / 2u));
        bad += check("autosave wear, worst case: at most one write a minute (480 in 8 h)", as_erases <= 8u * 60u + 1u);
    }
    return bad;
}


/* ---- TILT-FM (the review's fixes): the start screen and the timed take against the autosave in flash, and the web
 * editor's settings write keeping TILT-FM's settings byte ---- */
static int tilt_autosave_test(void)
{
    int bad = 0, ok;
    uint32_t w;
    static uint8_t before_scratch[sizeof proj_scratch];
    reset();
    as_power_on();
    autosave_boot(1);
    as_edit(0);
    as_run(11000u);                                      /* (an autosave in flash) */
    w = as.writes;
    as_power_on();                                       /* after a crash: there (1), never parsed */
    memset(&proj_scratch, 0xA5, sizeof proj_scratch);
    memcpy(before_scratch, &proj_scratch, sizeof proj_scratch);
    ok = w == 1u && autosave_peek(0) == 1 && !memcmp(before_scratch, &proj_scratch, sizeof proj_scratch);
    bad += check("TILT-FM start screen after a crash: the autosave is there, not parsed (the import buffer untouched)", ok);
    ok = autosave_peek(1) == 1 && memcmp(before_scratch, &proj_scratch, sizeof proj_scratch);
    bad += check("TILT-FM start screen after a clean boot: the autosave there and imported to look at", ok);
    as_power_on();
    autosave_peek(1);
    w = as.writes;                                       /* (a power-on: the count from 0) */
    tilt_ns_open = TILT_START;                           /* the start screen up: a change (a MIDI CC) writes nothing */
    as_edit(5);
    as_run(120000u);
    ok = as.writes == w;
    tilt_ns_open = 0;
    autosave_rebase();                                   /* (BLANK: tilt.c st_do) */
    as_run(120000u);
    ok &= as.writes == w;
    bad += check("TILT-FM start screen: a change while it shows (a MIDI CC), then BLANK: nothing written", ok);
    as_edit(6);
    as_run(11000u);
    bad += check("  .. an edit after it: written once idle 10 s", as.writes == w + 1u);
    w = as.writes;
    tilt_start_pending = 1;                              /* LOAD / NEW SONG chosen: what they made is the baseline */
    song.g[G_BPM] = 97;
    autosave_rebase();
    as_run(120000u);
    ok = as.writes == w && tilt_start_pending;
    as_edit(7);
    as_run(300u);
    ok &= !tilt_start_pending;
    as_run(11000u);
    bad += check("TILT-FM start screen LOAD / NEW SONG: nothing written until an edit; the edit ends the choice, written",
                 ok && as.writes == w + 1u);
    w = as.writes;                                       /* the timed take */
    song.playing = 0;
    ok = autosave_now() == 2 && as.writes == w;
    as_edit(8);
    tilt_start_pending = 1;
    ok &= autosave_now() == 2 && as.writes == w;
    tilt_start_pending = 0;
    ok &= autosave_now() == 1 && as.writes == w + 1u;
    ok &= autosave_now() == 2 && as.writes == w + 1u;
    bad += check("TILT-FM timed take: unchanged, or the start screen's choice: nothing written (2); changed: written (1)", ok);
    {
        const uint8_t pal[] = {0, 3};
        persist_t got;
        song.playing = 0;
        favorites.factory[15][13] = 1;                   /* START SCREEN OFF */
        ok = ed_ui_set(pal, sizeof pal) == 0 && persist_saved.favorites.factory[15][13] == 1u &&
             st_load(OBJ_SETTINGS, &got, sizeof got) == (int)sizeof got && got.favorites.factory[15][13] == 1u;
        favorites.factory[15][13] = 0;
        settings_save();
        bad += check("TILT-FM: the web editor's settings write keeps [15][13] (START SCREEN), in RAM and in flash", ok);
    }
    return bad;
}
/* ---- round 2: 8 scenes x 3 songs (storage.c's map), the first-boot migration, the bar-line switch ---- */
static project_store_t old_img[4];
static chain_config_t old_chain;
/* a flash as the update finds it: scenes A..D stored (notes 60..63, a chain with a silent lane), the three user sample
 * slots full of whatever they held (junk, with an "FSMP" header at the first) */
static void old_world(void)
{
    uint32_t i;
    reset();
    memset(nor + 0xA0000, 0x5A, 0xDC000u - 0xA0000u);
    memcpy(nor + 0xA0000, "FSMP", 4);
    memset(nor + 0xB4000, 0xA5, 0x1000);
    chain_defaults(&chain_config);
    chain_config.count = 2;
    chain_config.row[0] = (chain_row_t){{0, 1, CHAIN_SILENT, 2}, 3};
    chain_config.row[1] = (chain_row_t){{3, 3, 3, 3}, 1};
    old_chain = chain_config;
    song.g[G_BPM] = 97;
    for (i = 0; i < 4u; i++) {
        trk[0].step[0] = (step_t){{(uint8_t)(60u + i)}, 1, ST_NOTE, 0, 96, 0, 0};
        project_capture(&proj_scratch);
        proj_pack(&proj_wire, &proj_scratch);
        old_img[i] = proj_wire;
        st_save(OBJ_PROJECT0 + i, &proj_wire, sizeof proj_wire);
    }
    scn_used = 0; sng_cur = 0;
    memset(song_rec, 0, sizeof song_rec);
    erases = progs = 0;
}
static int old_intact(void)                        /* scenes A..D are in flash exactly as they were */
{
    uint32_t i;
    project_store_t got2;
    for (i = 0; i < 4u; i++)
        if (st_load(OBJ_PROJECT0 + i, &got2, sizeof got2) != (int)sizeof got2 || memcmp(&got2, &old_img[i], sizeof got2))
            return 0;
    return 1;
}
static int region_clean(void)                      /* every sector of the region is blank or a record of ours */
{
    uint32_t off;
    for (off = 0xA0000u; off < 0xDC000u; off += 4096u) {
        uint32_t i, blank = 1, magic;
        for (i = 0; i < 4096u && blank; i++)
            if (nor[off + i] != 0xFF) blank = 0;
        memcpy(&magic, nor + off, 4);
        if (!blank && magic != ST_MAGIC) return 0;
    }
    return 1;
}
static int migrated_ok(void)                       /* the state a finished migration leaves */
{
    scn_tag_t tg;
    chain_config_t c;
    if (st_load(OBJ_SCFMT, &tg, sizeof tg) != (int)sizeof tg || tg.magic != SCN_TAG) return 0;
    if (!old_intact() || !region_clean()) return 0;
    if (scn_used != 0xFu || !song_stored(0) || song_stored(1) || song_stored(2)) return 0;
    c = song_rec[0].chain;
    chain_dec(&c);
    return !memcmp(&c, &old_chain, sizeof c) && song_rec[0].g[G_BPM] == 97;
}
static int scenes_round2(void)
{
    int bad = 0, ok;
    uint32_t g, a, b, n_ops, k, bad_cut = 0;
    /* the map */
    ok = 1;
    for (g = 0; g < SCN_N; g++) {
        uint32_t o = st_scene_obj(g), c;
        for (c = 0; c < 2u; c++) {
            uint32_t s = st_sector(o, c);
            ok &= s >= (g < 4u ? 0x97000u : 0xA0000u) && s + 4096u <= (g < 4u ? 0x9F000u : 0xC8000u);
            for (a = 0; a < SCN_N; a++)
                for (b = 0; b < 2u; b++)
                    if ((a != g || b != c) && st_sector(st_scene_obj(a), b) == s) ok = 0;
        }
    }
    for (a = 0; a < NSONG; a++)
        for (b = 0; b < 2u; b++)
            ok &= st_sector(OBJ_SONG0 + a, b) >= 0xC8000u && st_sector(OBJ_SONG0 + a, b) + 4096u <= 0xCE000u;
    ok &= st_sector(OBJ_SCFMT, 0) == 0xCE000u && st_sector(OBJ_SCFMT, 1) == 0xCF000u;
    bad += check("round 2 map: 24 scenes x A/B, 3 songs, the tag: each its own sectors, inside 0x97000..0xCFFFF", ok);
    bad += check("round 2 map: the types are apart from the voice bank's record (32) and refused when out of range",
                 !st_obj_ok(OBJ_FM6VB) && st_obj_ok(OBJ_SCN0) && st_obj_ok(OBJ_SCFMT) && !st_obj_ok(OBJ_TOP) &&
                 st_load(OBJ_FM6VB, &old_img[0], sizeof old_img[0]) < 0);

    /* 24 scenes: each stores, reads back its own notes, and nothing else changes */
    reset();
    ok = 1;
    for (g = 0; g < SCN_N; g++) {
        sng_cur = (uint8_t)(g / NSCN);
        trk[0].step[0] = (step_t){{(uint8_t)(30u + g)}, 1, ST_NOTE, 0, 96, 0, 0};
        ok &= project_save_as(g % NSCN, 0) == 0 && project_used(g % NSCN);
    }
    for (g = 0; g < SCN_N; g++) {
        sng_cur = (uint8_t)(g / NSCN);
        ok &= project_used(g % NSCN) && scn_get(g, &proj_scratch) && proj_scratch.t[0].step[0].note[0] == 30u + g;
    }
    sng_cur = 0;
    bad += check("round 2: 24 scenes (3 songs x A..H) each store and read back their own notes", ok);
    bad += check("round 2: the writes stayed in 0x97000..0xCFFFF (nothing below 0x4000, the OTA, autosave or settings touched)",
                 off_min >= 0x97000u && off_max <= 0xD0000u && erases == 24u);
    for (g = 0; g < SCN_N; g++) nor[st_sector(st_scene_obj(g), 0) + 20] ^= 0u;
    scn_used = 0;
    sng_cur = 0;
    scn_rescan();
    bad += check("round 2: a rescan (power-on) finds all 24", scn_used == 0xFFFFFFu);

    /* the migration */
    old_world();
    scn_boot();
    bad += check("round 2 migration: old scenes A..D, their chain (with a silent lane) and tempo = song 1; E..H and songs 2, 3 empty; "
                 "the sample region wiped, the tag set", migrated_ok());
    bad += check("round 2 migration: scenes A..D byte for byte as they were; song 1's chain decodes to the old one",
                 old_intact() && !memcmp(&old_chain.row[0], &(chain_row_t){{0, 1, CHAIN_SILENT, 2}, 3}, sizeof(chain_row_t)));
    k = erases + progs;
    scn_boot();
    bad += check("round 2 migration: it runs once (the second boot writes nothing)", erases + progs == k && migrated_ok());
    old_world();
    wdt_feeds = 0;
    scn_boot();
    bad += check("round 2 migration: the watchdog is fed between the sector erases (a first boot may take seconds)", wdt_feeds >= 4u);
    /* a power cut at every erase / program call of it, and a torn one */
    old_world();
    scn_boot();
    n_ops = erases + progs;
    for (k = 0; k <= n_ops + 1u; k++) {
        old_world();
        cut_ops = (int)k; cut_dead = 0;
        scn_boot();
        cut_ops = -1; cut_dead = 0;
        if (!old_intact()) { bad_cut++; continue; }              /* the old data, at the cut */
        scn_boot();                                               /* power on again */
        if (!migrated_ok()) bad_cut++;
    }
    bad += check("round 2 migration: a power cut at any erase or program (torn ones too): the old scenes are there at the cut and "
                 "after the next boot, the migration finishes", bad_cut == 0 && n_ops > 10u);
    printf("persist:   (%u erase/program calls cut one by one, %u bad)\n", (unsigned)n_ops, (unsigned)bad_cut);
    /* a flash that fails on the first boot still lets the old scenes work */
    old_world();
    erase_error = 1;
    scn_boot();
    erase_error = 0;
    bad += check("round 2 migration: a flash that cannot erase leaves the old scenes usable (A..D), E..H and the songs absent",
                 old_intact() && scn_used == 0xFu && !song_stored(0));

    /* going back (round 2): an older firmware sees the sample slots at 0xA0000 / 0xB4000 / 0xC8000 and the four projects at
     * 0x97000: no valid sample header, the projects as they were. Then an older firmware writes samples over the region: the
     * next boot migrates again, or takes the scenes that survived; neither hangs nor loops */
    {
        uint32_t k, i, e0;
        project_store_t keep4[4];
        old_world();
        scn_boot();                                            /* migrated: tag, song 1 */
        for (g = 4; g < SCN_N; g++) {                          /* scenes in every slot of the region, songs 2 and 3 */
            sng_cur = (uint8_t)(g / NSCN);
            trk[0].step[0] = (step_t){{(uint8_t)(30u + g)}, 1, ST_NOTE, 0, 96, 0, 0};
            project_save_as(g % NSCN, 0);
        }
        sng_cur = 1; song_save_named(1, "TWO"); sng_cur = 0;
        ok = 1;
        for (k = 0; k < 3u; k++) ok &= memcmp(nor + 0xA0000u + k * 0x14000u, "FSMP", 4) != 0;      /* (eng_sample.c's header magic) */
        bad += check("round 2 going back: an older firmware finds no valid sample header in the three slots, the projects A..D as they were", ok && old_intact());
        for (i = 0; i < 4u; i++) keep4[i] = old_img[i];
        /* the old firmware uploaded a sample into slot 1 only (0xA0000..0xB3FFF) */
        memset(nor + 0xA0000, 0x33, 0x14000); memcpy(nor + 0xA0000, "FSMP", 4);
        e0 = erases;
        scn_boot();
        bad += check("round 2 going back: a sample over slot 1 only: the tag survives (no migration), the scenes there are empty, the rest and A..D kept",
                     erases == e0 && old_intact() && !((scn_used >> 4) & 0x3FFu) && ((scn_used >> 14) & 0x3FFu) == 0x3FFu && song_stored(0) == 1);
        /* ... and over the whole region (the three slots, the tag with them) */
        for (k = 0; k < 3u; k++) { memset(nor + 0xA0000u + k * 0x14000u, 0x44, 0x14000); memcpy(nor + 0xA0000u + k * 0x14000u, "FSMP", 4); }
        scn_boot();
        bad += check("round 2 going back: samples in all three slots: the next boot migrates again (A..D and song 1 back, E..H and songs 2, 3 empty)",
                     migrated_ok());
        e0 = erases;
        scn_boot();
        bad += check("round 2 going back: ... and then it is quiet (no loop)", erases == e0 && migrated_ok());
        (void)keep4;
    }

    /* playing: a scene switch is read from flash and flips on the bar line; flash is never written meanwhile */
    {
        uint32_t e0, p0, r0, n, t, as0;
        project_store_t fl;
        project_t dd;
        reset();
        sng_cur = 0;
        tilt_ns_open = 0;
        for (t = 0; t < NTRK; t++) { trk[t].p[P_SLEN] = 16; trk[t].p[P_SDIV] = 2; trk[t].p[P_SSWING] = 0; }
        trk[0].step[0] = (step_t){{60}, 1, ST_NOTE, 0, 96, 0, 0};
        ok = project_save_as(0, 0) == 0;                      /* scene A */
        trk[0].step[0] = (step_t){{64}, 1, ST_NOTE, 0, 96, 0, 0};
        ok &= project_save_as(5, 0) == 0;                     /* scene F (a song past the old four) */
        ok &= scene_load(0) == 0 && scene_cur() == 0u && trk[0].step[0].note[0] == 60u;
        scene_based();
        transport_req = 1;
        events_block(32);
        for (n = 1; n < 300u; n++) events_block(32);
        trk[0].step[2] = (step_t){{67}, 1, ST_NOTE, 0, 96, 0, 0};   /* A edited while it plays */
        e0 = erases; p0 = progs; r0 = reads;
        /* (what tilt.c tilt_scene_go does for a transport that runs: the edited scene to RAM, the cue staged from flash) */
        ok &= scene_edited() && scene_store_ram(0) == 0 && scene_stage_from(5) == 0;
        tsc.cue = 5; RING_PUBLISH(); scene_armed = 1;
        for (n = 0; n < 4000u && scene_armed == 1u; n++) { events_block(32); scene_flush(); }
        if (scene_armed == 2u) { scene_armed = 0; scene_enter(5); tsc.cue = SCENE_NONE; pat_from_set(6); scene_based(); }
        bad += check("round 2 playing: scene F cued, read from flash, flipped on the bar line: scene F is the music",
                     ok && scene_armed == 0u && scene_cur() == 5u && trk[0].step[0].note[0] == 64u && !scene_edited() && reads > r0);
        bad += check("round 2 playing: no flash was erased or programmed during the cue, the flip and the following passes",
                     erases == e0 && progs == p0);
        bad += check("round 2 playing: the scene left (A, edited) waits in RAM for the stop (its dot), flash still has the old A",
                     (tsc.pend & 1u) && scn_pend_g[0] == 0u && scn_get(0, &proj_scratch) && proj_scratch.t[0].step[2].note[0] == 67u &&
                     st_load(OBJ_PROJECT0, &fl, sizeof fl) == (int)sizeof fl && proj_import(&dd, &fl, sizeof fl) && dd.t[0].step[2].time != ST_NOTE);
        seq_stop(); transport_req = 0; events_block(32);
        e0 = erases; as0 = as_erases;
        scene_flush();
        bad += check("round 2 playing: at the stop A is written (one erase, then the autosave's), the dot goes, A has its edit in flash",
                     erases == e0 + 2u && as_erases == as0 + 1u && !tsc.pend && scn_pend_g[0] == 0xFFu && st_load(OBJ_PROJECT0, &fl, sizeof fl) == (int)sizeof fl &&
                     proj_import(&dd, &fl, sizeof fl) && dd.t[0].step[2].note[0] == 67u);
        /* three scenes can wait; a fourth is refused (STOP TO SAVE), nothing lost */
        transport_req = 1; events_block(32);
        for (n = 1; n < 40u; n++) events_block(32);
        ok = 1;
        for (t = 0; t < 4u; t++) {
            trk[0].step[1] = (step_t){{(uint8_t)(50u + t)}, 1, ST_NOTE, 0, 96, 0, 0};
            ok &= scene_store_ram(t) == (t < 3u ? 0 : 1);
        }
        bad += check("round 2 playing: three edited scenes wait in RAM, a fourth is refused (STOP TO SAVE): nothing is written meanwhile",
                     ok && !strcmp(ui.msg, "STOP TO SAVE") && erases == e0 + 2u);
        seq_stop(); transport_req = 0; events_block(32);
        scene_flush();
        bad += check("round 2 playing: the stop writes the three (and the autosave)", !tsc.pend && erases == e0 + 6u && scn_get(2, &proj_scratch) &&
                     proj_scratch.t[0].step[1].note[0] == 52u);
    }
    return bad;
}

int main(void)
{
    int bad = 0, ok;
    uint32_t before;
    persist_t p;
    project_store_t old, got;
    project_v5_t v5;
    up_bank_t bank;
    up_rec_t r;
    reset();
    settings_init();
    {
        const uint8_t amber[] = {0, 2};
        song.playing = 1;
        bad += check("editor display change while playing is applied and reports deferred saving",
                      ed_ui_set(amber, sizeof amber) == 4 && settings.palette == 2u && persist_pending == 1 && !erases);
        song.playing = 0;
        settings_poll();
        bad += check("STOP commits the deferred palette setting",
                      !persist_pending && erases == 1 && persist_saved.palette == palette_to_stored(2));
        before = erases;
        bad += check("repeating an unchanged editor preference succeeds without erasing",
                      ed_ui_set(amber, sizeof amber) == 0 && erases == before);
        erase_error = 1;
        const uint8_t mono[] = {0, 0};
        bad += check("editor reports failed saving while preserving the previous flash setting",
                      ed_ui_set(mono, sizeof mono) == 3 && settings.palette == 0u && persist_pending == 2 &&
                      persist_saved.palette == palette_to_stored(2));
        erase_error = 0;
        fm1_ms += 1000;
        settings_poll();
        bad += check("failed editor preference retries through the existing persistence path",
                      !persist_pending && persist_saved.palette == palette_to_stored(0));
        const uint8_t bold[] = {1, 1};
        bad += check("the retired font weight is refused without a write", ed_ui_set(bold, sizeof bold) == 2);
    }
    {   /* MENU_SET (editor_menu.c): the same settings record, the same deferral, retry and idempotence */
        const uint8_t leds_off[] = {4, 0, 64}, large_on[] = {2, 1, 64}, hold_06[] = {5, 3, 64}, eq_bass[] = {9, 2, 64};
        persist_t q;
        reset();
        settings_init();
        song.playing = 1;
        ed_repn = 0; ed_menu_handle(ED_MENU_SET, leds_off, 3);
        bad += check("MENU_SET while playing: applied, rc 4 (queued until STOP), no erase",
                     ed_repn == 4 && ed_rep[0] == 4 && ed_rep[1] == 4 && settings_leds == LEDS_OFF && persist_pending == 1 && !erases);
        song.playing = 0;
        settings_poll();
        bad += check("STOP commits the MENU setting (LEDS OFF in the settings record)",
                     !persist_pending && erases == 1 && st_load(OBJ_SETTINGS, &q, sizeof q) == (int)sizeof q &&
                     leds_from_stored(q.zoom) == LEDS_OFF);
        ed_repn = 0; ed_menu_handle(ED_MENU_SET, large_on, 3);
        ok = ed_rep[0] == 0 && (ui_prefs & PREF_LARGE);
        ed_repn = 0; ed_menu_handle(ED_MENU_SET, hold_06, 3);
        ok &= ed_rep[0] == 0 && settings_hold == 3u;
        ed_repn = 0; ed_menu_handle(ED_MENU_SET, eq_bass, 3);
        ok &= ed_rep[0] == 0 && st_load(OBJ_SETTINGS, &q, sizeof q) == (int)sizeof q && q.lowcut == 2u &&
              hold_from_stored(q.bold) == 3u && (q.favorites.factory[15][30] & PREF_LARGE) && leds_from_stored(q.zoom) == LEDS_OFF;
        bad += check("MENU_SET stopped: rc 0, saved at once (LARGE, HOLD, SPEAKER EQ), the others kept", ok);
        before = erases;
        ed_repn = 0; ed_menu_handle(ED_MENU_SET, hold_06, 3);
        bad += check("repeating an unchanged MENU_SET: rc 0, no erase", ed_rep[0] == 0 && erases == before);
        erase_error = 1;
        ed_repn = 0; ed_menu_handle(ED_MENU_SET, leds_off, 3);   /* unchanged: still rc 0 (no write) */
        ok = ed_rep[0] == 0;
        {
            const uint8_t leds_inv[] = {4, 3, 64};
            ed_repn = 0; ed_menu_handle(ED_MENU_SET, leds_inv, 3);
        }
        ok &= ed_rep[0] == 3 && settings_leds == LEDS_INV && persist_pending == 2 && leds_from_stored(persist_saved.zoom) == LEDS_OFF;
        bad += check("MENU_SET with a failing flash: applied, rc 3, the saved setting kept", ok);
        erase_error = 0;
        fm1_ms += 1000;
        settings_poll();
        bad += check("... and retried by the settings path", !persist_pending && leds_from_stored(persist_saved.zoom) == LEDS_INV);
        settings_hold = HOLD_DEF; settings_leds = LEDS_DIM;
    }
    reset();
    settings.palette = 3;
    settings.lowcut = 2;
    settings.zoom = 1;
    persist_saved.bold = 1;                       /* (an earlier firmware's font weight: kept as saved) */
    favorites.factory[8][0] = 1;
    favorites.user = 1u << 31;
    song.playing = 1;
    settings_save();
    bad += check("playing: settings queued, no erase, runtime kept", persist_pending && !erases &&
                  settings.palette == 3u && settings.lowcut == 2u && settings.zoom == 1u);
    song.playing = 0;
    transport_req = 1;
    settings_poll();
    bad += check("pending PLAY: settings still queued, no erase", persist_pending && !erases);
    transport_req = 0;
    chain.armed = 1;
    settings_poll();
    bad += check("pending SONG: settings still queued, no erase", persist_pending && !erases);
    chain.armed = 0;
    settings.palette = 2;
    settings_poll();
    bad += check("STOP: latest settings saved and request cleared", !persist_pending && erases == 1u &&
                  st_load(OBJ_SETTINGS, &p, sizeof p) == (int)sizeof p && p.palette == palette_to_stored(2) &&
                  p.lowcut == 2u && p.zoom == 1u && p.bold == 1u &&
                  p.favorites.factory[8][0] == 1 && p.favorites.user == (1u << 31) &&
                  !memcmp(&p.panel, &panel, sizeof panel));
    settings_save();
    bad += check("unchanged settings do not erase", !persist_pending && erases == 1u);
    settings.palette = 1;
    erase_error = 1;
    fm1_ms = 0xFFFFFF00u;
    settings_save();
    before = erases;
    settings_poll();
    fm1_ms += 999u;
    settings_poll();
    bad += check("failed settings retry waits through clock wrap", persist_pending && erases == before &&
                  persist_saved.palette == palette_to_stored(2) && settings.palette == 1u);
    erase_error = 0;
    fm1_ms++;
    settings_poll();
    bad += check("settings retry succeeds after 1000 ms", !persist_pending && erases == before + 1u &&
                  st_load(OBJ_SETTINGS, &p, sizeof p) == (int)sizeof p && p.palette == palette_to_stored(1));

    reset();
    trk[0].step[0] = (step_t){{60}, 1, ST_NOTE, 0, 96, 0, 0};
    project_save(0);
    old = slot_img(0);
    trk[0].step[0].note[0] = 72;
    fail_after = 1;
    project_save(0);
    fail_after = -1;
    got = slot_img(0);
    bad += check("failed project save keeps the scene as it was (flash)", !strcmp(ui.msg, "SAVE ERROR") &&
                  !memcmp(&got, &old, sizeof old) && st_load(OBJ_PROJECT0, &got, sizeof got) == (int)sizeof got &&
                  !memcmp(&got, &old, sizeof got));
    project_save(0);
    bad += check("successful project save publishes the new scene", !strcmp(ui.msg, "SAVED") &&
                  scn_get(0, &proj_scratch) &&
                  proj_scratch.t[0].step[0].note[0] == 72u && proj_ok(&proj_scratch));
    before = erases;
    transport_req = 1;
    project_save(1);
    bad += check("pending PLAY: project save leaves slot and flash", !project_used(1) && erases == before &&
                  !strcmp(ui.msg, "STOP TO SAVE"));
    transport_req = 0;
    erase_error = 1;
    project_save(2);
    bad += check("failed first project save leaves slot empty", !project_used(2));
    erase_error = 0;
    {   /* names: a project's through flash; a failed rename keeps the old one in RAM and flash */
        char n[16];
        project_store_t keep;
        step_t st0 = trk[0].step[0];
        project_save_as(0, "LOFI JAM");
        bad += check("a project's name goes to flash and comes back", project_name(0, n) && !strcmp(n, "LOFI JAM") &&
                      !strcmp(proj_name, "LOFI JAM"));
        keep = slot_img(0);
        fail_after = 1;
        bad += check("failed project rename: SAVE ERROR, the slot and flash as they were",
                      project_rename(0, "DUB") == 2 && !strcmp(ui.msg, "SAVE ERROR") &&
                      st_load(OBJ_PROJECT0, &got, sizeof got) == (int)sizeof got && !memcmp(&got, &keep, sizeof got));
        fail_after = -1;
        before = erases;
        transport_req = 1;
        bad += check("pending PLAY: no rename, no erase", project_rename(0, "DUB") == 1 && erases == before &&
                      !strcmp(ui.msg, "STOP TO SAVE"));
        transport_req = 0;
        bad += check("project rename: flash has the new name, the music as it was", project_rename(0, "DUB") == 0 &&
                      !strcmp(ui.msg, "RENAMED") && (scn_inf[0].ok = 0, project_name(0, n)) &&
                      !strcmp(n, "DUB") && scn_get(0, &proj_scratch) && proj_scratch.t[0].step[0].note[0] == st0.note[0]);
    }
    {   /* 1.5.5 (#199): SAVE > PROJECT ERASE: an empty record committed as a save is (A/B, the record last) */
        project_store_t keep = slot_img(0);
        int ok2;
        fail_after = 0;                                        /* (the commit record's write fails: a power cut) */
        ok2 = project_erase(0) == 2 && !strcmp(ui.msg, "ERASE ERROR") && project_used(0) &&
              st_load(OBJ_PROJECT0, &got, sizeof got) == (int)sizeof got && !memcmp(&got, &keep, sizeof got);
        fail_after = -1;
        bad += check("failed project erase (cut before its record): ERASE ERROR, the project in RAM and flash", ok2);
        before = erases;
        transport_req = 1;
        ok2 = project_erase(0) == 1 && erases == before && !strcmp(ui.msg, "STOP TO ERASE") && project_used(0);
        transport_req = 0;
        ok2 &= project_erase(3) == 1 && erases == before && !strcmp(ui.msg, "EMPTY SLOT");
        bad += check("project erase refused while playing and on an empty slot (no flash erase)", ok2);
        (void)project_save(1);
        before = erases;
        proj_cur = 0;
        pat_from[0] = 1; pat_from[1] = 2;
        ok2 = project_erase(0) == 0 && !strcmp(ui.msg, "SCENE A ERASED") && !project_used(0) && erases == before + 1u &&
              st_load(OBJ_PROJECT0, &got, sizeof got) == 0 && proj_cur == PROJ_NO_SLOT && !pat_from[0] && pat_from[1] == 2u;
        scn_rescan();
        ok2 &= !project_used(0) && project_used(1);           /* (read back from flash: empty; the next slot kept) */
        bad += check("project erase: the slot empty in RAM and flash (no older copy comes back), the others kept", ok2);
        bad += check("an erased slot takes a new save", project_save(0) == 0 && project_used(0) &&
                      (scn_rescan(), project_used(0)));
    }
    {   /* FUN7 itself refuses a step outside its fields: a damaged slot is empty, the tracks stay */
        step_t keep = trk[0].step[0];
        old = slot_img(0);
        {
            project_store_t dmg = old;
            dmg.raw[68 + P_COUNT + 2 + 4] = 7;               /* track 1 step 1: n = 7 */
            memcpy(dmg.raw + PROJ_STORE_SIZE - 4, &(uint32_t){proj_hash(dmg.raw, PROJ_STORE_SIZE - 4)}, 4);
            st_save(OBJ_PROJECT0, &dmg, sizeof dmg);
            scn_rescan();
            bad += check("damaged FUN7 slot (hash ok, n > 4) is refused", !project_used(0));
            scn_used |= 1u;                                      /* (stored, but it does not load) */
        }
        ui.msg[0] = 0;
        project_load(0);
        bad += check("loading it says EMPTY SLOT and keeps the steps", !strcmp(ui.msg, "EMPTY SLOT") &&
                      !memcmp(&trk[0].step[0], &keep, sizeof keep));
        st_save(OBJ_PROJECT0, &old, sizeof old);
        scn_rescan();
    }
    {   /* an older format (FUN5) with values FUN7 cannot pack still loads, bounded */
        uint32_t i;
        memset(&v5, 0, sizeof v5);
        v5.magic = PROJ_MAGIC_V5;
        v5.size = sizeof v5;
        for (i = 0; i < G_COUNT; i++)
            v5.g[i] = GP[i].def;
        for (i = 0; i < NTRK; i++) {
            v5.t[i].engine = trk[i].engine;
            memcpy(v5.t[i].p, trk[i].p, sizeof v5.t[i].p);
        }
        v5.t[0].p[P_LEVEL] = 900;                                  /* out of range */
        v5.t[0].step[0] = (step10_t){{255, 128, 160, 200}, 255, 255, 0, 96, 255, 5};
        v5.sum = proj_hash(&v5, sizeof v5 - 4u);
        st_save(OBJ_PROJECT0 + 3u, &v5, sizeof v5);
        scn_rescan();
        project_load(3);
        bad += check("FUN5 load bounds steps and masks lane accents", trk[0].step[0].n == 4u &&
                      trk[0].step[0].time == ST_REST && trk[0].step[0].note[0] == 127u &&
                      !trk[0].step[0].note[1] && trk[0].step[0].acc == 5u && project_used(3));
        bad += check("FUN5 load clamps a parameter into its range", trk[0].p[P_LEVEL] == TP[P_LEVEL].max);
        for (i = 0; i < P_COUNT; i++) {
            uint32_t e;
            for (e = 0; e < NENGINES; e++)
                if (param_desc_of(e, i)->min < -64 || param_desc_of(e, i)->max > 127) {
                    printf("param %u engine %u: range %d..%d does not fit FUN7's byte\n", i, e,
                           param_desc_of(e, i)->min, param_desc_of(e, i)->max);
                    bad++;
                }
        }
    }
    chain_config.count = 1;
    chain_config.row[0] = chain_row_of(3, 1);
    bad += check("SONG sources use the same step bounds", chain_prepare() == 0 &&
                  !memcmp(&chain.source[3].step[0][0], &trk[0].step[0], sizeof(step_t)));
    seq_stop();
    transport_req = 0;
    memset(&r, 0, sizeof r);
    r.used = UP_USED;
    r.ver = UP_VER;
    r.np = P_COUNT;
    memcpy(r.name, "First", 6);
    erase_error = 1;
    bank = up_bank[1];
    ok = up_put(17, &r) == 2;
    erase_error = 0;
    bad += check("failed first preset save restores empty bank", ok && !memcmp(&up_bank[1], &bank, sizeof bank));
    bad += check("preset initial save", up_put(3, &r) == 0 && up_used(3));
    bank = up_bank[0];
    before = up_gen;
    r.p[0] = 77;
    fail_after = 1;
    ok = up_put(3, &r) == 2;
    fail_after = -1;
    bad += check("failed preset save rolls back bank and generation", ok && up_gen == before &&
                  !memcmp(&up_bank[0], &bank, sizeof bank));
    favorite_set(NENGINES, 3, 1);
    fail_after = 1;
    ok = up_put(3, 0) == 2;
    fail_after = -1;
    bad += check("failed preset erasure retains both the sound and its favorite", ok && up_used(3) &&
                  favorite_has(NENGINES, 3) && !memcmp(&up_bank[0], &bank, sizeof bank));
    memset(&up_bank[0], 0, sizeof up_bank[0]);
    up_boot();
    bad += check("failed preset save keeps old flash", !memcmp(&up_bank[0], &bank, sizeof bank));
    trk[0].user = 4;
    erase_error = 1;
    ok = up_put(3, 0) == 2;
    erase_error = 0;
    bad += check("failed preset erase keeps record and loaded label", ok && trk[0].user == 4u && up_gen == before &&
                  !memcmp(&up_bank[0], &bank, sizeof bank));
    bad += check("successful preset erase clears loaded label", up_put(3, 0) == 0 && !up_used(3) && !trk[0].user);
    before = erases;
    transport_req = 1;
    bad += check("pending PLAY: preset save blocked", up_put(3, &r) == 2 && erases == before && !up_used(3));
    transport_req = 0;
    chain.armed = 1;
    bad += check("pending SONG: preset save blocked", up_put(3, &r) == 2 && erases == before && !up_used(3));
    chain.armed = 0;
    bad += check("preset invalid slot blocked before access", up_put(UP_SLOTS, &r) == 1 && erases == before);
    song.playing = 0; transport_req = 1; irq_start_race = 1;
    bad += check("PLAY consumed before the save snapshot still blocks a project write",
                  project_save(1) != 0 && irq_races == 1u && song.playing && !transport_req && erases == before);
    song.playing = 0; transport_req = 1; irq_start_race = 1;
    bad += check("PLAY consumed before the preset snapshot still blocks a bank write",
                  up_put(3, &r) == 2 && irq_races == 2u && song.playing && !transport_req && erases == before);
    song.playing = 0; transport_req = 1; irq_start_race = 1;
    settings.palette = 4;
    settings_save();
    bad += check("PLAY consumed before the settings snapshot defers the write",
                  irq_races == 3u && song.playing && !transport_req && persist_pending && erases == before);
    bad += mig_test();
    bad += autosave_test();
    bad += tilt_autosave_test();                     /* (TILT-FM) */
    printf("%s\n", bad ? "PERSISTENCE TEST FAILED" : "persistence test passed");
    bad += scenes_round2();
    return bad != 0;
}
