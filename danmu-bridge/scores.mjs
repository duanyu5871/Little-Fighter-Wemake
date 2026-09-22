import { existsSync, readFileSync, writeFileSync } from "node:fs";

const num = (v) => (typeof v === "number" && Number.isFinite(v) ? v : 0);
const zero = () => ({ spawns: 0, kills: 0, deads: 0, damages: 0, cheers: 0 });

export function make_scoreboard({ file = "", weights = {} } = {}) {
  const w = { kills: 10, spawns: 1, cheers: 1, deads: 0, damages: 0, ...weights };
  const players = new Map();
  const last = new Map();
  let updated_at = 0;
  let dirty = false;

  if (file && existsSync(file)) {
    try {
      const raw = JSON.parse(readFileSync(file, "utf8"));
      updated_at = num(raw?.updated_at);
      for (const [uid, p] of Object.entries(raw?.players ?? {})) players.set(uid, normalize(p));
      for (const [uid, p] of Object.entries(raw?.last ?? {})) last.set(uid, normalize(p));
    } catch {
      void 0;
    }
  }

  function normalize(p) {
    return { name: "", games: 0, last_seen: 0, ...zero(), ...p };
  }
  function score_of(p) {
    return p.kills * w.kills + p.spawns * w.spawns + p.cheers * w.cheers + p.deads * w.deads + p.damages * w.damages;
  }
  function merge(stats) {
    let count = 0;
    for (const s of stats ?? []) {
      const uid = String(s?.uid ?? "");
      if (!uid) continue;
      const cur = { spawns: num(s.spawns), kills: num(s.kills), deads: num(s.deads), damages: num(s.damages), cheers: num(s.cheers) };
      const prev = last.get(uid);
      let p = players.get(uid);
      if (!p) {
        p = normalize({});
        p.games = 1;
        players.set(uid, p);
      }
      const reset = !!prev && (
        cur.spawns < prev.spawns || cur.kills < prev.kills || cur.deads < prev.deads ||
        cur.damages < prev.damages || cur.cheers < prev.cheers
      );
      if (reset) p.games += 1;
      const base = reset || !prev ? zero() : prev;
      for (const k of ["spawns", "kills", "deads", "damages", "cheers"])
        p[k] += Math.max(0, cur[k] - base[k]);
      if (s.name) p.name = String(s.name);
      p.last_seen = Date.now();
      last.set(uid, cur);
      dirty = true;
      ++count;
    }
    return count;
  }
  function top(n = 100) {
    return [...players.entries()]
      .map(([uid, p]) => ({ uid, ...p, score: score_of(p) }))
      .sort((a, b) => b.score - a.score || b.kills - a.kills || String(a.name).localeCompare(String(b.name)))
      .slice(0, n);
  }
  function save() {
    if (!dirty || !file) return false;
    updated_at = Date.now();
    writeFileSync(file, JSON.stringify({
      updated_at,
      players: Object.fromEntries(players),
      last: Object.fromEntries(last),
    }, null, 2));
    dirty = false;
    return true;
  }
  return { merge, save, top, size: () => players.size, get updated_at() { return updated_at; } };
}
