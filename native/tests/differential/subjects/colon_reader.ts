import { ColonValueReader } from "../../../../src/LFW/dat_translator/ColonValueReader";

import { parseValue, readCaseLines, renderValue, splitWs } from "./trace_util";

const out: string[] = [];

function render(v: unknown): string {
  return renderValue(v);
}

function main(): void {
  const casePath = process.argv[2];
  if (!casePath) {
    process.stderr.write("usage: lfw_trace_colon_reader.mjs <case-file>\n");
    process.exit(2);
  }

  for (const raw of readCaseLines(casePath)) {
    const t = splitWs(raw);
    if (t.length === 0) continue;
    const op = t[0]!;
    const i = [1];

    if (op === "read") {
      const spec = t[i[0]!++]!;
      const text = String(parseValue(t, i));
      const r = new ColonValueReader<Record<string, unknown>>();
      if (spec !== "-") {
        for (const part of spec.split(",")) {
          const kind = part.slice(0, 1);
          const name = part.slice(2);
          if (kind === "s") r.str(name as never);
          else if (kind === "i") r.int(name as never);
          else r.int_2(name as never);
        }
      }
      const [result, rem] = r.read(text, {});
      out.push(`R ${render(result)} ${render(rem)}`);
    } else {
      process.stderr.write(`unknown op '${op}'\n`);
      process.exit(2);
    }
  }

  process.stdout.write(out.join("\n") + (out.length ? "\n" : ""));
}

main();
