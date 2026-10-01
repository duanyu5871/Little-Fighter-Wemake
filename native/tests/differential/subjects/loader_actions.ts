import { preprocess_action } from "../../../../src/LFW/loader/preprocess_action";
import { preprocess_bot_data } from "../../../../src/LFW/loader/preprocess_bot_data";
import { preprocess_next_frame } from "../../../../src/LFW/loader/preprocess_next_frame";

import { parseValue, readCaseLines, renderValue, splitWs } from "./trace_util";

const out: string[] = [];

const lfwStub = {
  sounds: { has: () => false, load: () => undefined },
};

type Rec = Record<string, unknown>;

function dropJudger(v: unknown): void {
  if (Array.isArray(v)) {
    for (const item of v) dropJudger(item);
    return;
  }
  if (v === null || typeof v !== "object") return;
  delete (v as Rec).__judger;
}

function dropActionExpressionFields(action: unknown): void {
  if (action === null || typeof action !== "object") return;
  const rec = action as Rec;
  delete rec.tester;
  dropJudger(rec.data);
}

function main(): void {
  const casePath = process.argv[2];
  if (!casePath) {
    process.stderr.write("usage: lfw_trace_loader_actions.mjs <case-file>\n");
    process.exit(2);
  }

  for (const raw of readCaseLines(casePath)) {
    const t = splitWs(raw);
    if (t.length === 0) continue;
    const op = t[0]!;
    const i = [1];

    if (op === "bd") {
      const data = parseValue(t, i) as never;
      let ok = true;
      try {
        preprocess_bot_data(data);
      } catch {
        ok = false;
      }
      out.push(`${ok ? "bd ok" : "bd throw"} ${renderValue(data)}`);
    } else if (op === "pa") {
      const action = parseValue(t, i) as never;
      let ok = true;
      try {
        preprocess_action(lfwStub as never, action, []);
      } catch {
        ok = false;
      }
      dropActionExpressionFields(action);
      out.push(`${ok ? "pa ok" : "pa throw"} ${renderValue(action)}`);
    } else if (op === "pnf") {
      const nf = parseValue(t, i) as never;
      let ok = true;
      try {
        preprocess_next_frame(nf);
      } catch {
        ok = false;
      }
      dropJudger(nf);
      out.push(`${ok ? "pnf ok" : "pnf throw"} ${renderValue(nf)}`);
    } else {
      process.stderr.write(`unknown op '${op}'\n`);
      process.exit(2);
    }
  }

  process.stdout.write(out.join("\n") + (out.length ? "\n" : ""));
}

main();
