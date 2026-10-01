import { Expression } from "../../../../src/LFW/base/Expression";
import { Ditto } from "../../../../src/LFW/ditto/Instance";

import { esc, parseValue, readCaseLines, splitWs, vtag } from "./trace_util";

type Ctx = number;

Ditto.warn = () => undefined;

const table = new Map<string, unknown>();
const exprs: Expression<Ctx>[] = [];

const getValGetter = (word: string) => {
  if (!table.has(word)) return undefined;
  return () => table.get(word);
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
      vtag(e.val_1),
      vtag(e.val_2),
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

    if (op === "b") {
      const e = new Expression<Ctx>(tok.slice(1).join(" "), getValGetter);
      exprs.push(e);
      out.push(line(op, exprs.length - 1));
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
