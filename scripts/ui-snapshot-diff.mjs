/**
 * UI 回归快照：归一化 + 比对。
 *
 *   node scripts/ui-snapshot-diff.mjs <scenario> [--update] [--tol=2]
 *
 *   current  : temp/ui-run.<scenario>.json
 *   baseline : scripts/ui-snapshots/<scenario>.json
 *
 * current.json 由应用内的 `ui_test_run("<scenario>")` 产出（见 src/ui_test.ts，
 * 在 `?headless_ui=1` 打开的页面里执行）。可用场景见 src/ui_test.ts 的 SCENARIOS。
 *
 * 归一化规则（去掉动画噪声，只留"会被人改动"的东西）：
 * - 只保留 visible && 有 id 的节点；
 * - 保留 id / parent / w / h / text / img / color / clip / x / y；
 * - x/y 比对时允许 ±tol（默认 2）的抖动，opacity 不参与比对（动画量）。
 *
 * 节点身份用 `parent>id#n`（n = 同一父节点下同名 id 的出现序），因为 id 会重复
 * （比如每个按钮下都有一个 back_label）。
 */
import { mkdirSync, readFileSync, writeFileSync } from "node:fs";
import path from "node:path";

const args = process.argv.slice(2);
const flags = new Map(
  args.filter((a) => a.startsWith("--") && a.includes("=")).map((a) => a.split("=")),
);
const positional = args.filter((a) => !a.startsWith("--"));
const scenario = positional[0] ?? "entry";
const current_path = `temp/ui-run.${scenario}.json`;
const baseline_path = `scripts/ui-snapshots/${scenario}.json`;
const tol = Number(flags.get("--tol") ?? 2);
const update = args.includes("--update");

if (!update) {
  try {
    readFileSync(current_path);
  } catch {
    console.error(`读不到 ${current_path}。先在 ?headless_ui=1 的页面里跑 ui_test_run("${scenario}")。`);
    process.exit(2);
  }
}

function normalize(run) {
  const steps = new Map();
  for (const step of run.steps ?? []) {
    const seen = new Map();
    const nodes = new Map();
    for (const n of step.nodes ?? []) {
      // 兼容两种输入：原始运行结果（IUISnapshotItem）与已归一化的基线（带 key）
      if (n.key) {
        nodes.set(n.key, {
          parent: n.parent,
          id: n.id,
          x: n.x,
          y: n.y,
          w: n.w,
          h: n.h,
          text: n.text,
          img: n.img,
          color: n.color,
          clip: !!n.clip,
        });
        continue;
      }
      if (!n.visible || !n.id) continue;
      const group = `${n.parent ?? "-"}>${n.id}`;
      const ord = seen.get(group) ?? 0;
      seen.set(group, ord + 1);
      nodes.set(`${group}#${ord}`, {
        parent: n.parent,
        id: n.id,
        x: Math.round(n.x),
        y: Math.round(n.y),
        w: Math.round(n.w),
        h: Math.round(n.h),
        text: n.text,
        img: n.img,
        color: n.color,
        clip: !!n.clip_children,
      });
    }
    steps.set(step.name, {
      page: step.page ?? null,
      settled: step.settled !== false,
      page_ok: step.page_ok !== false,
      error: step.error,
      nodes,
    });
  }
  return steps;
}

function diff_step(base, cur) {
  const lines = [];
  if (base.page !== cur.page) lines.push(`  ~ 页面: ${base.page} → ${cur.page}`);
  for (const [key, b] of base.nodes) {
    const c = cur.nodes.get(key);
    if (!c) {
      lines.push(`  - 消失: ${key}`);
      continue;
    }
    const changed = [];
    if (Math.abs(b.x - c.x) > tol || Math.abs(b.y - c.y) > tol)
      changed.push(`pos (${b.x},${b.y}) → (${c.x},${c.y})`);
    for (const f of ["w", "h", "text", "img", "color", "clip"]) {
      if (b[f] !== c[f]) changed.push(`${f}: ${JSON.stringify(b[f])} → ${JSON.stringify(c[f])}`);
    }
    if (changed.length) lines.push(`  ~ 变化: ${key}\n      ${changed.join("\n      ")}`);
  }
  for (const key of cur.nodes.keys()) {
    if (!base.nodes.has(key)) lines.push(`  + 新增: ${key}`);
  }
  return lines;
}

if (update) {
  const run = JSON.parse(readFileSync(current_path, "utf8"));
  const normalized = normalize(run);
  const out = {
    note: "由 scripts/ui-snapshot-diff.mjs --update 生成；改动 UI 后请重新生成并 review diff",
    steps: [...normalized].map(([name, v]) => ({
      name,
      page: v.page,
      nodes: [...v.nodes].map(([key, n]) => ({ key, ...n })),
    })),
  };
  mkdirSync(path.dirname(baseline_path), { recursive: true });
  writeFileSync(baseline_path, JSON.stringify(out, null, 1) + "\n");
  console.log(`基线已写入 ${baseline_path}（${out.steps.length} 个步骤）`);
  process.exit(0);
}

const base_run = JSON.parse(readFileSync(baseline_path, "utf8"));
const cur_run = JSON.parse(readFileSync(current_path, "utf8"));
const base = normalize(base_run);
const cur = normalize(cur_run);

console.log(`基线: ${baseline_path}`);
console.log(`当前: ${current_path}`);
console.log(`容差: x/y ±${tol}\n`);

let bad = 0;
for (const [name, cur_step] of cur) {
  const base_step = base.get(name);
  if (!base_step) {
    console.log(`步骤 ${name}  ✗ 基线里没有这个步骤`);
    ++bad;
    continue;
  }
  const lines = diff_step(base_step, cur_step);
  if (cur_step.error) lines.unshift(`  ! ${cur_step.error}`);
  if (!cur_step.page_ok) lines.unshift(`  ! 未切到期望页面（当前 ${cur_step.page}）`);
  if (!cur_step.settled) lines.unshift("  ! 未在超时内稳定（界面还在动）");
  if (!lines.length) {
    console.log(`步骤 ${name}  ✓ 一致（${cur_step.nodes.size} 个可见节点）`);
  } else {
    ++bad;
    console.log(`步骤 ${name}  ✗ ${lines.length} 处`);
    console.log(lines.join("\n"));
  }
}
for (const name of base.keys()) {
  if (!cur.has(name)) {
    console.log(`步骤 ${name}  ✗ 当前运行里缺失`);
    ++bad;
  }
}

console.log(`\n结果: ${cur.size - bad < 0 ? 0 : cur.size - bad}/${cur.size} 个步骤一致`);
if (bad) process.exitCode = 1;
