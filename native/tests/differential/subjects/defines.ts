import { ENUMS } from "./gen/defines_enums";

import { esc, readCaseLines } from "./trace_util";

function line(...parts: (string | number)[]): string {
  return parts.map((p) => String(p)).join(" ");
}

function isNumberKey(k: string): boolean {
  return /^-?\d+$/.test(k);
}

function compareName(a: string, b: string): number {
  return a < b ? -1 : a > b ? 1 : 0;
}

function main(): void {
  const casePath = process.argv[2];
  if (!casePath) {
    process.stderr.write("usage: lfw_trace_defines.mjs <case-file>\n");
    process.exit(2);
  }
  for (const _ of readCaseLines(casePath)) void _;

  const out: string[] = [];

  const sorted = [...ENUMS].sort((a, b) => compareName(a.name, b.name));

  for (const entry of sorted) {
    if (entry.value === undefined || entry.value === null) {
      process.stderr.write(`enum not reachable as module member: ${entry.name}\n`);
      process.exit(2);
    }
    const obj = entry.value as Record<string, unknown>;
    const pairs = Object.entries(obj);
    const isNumber = pairs.some(([k, v]) => typeof v === "number" && !isNumberKey(k));

    out.push(line("E", entry.name));

    if (!isNumber) {
      const members: [string, string][] = [];
      for (const [k, v] of pairs) if (typeof v === "string") members.push([k, v]);
      members.sort((a, b) => compareName(a[0], b[0]));
      for (const [k, v] of members) out.push(line("F", k, esc(v)));
      continue;
    }

    const forward: [string, number][] = [];
    for (const [k, v] of pairs) if (typeof v === "number" && !isNumberKey(k)) forward.push([k, v]);
    forward.sort((a, b) => compareName(a[0], b[0]));
    for (const [k, v] of forward) out.push(line("F", k, String(v)));

    const reverse = new Map<number, string>();
    for (const [k, v] of pairs) if (typeof v === "string" && isNumberKey(k)) reverse.set(Number(k), v);
    const values = [...reverse.keys()].sort((a, b) => a - b);
    for (const v of values) out.push(line("R", String(v), reverse.get(v)!));
  }

  process.stdout.write(out.join("\n") + (out.length ? "\n" : ""));
}

main();
