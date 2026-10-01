import { FIELD_TABLES } from "./gen/defines_fields";

import { readCaseLines, renderValue } from "./trace_util";

function compareName(a: string, b: string): number {
  return a < b ? -1 : a > b ? 1 : 0;
}

function main(): void {
  const casePath = process.argv[2];
  if (!casePath) {
    process.stderr.write("usage: lfw_trace_defines_fields.mjs <case-file>\n");
    process.exit(2);
  }
  for (const _ of readCaseLines(casePath)) void _;

  const out: string[] = [];
  const sorted = [...FIELD_TABLES].sort((a, b) => compareName(a.name, b.name));
  for (const t of sorted) out.push(`T ${t.name} ${renderValue(t.value)}`);

  process.stdout.write(out.join("\n") + (out.length ? "\n" : ""));
}

main();
