import { cross_bounding } from "../../../../src/LFW/utils/cross_bounding";
import { ease_in_out_quint } from "../../../../src/LFW/utils/easing/ease_in_out_quint";
import { ease_in_out_sine } from "../../../../src/LFW/utils/easing/ease_in_out_sine";
import { ease_linearity } from "../../../../src/LFW/utils/easing/ease_linearity";
import { Times } from "../../../../src/LFW/utils/Times";
import { decodeUTF8, encodeUTF8 } from "../../../../src/LFW/utils/utf8";

import { bitsHex, qBits, readCaseLines, splitWs } from "./trace_util";

function line(...parts: (string | number | boolean)[]): string {
  return parts.map((p) => String(p)).join(" ");
}

function arg(tok: string[], i: number, fallback: number): number {
  return i < tok.length ? Number(tok[i]) : fallback;
}

function main(): void {
  const casePath = process.argv[2];
  if (!casePath) {
    process.stderr.write("usage: lfw_trace_utils.mjs <case-file>\n");
    process.exit(2);
  }

  const out: string[] = [];
  let times = new Times();
  let lineno = 0;

  const state = (op: string) =>
    line(op, bitsHex(times.value), bitsHex(times.min), bitsHex(times.max), bitsHex(times.lifes), bitsHex(times.remains));

  for (const raw of readCaseLines(casePath)) {
    ++lineno;
    const tok = splitWs(raw);
    if (tok.length === 0) continue;
    const op = tok[0]!;

    switch (op) {
      case "ease_linearity":
        out.push(line(op, bitsHex(ease_linearity(arg(tok, 1, 0), arg(tok, 2, 0), arg(tok, 3, 1)))));
        break;

      case "ease_linearity_backward":
        out.push(line(op, bitsHex(ease_linearity.backward(arg(tok, 1, 0), arg(tok, 2, 0), arg(tok, 3, 1)))));
        break;

      case "ease_in_out_sine":
        out.push(line(op, qBits(ease_in_out_sine(arg(tok, 1, 0), arg(tok, 2, 0), arg(tok, 3, 1)))));
        break;

      case "ease_in_out_sine_backward":
        out.push(line(op, qBits(ease_in_out_sine.backward(arg(tok, 1, 0), arg(tok, 2, 0), arg(tok, 3, 1)))));
        break;

      case "ease_in_out_quint":
        out.push(line(op, qBits(ease_in_out_quint(arg(tok, 1, 0), arg(tok, 2, 0), arg(tok, 3, 1)))));
        break;

      case "ease_in_out_quint_backward":
        out.push(line(op, qBits(ease_in_out_quint.backward(arg(tok, 1, 0), arg(tok, 2, 0), arg(tok, 3, 1)))));
        break;

      case "cross_bounding": {
        const r = cross_bounding(
          { left: arg(tok, 1, 0), right: arg(tok, 2, 0), top: arg(tok, 3, 0), bottom: arg(tok, 4, 0), near: arg(tok, 5, 0), far: arg(tok, 6, 0) },
          { left: arg(tok, 7, 0), right: arg(tok, 8, 0), top: arg(tok, 9, 0), bottom: arg(tok, 10, 0), near: arg(tok, 11, 0), far: arg(tok, 12, 0) });
        out.push(line(op, bitsHex(r.left), bitsHex(r.right), bitsHex(r.top), bitsHex(r.bottom), bitsHex(r.near), bitsHex(r.far)));
        break;
      }

      case "utf8_encode": {
        let s = "";
        for (let i = 1; i < tok.length; i++) s += String.fromCharCode(parseInt(tok[i]!, 16));
        const bytes = encodeUTF8(s);
        out.push(line(op, bytes.length, ...Array.from(bytes)));
        break;
      }

      case "utf8_decode": {
        const bytes = new Uint8Array(tok.slice(1).map((t) => parseInt(t, 16)));
        const s = decodeUTF8(bytes);
        const codes: number[] = [];
        for (let i = 0; i < s.length; i++) codes.push(s.charCodeAt(i));
        out.push(line(op, codes.length, ...codes));
        break;
      }

      case "times_new":
        times = new Times(arg(tok, 1, 0), arg(tok, 2, 9007199254740991));
        out.push(state(op));
        break;

      case "times_set_range":
        times.set_range(arg(tok, 1, 0), arg(tok, 2, 0));
        out.push(state(op));
        break;

      case "times_set_lifes":
        times.set_lifes(arg(tok, 1, -1));
        out.push(state(op));
        break;

      case "times_set_min":
        times.min = arg(tok, 1, 0);
        out.push(state(op));
        break;

      case "times_set_max":
        times.max = arg(tok, 1, 0);
        out.push(state(op));
        break;

      case "times_set_value":
        times.value = arg(tok, 1, 0);
        out.push(state(op));
        break;

      case "times_reset":
        times.reset();
        out.push(state(op));
        break;

      case "times_reborn":
        times.reborn();
        out.push(state(op));
        break;

      case "times_state":
        out.push(state(op));
        break;

      case "times_add": {
        const r = times.add(arg(tok, 1, 1));
        out.push(line(op, r, bitsHex(times.value), bitsHex(times.min), bitsHex(times.max), bitsHex(times.lifes), bitsHex(times.remains)));
        break;
      }

      case "times_write_nums": {
        const nums = [0, 0, 0, 0, 0];
        times.write_nums(nums, 0);
        out.push(line(op, ...nums.map(bitsHex)));
        break;
      }

      case "times_read_nums": {
        const nums = [0, 1, 2, 3, 4].map((i) => arg(tok, i + 1, 0));
        times.read_nums(nums, 0);
        out.push(state(op));
        break;
      }

      case "times_snapshot":
        out.push(line(op, ...times.to_snapshot().nums.map(bitsHex)));
        break;

      case "times_read_snapshot": {
        const nums = [0, 1, 2, 3, 4].map((i) => arg(tok, i + 1, 0));
        times.read_snapshot({ nums });
        out.push(state(op));
        break;
      }

      default:
        process.stderr.write(`line ${lineno}: unknown op '${op}'\n`);
        process.exit(2);
    }
  }

  process.stdout.write(out.join("\n") + (out.length ? "\n" : ""));
}

main();
