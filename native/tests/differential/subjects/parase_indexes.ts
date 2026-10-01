import { parase_indexes } from "../../../../src/LFW/dat_translator/parase_indexes";

import { parseValue, readCaseLines, renderValue, splitWs } from "./trace_util";

const out: string[] = [];

function main(): void {
  const casePath = process.argv[2];
  if (!casePath) {
    process.stderr.write("usage: lfw_trace_parase_indexes.mjs <case-file>\n");
    process.exit(2);
  }

  for (const raw of readCaseLines(casePath)) {
    const t = splitWs(raw);
    if (t.length === 0) continue;
    const op = t[0]!;
    const i = [1];

    if (op === "parse") {
      const suffix = parseValue(t, i) as never;
      const text = parseValue(t, i) as never;
      try {
        const res = parase_indexes(text, suffix as "json5");
        out.push(`parse ok ${renderValue(res)}`);
      } catch (e) {
        out.push(`parse err ${(e as Error).message}`);
      }
    } else {
      process.stderr.write(`unknown op '${op}'\n`);
      process.exit(2);
    }
  }

  process.stdout.write(out.join("\n") + (out.length ? "\n" : ""));
}

main();
