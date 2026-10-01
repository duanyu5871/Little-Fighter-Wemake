import { numHex, parseJsStringLiteral, readCaseLines, splitWs } from "./trace_util";

function line(...parts: (string | number)[]): string {
  return parts.map((p) => String(p)).join(" ");
}

function arg(tok: string[], i: number, fallback: number): number {
  return i < tok.length ? Number(tok[i]) : fallback;
}

function main(): void {
  const casePath = process.argv[2];
  if (!casePath) {
    process.stderr.write("usage: lfw_trace_core.mjs <case-file>\n");
    process.exit(2);
  }

  const out: string[] = [];
  let lineno = 0;

  for (const raw of readCaseLines(casePath)) {
    ++lineno;
    const tok = splitWs(raw);
    if (tok.length === 0) continue;
    const op = tok[0]!;

    switch (op) {
      case "to_number":
        out.push(line(op, numHex(Number(parseJsStringLiteral(tok[1]!)))));
        break;

      case "round":
        out.push(line(op, numHex(Math.round(arg(tok, 1, 0)))));
        break;

      case "floor":
        out.push(line(op, numHex(Math.floor(arg(tok, 1, 0)))));
        break;

      case "ceil":
        out.push(line(op, numHex(Math.ceil(arg(tok, 1, 0)))));
        break;

      case "abs":
        out.push(line(op, numHex(Math.abs(arg(tok, 1, 0)))));
        break;

      case "to_uint32":
        out.push(line(op, arg(tok, 1, 0) >>> 0));
        break;

      case "to_int32":
        out.push(line(op, arg(tok, 1, 0) | 0));
        break;

      case "bits_roundtrip": {
        const v = arg(tok, 1, 0);
        out.push(line(op, numHex(v)));
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
