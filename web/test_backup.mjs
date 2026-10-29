// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
// fm1backup.js against a simulated device: 7-bit packing, CRC, manifest checks, a full capture,
// validation before the first write, restore order (live music last) and abort on a failed chunk.
// Format v2 (8 scenes x 3 songs: 33 objects) and format v1 (13/12/11 objects: older devices and archives).
import { BACKUP_IDS, LEGACY_IDS, BACKUP_CMD, bkU32, bkR32, bkPack, bkUnpack, bkCrc, bkManifest, readBackup, captureBackup, restoreBackup } from "./fm1backup.js";

let fails = 0;
const ok = (c, what) => { console.log(`${what.padEnd(72)} ${c ? "ok" : "FAIL"}`); if (!c) fails++; };
const throws = (f) => { try { f(); return false; } catch { return true; } };
const athrows = async (f) => { try { await f(); return false; } catch { return true; } };
const msgOf = async (f) => { try { await f(); return ""; } catch (e) { return String(e.message); } };

const rnd = (n, seed) => Uint8Array.from({ length: n }, (_, i) => (i * 131 + seed * 17 + (i >> 3)) & 255);
const clone = (x) => JSON.parse(JSON.stringify(x));
const same = (a, b) => a && b && a.length === b.length && a.every((v, i) => v === b[i]);
ok(bkR32(bkU32(0xdeadbeef)) === 0xdeadbeef && bkCrc(new TextEncoder().encode("123456789")) === 0xcbf43926, "backup: numbers and CRC-32");
const b = rnd(1000, 1);
ok(bkUnpack(bkPack(b), b.length).every((v, i) => v === b[i]), "backup: 7-bit pack / unpack round trip");
ok(throws(() => bkUnpack([...bkPack(b.subarray(0, 14)), 0], 14)), "backup: trailing bytes refused");

/* a user sample object as the old firmware backs it up: 512 B header (PMSF) + 4-bit payload */
function sampleObj(seed) {
  const payload = rnd(100, seed), out = new Uint8Array(512 + 100), v = new DataView(out.buffer);
  v.setUint32(0, 0x504d5346, true); v.setUint16(4, 1, true); out[6] = 1;
  v.setUint32(16, 100, true); v.setUint32(20, bkCrc(payload), true);
  v.setUint32(32, 0, true); v.setUint32(36, 100, true); v.setUint32(40, 0, true); v.setUint32(44, 50, true); v.setUint32(48, 22050, true);
  out[56] = 60; out[57] = 0; out[58] = 127; out.set(payload, 512);
  return out;
}
/* a song record: the SONG magic, then anything */
const songRec = (seed) => { const r = rnd(900, seed); r.set([0x53, 0x4f, 0x4e, 0x47]); return r; };

/* a device: objects by id, the order of writes. opt.legacy = an older firmware (13 objects, ids 2..5, samples 11..14) */
function device(objs, opt = {}) {
  const d = { objs: new Map(objs), log: [], staged: null };
  const listIds = opt.listIds || (opt.legacy ? LEGACY_IDS : BACKUP_IDS);
  const takes = (id) => (opt.legacy ? id <= (opt.maxId ?? 9) : BACKUP_IDS.includes(id) && id <= (opt.maxId ?? 255));
  d.request = async ([cmd, a]) => {
    if (cmd === BACKUP_CMD.LIST) {
      const out = [1, 0, listIds.length];
      for (const id of listIds) { const v = d.objs.get(id) || new Uint8Array(0); out.push(id, ...bkU32(v.length), ...bkU32(v.length ? bkCrc(v) : 0)); }
      return out;
    }
    if (cmd === BACKUP_CMD.GET) {
      const id = a[0], off = bkR32(a, 1), n = a[6] | a[7] << 7, v = d.objs.get(id);
      if (opt.changeOnGet === id && off === 0) v[0] ^= 1;
      return [id, 0, ...bkU32(off), n & 127, n >> 7, ...bkPack(v.subarray(off, off + n))];
    }
    if (cmd === BACKUP_CMD.PUT) {
      const [op, id] = a;
      if (op === 0 && !takes(id)) { d.log.push(`refused ${id}`); return [op, id, 1]; }   // (an older firmware, or an id that is not an object)
      if (op === 0) { d.staged = { id, size: bkR32(a, 2), crc: bkR32(a, 7), bytes: [] }; return [op, id, 0]; }
      if (op === 1) {
        if (opt.failChunk === id) return [op, id, 2];
        const off = bkR32(a, 2), p = a.slice(7), n = Math.min(256, d.staged.size - off);
        d.staged.bytes.push(...bkUnpack(p, n)); return [op, id, 0];
      }
      if (op === 2) { const s = Uint8Array.from(d.staged.bytes); const rc = bkCrc(s) === d.staged.crc ? 0 : 2; if (!rc) { d.objs.set(id, s); d.log.push(id); } return [op, id, rc]; }
      if (op === 3) { d.log.push(`abort ${id}`); return [op, id, 0]; }
    }
    if (opt.legacy && (cmd === 11 || cmd === 12 || cmd === 13 || cmd === 14)) { if (cmd === 13 || cmd === 14) d.log.push(32 + a[0]); return [a[0], 0]; }
    throw new Error(`unexpected ${cmd}`);   // (the 8 x 3 firmware refuses the sample commands: no reply)
  };
  return d;
}
const writes = (d) => d.log.filter((x) => typeof x === "number");

