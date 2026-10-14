#!/usr/bin/env node
// 把「已解包的包」打成一个 `bundle.json`（索引）+ `bundle.bin`（字节），给浏览器里的
// `host.mjs` 用（W9：JS 先把条目索引 + 字节备好，解析留在 WASM）。
//
// 用法：
//   node native/web/make-bundle.mjs <outDir> [<name>=<dir> ...]
//     name 就是 MEMFS 里的顶层目录名（`/prel` 之类）；不给参数时默认
//     `builtin_data=dist/builtin_data prel=native/build/host-data/prel data=native/build/host-data/data`
//
// `bundleEntries()` 认这份索引；两条路（Node 读目录 / 浏览器读 bundle）灌进 MEMFS 的结果
// 逐字节相同 —— `smoke.mjs --bundle` 就是它的验收。
import { mkdirSync, readdirSync, readFileSync, statSync, writeFileSync } from "node:fs";
import { dirname, join, resolve } from "node:path";
import { fileURLToPath } from "node:url";

const HERE = dirname(fileURLToPath(import.meta.url));
const REPO = resolve(HERE, "..", "..");

const argv = process.argv.slice(2);
if (argv.length === 0 || argv[0] === "--help" || argv[0] === "-h") {
  console.log("usage: node native/web/make-bundle.mjs <outDir> [<name>=<dir> ...]");
  process.exit(0);
}

const outDir = resolve(argv[0]);
const specs =
  argv.length > 1
    ? argv.slice(1).map((s) => {
        const i = s.indexOf("=");
        return { name: s.slice(0, i), dir: resolve(s.slice(i + 1)) };
      })
    : [
        { name: "builtin_data", dir: join(REPO, "dist", "builtin_data") },
        { name: "prel", dir: join(REPO, "native", "build", "host-data", "prel") },
        { name: "data", dir: join(REPO, "native", "build", "host-data", "data") },
      ];

const entries = [];
const chunks = [];
let offset = 0;

function walk(dir, prefix) {
  for (const ent of readdirSync(dir, { withFileTypes: true })) {
    const abs = join(dir, ent.name);
    if (ent.isDirectory()) {
      walk(abs, `${prefix}/${ent.name}`);
      continue;
    }
    const bytes = readFileSync(abs);
    entries.push({ path: `${prefix}/${ent.name}`.replace(/^\/+/, ""), offset, size: bytes.length });
    chunks.push(bytes);
    offset += bytes.length;
  }
}

for (const spec of specs) {
  statSync(spec.dir); // 目录不存在就抛（宁可失败也不要静默的空包）
  walk(spec.dir, spec.name);
}

const index = {
  abi: 1,
  roots: specs.map((s) => s.name),
  bytes: offset,
  entries,
};

mkdirSync(outDir, { recursive: true });
writeFileSync(join(outDir, "bundle.json"), JSON.stringify(index));
writeFileSync(join(outDir, "bundle.bin"), Buffer.concat(chunks));

console.log(`[bundle] ${join(outDir, "bundle.json")} (${entries.length} entries)`);
console.log(`[bundle] ${join(outDir, "bundle.bin")} (${offset} bytes)`);
for (const spec of specs) {
  const n = entries.filter((e) => e.path.startsWith(`${spec.name}/`)).length;
  console.log(`[bundle]   ${spec.name} <- ${spec.dir} (${n} entries)`);
}
