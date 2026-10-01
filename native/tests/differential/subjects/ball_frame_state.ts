import { cook_ball_frame_state_15 } from "../../../../src/LFW/dat_translator/cook_ball_frame_state_15";
import { cook_ball_frame_state_3000 } from "../../../../src/LFW/dat_translator/cook_ball_frame_state_3000";
import { cook_ball_frame_state_3001 } from "../../../../src/LFW/dat_translator/cook_ball_frame_state_3001";
import { cook_ball_frame_state_3005 } from "../../../../src/LFW/dat_translator/cook_ball_frame_state_3005";
import { cook_ball_frame_state_3006 } from "../../../../src/LFW/dat_translator/cook_ball_frame_state_3006";

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
  return parseValue(t, i);
}

function main(): void {
  const casePath = process.argv[2];
  if (!casePath) {
    process.stderr.write("usage: lfw_trace_ball_frame_state.mjs <case-file>\n");
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
      if (i[0] !== t.length) {
        process.stderr.write(`trailing token(s): ${raw}\n`);
        process.exit(2);
      }
    } else if (op === "dump") {
      const id = next();
      out.push(`D ${id} ${render(objs.get(id))}`);
    } else if (
      op === "fs15" ||
      op === "fs3000" ||
      op === "fs3001" ||
      op === "fs3005" ||
      op === "fs3006"
    ) {
      const id = next();
      const ctx = objOf(id) as never;
      if (op === "fs15") cook_ball_frame_state_15(ctx);
      else if (op === "fs3000") cook_ball_frame_state_3000(ctx);
      else if (op === "fs3001") cook_ball_frame_state_3001(ctx);
      else if (op === "fs3005") cook_ball_frame_state_3005(ctx);
      else cook_ball_frame_state_3006(ctx);
      out.push(`${op} ${id} ${render(objs.get(id))}`);
    } else {
      process.stderr.write(`unknown op '${op}'\n`);
      process.exit(2);
    }
  }

  process.stdout.write(out.join("\n") + (out.length ? "\n" : ""));
}

main();
