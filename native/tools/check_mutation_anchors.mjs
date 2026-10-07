#!/usr/bin/env node

// 变异档（native/tests/differential/mutations/*.mjs）里的 `from` 是逐字节锚定的：
// 每条必须在目标文件里恰好出现一次，否则 mutate.mjs 会直接中止。库代码一移动，
// 锚点就会悄悄失效，整份变异档随之变成跑不起来的死历史。这里在 lint 阶段把它们
// 全部对一遍，坏锚点立刻可见。

import { existsSync, readFileSync, readdirSync } from "node:fs";
import { dirname, join, relative, resolve } from "node:path";
import { fileURLToPath, pathToFileURL } from "node:url";

const HERE = dirname(fileURLToPath(import.meta.url));
const ROOT = resolve(HERE, "..", "..");
const MUTATIONS = join(ROOT, "native", "tests", "differential", "mutations");

const C = {
  red: (s) => `\x1b[31m${s}\x1b[0m`,
  green: (s) => `\x1b[32m${s}\x1b[0m`,
  dim: (s) => `\x1b[2m${s}\x1b[0m`,
  bold: (s) => `\x1b[1m${s}\x1b[0m`,
};

const specs = readdirSync(MUTATIONS)
  .filter((n) => n.endsWith(".mjs"))
  .sort();

const fileCache = new Map();
function readTarget(file) {
  const abs = resolve(ROOT, file);
  if (!fileCache.has(abs)) {
    fileCache.set(abs, existsSync(abs) ? readFileSync(abs, "utf8") : null);
  }
  return fileCache.get(abs);
}

let anchors = 0;
const bad = [];

for (const spec of specs) {
  const mod = (await import(pathToFileURL(join(MUTATIONS, spec)).href)).default;
  for (const m of mod.mutations ?? []) {
    ++anchors;
    const text = readTarget(m.file);
    const count = text === null ? -1 : text.split(m.from).length - 1;
    if (count !== 1) bad.push({ spec, count, note: m.note, file: m.file });
  }
}

console.log(C.bold(`checking mutation anchors in ${specs.length} specs under ${relative(process.cwd(), MUTATIONS)}`));
console.log("");

for (const b of bad) {
  console.log(`${C.red("x")} ${b.spec} count=${b.count} :: ${b.note}`);
}

console.log("");
const badSpecs = new Set(bad.map((b) => b.spec)).size;
console.log(
  `${C.green(String(anchors - bad.length))} ok / ` +
    `${bad.length ? C.red(String(bad.length)) : "0"} bad anchors ` +
    C.dim(`(of ${anchors}, ${badSpecs} spec(s) unrunnable)`),
);

process.exit(bad.length === 0 ? 0 : 1);
