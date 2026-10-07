// `src/LFW/animation/*` 的 TS 侧台面，op 与 `subjects/animation.cpp` 一一对应。
// 用例：`cases/animation/*.txt`。
//
// 数值观察一律打**量化位**（`qBits`，1e-3 粒度）：`cos`/`sin`/`tan` 在 V8 与 UCRT 上
// 可能差 ulp，量化后结构性差异照样可见。**NaN 一律打 `nan`**（JS 只有一个 NaN 值，
// payload 不可观察）。布尔走 `renderValue` 的 `b1`/`b0`，空序列的 `curr` 打 `u`。
import { Animation } from "../../../../src/LFW/animation/Animation";
import { Cosine } from "../../../../src/LFW/animation/Cosine";
import { Delay } from "../../../../src/LFW/animation/Delay";
import { Easing } from "../../../../src/LFW/animation/Easing";
import { Loop } from "../../../../src/LFW/animation/Loop";
import { Sequence } from "../../../../src/LFW/animation/Sequence";
import { Sine } from "../../../../src/LFW/animation/Sine";
import { Tangent } from "../../../../src/LFW/animation/Tangent";
import { ease_in_out_quint } from "../../../../src/LFW/utils/easing/ease_in_out_quint";
import { ease_in_out_sine } from "../../../../src/LFW/utils/easing/ease_in_out_sine";
import { ease_linearity } from "../../../../src/LFW/utils/easing/ease_linearity";

import { qBits, readCaseLines, renderValue, splitWs } from "./trace_util";

interface Item {
  kind: string;
  obj: any;
}

const items = new Map<string, Item>();
const log: string[] = [];

function fail(msg: string): never {
  process.stderr.write(msg + "\n");
  process.exit(2);
}

function entry(id: string): Item {
  const it = items.get(id);
  if (it === undefined) fail(`unknown item '${id}'`);
  return it;
}

/** 量化位；NaN 一律 `nan`（payload 在 JS 里不可观察）。 */
function qb(v: number): string {
  return Number.isNaN(v) ? "nan" : qBits(v);
}

function animOf(id: string): any {
  return entry(id).obj;
}

function isPeriodic(kind: string): boolean {
  return kind === "sine" || kind === "cosine" || kind === "tangent";
}

function emitGet(id: string, prop: string): void {
  const it = entry(id);
  const o = it.obj;
  let payload: string;
  if (it.kind === "loop") {
    if (prop === "count") payload = qb(o.count);
    else if (prop === "times") payload = qb(o.times);
    else if (prop === "done") payload = renderValue(o.done());
    else return fail(`unknown loop prop '${prop}'`);
  } else if (prop === "value") payload = qb(o.value);
  else if (prop === "time") payload = qb(o.time);
  else if (prop === "duration") payload = qb(o.duration);
  else if (prop === "direction") payload = qb(o.direction);
  else if (prop === "fill_mode") payload = qb(o.fill_mode);
  else if (prop === "reverse") payload = renderValue(o.reverse);
  else if (prop === "count") payload = qb(o.count);
  else if (prop === "times") payload = qb(o.times);
  else if (prop === "done") payload = renderValue(o.done);
  else if (prop === "offset" || prop === "bottom" || prop === "height" || prop === "scale") {
    if (!isPeriodic(it.kind)) return fail(`unknown anim prop '${prop}'`);
    payload = qb(o[prop]);
  } else if (prop === "val_1" || prop === "val_2") {
    if (it.kind !== "easing") return fail(`unknown anim prop '${prop}'`);
    payload = qb(o[prop]);
  } else if (prop === "n") {
    if (it.kind !== "seq") return fail(`unknown anim prop '${prop}'`);
    payload = qb(o.anims.length);
  } else if (prop === "curr") {
    if (it.kind !== "seq") return fail(`unknown anim prop '${prop}'`);
    const cur = o.curr_anim;
    const idx = cur ? o.anims.indexOf(cur) : -1;
    payload = idx < 0 ? "u" : qb(idx);
  } else {
    return fail(`unknown anim prop '${prop}'`);
  }
  log.push(`get|${id}|${prop}|${payload}`);
}

