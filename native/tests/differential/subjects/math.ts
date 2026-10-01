import { calc_plane } from "../../../../src/LFW/utils/math/calc_plane";
import { clamp } from "../../../../src/LFW/utils/math/clamp";
import { clamp_add } from "../../../../src/LFW/utils/math/clamp_add";
import { equal, eqgt, eqlt, float_equal } from "../../../../src/LFW/utils/math/float_equal";
import { line_plane_intersection } from "../../../../src/LFW/utils/math/line_plane_intersection";
import { normalize } from "../../../../src/LFW/utils/math/normalize";
import { normalize_plane } from "../../../../src/LFW/utils/math/normalize_plane";
import { probability } from "../../../../src/LFW/utils/math/probability";
import { project_to_line } from "../../../../src/LFW/utils/math/project_to_line";
import { range } from "../../../../src/LFW/utils/math/range";

import { bitsHex, qBits, readCaseLines, splitWs } from "./trace_util";

function line(...parts: (string | number | boolean)[]): string {
  return parts.map((p) => String(p)).join(" ");
}

function arg(tok: string[], i: number, fallback: number): number {
  return i < tok.length ? Number(tok[i]) : fallback;
}

function flag(tok: string[], i: number): boolean {
  return i < tok.length && (tok[i] === "1" || tok[i] === "true");
}

function main(): void {
  const casePath = process.argv[2];
  if (!casePath) {
    process.stderr.write("usage: lfw_trace_math.mjs <case-file>\n");
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
      case "clamp":
        out.push(line("clamp", bitsHex(clamp(arg(tok, 1, 0), arg(tok, 2, 0), arg(tok, 3, 0)))));
        break;

      case "clamp_add":
        out.push(line("clamp_add", bitsHex(clamp_add(arg(tok, 1, 0), arg(tok, 2, 0), arg(tok, 3, 0), arg(tok, 4, 0)))));
        break;

      case "normalize":
        out.push(line("normalize", bitsHex(normalize(arg(tok, 1, 0), arg(tok, 2, 1000)))));
        break;

      case "float_equal":
        out.push(line(op, float_equal(arg(tok, 1, 0), arg(tok, 2, 0))));
        break;

      case "equal":
        out.push(line(op, equal(arg(tok, 1, 0), arg(tok, 2, 0))));
        break;

      case "eqgt":
        out.push(line(op, eqgt(arg(tok, 1, 0), arg(tok, 2, 0))));
        break;

      case "eqlt":
        out.push(line(op, eqlt(arg(tok, 1, 0), arg(tok, 2, 0))));
        break;

      case "range": {
        try {
          const r = range(arg(tok, 1, 0), arg(tok, 2, 0), arg(tok, 3, 1));
          out.push(line("range", r.length, ...r.map(bitsHex)));
        } catch {
          out.push("range null");
        }
        break;
      }

      case "probability":
        out.push(line("probability", qBits(probability(arg(tok, 1, 0), arg(tok, 2, 0)))));
        break;

      case "normalize_plane": {
        const p = normalize_plane(arg(tok, 1, 0), arg(tok, 2, 0), arg(tok, 3, 0), arg(tok, 4, 0));
        out.push(line("normalize_plane", bitsHex(p.a), bitsHex(p.b), bitsHex(p.c), bitsHex(p.d)));
        break;
      }

      case "calc_plane": {
        const p = calc_plane(
          arg(tok, 1, 0), arg(tok, 2, 0), arg(tok, 3, 0),
          arg(tok, 4, 0), arg(tok, 5, 0), arg(tok, 6, 0),
          arg(tok, 7, 0), arg(tok, 8, 0), arg(tok, 9, 0));
        out.push(p
          ? line("calc_plane", bitsHex(p.a), bitsHex(p.b), bitsHex(p.c), bitsHex(p.d))
          : "calc_plane null");
        break;
      }

      case "line_plane": {
        const v = line_plane_intersection(
          arg(tok, 1, 0), arg(tok, 2, 0), arg(tok, 3, 0), arg(tok, 4, 0),
          arg(tok, 5, 0), arg(tok, 6, 0), arg(tok, 7, 0),
          arg(tok, 8, 0), arg(tok, 9, 0), arg(tok, 10, 0),
          flag(tok, 11), flag(tok, 12));
        out.push(v
          ? line("line_plane", bitsHex(v.x), bitsHex(v.y), bitsHex(v.z))
          : "line_plane null");
        break;
      }

      case "project_to_line": {
        try {
          const r = project_to_line(arg(tok, 1, 0), arg(tok, 2, 0), arg(tok, 3, 0), arg(tok, 4, 0));
          out.push(r ? line("project_to_line", bitsHex(r[0]), bitsHex(r[1])) : "project_to_line null");
        } catch {
          out.push("project_to_line null");
        }
        break;
      }

      case "alias_normalize_plane": {
        const r1 = normalize_plane(arg(tok, 1, 0), arg(tok, 2, 0), arg(tok, 3, 0), arg(tok, 4, 0));
        const r2 = normalize_plane(arg(tok, 5, 0), arg(tok, 6, 0), arg(tok, 7, 0), arg(tok, 8, 0));
        out.push(line("alias_normalize_plane",
          bitsHex(r1.a), bitsHex(r1.b), bitsHex(r1.c), bitsHex(r1.d),
          bitsHex(r2.a), bitsHex(r2.b), bitsHex(r2.c), bitsHex(r2.d)));
        break;
      }

      case "alias_calc_plane": {
        const r1 = calc_plane(
          arg(tok, 1, 0), arg(tok, 2, 0), arg(tok, 3, 0),
          arg(tok, 4, 0), arg(tok, 5, 0), arg(tok, 6, 0),
          arg(tok, 7, 0), arg(tok, 8, 0), arg(tok, 9, 0));
        const r2 = calc_plane(
          arg(tok, 10, 0), arg(tok, 11, 0), arg(tok, 12, 0),
          arg(tok, 13, 0), arg(tok, 14, 0), arg(tok, 15, 0),
          arg(tok, 16, 0), arg(tok, 17, 0), arg(tok, 18, 0));
        out.push(r1 && r2
          ? line("alias_calc_plane",
            bitsHex(r1.a), bitsHex(r1.b), bitsHex(r1.c), bitsHex(r1.d),
            bitsHex(r2.a), bitsHex(r2.b), bitsHex(r2.c), bitsHex(r2.d))
          : "alias_calc_plane null");
        break;
      }

      case "alias_line_plane": {
        const r1 = line_plane_intersection(
          arg(tok, 1, 0), arg(tok, 2, 0), arg(tok, 3, 0), arg(tok, 4, 0),
          arg(tok, 5, 0), arg(tok, 6, 0), arg(tok, 7, 0),
          arg(tok, 8, 0), arg(tok, 9, 0), arg(tok, 10, 0));
        const r2 = line_plane_intersection(
          arg(tok, 11, 0), arg(tok, 12, 0), arg(tok, 13, 0), arg(tok, 14, 0),
          arg(tok, 15, 0), arg(tok, 16, 0), arg(tok, 17, 0),
          arg(tok, 18, 0), arg(tok, 19, 0), arg(tok, 20, 0));
        out.push(r1 && r2
          ? line("alias_line_plane",
            bitsHex(r1.x), bitsHex(r1.y), bitsHex(r1.z),
            bitsHex(r2.x), bitsHex(r2.y), bitsHex(r2.z))
          : "alias_line_plane null");
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
