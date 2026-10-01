import { make_bg_data } from "../../../../src/LFW/dat_translator/make_bg_data";

import { parseValue, readCaseLines, renderValue, splitWs } from "./trace_util";

const out: string[] = [];
const objs = new Map<string, unknown>();

function render(v: unknown): string {
  return renderValue(v);
}

function objOf(id: string): Record<string, unknown> {
  let o = objs.get(id) as Record<string, unknown> | undefined;
  if (o === undefined) {
    o = {};
    objs.set(id, o);
  }
  return o;
}

function fieldOf(id: string, key: string): unknown {
  const o = objs.get(id);
  if (o === undefined || o === null) return undefined;
  return (o as Record<string, unknown>)[key];
}

function valueOf(t: string[], i: number[]): unknown {
  return parseValue(t, i);
}

function main(): void {
  const casePath = process.argv[2];
  if (!casePath) {
    process.stderr.write("usage: lfw_trace_bg_data.mjs <case-file>\n");
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
    } else if (op === "dump") {
      const id = next();
      out.push(`D ${id} ${render(objs.get(id))}`);
    } else if (op === "bgd") {
      const id = next();
      const text = fieldOf(id, "text") as string;
      const index = fieldOf(id, "index") as never;
      out.push(`BGD ${id} ${render(make_bg_data(text, index))}`);
    } else {
      process.stderr.write(`unknown op '${op}'\n`);
      process.exit(2);
    }
  }

  process.stdout.write(out.join("\n") + (out.length ? "\n" : ""));
}

main();
