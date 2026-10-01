import { make_fighter_data } from "../../../../src/LFW/dat_translator/make_fighter_data";

import { parseValue, readCaseLines, renderValue, splitWs } from "./trace_util";

const out: string[] = [];
let ctx: Record<string, unknown> = {};

function slot(owner: Record<string, unknown>, key: string): Record<string, unknown> {
  let o = owner[key] as Record<string, unknown> | undefined;
  if (o === undefined || o === null || typeof o !== "object" || Array.isArray(o)) {
    o = {};
    owner[key] = o;
  }
  return o;
}

function main(): void {
  const casePath = process.argv[2];
  if (!casePath) {
    process.stderr.write("usage: lfw_trace_make_fighter_data.mjs <case-file>\n");
    process.exit(2);
  }

  for (const raw of readCaseLines(casePath)) {
    const t = splitWs(raw);
    if (t.length === 0) continue;
    const op = t[0]!;
    const i = [1];
    const next = (): string => t[i[0]!++]!;

    if (op === "reset") {
      ctx = {};
    } else if (op === "c") {
      const key = next();
      const v = parseValue(t, i);
      ctx[key] = v;
      if (i[0] !== t.length) {
        process.stderr.write(`trailing token(s): ${raw}\n`);
        process.exit(2);
      }
    } else if (op === "f") {
      const frameKey = next();
      const v = parseValue(t, i);
      slot(ctx, "frames")[frameKey] = v;
      if (i[0] !== t.length) {
        process.stderr.write(`trailing token(s): ${raw}\n`);
        process.exit(2);
      }
    } else if (op === "s") {
      const frameKey = next();
      const subKey = next();
      const v = parseValue(t, i);
      slot(slot(ctx, "frames"), frameKey)[subKey] = v;
      if (i[0] !== t.length) {
        process.stderr.write(`trailing token(s): ${raw}\n`);
        process.exit(2);
      }
    } else if (op === "run") {
      const ret = make_fighter_data(ctx as never);
      out.push(`R ${renderValue(ret)}`);
    } else {
      process.stderr.write(`unknown op '${op}'\n`);
      process.exit(2);
    }
  }

  process.stdout.write(out.join("\n") + (out.length ? "\n" : ""));
}

main();