/* ---- format v2: the 8 x 3 firmware ---- */
const live = rnd(3840, 2), settings = rnd(1200, 3), presetA = rnd(3080, 5);
const scenes = new Map([[64, rnd(3840, 11)], [66, rnd(3840, 12)], [71, rnd(3840, 13)], [80, rnd(3840, 14)], [87, rnd(3840, 15)]]);
const songs = new Map([[96, songRec(21)], [98, songRec(22)]]);
const objs2 = [[0, live], [1, settings], [6, presetA], ...scenes, ...songs];
const dev2 = device(objs2);
const file2 = await captureBackup(dev2.request, "TFM");
ok(file2.version === 2 && file2.objects.length === 33 && file2.objects.every((o, i) => o.id === BACKUP_IDS[i]), "v2: capture from the 8 x 3 firmware: version 2, the 33 objects in order");
ok(BACKUP_IDS.length === 33 && BACKUP_IDS.slice(0, 6).join() === "0,1,6,7,8,9" && BACKUP_IDS[6] === 64 && BACKUP_IDS[29] === 87 && BACKUP_IDS.slice(30).join() === "96,97,98",
  "v2: the ids: 0 1 6 7 8 9, scenes 64..87, songs 96..98");
ok(file2.objects.find((o) => o.id === 65).size === 0 && file2.objects.find((o) => o.id === 97).size === 0 && file2.objects.find((o) => o.id === 8).size === 0, "v2: empty scenes, songs and the retired bank are listed as size 0");
const read2 = readBackup(JSON.stringify(file2));
ok([...objs2].every(([id, v]) => same(read2.objects.find((o) => o.id === id).bytes, v)), "v2: capture -> file -> readBackup: every object equal");
ok(await athrows(() => captureBackup(device(objs2.map(([i, v]) => [i, v.slice()]), { changeOnGet: 64 }).request, "TFM")), "v2: a device that changes during capture fails the capture");

const target2 = device([]);
const res2 = await restoreBackup(target2.request, JSON.parse(JSON.stringify(file2)));
ok([...objs2].every(([id, v]) => same(target2.objs.get(id), v)) && res2.skippedSamples === 0 && res2.objects.length === 33, "v2: restore onto a fresh device: every object equal, nothing skipped");
const w2 = writes(target2);
ok(w2.at(-1) === 0 && w2.at(-2) === 1 && w2.indexOf(6) < w2.indexOf(64) && w2.indexOf(87) < w2.indexOf(96) && w2.indexOf(98) < w2.indexOf(1), "v2: restore order: presets, scenes, songs, settings, live music last");
ok(w2.length === 33 && !target2.log.some((x) => typeof x === "string" && x.startsWith("refused")), "v2: all 33 objects written (empty ones too, to clear them)");
{   /* progress ends at the total */
  let last = 0, tot = 0; await restoreBackup(device([]).request, file2, (d, t) => { last = d; tot = t; });
  ok(last === tot && tot === file2.objects.reduce((n, o) => n + o.size, 0), "v2: restore progress reaches the total");
}

