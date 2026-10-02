// ts_scope —— 量某个 TS 入口的"值可达依赖"，按目录汇总，并标注 native/lfw 是否已有对应目录
//
// 用法：
//   node native/tools/ts_scope.mjs src/LFW/entity/Entity.ts src/LFW/World.ts
//   node native/tools/ts_scope.mjs --dir src/LFW/state
//
// 判定规则：`import type` 以及"全部具名都带 type 前缀"的 import 记为类型边；
// 只沿着非类型边递归 ⇒ 值闭包（=真正必须先存在的东西），全部边 ⇒ 全闭包。
import { readFileSync, readdirSync, existsSync } from "node:fs";
import { basename, dirname, resolve } from "node:path";

const PORTED_DIRS = readdirSync("native/lfw", { withFileTypes: true })
  .filter((d) => d.isDirectory())
  .map((d) => d.name);

const norm = (s) => s.replace(/\.(ts|cpp|h)$/, "").replace(/_/g, "").toLowerCase();
const PORTED_STEMS = new Set();
{
  const walk = (dir) => {
    for (const e of readdirSync(dir, { withFileTypes: true })) {
      const p = resolve(dir, e.name);
      if (e.isDirectory()) walk(p);
      else if (/\.(cpp|h)$/.test(e.name)) PORTED_STEMS.add(norm(e.name));
    }
  };
  walk("native/lfw");
}

function importsOf(file) {
  const text = readFileSync(file, "utf8");
  const out = [];
  const re = /import\s+(type\s+)?([^;]*?)\s*from\s*"([^"]+)"/g;
  let m;
  while ((m = re.exec(text))) {
    if (!m[3].startsWith(".")) continue;
    let isType = Boolean(m[1]);
    if (!isType) {
      const names = m[2].replace(/[{}]/g, "").split(",").map((s) => s.trim()).filter(Boolean);
      isType = names.length > 0 && names.every((n) => n.startsWith("type "));
    }
    const base = resolve(dirname(file), m[3]);
    const p = existsSync(base + ".ts") ? base + ".ts" : resolve(base, "index.ts");
    if (existsSync(p)) out.push({ p, isType });
  }
  return { out, lines: text.split("\n").length };
}

function closure(roots, followType) {
  const seen = new Set();
  const stack = [...roots];
  const res = [];
  while (stack.length) {
    const f = stack.pop();
    if (seen.has(f)) continue;
    seen.add(f);
    const { out, lines } = importsOf(f);
    res.push({ f, lines });
    for (const i of out) if (followType || !i.isType) stack.push(i.p);
  }
  return res;
}

const argv = process.argv.slice(2);
if (!argv.length) {
  process.stderr.write("usage: node native/tools/ts_scope.mjs <root.ts>... [--dir <dir>]\n");
  process.stderr.write("       node native/tools/ts_scope.mjs --rank <dir> [--top N]\n");
  process.exit(2);
}

const rankAt = argv.indexOf("--rank");
if (rankAt >= 0) {
  const dir = argv[rankAt + 1];
  const topAt = argv.indexOf("--top");
  const top = topAt >= 0 ? Number(argv[topAt + 1]) : 12;
  const rows = [];
  for (const f of readdirSync(dir).filter((x) => x.endsWith(".ts"))) {
    if (norm(f) === "index") continue;
    const root = resolve(dir, f);
    const clos = closure([root], false).filter((x) => x.f !== root);
    const news = clos.filter((x) => !PORTED_STEMS.has(norm(basename(x.f))));
    rows.push({
      f,
      own: importsOf(root).lines,
      newLines: news.reduce((s, x) => s + x.lines, 0),
      newFiles: news.length,
      allLines: clos.reduce((s, x) => s + x.lines, 0),
    });
  }
  rows.sort((a, b) => a.newLines - b.newLines);
  process.stdout.write(`未移植依赖排序（${dir}，值闭包，已排除 native/lfw 已有同名文件；含自身行数）\n`);
  for (const r of rows.slice(0, top)) {
    process.stdout.write(
      `  新 ${String(r.newLines).padStart(5)} 行 / ${String(r.newFiles).padStart(3)} 文件   自身 ${String(r.own).padStart(5)}   总闭包 ${String(r.allLines).padStart(5)}   ${r.f}\n`,
    );
  }
  process.exit(0);
}
const roots = [];
for (let i = 0; i < argv.length; i++) {
  if (argv[i] === "--dir") {
    const dir = argv[++i];
    for (const f of readdirSync(dir)) if (f.endsWith(".ts")) roots.push(resolve(dir, f));
  } else if (argv[i].endsWith(".ts")) {
    roots.push(resolve(argv[i]));
  }
}

const rel = (f) => f.replace(/\\/g, "/").replace(/^.*\/src\/LFW\//, "").replace(/^.*\/src\//, "src/");
const group = (f) => {
  const r = rel(f);
  return r.includes("/") ? r.slice(0, r.indexOf("/")) : "(root)";
};
const tally = (list) => {
  const m = new Map();
  for (const x of list) {
    const k = group(x.f);
    const cur = m.get(k) ?? { lines: 0, files: 0 };
    cur.lines += x.lines;
    cur.files += 1;
    m.set(k, cur);
  }
  return [...m].sort((a, b) => b[1].lines - a[1].lines);
};

const val = closure(roots, false);
const all = closure(roots, true);
const valSet = new Set(val.map((x) => x.f));
const rootsSet = new Set(roots);
const outVal = val.filter((x) => !rootsSet.has(x.f));
const typeOnly = all.filter((x) => !valSet.has(x.f) && !rootsSet.has(x.f));

const show = (title, list) => {
  process.stdout.write(`\n${title}：${list.length} 文件 / ${list.reduce((s, x) => s + x.lines, 0)} 行\n`);
  for (const [k, v] of tally(list)) {
    const flag = k === "(root)" ? "" : PORTED_DIRS.includes(k) ? "  [native/lfw 已有该目录]" : "  ← 全新";
    process.stdout.write(`  ${String(v.lines).padStart(6)}  ${String(v.files).padStart(3)} 文件  ${k}${flag}\n`);
  }
};
process.stdout.write(`入口 ${roots.length} 个：${roots.map(rel).join(" ")}\n`);
show("值闭包（真正必须先移植）", outVal);
show("仅类型闭包（只影响头文件声明）", typeOnly);
