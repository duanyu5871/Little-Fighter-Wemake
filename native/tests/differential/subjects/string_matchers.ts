import { delete_undefined } from "../../../../src/LFW/dat_translator/xml/delete_undefined";
import { match_block_once } from "../../../../src/LFW/utils/string_parser/match_block";
import { match_colon_value } from "../../../../src/LFW/utils/string_parser/match_colon_value";
import { take_blocks } from "../../../../src/LFW/utils/string_parser/take_blocks";
import { is_str } from "../../../../src/LFW/utils/type_check";

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

function fieldOf(id: string, key: string): unknown {
  const o = objs.get(id);
  if (o === undefined) return undefined;
  return o[key];
}

function valueOf(t: string[], i: number[]): unknown {
  return parseValue(t, i);
}

function main(): void {
  const casePath = process.argv[2];
  if (!casePath) {
    process.stderr.write("usage: lfw_trace_string_matchers.mjs <case-file>\n");
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
    } else if (op === "setarr") {
      const id = next();
      const key = next();
      const arr: unknown[] = [];
      while (i[0]! < t.length) arr.push(valueOf(t, i));
      objOf(id)[key] = arr;
    } else if (op === "dump") {
      const id = next();
      out.push(`D ${id} ${render(objs.get(id))}`);
    } else if (op === "mcv") {
      const id = next();
      const text = fieldOf(id, "text");
      const pairs = is_str(text) ? match_colon_value(text) : [];
      out.push(`MCV ${id} ${render(pairs.map(([k, v]) => ({ k, v })))}`);
    } else if (op === "mbo") {
      const id = next();
      const start = String(valueOf(t, i));
      const end = String(valueOf(t, i));
      const text = fieldOf(id, "text");
      const body = is_str(text) ? match_block_once(text, start, end) : null;
      out.push(`MBO ${id} ${render(body)}`);
    } else if (op === "tb") {
      const id = next();
      const start = String(valueOf(t, i));
      const end = String(valueOf(t, i));
      const text = fieldOf(id, "text");
      const r = is_str(text) ? take_blocks(text, start, end) : { blocks: [], remains: null };
      out.push(`TB ${id} ${render({ blocks: r.blocks, remains: r.remains })}`);
    } else if (op === "du") {
      const id = next();
      const o = objs.get(id);
      if (o !== undefined && typeof o === "object") delete_undefined(o as object);
      out.push(`DU ${id} ${render(objs.get(id))}`);
    } else {
      process.stderr.write(`unknown op '${op}'\n`);
      process.exit(2);
    }
  }

  process.stdout.write(out.join("\n") + (out.length ? "\n" : ""));
}

main();
