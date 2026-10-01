import {
  as_array,
  bool,
  fields,
  fields_map_2_fields_obj,
  flt,
  int,
  map,
  obj,
  reorder_fields,
  str,
  todo,
  validate_fields,
} from "../../../../src/LFW/fields";

import { esc, parseValue, parseJsStringLiteral, readCaseLines, renderValue, splitWs } from "./trace_util";

type AnyFn = (...a: never[]) => unknown;

const FACTORY: Record<string, AnyFn> = {
  "": todo as unknown as AnyFn,
  string: str as unknown as AnyFn,
  float: flt as unknown as AnyFn,
  int: int as unknown as AnyFn,
  boolean: bool as unknown as AnyFn,
  object: obj as unknown as AnyFn,
  map: map as unknown as AnyFn,
};

function mapify(v: unknown): unknown {
  if (Array.isArray(v)) return v.map(mapify);
  if (v === null || typeof v !== "object" || v instanceof Map) return v;
  const o = v as Record<string, unknown>;
  const ret: Record<string, unknown> = {};
  for (const k of Object.keys(o)) {
    const val = o[k];
    if (k === "fields" && val !== null && typeof val === "object" && !Array.isArray(val)) {
      const inner = new Map<string, unknown>();
      for (const ik of Object.keys(val as Record<string, unknown>)) {
        inner.set(ik, mapify((val as Record<string, unknown>)[ik]));
      }
      ret[k] = inner;
    } else {
      ret[k] = mapify(val);
    }
  }
  return ret;
}

function asMap(v: unknown): Map<string, unknown> {
  if (v instanceof Map) return v;
  return new Map(Object.entries(mapify(v) as Record<string, unknown>));
}

function line(...parts: (string | number)[]): string {
  return parts.map((p) => String(p)).join(" ");
}

function main(): void {
  const casePath = process.argv[2];
  if (!casePath) {
    process.stderr.write("usage: lfw_trace_fields.mjs <case-file>\n");
    process.exit(2);
  }

  const out: string[] = [];
  let lineno = 0;

  for (const raw of readCaseLines(casePath)) {
    ++lineno;
    const tok = splitWs(raw);
    if (tok.length === 0) continue;
    const op = tok[0]!;
    const idx = [1];

    if (op === "fw") {
      const type = parseJsStringLiteral(tok[1]!);
      const n = Number(tok[2]!);
      idx[0] = 3;
      const args: unknown[] = [];
      for (let j = 0; j < n; j++) args.push(mapify(parseValue(tok, idx)));
      const fn = FACTORY[type];
      if (fn === undefined) {
        process.stderr.write(`line ${lineno}: unknown field type '${type}'\n`);
        process.exit(2);
      }
      out.push(line(op, renderValue(fn(...(args as never[])))));
      continue;
    }

    if (op === "ff") {
      out.push(line(op, renderValue(fields(mapify(parseValue(tok, idx)) as never))));
      continue;
    }

    if (op === "fm") {
      out.push(line(op, renderValue(fields_map_2_fields_obj(asMap(parseValue(tok, idx)) as never))));
      continue;
    }

    if (op === "fr") {
      const o = mapify(parseValue(tok, idx)) as Record<string, unknown>;
      reorder_fields(o as never, asMap(parseValue(tok, idx)) as never);
      out.push(line(op, renderValue(o)));
      continue;
    }

    if (op === "fa") {
      out.push(line(op, renderValue(as_array(parseValue(tok, idx) as never))));
      continue;
    }

    if (op === "fas") {
      const v = parseValue(tok, idx);
      out.push(line(op, as_array(v as never) === (v as never) ? "true" : "false"));
      continue;
    }

    if (op === "fv") {
      const data = mapify(parseValue(tok, idx));
      const m = asMap(parseValue(tok, idx));
      const errors: string[] = [];
      const warnings: string[] = [];
      const ok = validate_fields(data, m as never, errors, warnings);
      out.push(line(op, ok ? "true" : "false"));
      for (const e of errors) out.push(line("e", esc(e)));
      for (const w of warnings) out.push(line("w", esc(w)));
      continue;
    }

    process.stderr.write(`line ${lineno}: unknown op '${op}'\n`);
    process.exit(2);
  }

  process.stdout.write(out.join("\n") + (out.length ? "\n" : ""));
}

main();
