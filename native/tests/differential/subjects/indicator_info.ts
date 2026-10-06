import { cook_frame_indicator_info } from "../../../../src/LFW/dat_translator/cook_frame_indicator_info";

import { parseValue, readCaseLines, renderValue, splitWs } from "./trace_util";

const out: string[] = [];

function main(): void {
  const casePath = process.argv[2];
  if (!casePath) {
    process.stderr.write("usage: lfw_trace_indicator_info.mjs <case-file>\n");
    process.exit(2);
  }

  for (const raw of readCaseLines(casePath)) {
    const t = splitWs(raw);
    if (t.length === 0) continue;
    const op = t[0]!;
    const i = [1];

    if (op === "cfi") {
      const frame = parseValue(t, i) as never;
      // TS 那边有些形态会抛（`"w" in 标量`、`.forEach` 不是函数、给标量挂 `__indicator_info`），
      // 这里只记录成功与否，帧的部分改动照样打印（两边都必须对齐“抛之前改了多少”）。
      let ok = true;
      try {
        cook_frame_indicator_info(frame);
      } catch {
        ok = false;
      }
      out.push(`cfi ${ok ? "ok" : "throw"} ${renderValue(frame)}`);
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
