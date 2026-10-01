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
  return s.trim().length ? s.trim().split(/\s+/) : [];
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
    .map((l) => l.replace(/#.*$/, ""));
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
  return "?";
}

export function num(tok: string): number {
  return Number(tok);
}

export function flag(tok: string): boolean {
  return tok === "1" || tok === "true";
}
