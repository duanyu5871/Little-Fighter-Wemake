import { bdy_kind_full_name, bdy_kind_name } from "../../../../src/LFW/defines/BdyKind";
import {
  HIT_FLAG_NAME_MAP,
  get_hit_flag_desc,
  get_hit_flag_full_name,
  get_hit_flag_name,
} from "../../../../src/LFW/defines/HitFlag";
import { wpoint_kind_full_name, wpoint_kind_name } from "../../../../src/LFW/defines/WpointKind";

import { parseValue, readCaseLines, renderValue, splitWs } from "./trace_util";

const out: string[] = [];
const line = (...p: (string | number)[]): string => p.map((x) => String(x)).join(" ");

function one(tag: string, v: unknown): void {
  out.push(line(tag, renderValue(v)));
}

function dumpMap(): void {
  for (const k of Object.keys(HIT_FLAG_NAME_MAP).sort()) {
    out.push(line("M", renderValue(k), renderValue((HIT_FLAG_NAME_MAP as never)[k as never])));
  }
}

function main(): void {
  const casePath = process.argv[2];
  if (!casePath) {
    process.stderr.write("usage: lfw_trace_labels.mjs <case-file>\n");
    process.exit(2);
  }

  for (const raw of readCaseLines(casePath)) {
    const t = splitWs(raw);
    if (t.length === 0) continue;
    const op = t[0]!;
    const i = [1];

    if (op === "dump") {
      dumpMap();
      continue;
    }
    const v = parseValue(t, i);
    if (op === "hit") one("H", get_hit_flag_name(v));
    else if (op === "hitfull") one("HF", get_hit_flag_full_name(v));
    else if (op === "hitdesc") one("HD", get_hit_flag_desc(v));
    else if (op === "bdy") one("B", bdy_kind_name(v));
    else if (op === "bdyfull") one("BF", bdy_kind_full_name(v));
    else if (op === "wp") one("W", wpoint_kind_name(v));
    else if (op === "wpfull") one("WF", wpoint_kind_full_name(v));
    else {
      process.stderr.write(`unknown op '${op}'\n`);
      process.exit(2);
    }
  }

  process.stdout.write(out.join("\n") + (out.length ? "\n" : ""));
}

main();
