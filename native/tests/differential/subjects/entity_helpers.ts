import { calc_v } from "../../../../src/LFW/entity/calc_v";
import { NSlot, SSlot, NUM_SLOTS, STR_SLOTS, num_or_null, to_tri, from_tri } from "../../../../src/LFW/entity/EntitySnapshot";
import {
  is_fighter_data,
  is_weapon_data,
  is_entity_data,
  is_ball_data,
  is_fighter,
  is_ball,
  is_weapon,
  is_entity,
  is_boss,
  is_object,
  is_object_data,
  is_bg_data,
  is_base_ctrl,
  is_bot_ctrl,
  is_human_ctrl,
  is_ball_ctrl,
} from "../../../../src/LFW/entity/type_check";
import { same_face, turn_face } from "../../../../src/LFW/entity/face_helper";
import { find_direction } from "../../../../src/LFW/entity/find_frame_direction";

import { parseValue, readCaseLines, renderValue, splitWs } from "./trace_util";

const out: string[] = [];

type Check = (v: unknown) => boolean;

const checks: Record<string, Check> = {
  fighter_data: is_fighter_data,
  weapon_data: is_weapon_data,
  entity_data: is_entity_data,
  ball_data: is_ball_data,
  fighter: is_fighter,
  ball: is_ball,
  weapon: is_weapon,
  entity: is_entity,
  boss: is_boss,
  object: is_object,
  object_data: is_object_data,
  bg_data: is_bg_data,
  base_ctrl: is_base_ctrl,
  bot_ctrl: is_bot_ctrl,
  human_ctrl: is_human_ctrl,
  ball_ctrl: is_ball_ctrl,
};

function slotLine(tag: string, e: Record<string, unknown>): string {
  const names = Object.keys(e).filter((k) => Number.isNaN(Number(k)));
  let line = tag;
  for (const name of names) line += ` ${name}=${e[name]}`;
  return line;
}

function main(): void {
  const casePath = process.argv[2];
  if (!casePath) {
    process.stderr.write("usage: lfw_trace_entity_helpers.mjs <case-file>\n");
    process.exit(2);
  }

  for (const raw of readCaseLines(casePath)) {
    const t = splitWs(raw);
    if (t.length === 0) continue;
    const op = t[0]!;
    const idx = [1];

    if (op === "ns") {
      out.push(slotLine("ns", NSlot as never));
      continue;
    }
    if (op === "ss") {
      out.push(slotLine("ss", SSlot as never));
      continue;
    }

    if (op === "cv") {
      const cur = parseValue(t, idx);
      const val = parseValue(t, idx);
      const mode = parseValue(t, idx);
      const acc = parseValue(t, idx);
      const dir = parseValue(t, idx);
      const res = calc_v(
        Number(cur),
        Number(val),
        mode as never,
        acc as never,
        dir as never,
      );
      out.push(`cv ${renderValue(res)}`);
    } else if (op === "fd") {
      const frame = parseValue(t, idx);
      const pair = parseValue(t, idx);
      out.push(`fd ${renderValue(find_direction(frame as never, pair as never))}`);
    } else if (op === "sf") {
      const a = parseValue(t, idx);
      const b = parseValue(t, idx);
      out.push(`sf ${renderValue(same_face(a as never, b as never))}`);
    } else if (op === "tf") {
      const f = parseValue(t, idx);
      out.push(`tf ${renderValue(turn_face(f as never))}`);
    } else if (op === "tc") {
      const name = t[1];
      const fn = name ? checks[name] : undefined;
      if (!name || !fn) {
        process.stderr.write(`unknown check '${name}'\n`);
        process.exit(2);
      }
      const v = parseValue(t, [2]);
      out.push(`tc ${name} ${fn(v) ? "b1" : "b0"}`);
      continue;
    } else if (op === "non") {
      out.push(`non ${renderValue(num_or_null(parseValue(t, idx) as never))}`);
    } else if (op === "tri") {
      out.push(`tri ${renderValue(to_tri(parseValue(t, idx) as never))}`);
    } else if (op === "ftri") {
      out.push(`ftri ${renderValue(from_tri(parseValue(t, idx) as never))}`);
    } else if (op === "nslots") {
      out.push(`nslots ${renderValue(NUM_SLOTS)}`);
    } else if (op === "sslots") {
      out.push(`sslots ${renderValue(STR_SLOTS)}`);
    } else {
      process.stderr.write(`unknown op '${op}'\n`);
      process.exit(2);
    }

    if (idx[0] !== t.length) {
      process.stderr.write(`trailing token(s) at line: ${raw}\n`);
      process.exit(2);
    }
  }

  process.stdout.write(out.join("\n") + (out.length ? "\n" : ""));
}

main();
