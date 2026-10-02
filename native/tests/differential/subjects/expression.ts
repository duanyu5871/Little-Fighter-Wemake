import { Expression } from "../../../../src/LFW/base/Expression";
import { Ditto } from "../../../../src/LFW/ditto/Instance";

import { esc, parseValue, readCaseLines, renderValue, splitWs } from "./trace_util";

type Ctx = number;

Ditto.warn = () => undefined;

const table = new Map<string, unknown>();
const exprs: Expression<Ctx>[] = [];
const log: string[] = [];

const getValGetter = (word: string) => {
  if (!table.has(word)) return undefined;
  return (_t: unknown, w: string) => {
    log.push(String(w));
    return table.get(w);
  };
};

function line(...parts: (string | number)[]): string {
  return parts.map((p) => String(p)).join(" ");
}

function walk(e: Expression<Ctx>, depth: number, out: string[]): void {
  out.push(
    line(
      "node",
      depth,
      e.before ? e.before : "-",
      e.not ? "true" : "false",
      e.children.length,
      e.op === undefined ? "-" : String(e.op),
      renderValue(e.val_1),
      renderValue(e.val_2),
      e.result === undefined ? "?" : String(e.result),
      esc(String(e.text)),
      e.err ? esc(String(e.err)) : "-",
    ),
  );
  for (const c of e.children) walk(c as unknown as Expression<Ctx>, depth + 1, out);
}

function main(): void {
  const casePath = process.argv[2];
  if (!casePath) {
    process.stderr.write("usage: lfw_trace_expression.mjs <case-file>\n");
    process.exit(2);
  }

  const out: string[] = [];
  let lineno = 0;

  for (const raw of readCaseLines(casePath)) {
    ++lineno;
    const tok = splitWs(raw);
    if (tok.length === 0) continue;
    const op = tok[0]!;

    if (op === "g") {
      table.set(tok[1]!, parseValue(tok, [2]));
      out.push(line(op, tok[1]!));
      continue;
    }

    if (op === "x") {
      table.delete(tok[1]!);
      out.push(line(op, tok[1]!));
      continue;
    }

    if (op === "clr") {
      log.length = 0;
      out.push(line(op));
      continue;
    }

    if (op === "log") {
      out.push(line("log", log.length, `[${log.map((w) => esc(w)).join(",")}]`));
      continue;
    }

    if (op === "b") {
      const e = new Expression<Ctx>(tok.slice(1).join(" "), getValGetter);
      exprs.push(e);
      out.push(line(op, exprs.length - 1));
      walk(e, 0, out);
      continue;
    }

    if (op === "d") {
      const idx = Number(tok[1]);
      const e = exprs[idx];
      if (!e) {
        process.stderr.write(`line ${lineno}: expression ${idx} out of range\n`);
        process.exit(2);
      }
      out.push(line(op, idx));
      walk(e, 0, out);
      continue;
    }

    if (op === "r") {
      const idx = Number(tok[1]);
      const e = exprs[idx];
      if (!e) {
        process.stderr.write(`line ${lineno}: expression ${idx} out of range\n`);
        process.exit(2);
      }
      out.push(line(op, tok[1]!, e.run(0) ? "true" : "false"));
      continue;
    }

    process.stderr.write(`line ${lineno}: unknown op '${op}'\n`);
    process.exit(2);
  }

  process.stdout.write(out.join("\n") + (out.length ? "\n" : ""));
}

main();
