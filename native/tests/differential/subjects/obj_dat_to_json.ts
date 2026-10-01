import { obj_dat_to_json } from "../../../../src/LFW/dat_translator/obj_dat_to_json";

import { parseValue, readCaseLines, renderValue, splitWs } from "./trace_util";

const out: string[] = [];
let text = "";
let index: Record<string, unknown> = {};

function main(): void {
  const casePath = process.argv[2];
  if (!casePath) {
    process.stderr.write("usage: lfw_trace_obj_dat_to_json.mjs <case-file>\n");
    process.exit(2);
  }

  for (const raw of readCaseLines(casePath)) {
    const t = splitWs(raw);
    if (t.length === 0) continue;
    const op = t[0]!;
    const i = [1];
    const next = (): string => t[i[0]!++]!;

    if (op === "reset") {
      text = "";
      index = {};
    } else if (op === "t") {
      const v = parseValue(t, i) as string;
      if (text !== "") text += "\n";
      text += v;
      if (i[0] !== t.length) {
        process.stderr.write(`trailing token(s): ${raw}\n`);
        process.exit(2);
      }
    } else if (op === "i") {
      const key = next();
      index[key] = parseValue(t, i);
      if (i[0] !== t.length) {
        process.stderr.write(`trailing token(s): ${raw}\n`);
        process.exit(2);
      }
    } else if (op === "run") {
      try {
        const data = obj_dat_to_json(text, index as never);
        out.push(`R ${renderValue(data)}`);
      } catch (e) {
        out.push(`E ${(e as Error).message}`);
      }
    } else {
      process.stderr.write(`unknown op '${op}'\n`);
      process.exit(2);
    }
  }

  process.stdout.write(out.join("\n") + (out.length ? "\n" : ""));
}

main();
