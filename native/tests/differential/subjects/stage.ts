// `stage/Expressions` + `stage/Status` 与 `bg/Background` + `bg/Layer` 的 TS 侧台面。
//
// 用例：`cases/stage/expr.txt`（Expressions + Status）、`cases/stage/bg.txt`（Background + Layer）。op：
//   it <b…>     追加一个假表达式（`run` 按脚本吐真假值，跑完最后一个就一直重复）
//   arg <v> | run | flow | next | resetsame | resetcopy | expdump | status
//   data <v> | new | bgdump | layer <i> | upd | disp | lset <i> <字段> <值>
//
// `resetsame` 传的是 `exp.list`（**内部那个数组**）⇒ 走 `reset` 的同一性早退；
// `resetcopy` 传一份内容相同的新数组 ⇒ 走「清空再灌」。两边都实现成 `reset(list)`。
import { Background } from "../../../../src/LFW/bg/Background";
import type { Layer } from "../../../../src/LFW/bg/Layer";
import { Expressions } from "../../../../src/LFW/stage/Expressions";
import { Status } from "../../../../src/LFW/stage/Status";

import { numHex, parseValue, readCaseLines, renderValue, splitWs } from "./trace_util";

const log: string[] = [];

function fail(msg: string): never {
  process.stderr.write(msg + "\n");
  process.exit(2);
}

function num(d: number): string {
  return numHex(d);
}

function vstr(v: unknown): string {
  return renderValue(v);
}

function flag(b: boolean): string {
  return b ? "1" : "0";
}

function number_of(t: string[], i: number[], op: string): number {
  const v = parseValue(t, i);
  if (typeof v !== "number") fail(`${op} expects a number literal`);
  return v;
}

// 假表达式：`run` 按脚本吐真假值（跑完最后一个就一直重复），并把每次调用记进日志。
class FakeExpr {
  readonly index: number;
  readonly script: boolean[];
  cursor = 0;
  constructor(index: number, script: boolean[]) {
    this.index = index;
    this.script = script;
  }
  run(arg: unknown): boolean {
    log.push(`call:${this.index}:arg=${vstr(arg)}`);
    if (this.script.length === 0) return false;
    const v = this.script[Math.min(this.cursor, this.script.length - 1)]!;
    if (this.cursor < this.script.length - 1) this.cursor++;
    return v;
  }
}

const exp = new Expressions<unknown>();
const items: FakeExpr[] = [];
let arg: unknown = undefined;

function dump_expr(): void {
  const index = (exp as unknown as { _index: number })._index;
  log.push(
    `dump|n=${exp.list.length}` +
      `|i=${num(index)}` +
      `|first=${flag(exp.is_first)}` +
      `|last=${flag(exp.is_last)}`,
  );
}

function dump_status(): void {
  log.push(`status|${Status.Running}|${Status.Completed}|${Status.End}|3`);
}

let data: { layers: Record<string, unknown>[] };
let bg: Background | undefined;

function dump_bg(): void {
  if (!bg) {
    log.push("bg|none");
    return;
  }
  const priv = bg as unknown as { _update_times: number; _layer_data_index: number };
  log.push(
    `bg|id=${vstr(bg.id)}` +
      `|name=${vstr(bg.name)}` +
      `|left=${num(bg.left)}` +
      `|right=${num(bg.right)}` +
      `|near=${num(bg.near)}` +
      `|far=${num(bg.far)}` +
      `|width=${num(bg.width)}` +
      `|height=${num(bg.height)}` +
      `|depth=${num(bg.depth)}` +
      `|mid=${num(bg.middle.x)},${num(bg.middle.z)}` +
      `|zoom=${num(bg.zoom_x)},${num(bg.zoom_y)},${num(bg.zoom_z)}` +
      `|n=${bg.layers.length}` +
      `|ut=${num(priv._update_times)}` +
      `|di=${num(priv._layer_data_index)}`,
  );
}

function dump_layer(index: number): void {
  if (!bg) {
    log.push("layer|none");
    return;
  }
  if (index < 0 || index >= bg.layers.length) {
    log.push("layer|oob");
    return;
  }
  const l = bg.layers[index] as Layer;
  log.push(
    `layer|di=${num(l.data_index)}` +
      `|li=${num(l.loop_index)}` +
      `|x=${num(Number(l.info.x))}` +
      `|y=${num(Number(l.info.y))}` +
      `|file=${vstr(l.info.file)}` +
      `|vis=${flag(l.visible)}` +
      `|st=${flag(l.is_static())}`,
  );
}

function main(): void {
  const casePath = process.argv[2];
  if (!casePath) fail("usage: lfw_trace_stage.mjs <case-file>");

  for (const raw of readCaseLines(casePath)) {
    const t = splitWs(raw);
    if (t.length === 0) continue;
    const op = t[0]!;
    const i = [1];

    if (op === "it") {
      const script: boolean[] = [];
      while (i[0]! < t.length) {
        const v = parseValue(t, i);
        if (typeof v !== "boolean") fail("it expects boolean literals");
        script.push(v);
      }
      items.push(new FakeExpr(items.length, script));
    } else if (op === "arg") {
      arg = parseValue(t, i);
    } else if (op === "run") {
      log.push(`run=${flag(exp.run(arg))}`);
    } else if (op === "flow") {
      log.push(`flow=${flag(exp.flow(arg))}`);
    } else if (op === "next") {
      exp.next();
    } else if (op === "resetsame") {
      exp.reset((exp as unknown as { list: unknown[] }).list as never);
    } else if (op === "resetcopy") {
      exp.reset([...items] as never);
    } else if (op === "expdump") {
      dump_expr();
    } else if (op === "status") {
      dump_status();
    } else if (op === "data") {
      data = parseValue(t, i) as never;
    } else if (op === "new") {
      // `world` 只被存起来、构造里不读 ⇒ 假对象够用。
      bg = new Background({} as never, data as never);
    } else if (op === "bgdump") {
      dump_bg();
    } else if (op === "layer") {
      dump_layer(number_of(t, i, op));
    } else if (op === "upd") {
      bg?.update();
    } else if (op === "disp") {
      bg?.dispose();
    } else if (op === "lset") {
      const index = number_of(t, i, op);
      const field = t[i[0]!++]!;
      const value = parseValue(t, i);
      data.layers[index]![field] = value;
    } else {
      fail(`unknown op '${op}'`);
    }

    if (i[0] !== t.length) fail(`trailing token(s): ${raw}`);
    while (log.length) process.stdout.write(log.shift() + "\n");
  }
}

main();
