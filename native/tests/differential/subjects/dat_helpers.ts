import { __JSON5 } from "../../../../src/DittoImpl/JSON5";
import { copy_bdy_info } from "../../../../src/LFW/dat_translator/copy_bdy_info";
import { copy_itr_info } from "../../../../src/LFW/dat_translator/copy_itr_info";
import { find_float } from "../../../../src/LFW/dat_translator/find_float";
import { fixed_float } from "../../../../src/LFW/dat_translator/fixed_float";
import { set_bdy_kind } from "../../../../src/LFW/dat_translator/set_bdy_kind";
import { set_hit_flag } from "../../../../src/LFW/dat_translator/set_hit_flag";
import { take, take_str } from "../../../../src/LFW/dat_translator/take";
import { take_not_zero_num } from "../../../../src/LFW/dat_translator/take_not_zero_num";
import { take_num } from "../../../../src/LFW/dat_translator/take_num";
import { take_positive_num } from "../../../../src/LFW/dat_translator/take_positive_num";
import { take_raw_frame_mp } from "../../../../src/LFW/dat_translator/take_raw_frame_mp";
import { Ditto } from "../../../../src/LFW/ditto/Instance";

import { parseValue, readCaseLines, renderValue, splitWs } from "./trace_util";

Ditto.JSON5 = __JSON5;

const out: string[] = [];
const objs = new Map<string, Record<string, unknown>>();

function objOf(id: string): Record<string, unknown> {
  let o = objs.get(id);
  if (o === undefined) {
    o = {};
    objs.set(id, o);
  }
  return o;
}

function render(v: unknown): string {
  return renderValue(v);
}

function optRender(v: unknown): string {
  return v === undefined ? "u" : render(v);
}

function valueOf(t: string[], i: number[]): unknown {
  const tok = t[i[0]!]!;
  if (tok === "nan") {
    i[0]!++;
    return NaN;
  }
  if (tok === "inf") {
    i[0]!++;
    return Infinity;
  }
  if (tok === "ninf") {
    i[0]!++;
    return -Infinity;
  }
  return parseValue(t, i);
}

function main(): void {
  const casePath = process.argv[2];
  if (!casePath) {
    process.stderr.write("usage: lfw_trace_dat_helpers.mjs <case-file>\n");
    process.exit(2);
  }

  for (const raw of readCaseLines(casePath)) {
    const t = splitWs(raw);
    if (t.length === 0) continue;
    const op = t[0]!;
    const i = [1];
    const next = (): string => t[i[0]!++]!;

    if (op === "new") {
      objs.set(next(), {});
    } else if (op === "set") {
      const id = next();
      const key = next();
      objOf(id)[key] = valueOf(t, i);
    } else if (op === "setobj") {
      const id = next();
      const key = next();
      objOf(id)[key] = objs.get(next()) ?? {};
    } else if (op === "setarr") {
      const id = next();
      const key = next();
      const arr: unknown[] = [];
      while (i[0]! < t.length) arr.push(valueOf(t, i));
      objOf(id)[key] = arr;
    } else if (op === "del") {
      delete objOf(next())[next()];
    } else if (op === "dump") {
      const id = next();
      out.push(`D ${id} ${render(objOf(id))}`);
    } else if (op === "take") {
      const id = next();
      const key = next();
      out.push(`T ${id} ${key} ${render(take(objOf(id), key))}`);
    } else if (op === "takestr") {
      const id = next();
      const key = next();
      out.push(`TS ${id} ${key} ${optRender(take_str(objOf(id), key))}`);
    } else if (op === "takenum" || op === "takepos" || op === "takenonzero") {
      const id = next();
      const key = next();
      const o = objOf(id);
      const r =
        op === "takenum" ? take_num(o, key) : op === "takepos" ? take_positive_num(o, key) : take_not_zero_num(o, key);
      out.push(`${op === "takenum" ? "TN" : op === "takepos" ? "TP" : "TZ"} ${id} ${key} ${optRender(r)}`);
    } else if (op === "takenumf") {
      const id = next();
      const key = next();
      const mul = Number(next());
      out.push(`TNF ${id} ${key} ${optRender(take_num(objOf(id), key, (n) => n * mul))}`);
    } else if (op === "mp") {
      const id = next();
      const r = take_raw_frame_mp(objOf(id));
      out.push(`MP ${id} ${render(r.mp)} ${render(r.hp)}`);
    } else if (op === "flag") {
      const id = next();
      const v = valueOf(t, i);
      set_hit_flag(objOf(id), v as never);
      out.push(`SG ${id} ${render(objs.get(id))}`);
    } else if (op === "bdykind") {
      const id = next();
      const v = valueOf(t, i);
      set_bdy_kind(objOf(id), v as never);
      out.push(`SB ${id} ${render(objs.get(id))}`);
    } else if (op === "ff") {
      const nv = valueOf(t, i);
      const n = typeof nv === "number" ? nv : 0;
      const d = i[0]! < t.length ? Number(next()) : undefined;
      out.push(`F ${render(d === undefined ? fixed_float(n) : fixed_float(n, d))}`);
    } else if (op === "find") {
      const id = next();
      const r = find_float(objs.get(id));
      out.push(`R ${id} ${render(r[0])} ${render(r[1])}`);
    } else if (op === "copybdy" || op === "copyitr") {
      const id = next();
      const r = op === "copybdy" ? copy_bdy_info(objOf(id), {}) : copy_itr_info(objOf(id), {});
      out.push(`${op === "copybdy" ? "CB" : "CI"} ${id} ${render(r)}`);
    } else if (op === "copybdye" || op === "copyitre") {
      const id = next();
      const key = next();
      const edit: Record<string, unknown> = {};
      edit[key] = valueOf(t, i);
      const r = op === "copybdye" ? copy_bdy_info(objOf(id), edit) : copy_itr_info(objOf(id), edit);
      out.push(`${op === "copybdye" ? "CBE" : "CIE"} ${id} ${render(r)}`);
    } else {
      process.stderr.write(`unknown op '${op}'\n`);
      process.exit(2);
    }
  }

  process.stdout.write(out.join("\n") + (out.length ? "\n" : ""));
}

main();