/* corrupted v2 files: refused before the first write */
{
  const cases = {
    "a bad CRC": (f) => { f.objects[10].crc ^= 1; },
    "ids out of order": (f) => { [f.objects[7], f.objects[8]] = [f.objects[8], f.objects[7]]; },
    "a wrong count (32 objects)": (f) => { f.objects.pop(); },
    "a wrong count (34 objects)": (f) => { f.objects.push({ ...f.objects.at(-1) }); },
    "a bad song magic": (f) => { const o = f.objects.find((x) => x.id === 96), by = Uint8Array.from(atob(o.data), (c) => c.charCodeAt(0)); by[0] ^= 1; o.data = btoa(String.fromCharCode(...by)); o.crc = bkCrc(by); },
    "an oversize object": (f) => { const o = f.objects.find((x) => x.id === 64), by = new Uint8Array(3841); o.size = 3841; o.data = btoa(String.fromCharCode(...by)); o.crc = bkCrc(by); },
    "a missing live music": (f) => { f.objects[0] = { ...f.objects[0], size: 0, crc: 0, data: "" }; },
    "the version 1 label on 33 objects": (f) => { f.version = 1; },
    "the version 2 label on 13 objects": (f) => { f.objects = f.objects.filter((o) => LEGACY_IDS.includes(o.id)); },
  };
  for (const [name, mutate] of Object.entries(cases)) {
    const f = clone(file2); mutate(f); const t = device([]);
    ok(throws(() => readBackup(f)) && await athrows(() => restoreBackup(t.request, f)) && t.log.length === 0, `v2: ${name}: refused before the first write`);
  }
}
{   /* the older firmware refuses id 64 at begin */
  const t0 = device([], { legacy: true });
  const msg = await msgOf(() => restoreBackup(t0.request, file2));
  ok(/8 x 3/.test(msg) && writes(t0).length === 0 && t0.objs.size === 0, "v2: onto an older device: fails with the 8 x 3 message and writes nothing");
  const t1 = device([], { maxId: 70 });
  ok(/8 x 3/.test(await msgOf(() => restoreBackup(t1.request, file2))), "v2: a device that refuses a later scene id also says 8 x 3");
}
{   /* a failed chunk aborts the object and stops before the live music */
  const failing = device([], { failChunk: 64 });
  ok(await athrows(() => restoreBackup(failing.request, file2)) && failing.log.includes("abort 64") && !failing.log.includes(0), "v2: a refused chunk aborts that object and stops before the live music");
}

/* ---- format v1: older devices and archives ---- */
const objs1 = [[0, rnd(3388, 2)], [1, rnd(1200, 3)], [2, rnd(3388, 4)], [3, rnd(3388, 6)], [4, rnd(3388, 7)], [6, rnd(3080, 5)], [32, sampleObj(9)]];
const old = device(objs1, { legacy: true });
const file1 = await captureBackup(old.request, "TEST");
ok(file1.version === 1 && file1.objects.length === LEGACY_IDS.length && file1.objects[0].size === 3388 && file1.objects[5].size === 0, "v1: capture from an old device still gives version 1 (13 objects, empty ones as 0)");
ok(readBackup(JSON.stringify(file1)).objects[2].bytes.every((v, i) => v === objs1[2][1][i]), "v1: capture -> file -> bytes round trip");
ok(await athrows(() => captureBackup(device(objs1.map(([i, v]) => [i, v.slice()]), { legacy: true, changeOnGet: 2 }).request, "TEST")), "v1: a device that changes during capture fails the capture");
{
  const bad = clone(file1); bad.objects[2].crc ^= 1;
  ok(throws(() => readBackup(bad)), "v1: a damaged file is refused");
  const noRun = clone(file1); noRun.objects[0] = { ...noRun.objects[0], size: 0, crc: 0, data: "" };
  ok(throws(() => readBackup(noRun)), "v1: a file without the current music is refused");
  ok(throws(() => bkManifest([1, 0, 3])), "backup: a short manifest is refused");
  const badSmp = clone(file1); const o = badSmp.objects.find((x) => x.id === 32), by = Uint8Array.from(atob(o.data), (c) => c.charCodeAt(0)); by[0] ^= 1;
  o.data = btoa(String.fromCharCode(...by)); o.crc = bkCrc(by);
  ok(throws(() => readBackup(badSmp)), "v1: an invalid sample object is refused (sample validation kept)");
}
{   /* 13 / 12 / 11 objects still read */
  ok(readBackup(file1).objects.length === 13 && file1.objects.some((o) => o.id === 8) && file1.objects.some((o) => o.id === 9),
    "v1: 13 objects: id 8 (the retired FM6 bank, empty) and id 9 (the user presets' FM6 patches)");
  const v12 = clone(file1); v12.objects = v12.objects.filter((o) => o.id !== 9);
  ok(readBackup(v12).objects.length === 12, "v1: an archive of 1.0..1.0.2 (12 objects, the bank as id 8) still reads");
  const o11 = clone(file1); o11.objects = o11.objects.filter((o) => o.id !== 8 && o.id !== 9);
  ok(readBackup(o11).objects.length === 11, "v1: an archive of the 11 objects before FM6 still reads");
  const odd = clone(file1); odd.objects = odd.objects.filter((o) => o.id !== 33);
  ok(throws(() => readBackup(odd)), "v1: an archive missing another object is refused");
  const dev12 = await captureBackup(async ([cmd, a]) => {   // (a 12-object device)
    const r = await old.request([cmd, a]); if (cmd !== BACKUP_CMD.LIST) return r;
    const keep = []; for (let i = 0; i < r[2]; i++) if (r[3 + i * 11] !== 9) keep.push(...r.slice(3 + i * 11, 14 + i * 11));
    return [1, 0, keep.length / 11, ...keep];
  }, "1.0.2");
  ok(dev12.version === 1 && dev12.objects.length === 12, "v1: capture from a 12-object device gives version 1");
  const v2in1 = clone(file1); v2in1.version = 2;
  ok(throws(() => readBackup(v2in1)), "v1: 13 objects labelled version 2 are refused");
}

