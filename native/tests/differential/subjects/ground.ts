import { Ground } from "../../../../src/LFW/Ground";

import { numHex, readCaseLines, splitWs } from "./trace_util";

interface Seg {
  id?: string;
  name?: string;
  type: number;
  x1: number;
  x2: number;
  z1: number;
  z2: number;
  h1: number;
  h2: number;
}

const world = { bg: { data: { terrain: [] as Seg[] } } };
const terrain: Seg[] = world.bg.data.terrain;
const g = new Ground(world as never);

function line(...parts: (string | number | boolean)[]): string {
  return parts.map((p) => String(p)).join(" ");
}

function arg(tok: string[], i: number): number {
  return Number(tok[i]);
}

function opt(tok: string[], i: number): number | undefined {
  return i < tok.length ? Number(tok[i]) : undefined;
}

function check(tok: string[], lineno: number, lo: number, hi: number): boolean {
  if (tok.length >= lo && tok.length <= hi) return true;
  process.stderr.write(`line ${lineno}: op '${tok[0]}' expects ${lo}..${hi} tokens, got ${tok.length}\n`);
  return false;
}

function segParts(s: Seg): (string | number)[] {
  return [s.id ?? "-", s.type, numHex(s.x1), numHex(s.x2), numHex(s.z1), numHex(s.z2), numHex(s.h1), numHex(s.h2)];
}

function segOf(tok: string[]): Seg {
  const s: Seg = {
    type: arg(tok, 1),
    x1: arg(tok, 2),
    x2: arg(tok, 3),
    z1: arg(tok, 4),
    z2: arg(tok, 5),
    h1: arg(tok, 6),
    h2: arg(tok, 7),
  };
  if (tok.length > 8) s.id = tok[8];
  return s;
}

function index(tok: string[], lineno: number, size: number): number | undefined {
  const i = arg(tok, 1);
  if (!Number.isInteger(i) || i < 0 || i >= size) {
    process.stderr.write(`line ${lineno}: segment index ${i} out of range\n`);
    return undefined;
  }
  return i;
}

function main(): void {
  const casePath = process.argv[2];
  if (!casePath) {
    process.stderr.write("usage: lfw_trace_ground.mjs <case-file>\n");
    process.exit(2);
  }

  const out: string[] = [];
  let lineno = 0;

  for (const raw of readCaseLines(casePath)) {
    ++lineno;
    const tok = splitWs(raw);
    if (tok.length === 0) continue;
    const op = tok[0]!;

    if (op === "clear") {
      if (!check(tok, lineno, 1, 1)) process.exit(2);
      terrain.length = 0;
      out.push(line(op, terrain.length));
    } else if (op === "seg") {
      if (!check(tok, lineno, 8, 9)) process.exit(2);
      const s = segOf(tok);
      terrain.push(s);
      out.push(line(op, terrain.length - 1, ...segParts(s)));
    } else if (op === "base") {
      if (!check(tok, lineno, 1, 1)) process.exit(2);
      out.push(line(op, ...segParts(g.base as Seg)));
    } else if (op === "abyss") {
      if (!check(tok, lineno, 1, 1)) process.exit(2);
      out.push(line(op, ...segParts(Ground.abyss as Seg)));
    } else if (op === "y") {
      if (!check(tok, lineno, 4, 4)) process.exit(2);
      const i = index(tok, lineno, terrain.length);
      if (i === undefined) process.exit(2);
      out.push(line(op, numHex(g.y(terrain[i]!, arg(tok, 2), arg(tok, 3)))));
    } else if (op === "segment") {
      if (!check(tok, lineno, 3, 3)) process.exit(2);
      out.push(line(op, ...segParts(g.segment(arg(tok, 1), arg(tok, 2)) as Seg)));
    } else if (op === "enterable") {
      if (!check(tok, lineno, 5, 5)) process.exit(2);
      const i = index(tok, lineno, terrain.length);
      if (i === undefined) process.exit(2);
      const r = g.enterable(terrain[i]!, arg(tok, 2), arg(tok, 3), arg(tok, 4));
      out.push(line(op, r === null ? "-" : numHex(r)));
    } else if (op === "block") {
      if (!check(tok, lineno, 5, 8)) process.exit(2);
      const i = index(tok, lineno, terrain.length);
      if (i === undefined) process.exit(2);
      const r = g.block(terrain[i]!, arg(tok, 2), arg(tok, 3), arg(tok, 4), opt(tok, 5), opt(tok, 6), opt(tok, 7));
      const parts: (string | number)[] = [op, r.length];
      for (const p of r) parts.push(numHex(p.x), numHex(p.z));
      out.push(line(...parts));
    } else if (op === "intersect") {
      if (!check(tok, lineno, 7, 7)) process.exit(2);
      const r = g.intersect(arg(tok, 1), arg(tok, 2), arg(tok, 3), arg(tok, 4), arg(tok, 5), arg(tok, 6));
      out.push(line(op, numHex(r[0].x), numHex(r[0].y), numHex(r[0].z), ...segParts(r[1] as Seg)));
    } else if (op === "wall") {
      if (!check(tok, lineno, 7, 7)) process.exit(2);
      const r = g.intersect_wall(arg(tok, 1), arg(tok, 2), arg(tok, 3), arg(tok, 4), arg(tok, 5), arg(tok, 6));
      if (r === null) out.push(line(op, "-"));
      else out.push(line(op, numHex(r.x), numHex(r.z)));
    } else if (op === "step") {
      if (!check(tok, lineno, 1, 1)) process.exit(2);
      out.push(line(op, numHex(g.step)));
    } else {
      process.stderr.write(`line ${lineno}: unknown op '${op}'\n`);
      process.exit(2);
    }
  }

  process.stdout.write(out.join("\n") + (out.length ? "\n" : ""));
}

main();
