import { readFileSync } from "node:fs";

import { MersenneTwister } from "../../../../src/LFW/utils/math/MersenneTwister";
import { bitsHex, f64Bits, qBits, splitWs } from "./trace_util";

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
        mt.reset(seed);
        out.push(`run ${bitsHex(seed)}`);
        break;
      }

      case "state": {
        const p = mt.pure();
        const h = new StateHash();
        h.u32(p.matrix);
        h.u32(p.upper_mask);
        h.u32(p.lower_mask);
        h.u32(p.index);
        h.f64(p.seed);
        h.u64(BigInt(p.times));
        for (let k = 0; k < p.mt.length; k++) h.u32(p.mt[k]!);
        out.push(`state ${h.hex()}`);
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
        out.push(`${op} ${v === undefined ? "-" : qBits(v)} ${arr.length}`);
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
