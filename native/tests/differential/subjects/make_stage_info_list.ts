import { make_stage_info_list } from "../../../../src/LFW/dat_translator/make_stage_info_list";

import { parseValue, readCaseLines, renderValue, splitWs } from "./trace_util";

const out: string[] = [];

function main(): void {
  const casePath = process.argv[2];
  if (!casePath) {
    process.stderr.write("usage: lfw_trace_make_stage_info_list.mjs <case-file>\n");
    process.exit(2);
  }

  for (const raw of readCaseLines(casePath)) {
    const t = splitWs(raw);
    if (t.length === 0) continue;
    const op = t[0]!;
    const i = [1];

    if (op === "mkstage") {
      const text = parseValue(t, i) as string;
      out.push(`mkstage ${renderValue(make_stage_info_list(text))}`);
    } else {
      process.stderr.write(`unknown op '${op}'\n`);
      process.exit(2);
    }
  }

  process.stdout.write(out.join("\n") + (out.length ? "\n" : ""));
}

main();
