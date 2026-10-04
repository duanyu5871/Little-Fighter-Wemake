import { readFileSync } from "node:fs";

import { mt_cases } from "../../../../src/LFW/cases_instances";
import { MersenneTwister } from "../../../../src/LFW/utils/math/MersenneTwister";
import { bitsHex, esc, f64Bits, hex16, keyOf, parseValue, qBits, renderValue, splitWs } from "./trace_util";

const FNV_OFFSET = 0xcbf29ce484222325n;
const FNV_PRIME = 0x100000001b3n;
const MASK64 = 0xffffffffffffffffn;

class StateHash {
  private h = FNV_OFFSET;

  private u8(v: number): void {
    this.h ^= BigInt(v & 0xff);
    this.h = (this.h * FNV_PRIME) & MASK64;
  }

  u32(v: number): void {
    const x = v >>> 0;
    for (let i = 0; i < 4; i++) this.u8((x >>> (8 * i)) & 0xff);
  }

  u64(v: bigint): void {
    for (let i = 0; i < 8; i++) this.u8(Number((v >> BigInt(8 * i)) & 0xffn));
  }

  f64(v: number): void {
    this.u64(f64Bits(v));
  }

  hex(): string {
    return this.h.toString(16).padStart(16, "0");
  }
}

function stateHashHex(mt: MersenneTwister): string {
  const p = mt.pure();
  const h = new StateHash();
  h.u32(p.matrix);
  h.u32(p.upper_mask);
  h.u32(p.lower_mask);
  h.u32(p.index);
  h.f64(p.seed);
  h.u64(BigInt(p.times));
  for (let k = 0; k < p.mt.length; k++) h.u32(p.mt[k]!);
  return h.hex();
}

function main(): void {
  const casePath = process.argv[2];
  if (!casePath) {
    process.stderr.write("usage: mt_trace_ts.mjs <case-file>\n");
    process.exit(2);
  }

  const lines = readFileSync(casePath, "utf8").split(/\r?\n/);
  const mt = new MersenneTwister(0);
  const out: string[] = [];

  for (let i = 0; i < lines.length; i++) {
    const tok = splitWs(lines[i]!.replace(/#.*$/, ""));
    if (tok.length === 0) continue;

    const op = tok[0];

    switch (op) {
      case "seed": {
        const seed = Number(tok[1]);
        const dbg = tok[2] === "d";
        mt.reset(seed, dbg);
        out.push(`run ${bitsHex(seed)}${dbg ? " debug=1" : ""}`);
        break;
      }

      case "state": {
        out.push(`state ${stateHashHex(mt)}`);
        break;
      }

      case "mark": {
        mt.mark = keyOf(tok[1]!);
        out.push(`mark ${esc(mt.mark)}`);
        break;
      }

      case "debug": {
        mt.debugging = tok[1] === "1";
        out.push(`debug ${mt.debugging ? 1 : 0}`);
        break;
      }

      case "case": {
        const idx = [1];
        const args: unknown[] = [];
        while (idx[0]! < tok.length) args.push(parseValue(tok, idx));
        mt.case(...args);
        out.push(`case ${args.length}`);
        break;
      }

      case "cases": {
        const text = mt_cases.submit();
        out.push(`cases ${esc(text)} ${mt_cases.cases.length}`);
        break;
      }

      case "cinfo": {
        out.push(`cinfo ${esc(mt_cases.name)} ${esc(mt_cases.separator)}`);
        break;
      }

      case "creset": {
        mt_cases.reset();
        out.push(`creset ${mt_cases.cases.length}`);
        break;
      }

      case "pure": {
        const p = mt.pure();
        // `twist()` 之后 `_mt` 里的词是 JS 位运算的 **signed int32** 值（`int()` 只在返回时 `>>> 0`），
        // 端口用 uint32 存同一批位模式 ⇒ 这里统一 `>>> 0` 后再比。
        out.push(
          `pure ${hex16(BigInt(p.matrix))} ${hex16(BigInt(p.upper_mask))} ${hex16(BigInt(p.lower_mask))}` +
            ` ${p.index} ${bitsHex(p.seed)} ${p.times} ${p.mt.length}` +
            ` ${hex16(BigInt(p.mt[0]! >>> 0))} ${hex16(BigInt(p.mt[1]! >>> 0))} ${hex16(BigInt(p.mt[623]! >>> 0))}` +
            ` ${esc(p.mark)}`,
        );
        break;
      }

      case "load": {
        const field = tok[1]!;
        const info = mt.pure();
        const v = tok.length > 2 ? Number(tok[2]) : 0;
        if (field === "matrix") info.matrix = v;
        else if (field === "upper") info.upper_mask = v;
        else if (field === "lower") info.lower_mask = v;
        else if (field === "index") info.index = v;
        else if (field === "seed") info.seed = v;
        else if (field === "times") info.times = v;
        else if (field === "mt") info.mt[Number(tok[2])] = Number(tok[3]);
        else if (field === "mark") info.mark = keyOf(tok[2]!);
        else {
          process.stderr.write(`line ${i + 1}: unknown load field '${field}'\n`);
          process.exit(2);
        }
        mt.load(info);
        out.push(`load ${tok.slice(1).join(" ")} ${stateHashHex(mt)} ${esc(mt.mark)}`);
        break;
      }

      case "pickv":
      case "takev": {
        const idx = [1];
        const a = parseValue(tok, idx);
        const v = op === "takev" ? mt.take(a as unknown[]) : mt.pick(a as unknown[]);
        out.push(`${op} ${renderValue(v)} ${renderValue(a)}`);
        break;
      }

      case "int": {
        const count = Number(tok[1]);
        for (let k = 0; k < count; k++) out.push(`int ${mt.int() >>> 0}`);
        break;
      }

      case "float": {
        const count = Number(tok[1]);
        for (let k = 0; k < count; k++) out.push(`float ${qBits(mt.float())}`);
        break;
      }

      case "range": {
        const min = Number(tok[1]);
        const max = Number(tok[2]);
        const count = Number(tok[3]);
        for (let k = 0; k < count; k++) {
          out.push(`range ${bitsHex(min)} ${bitsHex(max)} ${qBits(mt.range(min, max))}`);
        }
        break;
      }

      case "pick":
      case "take": {
        const arr: number[] = tok.slice(1).map(Number);
        const v = op === "take" ? mt.take(arr) : mt.pick(arr);
        const rest = arr.map((x) => qBits(x)).join(" ");
        out.push(`${op} ${v === undefined ? "-" : qBits(v)} ${arr.length}${rest ? " " + rest : ""}`);
        break;
      }

      default:
        process.stderr.write(`line ${i + 1}: unknown op '${op}'\n`);
        process.exit(2);
    }
  }

  process.stdout.write(out.join("\n") + (out.length ? "\n" : ""));
}

main();