/* a v1 file onto the 8 x 3 firmware: slots 2..5 -> scenes A..D of song 1, samples skipped, song 1 cleared */
{
  const tNew = device([[96, songRec(40)], [97, songRec(41)], [64, rnd(3840, 42)]]);   // a device already holding a song-1 record and a scene
  const res = await restoreBackup(tNew.request, JSON.parse(JSON.stringify(file1)));
  const w = writes(tNew);
  ok(same(tNew.objs.get(64), objs1[2][1]) && same(tNew.objs.get(65), objs1[3][1]) && same(tNew.objs.get(66), objs1[4][1]) && tNew.objs.get(67).length === 0,
    "v1 -> 8x3: project slots 2..5 are written as scenes 64..67 (A..D of song 1; an empty slot clears)");
  ok(tNew.objs.get(96).length === 0 && same(tNew.objs.get(97), songRec(41)) && w.includes(96) && w.indexOf(96) < w.indexOf(0) && w.indexOf(96) > w.indexOf(67),
    "v1 -> 8x3: song 1's record is cleared (after the scenes), the other songs untouched");
  ok(res.skippedSamples === 1 && !tNew.objs.has(32) && !w.includes(32) && !w.includes(2) && !tNew.log.includes("refused 2"), "v1 -> 8x3: the sample is skipped and reported (skippedSamples 1); ids 2..5 and 32..34 are never sent");
  ok(w.at(-1) === 0 && w.at(-2) === 1 && same(tNew.objs.get(6), objs1[5][1]) && same(tNew.objs.get(0), objs1[0][1]), "v1 -> 8x3: presets, settings and the live music restored, live music last");
  let last = 0, tot = 0; await restoreBackup(device([]).request, file1, (d, t) => { last = d; tot = t; });
  ok(last === tot && tot === file1.objects.reduce((n, o) => n + o.size, 0), "v1 -> 8x3: progress counts the skipped bytes as done");
  const noSmp = clone(file1); const so = noSmp.objects.find((o) => o.id === 32); so.size = 0; so.data = ""; so.crc = 0;
  ok((await restoreBackup(device([]).request, noSmp)).skippedSamples === 0, "v1 -> 8x3: an archive without samples reports skippedSamples 0");
  const o11 = clone(file1); o11.objects = o11.objects.filter((o) => o.id !== 8 && o.id !== 9);
  const t11 = device([]); const r11 = await restoreBackup(t11.request, o11);
  ok(r11.skippedSamples === 1 && same(t11.objs.get(64), objs1[2][1]) && writes(t11).at(-1) === 0, "v1 (11 objects) -> 8x3: restores the same way");
  const damaged = clone(file1); damaged.objects[6].crc ^= 1;
  const tD = device([]);
  ok(await athrows(() => restoreBackup(tD.request, damaged)) && tD.log.length === 0, "v1 -> 8x3: a damaged file is refused before the first write");
}

