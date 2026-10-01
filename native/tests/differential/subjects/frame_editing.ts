import { FrameEditing } from "../../../../src/LFW/dat_translator/FrameEditing";

import { parseValue, readCaseLines, renderValue, splitWs } from "./trace_util";

const out: string[] = [];
const objs = new Map<string, Record<string, unknown>>();
const costs = new Map<string, { mp: number; hp: number }>();

function objOf(id: string): Record<string, unknown> {
  let o = objs.get(id);
  if (o === undefined) {
    o = {};
    objs.set(id, o);
  }
  return o;
}

function main(): void {
  const casePath = process.argv[2];
  if (!casePath) {
    process.stderr.write("usage: lfw_trace_frame_editing.mjs <case-file>\n");
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
      const v = parseValue(t, i);
      if (id === "C") costs.set(key, v as never);
      else objOf(id)[key] = v;
      if (i[0] !== t.length) {
        process.stderr.write(`trailing token(s): ${raw}\n`);
        process.exit(2);
      }
    } else if (op === "dump") {
      const id = next();
      out.push(`D ${id} ${renderValue(objs.get(id))}`);
    } else if (op === "kd" || op === "ht" || op === "sq") {
      const id = next();
      const key = parseValue(t, i);
      const nexts: unknown[] = [];
      while (i[0] < t.length) nexts.push(parseValue(t, i));
      const ed = new FrameEditing(objOf(id) as never, costs as never);
      if (op === "kd") ed.keydown(key as never, ...(nexts as never[]));
      else if (op === "ht") ed.hit(key as never, ...(nexts as never[]));
      else ed.seq(key as never, ...(nexts as never[]));
      out.push(`${op} ${id} ${renderValue(objs.get(id))}`);
    } else {
      process.stderr.write(`unknown op '${op}'\n`);
      process.exit(2);
    }
  }

  process.stdout.write(out.join("\n") + (out.length ? "\n" : ""));
}

main();
