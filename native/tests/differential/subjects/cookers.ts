import { cook_bdy } from "../../../../src/LFW/dat_translator/cook_bdy";
import { cook_cpoint } from "../../../../src/LFW/dat_translator/cook_cpoint";
import { cook_itr } from "../../../../src/LFW/dat_translator/cook_itr";
import { cook_opoint } from "../../../../src/LFW/dat_translator/cook_opoint";
import { cook_wpoint } from "../../../../src/LFW/dat_translator/cook_wpoint";
import { float_scaling_itr } from "../../../../src/LFW/dat_translator/float_scaling_itr";
import { get_next_frame_by_raw_id } from "../../../../src/LFW/dat_translator/get_the_next";
import { make_frame_state } from "../../../../src/LFW/dat_translator/make_frame_state";
import { make_frame_behavior } from "../../../../src/LFW/dat_translator/make_frame_behavior";
import { make_fb_bat_chase_start } from "../../../../src/LFW/dat_translator/frame_behavior/make_fb_bat_chase_start";
import { make_fb_bat_chase } from "../../../../src/LFW/dat_translator/frame_behavior/make_fb_bat_chase";
import { make_fb_boomerang } from "../../../../src/LFW/dat_translator/frame_behavior/make_fb_boomerang";
import { make_fb_chasing_same_enemy } from "../../../../src/LFW/dat_translator/frame_behavior/make_fb_chasing_same_enemy";
import { make_fb_dennis_chase } from "../../../../src/LFW/dat_translator/frame_behavior/make_fb_dennis_chase";
import { make_fb_firzen_disater_start } from "../../../../src/LFW/dat_translator/frame_behavior/make_fb_firzen_disater_start";
import { make_fb_firzen_volcano_start } from "../../../../src/LFW/dat_translator/frame_behavior/make_fb_firzen_volcano_start";
import { make_fb_jan_angle_blessing } from "../../../../src/LFW/dat_translator/frame_behavior/make_fb_jan_angle_blessing";
import { make_fb_jan_chase_start } from "../../../../src/LFW/dat_translator/frame_behavior/make_fb_jan_chase_start";
import { make_fb_jan_chaseh_start } from "../../../../src/LFW/dat_translator/frame_behavior/make_fb_jan_chaseh_start";
import { make_fb_john_chase } from "../../../../src/LFW/dat_translator/frame_behavior/make_fb_john_chase";
import { make_fb_john_chase_leaving } from "../../../../src/LFW/dat_translator/frame_behavior/make_fb_john_chase_leaving";
import { make_fb_julian_ball } from "../../../../src/LFW/dat_translator/frame_behavior/make_fb_julian_ball";
import { make_fb_julian_ball_start } from "../../../../src/LFW/dat_translator/frame_behavior/make_fb_julian_ball_start";

import { parseValue, readCaseLines, renderValue, splitWs } from "./trace_util";

const out: string[] = [];
const objs = new Map<string, Record<string, unknown>>();
const NO_FRAME = {} as never;

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

function dispatchFb(fn: string, v: Record<string, unknown>, x: unknown, y: unknown): void {
  const fx = x as number;
  const fy = y as number;
  switch (fn) {
    case "bat_chase_start": make_fb_bat_chase_start(v as never); break;
    case "bat_chase": make_fb_bat_chase(v as never); break;
    case "boomerang": make_fb_boomerang(v as never); break;
    case "chasing_same_enemy": make_fb_chasing_same_enemy(v as never, (x ?? "x") as string); break;
    case "dennis_chase": make_fb_dennis_chase(v as never); break;
    case "firzen_disater_start": make_fb_firzen_disater_start(v as never, fx, fy); break;
    case "firzen_volcano_start": make_fb_firzen_volcano_start(v as never, fx, fy); break;
    case "jan_angle_blessing": make_fb_jan_angle_blessing(v as never); break;
    case "jan_chase_start": make_fb_jan_chase_start(v as never, fx, fy); break;
    case "jan_chaseh_start": make_fb_jan_chaseh_start(v as never, fx, fy); break;
    case "john_chase_leaving": make_fb_john_chase_leaving(v as never); break;
    case "john_chase": make_fb_john_chase(v as never); break;
    case "julian_ball_start": make_fb_julian_ball_start(v as never); break;
    case "julian_ball": make_fb_julian_ball(v as never); break;
    default:
      process.stderr.write(`unknown fb '${fn}'\n`);
      process.exit(2);
  }
}

function main(): void {
  const casePath = process.argv[2];
  if (!casePath) {
    process.stderr.write("usage: lfw_trace_cookers.mjs <case-file>\n");
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
    } else if (op === "setobj") {
      const id = next();
      const key = next();
      objOf(id)[key] = objs.get(next()) ?? {};
    } else if (op === "dump") {
      const id = next();
      out.push(`D ${id} ${render(objs.get(id))}`);
    } else if (op === "copo") {
      const id = next();
      const fid = next();
      const v = objOf(id);
      cook_opoint(v as never, objOf(fid));
      out.push(`CO ${id} ${render(v)}`);
    } else if (op === "mfstate") {
      const id = next();
      const v = objOf(id);
      make_frame_state(v as never);
      out.push(`MS ${id} ${render(v)}`);
    } else if (op === "fb") {
      const id = next();
      const fn = next();
      const v = objOf(id);
      const xt = t[i[0]!];
      const x = xt === "-" ? (i[0]!++, undefined) : valueOf(t, i);
      const yt = t[i[0]!];
      const y = yt === "-" ? (i[0]!++, undefined) : valueOf(t, i);
      dispatchFb(fn, v, x, y);
      out.push(`FB ${id} ${render(v)}`);
    } else if (op === "fbd") {
      const id = next();
      const oid = next();
      const v = objOf(id);
      make_frame_behavior(v as never, oid);
      out.push(`FBD ${id} ${render(v)}`);
    } else if (op === "bdy" || op === "wp" || op === "cp" || op === "itr" || op === "fsitr") {
      const id = next();
      const v = objOf(id);
      if (op === "bdy") cook_bdy(v as never, NO_FRAME);
      else if (op === "wp") cook_wpoint(v as never, NO_FRAME);
      else if (op === "cp") cook_cpoint(v as never, NO_FRAME);
      else if (op === "itr") cook_itr(v as never, NO_FRAME);
      else float_scaling_itr(v as never);
      const tag = op === "bdy" ? "CB" : op === "wp" ? "CW" : op === "cp" ? "CC" : op === "itr" ? "CI" : "FS";
      out.push(`${tag} ${id} ${render(v)}`);
    } else {
      process.stderr.write(`unknown op '${op}'\n`);
      process.exit(2);
    }
  }

  process.stdout.write(out.join("\n") + (out.length ? "\n" : ""));
}

main();
