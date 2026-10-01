import { preprocess_ball_frame } from "../../../../src/LFW/loader/preprocess_ball_frame";
import { preprocess_bg_data } from "../../../../src/LFW/loader/preprocess_bg_data";
import { prefab_error_message, resolve_prefab } from "../../../../src/LFW/loader/resolve_prefab";

import { Ditto } from "../../../../src/LFW/ditto";

import { parseValue, readCaseLines, renderValue, splitWs } from "./trace_util";

(Ditto as unknown as Record<string, unknown>).error = () => undefined;
(Ditto as unknown as Record<string, unknown>).warn = () => undefined;

const out: string[] = [];

const lfwStub = {
  images: { load_img: () => undefined, load_by_pic_info: () => undefined },
  sounds: { load: () => undefined },
};

function describe(r: ReturnType<typeof resolve_prefab>): string {
  if (r.ok) return `ok ${renderValue(r.value)}`;
  if (r.reason === "cycle") return `cycle ${r.chain.join(" -> ")}`;
  return `missing ${r.chain.join(" -> ")}`;
}

function main(): void {
  const casePath = process.argv[2];
  if (!casePath) {
    process.stderr.write("usage: lfw_trace_loader_more.mjs <case-file>\n");
    process.exit(2);
  }

  for (const raw of readCaseLines(casePath)) {
    const t = splitWs(raw);
    if (t.length === 0) continue;
    const op = t[0]!;
    const i = [1];
    const next = (): string => t[i[0]!++]!;

    if (op === "bf") {
      const ctx = parseValue(t, i) as never;
      preprocess_ball_frame(ctx);
      out.push(`bf ${renderValue(ctx)}`);
    } else if (op === "bg") {
      const data = parseValue(t, i) as never;
      out.push(`bg ${renderValue(preprocess_bg_data(lfwStub as never, data, []))}`);
    } else if (op === "rp") {
      const obj = parseValue(t, i) as never;
      const prefabs = parseValue(t, i) as never;
      out.push(`rp ${describe(resolve_prefab(obj as never, prefabs as never))}`);
    } else if (op === "rpm") {
      const tag = parseValue(t, i) as string;
      const who = parseValue(t, i) as string;
      const what = parseValue(t, i) as string;
      const obj = parseValue(t, i) as never;
      const prefabs = parseValue(t, i) as never;
      const r = resolve_prefab(obj as never, prefabs as never);
      if (r.ok) {
        out.push("rpm ");
      } else {
        out.push(`rpm ${prefab_error_message(tag, who, what, r as never)}`);
      }
    } else {
      process.stderr.write(`unknown op '${op}'\n`);
      process.exit(2);
    }
  }

  process.stdout.write(out.join("\n") + (out.length ? "\n" : ""));
}

main();
