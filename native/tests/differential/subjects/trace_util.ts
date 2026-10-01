import { readFileSync } from "node:fs";

import { round_float } from "../../../../src/LFW/utils/math/round_float";

const _buf = new ArrayBuffer(8);
const _dv = new DataView(_buf);

export function f64Bits(v: number): bigint {
  _dv.setFloat64(0, v, true);
  return _dv.getBigUint64(0, true);
}

export function f64FromBits(v: bigint): number {
  _dv.setBigUint64(0, v, true);
  return _dv.getFloat64(0, true);
}

export function hex16(v: bigint): string {
  return v.toString(16).padStart(16, "0");
}

export function bitsHex(d: number): string {
  return hex16(f64Bits(d));
}

export function qBits(d: number): string {
  return bitsHex(round_float(d));
}

export function numHex(d: number): string {
  return Number.isNaN(d) ? "nan" : bitsHex(d);
}

export function renderValue(v: unknown): string {
  if (v === undefined) return "u";
  if (v === null) return "z";
  if (typeof v === "boolean") return v ? "b1" : "b0";
  if (typeof v === "number") return "n" + String(v) + ":" + numHex(v);
  if (typeof v === "string") return "s" + esc(v);
  if (Array.isArray(v)) return "[" + v.map(renderValue).join(",") + "]";
  if (v instanceof Map) {
    const parts: string[] = [];
    for (const [k, val] of v) parts.push(esc(String(k)) + ":" + renderValue(val));
    return "{" + parts.join(",") + "}";
  }
  if (typeof v === "object") {
    const o = v as Record<string, unknown>;
    const parts: string[] = [];
    for (const k of Object.keys(o)) parts.push(esc(k) + ":" + renderValue(o[k]));
    return "{" + parts.join(",") + "}";
  }
  return "?";
}

export function parseJsStringLiteral(tok: string): string {
  let out = "";
  let i = 0;
  if (tok[i] === '"') i++;
  for (; i < tok.length; i++) {
    const c = tok[i]!;
    if (c === '"') break;
    if (c === "\\" && i + 1 < tok.length) {
      const e = tok[++i]!;
      if (e === "s") out += " ";
      else if (e === "t") out += "\t";
      else if (e === "n") out += "\n";
      else if (e === "r") out += "\r";
      else if (e === "\\") out += "\\";
      else if (e === '"') out += '"';
      else if (e === "u" && i + 4 < tok.length) {
        out += String.fromCharCode(parseInt(tok.slice(i + 1, i + 5), 16));
        i += 4;
      } else out += e;
    } else out += c;
  }
  return out;
}

export function splitWs(s: string): string[] {
  const out: string[] = [];
  const isWs = (c: string): boolean =>
    c === " " || c === "\t" || c === "\n" || c === "\r" || c === "\v" || c === "\f";
  let i = 0;
  while (i < s.length) {
    if (isWs(s[i]!)) {
      i++;
      continue;
    }
    let j = i;
    if (s[i] === '"') {
      j++;
      while (j < s.length && s[j] !== '"') {
        if (s[j] === "\\" && j + 1 < s.length) j++;
        j++;
      }
      if (j < s.length) j++;
    } else {
      while (j < s.length && !isWs(s[j]!)) j++;
    }
    out.push(s.slice(i, j));
    i = j;
  }
  return out;
}

export function stripComment(line: string): string {
  let quoted = false;
  for (let i = 0; i < line.length; i++) {
    const c = line[i]!;
    if (quoted) {
      if (c === "\\") i++;
      else if (c === '"') quoted = false;
      continue;
    }
    if (c === '"') quoted = true;
    else if (c === "#") return line.slice(0, i);
  }
  return line;
}

export function esc(s: string): string {
  let out = '"';
  for (let i = 0; i < s.length; i++) {
    const c = s.charCodeAt(i);
    if (c === 0x22) out += '\\"';
    else if (c === 0x5c) out += "\\\\";
    else if (c < 0x20 || c > 0x7e) out += "\\u" + c.toString(16).padStart(4, "0");
    else out += s.charAt(i);
  }
  return out + '"';
}

export function readCaseLines(path: string): string[] {
  return readFileSync(path, "utf8")
    .split(/\r?\n/)
    .map((l) => stripComment(l));
}

export function keyOf(tok: string): string {
  return tok.startsWith('"') ? parseJsStringLiteral(tok) : tok;
}

export function parseValue(tok: string[], idx: number[]): unknown {
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
    case "o": {
      const n = Number(tok[idx[0]!++]!);
      const obj: Record<string, unknown> = {};
      for (let j = 0; j < n; j++) {
        const key = keyOf(tok[idx[0]!++]!);
        obj[key] = parseValue(tok, idx);
      }
      return obj;
    }
    default:
      process.stderr.write(`bad value literal '${kind}'\n`);
      return process.exit(2);
  }
}

export function vtag(v: unknown): string {
  if (v === undefined) return "u";
  if (v === null) return "z";
  if (typeof v === "boolean") return "b";
  if (typeof v === "number") return "n";
  if (typeof v === "string") return "s";
  if (Array.isArray(v)) return "a" + v.length;
  if (typeof v === "object" && v !== null) return "o" + Object.keys(v).length;
  return "?";
}

export function num(tok: string): number {
  return Number(tok);
}

export function flag(tok: string): boolean {
  return tok === "1" || tok === "true";
}
