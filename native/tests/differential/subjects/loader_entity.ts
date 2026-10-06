import { Ditto } from "../../../../src/LFW/ditto";
import { preprocess_entity_data } from "../../../../src/LFW/loader/preprocess_entity_data";

import { parseValue, readCaseLines, renderValue, splitWs } from "./trace_util";

// TS 里 `new Expression(...)` 遇到不认识的词会调 `Ditto.warn(...)`（harness 没装 sink），末尾
// `if (errors.length) Ditto.warn(errors)` 同理 —— 这是加载期副作用，不是数据。
(Ditto as unknown as Record<string, unknown>).error = () => undefined;
(Ditto as unknown as Record<string, unknown>).warn = () => undefined;

const out: string[] = [];

type Rec = Record<string, unknown>;

// `lfw` 只用来把加载任务塞进 `jobs`（端口整个不落地，见 DESIGN §66）。用例没写 `lfw` 的补一个
// 最小桩；写了就照用例给的用（`lfw u` / `lfw z` 会被 `const { images, sounds } = lfw` 抛掉）。
const lfwStub = {
  images: {
    load_img: () => undefined,
    load_by_pic_info: () => undefined,
  },
  sounds: { load: () => undefined, has: () => false },
};

const GEN_KEYS = [
  "__gen_x",
  "__gen_y",
  "__gen_z",
  "__gen_dvx",
  "__gen_dvy",
  "__gen_dvz",
  "__gen_spread_x",
  "__gen_spread_y",
  "__gen_spread_z",
  "__gen_facing",
];

// TS 把编译好的 `Expression` / `ValExpression` 挂在这些键上（`__tester` / `action.tester` /
// `__judger` / bot 动作的 `judger` / `opoint.__gen_*`），两端都不可比：端口存的是**源串**或不写。
// 剥掉后再比其余字段。`__pre_hitkeys_map` / `__post_hitkeys_map` 是 `Map`，它里面的帧和
// `data.<x>_hitkeys` 是同一批对象，也要走一遍。
function stripCompiled(v: unknown): void {
  if (Array.isArray(v)) {
    for (const item of v) stripCompiled(item);
    return;
  }
  if (v instanceof Map) {
    for (const item of v.values()) stripCompiled(item);
    return;
  }
  if (v === null || typeof v !== "object") return;
  const rec = v as Rec;
  delete rec.__tester;
  delete rec.__judger;
  delete rec.judger;
  delete rec.tester;
  for (const k of GEN_KEYS) delete rec[k];
  for (const k of Object.keys(rec)) stripCompiled(rec[k]);
}

// TS 的 TypeError 文本与端口不可比，只有 prefab 错误（`prefab_error(...)` 的 message，以 `[`
// 开头）才比较。
function messageOf(err: unknown): string {
  const msg = err instanceof Error ? err.message : String(err);
  return msg.startsWith("[") ? msg : "-";
}

async function main(): Promise<void> {
  const casePath = process.argv[2];
  if (!casePath) {
    process.stderr.write("usage: lfw_trace_loader_entity.mjs <case-file>\n");
    process.exit(2);
  }

  for (const raw of readCaseLines(casePath)) {
    const t = splitWs(raw);
    if (t.length === 0) continue;
    const op = t[0]!;
    const i = [1];

    if (op === "ed") {
      const ctx = parseValue(t, i);
      if (ctx !== null && typeof ctx === "object" && !("lfw" in (ctx as Rec)))
        (ctx as Rec).lfw = lfwStub;
      let ok = true;
      let msg = "-";
      try {
        await preprocess_entity_data(ctx as never);
      } catch (err) {
        ok = false;
        msg = messageOf(err);
      }
      const data = (ctx as Rec | null)?.data;
      // 末尾那句 `data.xml = () => …` 是函数（`renderValue` 打不出来），端口不落地。
      if (data !== null && typeof data === "object") delete (data as Rec).xml;
      stripCompiled(data);
      const parts = [`ed ${ok ? "ok" : "throw"}`, renderValue(data)];
      if (!ok) parts.push(`msg=${msg}`);
      out.push(parts.join(" "));
      continue;
    }

    process.stderr.write(`unknown op '${op}'\n`);
    process.exit(2);
  }

  process.stdout.write(out.join("\n") + (out.length ? "\n" : ""));
}

void main();
