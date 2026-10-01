import { bitsHex, esc, parseJsStringLiteral, parseValue, readCaseLines, splitWs, vtag } from "./trace_util";

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

function main(): void {
  const casePath = process.argv[2];
  if (!casePath) {
    process.stderr.write("usage: lfw_trace_json.mjs <case-file>\n");
    process.exit(2);
  }

  const out: string[] = [];
  let lineno = 0;

  for (const raw of readCaseLines(casePath)) {
    ++lineno;
    const tok = splitWs(raw);
    if (tok.length === 0) continue;
    const op = tok[0]!;

    if (op === "jstr") {
      const s = JSON.stringify(parseValue(tok, [1]));
      out.push(line(op, s === undefined ? "-" : esc(s)));
      continue;
    }

    if (op === "jparse") {
      const text = parseJsStringLiteral(tok[1]!);
      let v: unknown;
      let ok = true;
      try {
        v = JSON.parse(text);
      } catch {
        ok = false;
      }
      out.push(line(op, ok ? "ok" : "err"));
      if (ok) dump(v, 0, out);
      continue;
    }

    process.stderr.write(`line ${lineno}: unknown op '${op}'\n`);
    process.exit(2);
  }

  process.stdout.write(out.join("\n") + (out.length ? "\n" : ""));
}

main();
