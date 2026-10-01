import { CondMaker } from "../../../../src/LFW/dat_translator/CondMaker";

import { esc, parseValue, readCaseLines, splitWs } from "./trace_util";

const out: string[] = [];
const makers = new Map<string, CondMaker<never, never>>();
const errs = new Map<string, string>();

function makerOf(id: string, reset: boolean): CondMaker<never, never> {
  let m = makers.get(id);
  if (m === undefined || reset) {
    m = new CondMaker<never, never>();
    makers.set(id, m);
    errs.delete(id);
  }
  return m;
}

function guard(id: string, fn: (m: CondMaker<never, never>) => void): void {
  if (errs.has(id)) return;
  try {
    fn(makerOf(id, false));
  } catch (e) {
    errs.set(id, (e as Error).message);
  }
}

function opOf(tag: string): string {
  if (tag === "empty") return "";
  if (tag === "or") return "||";
  if (tag === "and") return "&&";
  if (tag === "not") return "!";
  return tag;
}

const OBJ_OPERAND = { a: 1 };
const ARR_OPERAND = [1, 2];

function boolOf(s: string): boolean {
  return s === "b1" || s === "1" || s === "true";
}

function main(): void {
  const casePath = process.argv[2];
  if (!casePath) {
    process.stderr.write("usage: lfw_trace_cond_maker.mjs <case-file>\n");
    process.exit(2);
  }

  for (const raw of readCaseLines(casePath)) {
    const t = splitWs(raw);
    if (t.length === 0) continue;
    const op = t[0]!;
    const i = [1];
    const next = (): string => t[i[0]!++]!;

    if (op === "mk") {
      makerOf(next(), true);
    } else if (op === "done") {
      const id = next();
      const e = errs.get(id);
      if (e !== undefined) out.push(`E ${id} ${esc(e)}`);
      else out.push(`D ${id} ${esc(makerOf(id, false).done())}`);
    } else if (op === "quote") {
      const id = next();
      const on = boolOf(next());
      const tag = next();
      const q = tag === "sq" ? "'" : tag === "bad" ? "@" : '"';
      guard(id, (m) => m.quote_strings(on, q as never));
    } else if (op === "term") {
      const id = next();
      const tag = next();
      if (tag === "wrap") {
        guard(id, (m) =>
          m.term_format((a, o, b) => `(${String(a)}${o}${String(b)})`),
        );
      }
    } else if (op === "add" || op === "or" || op === "and") {
      const id = next();
      const v1 = parseValue(t, i);
      const o = next();
      const v2 = parseValue(t, i);
      guard(id, (m) => {
        if (op === "add") m.add(v1 as never, o, v2 as never);
        else if (op === "or") m.or(v1 as never, o, v2 as never);
        else m.and(v1 as never, o, v2 as never);
      });
    } else if (op === "addtag" || op === "addobj" || op === "addarr") {
      const id = next();
      const o = opOf(next());
      guard(id, (m) => {
        if (op === "addtag") {
          const v1 = parseValue(t, i);
          const v2 = parseValue(t, i);
          m.add(v1 as never, o, v2 as never);
        } else if (op === "addobj") {
          const v2 = parseValue(t, i);
          m.add(OBJ_OPERAND as never, o, v2 as never);
        } else {
          const v2 = parseValue(t, i);
          m.add(ARR_OPERAND as never, o, v2 as never);
        }
      });
    } else if (op === "not" || op === "wrap") {
      const id = next();
      const v1 = parseValue(t, i);
      const o = next();
      const v2 = parseValue(t, i);
      guard(id, (m) => {
        const fn = (c: CondMaker<never, never>): void => {
          c.add(v1 as never, o, v2 as never);
        };
        if (op === "not") m.not(fn as never);
        else m.wrap(fn as never);
      });
    } else if (op === "nested") {
      const id = next();
      const a = parseValue(t, i);
      const oa = next();
      const b = parseValue(t, i);
      const c1 = parseValue(t, i);
      const ob = next();
      const d = parseValue(t, i);
      guard(id, (m) =>
        m.wrap((((c: CondMaker<never, never>): void => {
          c.add(a as never, oa, b as never);
          c.or(c1 as never, ob, d as never);
        }) as never)),
      );
    } else if (
      op === "oneof" ||
      op === "andoneof" ||
      op === "oroneof" ||
      op === "notin" ||
      op === "andnotin" ||
      op === "ornotin"
    ) {
      const id = next();
      const v1 = parseValue(t, i);
      const vs: unknown[] = [];
      while (i[0]! < t.length) vs.push(parseValue(t, i));
      if (vs.length === 0) {
        process.stderr.write(`${op} needs values\n`);
        process.exit(2);
      }
      guard(id, (m) => {
        if (op === "oneof") m.one_of(v1 as never, ...(vs as never[]));
        else if (op === "andoneof") m.and_one_of(v1 as never, ...(vs as never[]));
        else if (op === "oroneof") m.or_one_of(v1 as never, ...(vs as never[]));
        else if (op === "notin") m.not_in(v1 as never, ...(vs as never[]));
        else if (op === "andnotin") m.and_not_in(v1 as never, ...(vs as never[]));
        else m.or_not_in(v1 as never, ...(vs as never[]));
      });
    } else if (op === "oneof0" || op === "notin0") {
      const id = next();
      const v1 = parseValue(t, i);
      guard(id, (m) => {
        if (op === "oneof0") m.one_of(v1 as never);
        else m.not_in(v1 as never);
      });
    } else {
      process.stderr.write(`unknown op '${op}'\n`);
      process.exit(2);
    }
  }

  process.stdout.write(out.join("\n") + (out.length ? "\n" : ""));
}

main();
