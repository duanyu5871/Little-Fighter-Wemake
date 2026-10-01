import { EditBdy } from "../../../../src/LFW/dat_translator/EditBdy";
import { cook_ball_bdy_get_hit_to_frame_20 } from "../../../../src/LFW/dat_translator/cook_ball_bdy_get_hit_to_frame_20";
import { cook_ball_bdy_get_hit_to_frame_30 } from "../../../../src/LFW/dat_translator/cook_ball_bdy_get_hit_to_frame_30";

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
  if (o === undefined || o === null) return undefined;
  return o[key];
}

function valueOf(t: string[], i: number[]): unknown {
  return parseValue(t, i);
}

function main(): void {
  const casePath = process.argv[2];
  if (!casePath) {
    process.stderr.write("usage: lfw_trace_ball_bdy.mjs <case-file>\n");
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
    } else if (op === "bb20" || op === "bb30") {
      const id = next();
      const ctx = objOf(id) as never;
      const ret =
        op === "bb20"
          ? cook_ball_bdy_get_hit_to_frame_20(ctx)
          : cook_ball_bdy_get_hit_to_frame_30(ctx);
      out.push(`${op === "bb20" ? "B20" : "B30"} ${id} ${render(ret)}`);
    } else if (op === "bedit") {
      const id = next();
      const o = objOf(id);
      EditBdy.edit(o["bdy"] as never, (o["fields"] ?? {}) as never);
      out.push(`BE ${id} ${render(o)}`);
    } else {
      process.stderr.write(`unknown op '${op}'\n`);
      process.exit(2);
    }
  }

  process.stdout.write(out.join("\n") + (out.length ? "\n" : ""));
}

main();
