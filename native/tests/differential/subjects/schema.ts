// `utils/schema` 家族（4W）的差分台面（TS 侧）：真 `SchemaValidator` + 生成表里的真 schema。
// 文法与 C++ 侧逐字一致，见 `schema.cpp` 头注。
import { check_phase_info, check_stage_info } from "../../../../src/LFW/loader/check_stage_info";
import { SchemaValidator } from "../../../../src/LFW/utils/schema/validate_schema";

import { SCHEMA_TABLES } from "./gen/defines_schemas";

import { esc, parseValue, readCaseLines, renderValue, splitWs } from "./trace_util";

const out: string[] = [];
const schemas = new Map<string, unknown>();
const validators = new Map<string, SchemaValidator>();

function fail(what: string, detail: string): never {
  process.stderr.write(`${what}: ${detail}\n`);
  process.exit(2);
}

function findSchemaTable(name: string): unknown | undefined {
  for (const t of SCHEMA_TABLES) if (t.name === name) return t.value;
  return undefined;
}

function printMessages(tag: string, vid: string, ms: readonly string[]): void {
  for (let j = 0; j < ms.length; j++) out.push(`${tag}|${vid}|${j}|${esc(ms[j]!)}`);
}

function main(): void {
  const casePath = process.argv[2];
  if (!casePath) {
    process.stderr.write("usage: lfw_trace_schema.mjs <case-file>\n");
    process.exit(2);
  }

  for (const raw of readCaseLines(casePath)) {
    const t = splitWs(raw);
    if (t.length === 0) continue;
    const op = t[0]!;
    const i = [1];
    const next = (): string => t[i[0]!++]!;

    if (op === "nv") {
      validators.set(next(), new SchemaValidator());
    } else if (op === "sch") {
      const sid = next();
      const name = next();
      const s = findSchemaTable(name);
      if (s === undefined) fail("unknown schema table", name);
      schemas.set(sid, s);
    } else if (op === "schv") {
      const sid = next();
      schemas.set(sid, parseValue(t, i));
    } else if (op === "val") {
      const vid = next();
      const sid = next();
      const sv = schemas.get(sid);
      if (sv === undefined) fail("unbound schema", sid);
      const vv = validators.get(vid);
      if (vv === undefined) fail("unbound validator", vid);
      const value = parseValue(t, i);
      const b = vv.validate(value, sv as never);
      out.push(`v|${vid}|${sid}|${b ? "b1" : "b0"}|e=${vv.errors.length}|w=${vv.warnings.length}`);
      printMessages("ve", vid, vv.errors);
      printMessages("vw", vid, vv.warnings);
      out.push(`vv|${vid}|${renderValue(value)}`);
    } else if (op === "rz") {
      const vid = next();
      const vv = validators.get(vid);
      if (vv === undefined) fail("unbound validator", vid);
      vv.reset();
      out.push(`rz|${vid}|e=${vv.errors.length}|w=${vv.warnings.length}`);
    } else if (op === "cst") {
      const value = parseValue(t, i);
      const errs: string[] = [];
      const b = check_stage_info(value as never, errs);
      out.push(`cst|${b ? "b1" : "b0"}|e=${errs.length}`);
      printMessages("ce", "-", errs);
    } else if (op === "cph") {
      const stage = parseValue(t, i);
      const info = parseValue(t, i);
      const errs: string[] = [];
      const b = check_phase_info(stage as never, info as never, 0, errs);
      out.push(`cph|${b ? "b1" : "b0"}|e=${errs.length}`);
      printMessages("ce", "-", errs);
    } else {
      process.stderr.write(`unknown op '${op}'\n`);
      process.exit(2);
    }
  }

  process.stdout.write(out.join("\n") + (out.length ? "\n" : ""));
}

main();
