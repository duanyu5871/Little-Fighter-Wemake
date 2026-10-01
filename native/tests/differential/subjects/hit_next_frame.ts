import {
  hit_next_frame_defend,
  hit_next_frame_drink,
  hit_next_frame_jump,
  hit_next_frame_jump_atk,
  hit_next_frame_punch,
  hit_next_frame_super_punch,
  hit_next_frame_turn_back,
  hit_next_frame_weapon_atk,
} from "../../../../src/LFW/dat_translator/hit_next_frame";

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

function main(): void {
  const casePath = process.argv[2];
  if (!casePath) {
    process.stderr.write("usage: lfw_trace_hit_next_frame.mjs <case-file>\n");
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
      out.push(`D ${id} ${render(objs.get(id))}`);
    } else if (op === "turn_back") {
      const id = next();
      const frame = objOf(id) as never;
      if (i[0] < t.length) {
        const back = parseValue(t, i);
        hit_next_frame_turn_back(frame, back as never);
      } else {
        hit_next_frame_turn_back(frame);
      }
      out.push(`turn_back ${id} ${render(objs.get(id))}`);
    } else if (
      op === "drink" ||
      op === "super_punch" ||
      op === "punch" ||
      op === "jump" ||
      op === "defend" ||
      op === "weapon_atk" ||
      op === "jump_atk"
    ) {
      let res: unknown;
      if (op === "drink") res = hit_next_frame_drink();
      else if (op === "super_punch") res = hit_next_frame_super_punch();
      else if (op === "punch") res = hit_next_frame_punch();
      else if (op === "jump") res = hit_next_frame_jump();
      else if (op === "defend") res = hit_next_frame_defend();
      else if (op === "weapon_atk") res = hit_next_frame_weapon_atk();
      else res = hit_next_frame_jump_atk();
      out.push(`${op} ${render(res)}`);
    } else {
      process.stderr.write(`unknown op '${op}'\n`);
      process.exit(2);
    }
  }

  process.stdout.write(out.join("\n") + (out.length ? "\n" : ""));
}

main();
