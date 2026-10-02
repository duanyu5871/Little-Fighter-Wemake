import { Transform } from "../../../../src/LFW/Transform";

import { numHex, readCaseLines, splitWs } from "./trace_util";

function line(...parts: (string | number | boolean)[]): string {
  return parts.map((p) => String(p)).join(" ");
}

function num(tok: string[], i: number): number | undefined {
  return i < tok.length ? Number(tok[i]) : undefined;
}

function arg(tok: string[], i: number): number {
  return Number(tok[i]);
}

function optsOf(tok: string[], i: number): { rate?: number } {
  return i < tok.length ? { rate: Number(tok[i]) } : {};
}

function check(tok: string[], lineno: number, lo: number, hi: number): boolean {
  if (tok.length >= lo && tok.length <= hi) return true;
  process.stderr.write(
    `line ${lineno}: op '${tok[0]}' expects ${lo}..${hi} tokens, got ${tok.length}\n`,
  );
  return false;
}

let t = new Transform();

function state(op: string): string {
  const d = t.d;
  return line(
    op,
    numHex(t.x),
    numHex(t.y),
    numHex(t.z),
    numHex(t.rotation),
    numHex(t.scale_x),
    numHex(t.scale_y),
    numHex(t.scale_z),
    numHex(d.x),
    numHex(d.y),
    numHex(d.z),
    numHex(d.rotation),
    numHex(d.scale_x),
    numHex(d.scale_y),
    numHex(d.scale_z),
    t.is_smoothing,
  );
}

function main(): void {
  const casePath = process.argv[2];
  if (!casePath) {
    process.stderr.write("usage: lfw_trace_transform.mjs <case-file>\n");
    process.exit(2);
  }

  const out: string[] = [];
  let lineno = 0;

  for (const raw of readCaseLines(casePath)) {
    ++lineno;
    const tok = splitWs(raw);
    if (tok.length === 0) continue;
    const op = tok[0]!;

    if (op === "new") {
      if (!check(tok, lineno, 1, 1)) process.exit(2);
      t = new Transform();
      out.push(state(op));
    } else if (op === "pos") {
      if (!check(tok, lineno, 1, 4)) process.exit(2);
      t.set_position(num(tok, 1), num(tok, 2), num(tok, 3));
      out.push(state(op));
    } else if (op === "scale") {
      if (!check(tok, lineno, 1, 4)) process.exit(2);
      t.set_scale(num(tok, 1), num(tok, 2), num(tok, 3));
      out.push(state(op));
    } else if (op === "rot") {
      if (!check(tok, lineno, 1, 2)) process.exit(2);
      t.set_rotation(num(tok, 1));
      out.push(state(op));
    } else if (op === "sx") {
      if (!check(tok, lineno, 2, 2)) process.exit(2);
      t.x = arg(tok, 1);
      out.push(state(op));
    } else if (op === "sy") {
      if (!check(tok, lineno, 2, 2)) process.exit(2);
      t.y = arg(tok, 1);
      out.push(state(op));
    } else if (op === "sz") {
      if (!check(tok, lineno, 2, 2)) process.exit(2);
      t.z = arg(tok, 1);
      out.push(state(op));
    } else if (op === "rx") {
      if (!check(tok, lineno, 2, 2)) process.exit(2);
      t.rotation = arg(tok, 1);
      out.push(state(op));
    } else if (op === "ssx") {
      if (!check(tok, lineno, 2, 2)) process.exit(2);
      t.scale_x = arg(tok, 1);
      out.push(state(op));
    } else if (op === "ssy") {
      if (!check(tok, lineno, 2, 2)) process.exit(2);
      t.scale_y = arg(tok, 1);
      out.push(state(op));
    } else if (op === "ssz") {
      if (!check(tok, lineno, 2, 2)) process.exit(2);
      t.scale_z = arg(tok, 1);
      out.push(state(op));
    } else if (op === "move") {
      if (!check(tok, lineno, 1, 5)) process.exit(2);
      t.move_to(num(tok, 1), num(tok, 2), num(tok, 3), optsOf(tok, 4));
      out.push(state(op));
    } else if (op === "scale_to") {
      if (!check(tok, lineno, 1, 5)) process.exit(2);
      t.scale_to(num(tok, 1), num(tok, 2), num(tok, 3), optsOf(tok, 4));
      out.push(state(op));
    } else if (op === "rotate_to") {
      if (!check(tok, lineno, 1, 3)) process.exit(2);
      t.rotate_to(num(tok, 1), optsOf(tok, 2));
      out.push(state(op));
    } else if (op === "update") {
      if (!check(tok, lineno, 1, 2)) process.exit(2);
      t.update(num(tok, 1));
      out.push(state(op));
    } else if (op === "arrived") {
      if (!check(tok, lineno, 1, 2)) process.exit(2);
      out.push(line(op, t.is_arrived(num(tok, 1))));
    } else if (op === "snap") {
      if (!check(tok, lineno, 1, 1)) process.exit(2);
      const s = t.snapshot();
      out.push(
        line(op, numHex(s.x), numHex(s.y), numHex(s.z), numHex(s.rotation), numHex(s.scale_x), numHex(s.scale_y), numHex(s.scale_z), t.is_smoothing),
      );
    } else {
      process.stderr.write(`line ${lineno}: unknown op '${op}'\n`);
      process.exit(2);
    }
  }

  process.stdout.write(out.join("\n") + (out.length ? "\n" : ""));
}

main();
