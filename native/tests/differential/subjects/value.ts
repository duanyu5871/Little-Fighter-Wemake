import { bitsHex, esc, parseJsStringLiteral, readCaseLines, splitWs } from "./trace_util";

function line(...parts: (string | number)[]): string {
  return parts.map((p) => String(p)).join(" ");
}

const handles: unknown[] = [];

function parseValue(tok: string[], idx: number[]): unknown {
  const kind = tok[idx[0]!++]!;
  switch (kind) {
    case "u":
      return undefined;
    case "z":
      return null;
    case "b":
      return tok[idx[0]!++] === "1";
    case "n":
      return Number(tok[idx[0]!++]!);
    case "s":
      return parseJsStringLiteral(tok[idx[0]!++]!);
    case "a": {
      const n = Number(tok[idx[0]!++]!);
      const arr: unknown[] = [];
      for (let j = 0; j < n; j++) arr.push(parseValue(tok, idx));
      return arr;
    }
    default:
      process.stderr.write(`bad value literal '${kind}'\n`);
      return process.exit(2);
  }
}

function pushHandle(v: unknown): number {
  handles.push(v);
  return handles.length - 1;
}

function main(): void {
  const casePath = process.argv[2];
  if (!casePath) {
    process.stderr.write("usage: lfw_trace_value.mjs <case-file>\n");
    process.exit(2);
  }

  const out: string[] = [];
  let lineno = 0;

  for (const raw of readCaseLines(casePath)) {
    ++lineno;
    const tok = splitWs(raw);
    if (tok.length === 0) continue;
    const op = tok[0]!;

    if (op === "typeof" || op === "truthy" || op === "is_array" || op === "length") {
      const v = parseValue(tok, [1]);
      if (op === "typeof") out.push(line(op, typeof v));
      else if (op === "truthy") out.push(line(op, v ? "true" : "false"));
      else if (op === "is_array") out.push(line(op, Array.isArray(v) ? "true" : "false"));
      else out.push(line(op, Array.isArray(v) ? v.length : "-"));
      continue;
    }

    if (op === "hold") {
      out.push(line(op, pushHandle(parseValue(tok, [1]))));
      continue;
    }

    if (op === "dup") {
      const idx = Number(tok[1]);
      if (!(idx >= 0 && idx < handles.length)) {
        process.stderr.write(`line ${lineno}: handle ${idx} out of range\n`);
        process.exit(2);
      }
      out.push(line(op, pushHandle(handles[idx])));
      continue;
    }

    if (op === "same") {
      const a = handles[Number(tok[1])];
      const b = handles[Number(tok[2])];
      out.push(line(op, Array.isArray(a) && Array.isArray(b) ? String(a === b) : "-"));
      continue;
    }

    if (op === "elem") {
      const held = handles[Number(tok[1])];
      const idx = Number(tok[2]);
      if (!Array.isArray(held) || !Number.isInteger(idx) || idx < 0 || idx >= held.length) {
        out.push(line(op, "-"));
      } else {
        out.push(line(op, pushHandle(held[idx])));
      }
      continue;
    }

    if (op === "eq" || op === "seq") {
      const idx = [1];
      const x = parseValue(tok, idx) as never;
      const y = parseValue(tok, idx) as never;
      out.push(line(op, (op === "eq" ? x == y : x === y) ? "true" : "false"));
      continue;
    }

    if (op === "ton") {
      const v = parseValue(tok, [1]);
      const d = Number(v);
      out.push(line(op, Number.isNaN(d) ? "nan" : bitsHex(d)));
      continue;
    }

    if (op === "tos") {
      out.push(line(op, esc(String(parseValue(tok, [1])))));
      continue;
    }

    if (op === "lt" || op === "gt" || op === "le" || op === "ge") {
      const idx = [1];
      const x = parseValue(tok, idx) as number;
      const y = parseValue(tok, idx) as number;
      const r = op === "lt" ? x < y : op === "gt" ? x > y : op === "le" ? x <= y : x >= y;
      out.push(line(op, r ? "true" : "false"));
      continue;
    }

    process.stderr.write(`line ${lineno}: unknown op '${op}'\n`);
    process.exit(2);
  }

  process.stdout.write(out.join("\n") + (out.length ? "\n" : ""));
}

main();
