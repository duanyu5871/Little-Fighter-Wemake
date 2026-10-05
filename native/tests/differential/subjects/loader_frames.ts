import { preprocess_bdy } from "../../../../src/LFW/loader/preprocess_bdy";
import { preprocess_itr } from "../../../../src/LFW/loader/preprocess_itr";

import { Ditto } from "../../../../src/LFW/ditto";

import { parseValue, readCaseLines, renderValue, splitWs } from "./trace_util";

// TS 的 `new Expression(...)` 在两端词都不认识时会调 `Ditto.warn(...)`（未装 sink 时是
// “not a function”），harness 里换成空实现 —— 这是加载期副作用，不是数据。
(Ditto as unknown as Record<string, unknown>).error = () => undefined;
(Ditto as unknown as Record<string, unknown>).warn = () => undefined;

const out: string[] = [];

const lfwStub = {
  sounds: { has: () => false, load: () => undefined },
};

type Rec = Record<string, unknown>;

function probeOf(v: unknown, key: string): string {
  if (v === null || typeof v !== "object") return "-";
  const rec = v as Rec;
  if (!(key in rec)) return "-";
  return rec[key] ? "s" : "u";
}

function testerProbe(v: unknown): string {
  const top = probeOf(v, "__tester");
  const acts = (v as Rec | null)?.actions;
  if (!Array.isArray(acts)) return top;
  return [top, ...acts.map((a) => probeOf(a, "tester"))].join(",");
}

// TS 把编译好的 `Expression` 挂在这些键上（`__tester` / `action.tester`，`next_frame` 上还有
// `__judger`）。这些对象（内部含函数字段）两端都不可比：端口存的是**源串**。所以两边都先探
// “键在不在、值真不真”，再把键剥掉，只比较剩下的字段。见 PROTOCOL §6.9.108。
function stripCompiled(v: unknown): void {
  if (Array.isArray(v)) {
    for (const item of v) stripCompiled(item);
    return;
  }
  if (v === null || typeof v !== "object") return;
  const rec = v as Rec;
  delete rec.__tester;
  delete rec.__judger;
  delete rec.tester;
  for (const k of Object.keys(rec)) stripCompiled(rec[k]);
}

// TS 的 TypeError（`bdy` 不是对象、`actions` 不是数组……）文本与端口不可比，只有
// prefab 错误（`prefab_error(...)` 的 message，以 `[` 开头）才比较。
function messageOf(err: unknown): string {
  const msg = err instanceof Error ? err.message : String(err);
  return msg.startsWith("[") ? msg : "-";
}

function main(): void {
  const casePath = process.argv[2];
  if (!casePath) {
    process.stderr.write("usage: lfw_trace_loader_frames.mjs <case-file>\n");
    process.exit(2);
  }

  for (const raw of readCaseLines(casePath)) {
    const t = splitWs(raw);
    if (t.length === 0) continue;
    const op = t[0]!;
    const i = [1];

    if (op === "bdy" || op === "itr") {
      const ctx = parseValue(t, i) as Rec;
      // TS 的 ctx 还要 `lfw`（`A_SOUND` 动作会往 `jobs` 里塞加载任务）与 `jobs`。
      ctx.lfw = ctx.lfw ?? lfwStub;
      ctx.jobs = Array.isArray(ctx.jobs) ? ctx.jobs : [];
      let ok = true;
      let msg = "-";
      try {
        // TS 的调用点是 `l[i] = preprocess_bdy({ ...ctx, bdy: n })`，返回值要回写。
        if (op === "bdy") ctx.bdy = preprocess_bdy(ctx as never);
        else ctx.itr = preprocess_itr(ctx as never);
      } catch (err) {
        ok = false;
        msg = messageOf(err);
      }
      const probe = testerProbe(ctx[op]);
      stripCompiled(ctx);
      const parts = [`${op} ${ok ? "ok" : "throw"}`];
      parts.push(renderValue(ctx.data));
      if (op === "bdy") parts.push(renderValue(ctx.frame));
      parts.push(renderValue(ctx[op]));
      parts.push(`t=${probe}`);
      if (!ok) parts.push(`msg=${msg}`);
      out.push(parts.join(" "));
    } else {
      process.stderr.write(`unknown op '${op}'\n`);
      process.exit(2);
    }
  }

  process.stdout.write(out.join("\n") + (out.length ? "\n" : ""));
}

main();
