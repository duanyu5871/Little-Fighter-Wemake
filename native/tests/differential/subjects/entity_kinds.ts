import { make_ball_data } from "../../../../src/LFW/dat_translator/make_ball_data";
import { make_weapon_data } from "../../../../src/LFW/dat_translator/make_weapon_data";

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

function valueOf(t: string[], i: number[]): unknown {
  return parseValue(t, i);
}

function main(): void {
  const casePath = process.argv[2];
  if (!casePath) {
    process.stderr.write("usage: lfw_trace_entity_kinds.mjs <case-file>\n");
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
    } else if (op === "setobj") {
      const id = next();
      const key = next();
      objOf(id)[key] = objs.get(next());
    } else if (op === "dump") {
      const id = next();
      out.push(`D ${id} ${render(objs.get(id))}`);
    } else if (op === "mbd") {
      const id = next();
      out.push(`MBD ${id} ${render(make_ball_data(objOf(id) as never))}`);
    } else if (op === "mwd") {
      const id = next();
      out.push(`MWD ${id} ${render(make_weapon_data(objOf(id) as never))}`);
    } else {
      process.stderr.write(`unknown op '${op}'\n`);
      process.exit(2);
    }
  }

  process.stdout.write(out.join("\n") + (out.length ? "\n" : ""));
}

main();
