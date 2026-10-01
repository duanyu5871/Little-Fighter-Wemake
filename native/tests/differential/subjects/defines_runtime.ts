import { is_cheat_type } from "../../../../src/LFW/defines/CheatType";
import { Defines } from "../../../../src/LFW/defines/defines";
import { is_difficulty } from "../../../../src/LFW/defines/Difficulty";
import { is_independent } from "../../../../src/LFW/defines/TeamEnum";

import { definesRuntimeEntries } from "./gen/defines_runtime";
import { parseValue, readCaseLines, renderValue, splitWs } from "./trace_util";

const out: string[] = [];
const line = (...p: (string | number)[]): string => p.map((x) => String(x)).join(" ");

function normalize(v: unknown): unknown {
  if (v instanceof Map) {
    const o: Record<string, unknown> = {};
    for (const [k, x] of v) o[String(k)] = normalize(x);
    return o;
  }
  if (Array.isArray(v)) return v.map((x) => (x === undefined ? null : normalize(x)));
  if (v !== null && typeof v === "object") {
    const src = v as Record<string, unknown>;
    const o: Record<string, unknown> = {};
    for (const k of Object.keys(src)) {
      if (src[k] === undefined) continue;
      o[k] = normalize(src[k]);
    }
    return o;
  }
  return v;
}

const re = (v: unknown): string => renderValue(normalize(v));

function outEntry(name: string, value: unknown): void {
  out.push(line("E", name, re(value)));
}

const KEY_FIELDS = ["L", "R", "U", "D", "a", "j", "d"] as const;

function outKeys(id: string): void {
  const k = Defines.get_default_keys(id) as unknown as Record<string, unknown> | undefined;
  if (k === undefined) out.push(line("K", re(id), "-"));
  else out.push(line("K", re(id), ...KEY_FIELDS.map((f) => re(k[f]))));
}

function main(): void {
  const casePath = process.argv[2];
  if (!casePath) {
    process.stderr.write("usage: lfw_trace_defines_runtime.mjs <case-file>\n");
    process.exit(2);
  }

  for (const raw of readCaseLines(casePath)) {
    const t = splitWs(raw);
    if (t.length === 0) continue;
    const op = t[0]!;
    const i = [1];

    if (op === "count") {
      out.push(
        line("N", definesRuntimeEntries.length, definesRuntimeEntries.filter((e) => e.isTopLevel).length),
      );
    } else if (op === "all") {
      for (const e of definesRuntimeEntries) outEntry(e.name, e.value);
    } else if (op === "top") {
      for (const e of definesRuntimeEntries) {
        if (e.isTopLevel) outEntry(e.name, e.value);
      }
    } else if (op === "prefix") {
      const p = t[i[0]!++]!;
      for (const e of definesRuntimeEntries) {
        if (e.name.startsWith(p)) outEntry(e.name, e.value);
      }
    } else if (op === "num") {
      const n = t[i[0]!++]!;
      const key = n.startsWith("Defines.") ? n.slice("Defines.".length) : n;
      out.push(line("D", n, re((Defines as Record<string, unknown>)[key])));
    } else if (op === "has") {
      const n = t[i[0]!++]!;
      const key = n.startsWith("Defines.") ? n.slice("Defines.".length) : n;
      out.push(line("H", n, re((Defines as Record<string, unknown>)[key] !== undefined)));
    } else if (op === "desire") {
      out.push(line("DS", re(Defines.desire(Number(t[i[0]!++]!)))));
    } else if (op === "keys") {
      outKeys(String(parseValue(t, i)));
    } else if (op === "isind") {
      out.push(line("I", re(is_independent(String(parseValue(t, i))))));
    } else if (op === "ischeat") {
      out.push(line("C", re(is_cheat_type(String(parseValue(t, i))))));
    } else if (op === "isdiff") {
      out.push(line("F", re(is_difficulty(Number(t[i[0]!++]!)))));
    } else {
      process.stderr.write(`unknown op '${op}'\n`);
      process.exit(2);
    }
  }

  process.stdout.write(out.join("\n") + (out.length ? "\n" : ""));
}

main();