function setProp(id: string, prop: string, tok: string): void {
  const it = entry(id);
  const o = it.obj;
  if (it.kind === "loop") {
    if (prop === "count") o.count = Number(tok);
    else if (prop === "times") o.times = Number(tok);
    else return fail(`unknown loop prop '${prop}'`);
    return;
  }
  if (prop === "duration") o.duration = Number(tok);
  else if (prop === "time") o.time = Number(tok);
  else if (prop === "value") o.value = Number(tok);
  else if (prop === "direction") o.direction = Number(tok);
  else if (prop === "fill_mode") o.fill_mode = Number(tok) as 0 | 1;
  else if (prop === "reverse") o.reverse = tok === "1" || tok === "true";
  else if (prop === "times") o.times = Number(tok);
  else if (prop === "count") o.count = Number(tok);
  else if (prop === "offset" || prop === "bottom" || prop === "height" || prop === "scale") {
    if (!isPeriodic(it.kind)) return fail(`unknown anim prop '${prop}'`);
    if (prop === "offset") o.set_offset(Number(tok));
    else if (prop === "bottom") o.bottom = Number(tok);
    else if (prop === "height") o.height = Number(tok);
    else o.scale = Number(tok);
  } else if (prop === "val_1" || prop === "val_2") {
    if (it.kind !== "easing") return fail(`unknown anim prop '${prop}'`);
    o[prop] = Number(tok);
  } else {
    return fail(`unknown anim prop '${prop}'`);
  }
}

function callOp(id: string, rest: string[]): void {
  const it = entry(id);
  const o = it.obj;
  const name = rest[0]!;
  if (it.kind === "loop") {
    if (name === "continue") o.continue();
    else if (name === "reset") o.reset();
    else if (name === "set") o.set(Number(rest[1]), Number(rest[2]));
    else return fail(`unknown loop call '${name}'`);
    return;
  }
  if (name === "start") o.start(rest.length > 1 ? rest[1] === "1" || rest[1] === "true" : undefined);
  else if (name === "end") o.end(rest.length > 1 ? rest[1] === "1" || rest[1] === "true" : undefined);
  else if (name === "calc") o.calc();
  else if (name === "update") o.update(Number(rest[1]));
  else if (name === "auto_trip") o.auto_trip(rest[1] === "1" || rest[1] === "true", Number(rest[2]));
  else if (name === "set") {
    // `Easing.set(begin, end)` / `Periodic.set(bottom, height, scale)`（后者的 set 会 calc）。
    if (it.kind === "easing") o.set(Number(rest[1]), Number(rest[2]));
    else if (isPeriodic(it.kind)) o.set(Number(rest[1]), Number(rest[2]), Number(rest[3]));
    else return fail(`unknown anim call '${name}'`);
  } else return fail(`unknown anim call '${name}'`);
}

function main(): void {
  const casePath = process.argv[2];
  if (!casePath) fail("usage: lfw_trace_animation.mjs <case-file>");

  for (const raw of readCaseLines(casePath)) {
    const t = splitWs(raw);
    if (t.length === 0) continue;
    const op = t[0]!;
    const i = [1];
    const next = (): string => t[i[0]!++]!;

    if (op === "mk") {
      const id = next();
      const kind = next();
      let obj: unknown;
      if (kind === "loop") obj = new Loop();
      else if (kind === "anim") obj = new Animation();
      else if (kind === "delay") obj = new Delay(Number(next()));
      else if (kind === "easing") {
        const begin = i[0]! < t.length ? Number(next()) : 0;
        const end = i[0]! < t.length ? Number(next()) : 1;
        obj = new Easing(begin, end);
      } else if (kind === "sine" || kind === "cosine" || kind === "tangent") {
        const bottom = i[0]! < t.length ? Number(next()) : 0;
        const height = i[0]! < t.length ? Number(next()) : 1;
        const scale = i[0]! < t.length ? Number(next()) : 1;
        if (kind === "sine") obj = new Sine(bottom, height, scale);
        else if (kind === "cosine") obj = new Cosine(bottom, height, scale);
        else obj = new Tangent(bottom, height, scale);
      } else if (kind === "seq") {
        const anims: Animation[] = [];
        while (i[0]! < t.length) anims.push(animOf(next()));
        obj = new Sequence(...anims);
      } else {
        return fail(`unknown mk kind '${kind}'`);
      }
      items.set(id, { kind, obj });
    } else if (op === "set") {
      setProp(next(), next(), next());
    } else if (op === "seteasing") {
      const id = next();
      const name = next();
      const o = animOf(id);
      if (name === "sine") o.set_easing(ease_in_out_sine);
      else if (name === "linearity") o.set_easing(ease_linearity);
      else if (name === "quint") o.set_easing(ease_in_out_quint);
      else return fail(`unknown easing '${name}'`);
    } else if (op === "call") {
      const id = next();
      callOp(id, t.slice(i[0]!));
    } else if (op === "seqpush") {
      const id = next();
      animOf(id).anims.push(animOf(next()));
    } else if (op === "get") {
      emitGet(next(), next());
    } else {
      return fail(`unknown op '${op}'`);
    }
  }

  for (const line of log) process.stdout.write(line + "\n");
}

main();
