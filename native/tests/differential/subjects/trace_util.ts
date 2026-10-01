import { readFileSync } from "node:fs";

import { round_float } from "../../../../src/LFW/utils/math/round_float";

const _buf = new ArrayBuffer(8);
const _dv = new DataView(_buf);

export function f64Bits(v: number): bigint {
  _dv.setFloat64(0, v, true);
  return _dv.getBigUint64(0, true);
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

export function splitWs(s: string): string[] {
  return s.trim().length ? s.trim().split(/\s+/) : [];
}

export function readCaseLines(path: string): string[] {
  return readFileSync(path, "utf8")
    .split(/\r?\n/)
    .map((l) => l.replace(/#.*$/, ""));
}

export function num(tok: string): number {
  return Number(tok);
}

export function flag(tok: string): boolean {
  return tok === "1" || tok === "true";
}
