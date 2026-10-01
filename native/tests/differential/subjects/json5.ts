import JSON5 from "json5";

import { bitsHex, esc, parseJsStringLiteral, readCaseLines, splitWs, vtag } from "./trace_util";

function line(...parts: (string | number)[]): string {
  return parts.map((p) => String(p)).join(" ");
}

function dump(v: unknown, depth: number, out: string[]): void {
  const parts: (string | number)[] = ["j", depth, vtag(v)];
  if (typeof v === "number") parts.push(Number.isNaN(v) ? "nan" : bitsHex(v));
  else if (typeof v === "boolean") parts.push(v ? "true" : "false");
  else if (typeof v === "string") parts.push(esc(v));
  else if (Array.isArray(v)) parts.push("-");
  else if (v !== null && typeof v === "object") parts.push(esc(Object.keys(v).join(",")));
  else parts.push("-");
  out.push(parts.map((p) => String(p)).join(" "));

  if (Array.isArray(v)) for (const item of v) dump(item, depth + 1, out);
  else if (v !== null && typeof v === "object") {
    for (const k of Object.keys(v)) dump((v as Record<string, unknown>)[k], depth + 1, out);
  }
}

function tryParse(text: string): { ok: boolean; value: unknown; message: string } {
  try {
    return { ok: true, value: JSON5.parse(text), message: "" };
  } catch (e) {
    return { ok: false, value: undefined, message: (e as Error).message };
  }
}

function main(): void {
  const casePath = process.argv[2];
  if (!casePath) {
    process.stderr.write("usage: lfw_trace_json5.mjs <case-file>\n");
    process.exit(2);
  }

  const out: string[] = [];
  let lineno = 0;

  for (const raw of readCaseLines(casePath)) {
    ++lineno;
    const tok = splitWs(raw);
    if (tok.length === 0) continue;
    const op = tok[0]!;

    if (op === "p5") {
      const labelled = tok.length >= 3;
      const label = labelled ? tok[1]! : "";
      const r = tryParse(parseJsStringLiteral(tok[labelled ? 2 : 1]!));
      const parts: (string | number)[] = [op];
      if (labelled) parts.push(label);
      if (r.ok) {
        parts.push("ok");
        out.push(parts.map((p) => String(p)).join(" "));
        dump(r.value, 0, out);
      } else {
        parts.push("err", esc(r.message));
        out.push(parts.map((p) => String(p)).join(" "));
      }
      continue;
    }

    if (op === "s5") {
      const r = tryParse(parseJsStringLiteral(tok[1]!));
      out.push(line(op, r.ok ? "ok" : esc(r.message)));
      continue;
    }

    if (op === "w5") {
      const labelled = tok.length >= 3;
      const label = labelled ? tok[1]! : "";
      const text = parseJsStringLiteral(tok[labelled ? 2 : 1]!);
      const r = tryParse(text);
      const parts: (string | number)[] = [op];
      if (labelled) parts.push(label);
      if (!r.ok) parts.push("perr", esc(r.message));
      else {
        try {
          parts.push("ok", esc(JSON5.stringify(r.value)));
        } catch (e) {
          parts.push("serr", esc((e as Error).message));
        }
      }
      out.push(parts.map((p) => String(p)).join(" "));
      continue;
    }

    if (op === "q5") {
      try {
        out.push(line(op, esc(JSON5.stringify(parseJsStringLiteral(tok[1]!)))));
      } catch (e) {
        out.push(line(op, esc((e as Error).message)));
      }
      continue;
    }

    if (op === "c5") {
      const holder: Record<string, unknown> = {};
      holder["self"] = holder;
      try {
        out.push(line(op, esc(JSON5.stringify(holder) as string)));
      } catch (e) {
        out.push(line(op, esc((e as Error).message)));
      }
      continue;
    }

    process.stderr.write(`line ${lineno}: unknown op '${op}'\n`);
    process.exit(2);
  }

  process.stdout.write(out.join("\n") + (out.length ? "\n" : ""));
}

main();