/* a v1 file onto an OLD device: exactly as before (ids 2..5, samples via commands 11..14) */
{
  const target = device([], { legacy: true });
  const damaged = clone(file1); damaged.objects[6].crc ^= 1;
  ok(await athrows(() => restoreBackup(target.request, damaged)) && target.log.length === 0, "v1 -> old: restore validates every byte before the first write");
  const res = await restoreBackup(target.request, file1);
  const order = writes(target);
  ok(order.at(-1) === 0 && order.at(-2) === 1 && order.indexOf(2) < order.indexOf(1) && order.indexOf(2) < order.indexOf(6) && order.indexOf(6) < order.indexOf(32),
    "v1 -> old: restore order: projects, banks, samples, settings, live music last");
  ok([0, 1, 2, 3, 4, 6].every((id) => same(target.objs.get(id), objs1.find((o) => o[0] === id)[1])) && res.skippedSamples === 0 && !order.includes(64) && !order.includes(96),
    "v1 -> old: restored objects equal the source; ids 2..5 kept, no 64/96, nothing skipped");
  ok(target.log.filter((x) => typeof x === "string" && x.startsWith("refused")).join() === "refused 64", "v1 -> old: only the probe of id 64 is refused");
  const forced = device([], { legacy: true }); await restoreBackup(forced.request, file1, () => {}, { legacyDevice: true });
  ok(!forced.log.includes("refused 64") && same(writes(forced), order), "v1 -> old: the caller option skips the probe, same writes");
}
{   /* 1.0.3: an old archive with a bank restores its id 8 (after 6, 7); a new archive onto 1.0.2 skips id 9 */
  const bankBytes = rnd(3472, 8), patches = rnd(3728, 9);
  const v2dev = device([...objs1, [8, bankBytes]], { legacy: true });
  const v2file = await captureBackup(async ([cmd, a]) => {   // (a 1.0.2 device: 12 objects)
    const r = await v2dev.request([cmd, a]);
    if (cmd !== BACKUP_CMD.LIST) return r;
    const n = r[2], keep = [];
    for (let i = 0; i < n; i++) if (r[3 + i * 11] !== 9) keep.push(...r.slice(3 + i * 11, 14 + i * 11));
    return [1, 0, keep.length / 11, ...keep];
  }, "1.0.2");
  const to103 = device([], { legacy: true });
  await restoreBackup(to103.request, v2file);
  const lg = writes(to103);
  ok(v2file.objects.length === 12 && lg.indexOf(8) > lg.indexOf(7) && lg.indexOf(7) > lg.indexOf(6) &&
     to103.objs.get(8).every((v, i) => v === bankBytes[i]), "v1 -> old: a 1.0.2 archive (id 8, the bank) restores, its bank after the user presets");
  const newFile = await captureBackup(device([...objs1, [9, patches]], { legacy: true }).request, "1.0.3");
  const to102 = device([], { legacy: true, maxId: 8 });
  await restoreBackup(to102.request, newFile);
  ok(to102.log.includes("refused 9") && !to102.objs.has(9) && to102.log.at(-1) === 0,
    "v1 -> old: a 1.0.3 archive onto older firmware: id 9 skipped, the rest restored");
  const to103b = device([], { legacy: true });
  await restoreBackup(to103b.request, newFile);
  ok(to103b.objs.get(9).every((v, i) => v === patches[i]), "v1 -> old: a 1.0.3 archive restores id 9");
  const toNew = device([]); await restoreBackup(toNew.request, newFile);
  ok(same(toNew.objs.get(9), patches), "v1 -> 8x3: the user presets' FM6 patches (id 9) restore");
}
{
  const failing = device([], { legacy: true, failChunk: 2 });
  ok(await athrows(() => restoreBackup(failing.request, file1)) && failing.log.includes("abort 2") && !failing.log.includes(0), "v1 -> old: a refused chunk aborts that object and stops before the live music");
  const failingNew = device([], { failChunk: 65 });
  ok(await athrows(() => restoreBackup(failingNew.request, file1)) && failingNew.log.includes("abort 65") && !failingNew.log.includes(0), "v1 -> 8x3: a refused chunk aborts that scene and stops before the live music");
}

console.log(fails ? `BACKUP WEB TESTS FAILED (${fails})` : "backup web tests passed");
process.exit(fails ? 1 : 0);
