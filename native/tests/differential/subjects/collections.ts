import { Graves } from "../../../../src/LFW/base/Graves";
import { loop_arr } from "../../../../src/LFW/utils/array/loop_arr";
import { make_arr } from "../../../../src/LFW/utils/array/make_arr";
import { map_arr } from "../../../../src/LFW/utils/array/map_arr";
import { ensure } from "../../../../src/LFW/utils/container_help/ensure";
import { filter } from "../../../../src/LFW/utils/container_help/filter";
import { find, find_last, intersection } from "../../../../src/LFW/utils/container_help/find";
import { fisrt, last } from "../../../../src/LFW/utils/container_help/fisrt";
import { loop_offset } from "../../../../src/LFW/utils/container_help/loop_offset";
import { map_no_void } from "../../../../src/LFW/utils/container_help/map_no_void";
import { NestedMap } from "../../../../src/LFW/utils/container_help/nested_map";
import { NestedMultiMap } from "../../../../src/LFW/utils/container_help/nested_multi_map";
import type { Unsafe } from "../../../../src/LFW/utils/type_check/Unsafe";

import { bitsHex, parseValue, readCaseLines, renderValue, splitWs } from "./trace_util";

function line(...parts: (string | number | boolean)[]): string {
  return parts.map((p) => String(p)).join(" ");
}

function arg(tok: string[], i: number, fallback: number): number {
  return i < tok.length ? Number(tok[i]) : fallback;
}

function items(tok: string[], from: number, to: number): number[] {
  const out: number[] = [];
  for (let i = from; i < to; i++) out.push(Number(tok[i]));
  return out;
}

function barIndex(tok: string[]): number {
  for (let i = 1; i < tok.length; i++) if (tok[i] === "|") return i;
  return tok.length;
}

function emitList(op: string, v: number[]): string {
  return line(op, v.length, ...v.map(bitsHex));
}

function opt(v: number | undefined | null): string {
  return v === undefined || v === null ? "-" : bitsHex(v);
}

