import { cook_bdy } from "../../../../src/LFW/dat_translator/cook_bdy";
import { cook_cpoint } from "../../../../src/LFW/dat_translator/cook_cpoint";
import { cook_itr } from "../../../../src/LFW/dat_translator/cook_itr";
import { cook_wpoint } from "../../../../src/LFW/dat_translator/cook_wpoint";
import { float_scaling_itr } from "../../../../src/LFW/dat_translator/float_scaling_itr";
import { get_next_frame_by_raw_id } from "../../../../src/LFW/dat_translator/get_the_next";

import { parseValue, readCaseLines, renderValue, splitWs } from "./trace_util";

const out: string[] = [];
const objs = new Map<string, Record<string, unknown>>();
const NO_FRAME = {} as never;

function render(v: unknown): string {
  return renderValue(v);
}

function objOf(id: string): Record<string, unknown> {
  let o = objs.get(id);
  if (o === undefined) {
    o = {};
    objs.set(id, o);
  }
  return o;
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
    process.stderr.write("usage: lfw_trace_cookers.mjs <case-file>\n");
    process.exit(2);
  }

  for (const raw of readCaseLines(casePath)) {
    const t = splitWs(raw);
    if (t.length === 0) continue;
    const op = t[0]!;
    const i = [1];
    const next = (): string => t[i[0]!++]!;

    if (op === "nf") {
      const id = valueOf(t, i);
      const zeroAs = next();
      out.push(`NF ${render(get_next_frame_by_raw_id(id as never, zeroAs as never))}`);
    } else if (op === "new") {
      objs.set(next(), {});
    } else if (op === "set") {
      const id = next();
      const key = next();
      objOf(id)[key] = valueOf(t, i);
    } else if (op === "setarr") {
      const id = next();
      const key = next();
      const arr: unknown[] = [];
      while (i[0]! < t.length) arr.push(valueOf(t, i));
      objOf(id)[key] = arr;
    } else if (op === "setobj") {
      const id = next();
      const key = next();
      objOf(id)[key] = objs.get(next()) ?? {};
    } else if (op === "dump") {
      const id = next();
      out.push(`D ${id} ${render(objs.get(id))}`);
    } else if (op === "bdy" || op === "wp" || op === "cp" || op === "itr" || op === "fsitr") {
      const id = next();
      const v = objOf(id);
      if (op === "bdy") cook_bdy(v as never, NO_FRAME);
      else if (op === "wp") cook_wpoint(v as never, NO_FRAME);
      else if (op === "cp") cook_cpoint(v as never, NO_FRAME);
      else if (op === "itr") cook_itr(v as never, NO_FRAME);
      else float_scaling_itr(v as never);
      const tag = op === "bdy" ? "CB" : op === "wp" ? "CW" : op === "cp" ? "CC" : op === "itr" ? "CI" : "FS";
      out.push(`${tag} ${id} ${render(v)}`);
    } else {
      process.stderr.write(`unknown op '${op}'\n`);
      process.exit(2);
    }
  }

  process.stdout.write(out.join("\n") + (out.length ? "\n" : ""));
}

main();
