import { decode_lf2_dat } from "../../../../src/LFW/dat_translator/decode_lf2_dat";
import { float_scaling_entity } from "../../../../src/LFW/dat_translator/float_scaling_entity";
import { edit_bdy_info } from "../../../../src/LFW/dat_translator/edit_bdy_info";

import { esc, parseValue, readCaseLines, renderValue, splitWs } from "./trace_util";

const out: string[] = [];

function makeBuffer(size: number, seed: number): Uint8Array {
  const buf = new Uint8Array(size);
  for (let i = 0; i < size; i++) buf[i] = (i * 7 + seed) % 256;
  return buf;
}

async function main(): Promise<void> {
  const casePath = process.argv[2];
  if (!casePath) {
    process.stderr.write("usage: lfw_trace_translator_tail.mjs <case-file>\n");
    process.exit(2);
  }

  for (const raw of readCaseLines(casePath)) {
    const t = splitWs(raw);
    if (t.length === 0) continue;
    const op = t[0]!;

    if (op === "ei") {
      const idx = [1];
      const target = parseValue(t, idx);
      const edits: unknown[] = [];
      while (idx[0] < t.length) edits.push(parseValue(t, idx));
      if (target === null || typeof target !== "object" || Array.isArray(target)) {
        process.stderr.write("ei target is not an object\n");
        process.exit(2);
      }
      edit_bdy_info(target as never, ...(edits as never[]));
      out.push(`ei ${renderValue(target)}`);
      continue;
    }

    if (op === "fse") {
      const idx = [1];
      const data = parseValue(t, idx);
      if (idx[0] !== t.length) {
        process.stderr.write("fse trailing token(s)\n");
        process.exit(2);
      }
      out.push(`fse ${renderValue(float_scaling_entity(data as never))}`);
      continue;
    }

    if (op === "dlf2") {
      const size = Number(t[1]);
      const seed = Number(t[2]);
      const buf = makeBuffer(size, seed);
      const text = await decode_lf2_dat(buf.buffer as ArrayBuffer);
      out.push(`dlf2 ${esc(text)}`);
      continue;
    }

    process.stderr.write(`unknown op '${op}'\n`);
    process.exit(2);
  }

  process.stdout.write(out.join("\n") + (out.length ? "\n" : ""));
}

void main();