function main(): void {
  const casePath = process.argv[2];
  if (!casePath) {
    process.stderr.write("usage: lfw_trace_collections.mjs <case-file>\n");
    process.exit(2);
  }

  const out: string[] = [];
  let graves = new Graves<number>();
  const nested = new NestedMap<number, number, number>();
  const multi = new NestedMultiMap<number, number, number>();
  let lineno = 0;

  for (const raw of readCaseLines(casePath)) {
    ++lineno;
    const tok = splitWs(raw);
    if (tok.length === 0) continue;
    const op = tok[0]!;

    switch (op) {
      case "graves_new":
        graves = new Graves<number>();
        out.push(line(op, graves.l.length));
        break;

      case "graves_add":
        graves.add(arg(tok, 1, 0));
        out.push(line(op, graves.l.length));
        break;

      case "graves_take":
        out.push(line(op, opt(graves.take())));
        break;

      case "graves_l":
        out.push(line(op, graves.l.length, ...graves.l.map((v) => (v === undefined ? "-" : bitsHex(v)))));
        break;

      case "filter_gt": {
        const t = arg(tok, 1, 0);
        out.push(emitList(op, filter(items(tok, 2, tok.length), (v) => v > t)));
        break;
      }

      case "find_gt": {
        const t = arg(tok, 1, 0);
        out.push(line(op, opt(find(items(tok, 2, tok.length), (v) => v > t))));
        break;
      }

      case "find_last_gt": {
        const t = arg(tok, 1, 0);
        out.push(line(op, opt(find_last(items(tok, 2, tok.length), (v) => v > t))));
        break;
      }

      case "fisrt_gt": {
        const t = arg(tok, 1, 0);
        out.push(line(op, opt(fisrt(items(tok, 2, tok.length), (v) => (v > t ? v : undefined)))));
        break;
      }

      case "last_gt": {
        const t = arg(tok, 1, 0);
        out.push(line(op, opt(last(items(tok, 2, tok.length), (v) => (v > t ? v : undefined)))));
        break;
      }

      case "fisrt_any":
      case "last_any": {
        const a = items(tok, 1, tok.length);
        out.push(line(op, opt(op === "fisrt_any" ? fisrt(a) : last(a))));
        break;
      }

      case "map_no_void_gt": {
        const t = arg(tok, 1, 0);
        out.push(emitList(op, map_no_void(items(tok, 2, tok.length), (v) => (v > t ? v * 2 : undefined))));
        break;
      }

      case "intersection": {
        const b = barIndex(tok);
        out.push(emitList(op, intersection(items(tok, 1, b), items(tok, b + 1, tok.length))));
        break;
      }

      case "intersection_lt": {
        const b = barIndex(tok);
        out.push(emitList(op, intersection(items(tok, 1, b), items(tok, b + 1, tok.length), (x, y) => x < y)));
        break;
      }

      case "ensure": {
        const b = barIndex(tok);
        const existing = items(tok, 1, b);
        const fresh = items(tok, b + 1, tok.length);
        const target = existing.length ? existing : undefined;
        const r = ensure(target, fresh[0]!, ...fresh.slice(1));
        out.push(emitList(op, r));
        break;
      }

      case "ensure_val": {
        const b = barIndex(tok);
        const i1 = [1];
        const target = parseValue(tok, i1);
        const vals: unknown[] = [];
        while (i1[0]! < b) vals.push(parseValue(tok, i1));
        const i2 = [b + 1];
        while (i2[0]! < tok.length) vals.push(parseValue(tok, i2));
        if (vals.length === 0) {
          process.stderr.write("ensure_val needs at least one item\n");
          process.exit(2);
        }
        const r = ensure(target as Unsafe<unknown[]>, vals[0], ...vals.slice(1));
        out.push(line(op, renderValue(r)));
        break;
      }

      case "loop_offset": {
        const current = arg(tok, 1, 0);
        const offset = arg(tok, 2, 0);
        out.push(line(op, opt(loop_offset(items(tok, 3, tok.length), current, offset))));
        break;
      }

      case "make_arr":
        out.push(emitList(op, make_arr(arg(tok, 1, 0), (i) => i)));
        break;

      case "map_arr_mul": {
        const k = arg(tok, 1, 0);
        out.push(emitList(op, map_arr(items(tok, 2, tok.length), (v, i, arr) => v * k + i * 10 + arr.length)));
        break;
      }

      case "map_arr_scalar": {
        const k = arg(tok, 1, 0);
        out.push(emitList(op, map_arr(arg(tok, 2, 0), (v, i, arr) => v * k + i * 10 + arr.length)));
        break;
      }

      case "map_arr_nil": {
        const k = arg(tok, 1, 0);
        out.push(emitList(op, map_arr(null, (v: number, i: number, arr: number[]) => v * k + i * 10 + arr.length)));
        break;
      }

      case "loop_arr_idx":
      case "loop_arr_scalar": {
        const idxs: number[] = [];
        const fn = (_v: number, i: number, arr: number[]) => {
          idxs.push(i * 10 + arr.length);
        };
        if (op === "loop_arr_idx") loop_arr(items(tok, 1, tok.length), fn);
        else loop_arr(arg(tok, 1, 0), fn);
        out.push(line(op, idxs.length, ...idxs));
        break;
      }

      case "nested_set":
        nested.set(arg(tok, 1, 0), arg(tok, 2, 0), arg(tok, 3, 0));
        out.push(line(op, bitsHex(arg(tok, 3, 0))));
        break;

      case "nested_get":
        out.push(line(op, opt(nested.get(arg(tok, 1, 0), arg(tok, 2, 0)))));
        break;

      case "nested_has":
        out.push(line(op, nested.has(arg(tok, 1, 0), arg(tok, 2, 0))));
        break;

      case "nested_del":
        out.push(line(op, nested.delete(arg(tok, 1, 0), arg(tok, 2, 0))));
        break;

      case "nested_clear":
        nested.clear();
        out.push(line(op));
        break;

      case "nested_multi_add": {
        multi.add(arg(tok, 1, 0), arg(tok, 2, 0), arg(tok, 3, 0));
        out.push(line(op, multi.collect(arg(tok, 1, 0), arg(tok, 2, 0)).length));
        break;
      }

      case "nested_multi_first":
        out.push(line(op, opt(multi.first(arg(tok, 1, 0), arg(tok, 2, 0)))));
        break;

      case "nested_multi_has":
        out.push(line(op, multi.has(arg(tok, 1, 0), arg(tok, 2, 0))));
        break;

      case "nested_multi_collect":
        out.push(emitList(op, multi.collect(arg(tok, 1, 0), arg(tok, 2, 0))));
        break;

      case "nested_multi_del":
        out.push(line(op, multi.delete(arg(tok, 1, 0), arg(tok, 2, 0))));
        break;

      case "nested_multi_clear":
        multi.clear();
        out.push(line(op));
        break;

      default:
        process.stderr.write(`line ${lineno}: unknown op '${op}'\n`);
        process.exit(2);
    }
  }

  process.stdout.write(out.join("\n") + (out.length ? "\n" : ""));
}

main();
