import { cook_next_frame_cost } from "../../../../src/LFW/dat_translator/cook_next_frame_cost";
import { add_next_frame, edit_next_frame } from "../../../../src/LFW/dat_translator/edit_next_frame";
import { get_next_frame_by_raw_id } from "../../../../src/LFW/dat_translator/get_the_next";

import { parseValue, readCaseLines, renderValue, splitWs } from "./trace_util";

const out: string[] = [];
const objs = new Map<string, Record<string, unknown>>();

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

function makeCosts(key: string, mp: string, hp: string): Map<string, { mp: number; hp: number }> {
  return new Map([[key, { mp: Number(mp), hp: Number(hp) }]]);
}

function typeOfToken(tok: string): "next" | "hit" | undefined {
  if (tok === "-") return undefined;
  return tok as "next" | "hit";
}

function main(): void {
  const casePath = process.argv[2];
  if (!casePath) {
    process.stderr.write("usage: lfw_trace_next_frame.mjs <case-file>\n");
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
    } else if (op === "nfc") {
      const id = valueOf(t, i);
      const zeroAs = next();
      const type = typeOfToken(next());
      const key = next();
      const mp = next();
      const hp = next();
      const costs = makeCosts(key, mp, hp);
      out.push(`NFC ${render(get_next_frame_by_raw_id(id as never, zeroAs as never, type, costs))}`);
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
    } else if (op === "setarrobj") {
      const id = next();
      const key = next();
      objOf(id)[key] = [objs.get(next()) ?? {}];
    } else if (op === "newarrobj") {
      const id = next();
      const arr: unknown[] = [];
      while (i[0]! < t.length) arr.push(objs.get(next()) ?? {});
      objs.set(id, arr as never);
    } else if (op === "dump") {
      const id = next();
      out.push(`D ${id} ${render(objs.get(id))}`);
    } else if (op === "cook") {
      const id = next();
      const type = typeOfToken(next());
      const key = next();
      const mp = next();
      const hp = next();
      const costs = makeCosts(key, mp, hp);
      cook_next_frame_cost(objs.get(id) as never, type, costs);
      out.push(`CK ${id} ${render(objs.get(id))}`);
    } else if (op === "addnext") {
      const srcid = next();
      const items: unknown[] = [];
      while (i[0]! < t.length) items.push(valueOf(t, i));
      const src = srcid === "-" ? undefined : objs.get(srcid);
      out.push(`AN ${render(add_next_frame(src as never, ...(items as never[])))}`);
    } else if (op === "editnext") {
      const id = next();
      const key = next();
      const val = valueOf(t, i);
      const v = objs.get(id);
      edit_next_frame(v as never, (item) => {
        (item as Record<string, unknown>)[key] = val;
      });
      out.push(`EN ${id} ${render(objs.get(id))}`);
    } else if (op === "nfmut") {
      const id = valueOf(t, i);
      const key = next();
      const val = valueOf(t, i);
      const v = get_next_frame_by_raw_id(id as never, "repeat");
      (v as Record<string, unknown>)[key] = val;
      out.push(`NFM ${render(v)}`);
    } else {
      process.stderr.write(`unknown op '${op}'\n`);
      process.exit(2);
    }
  }

  process.stdout.write(out.join("\n") + (out.length ? "\n" : ""));
}

main();
