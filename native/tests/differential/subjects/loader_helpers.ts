import { make_buring_smoke } from "../../../../src/LFW/dat_translator/make_buring_smoke";
import { preprocess_frame_pic } from "../../../../src/LFW/loader/preprocess_frame_pic";
import { preprocess_pic } from "../../../../src/LFW/loader/preprocess_pic";
import { preprocess_stage_phase } from "../../../../src/LFW/loader/preprocess_stage_phase";
import { preprocess_stage } from "../../../../src/LFW/loader/preprocess_stage";
import { preprocess_wpoint } from "../../../../src/LFW/loader/preprocess_wpoint";

import { parseValue, readCaseLines, renderValue, splitWs } from "./trace_util";

const out: string[] = [];

function stripGenFacing(v: unknown): void {
  if (v === null || typeof v !== "object") return;
  const a = (v as Record<string, unknown>).action;
  if (a !== null && typeof a === "object") {
    delete (a as Record<string, unknown>).__gen_facing;
  }
}

function main(): void {
  const casePath = process.argv[2];
  if (!casePath) {
    process.stderr.write("usage: lfw_trace_loader_helpers.mjs <case-file>\n");
    process.exit(2);
  }

  for (const raw of readCaseLines(casePath)) {
    const t = splitWs(raw);
    if (t.length === 0) continue;
    const op = t[0]!;
    const i = [1];

    if (op === "b") {
      const n = parseValue(t, i) as number;
      const v = make_buring_smoke(n as never);
      stripGenFacing(v);
      out.push(`b ${renderValue(v)}`);
      if (i[0] !== t.length) {
        process.stderr.write(`trailing token(s): ${raw}\n`);
        process.exit(2);
      }
    } else if (op === "pic" || op === "fpic" || op === "wp" || op === "phase" || op === "stage") {
      const v = parseValue(t, i) as never;
      let res: unknown;
      if (op === "pic") res = preprocess_pic(null as never, null as never, v);
      else if (op === "fpic") res = preprocess_frame_pic(null as never, null as never, v);
      else if (op === "wp") res = preprocess_wpoint(v);
      else if (op === "phase") res = preprocess_stage_phase(v);
      else res = preprocess_stage(v);
      out.push(`${op} ${renderValue(res)}`);
      if (i[0] !== t.length) {
        process.stderr.write(`trailing token(s): ${raw}\n`);
        process.exit(2);
      }
    } else {
      process.stderr.write(`unknown op '${op}'\n`);
      process.exit(2);
    }
  }

  process.stdout.write(out.join("\n") + (out.length ? "\n" : ""));
}

main();
