import * as bp from "../../../../src/LFW/dat_translator/broken_piece_frames";
import { make_weapon_special } from "../../../../src/LFW/dat_translator/make_weapon_special";

import { parseValue, readCaseLines, renderValue, splitWs } from "./trace_util";

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

function pieceOf(name: string): unknown {
  return (bp as unknown as Record<string, unknown>)[name];
}

function main(): void {
  const casePath = process.argv[2];
  if (!casePath) {
    process.stderr.write("usage: lfw_trace_make_weapon_special.mjs <case-file>\n");
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
      objOf(id)[key] = parseValue(t, i);
      if (i[0] !== t.length) {
        process.stderr.write(`trailing token(s): ${raw}\n`);
        process.exit(2);
      }
    } else if (op === "dump") {
      const id = next();
      out.push(`D ${id} ${renderValue(objs.get(id))}`);
    } else if (op === "wsp") {
      const id = next();
      make_weapon_special(objOf(id) as never);
      out.push(`wsp ${id} ${renderValue(objs.get(id))}`);
    } else if (op === "piece") {
      const name = next();
      out.push(`piece ${name} ${renderValue(pieceOf(name))}`);
    } else if (op === "pmut") {
      const name = next();
      const idx = Number(next());
      const v = parseValue(t, i);
      const arr = pieceOf(name) as unknown[];
      if (Array.isArray(arr)) arr[idx] = v;
      out.push(`pmut ${name} ${renderValue(pieceOf(name))}`);
    } else {
      process.stderr.write(`unknown op '${op}'\n`);
      process.exit(2);
    }
  }

  process.stdout.write(out.join("\n") + (out.length ? "\n" : ""));
}

main();
