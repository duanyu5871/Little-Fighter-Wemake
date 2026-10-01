#!/usr/bin/env node

import { readdirSync, readFileSync, statSync, writeFileSync } from "node:fs";
import { dirname, join, relative, resolve } from "node:path";
import { fileURLToPath } from "node:url";

const here = dirname(fileURLToPath(import.meta.url));
const root = resolve(here, "..", "..");
const src_root = resolve(root, "lf2s");
const out_path = resolve(root, "native/tests/differential/cases/json5/real_data.txt");
const tree_out_path = resolve(root, "native/tests/differential/cases/json5/real_data_tree.txt");

function walk(dir, out) {
  for (const name of readdirSync(dir).sort()) {
    const p = join(dir, name);
    if (statSync(p).isDirectory()) walk(p, out);
    else if (name.endsWith(".json5")) out.push(p);
  }
  return out;
}

function esc_case(s) {
  let out = '"';
  for (let i = 0; i < s.length; i++) {
    const c = s.charCodeAt(i);
    if (c === 0x22) out += '\\"';
    else if (c === 0x5c) out += "\\\\";
    else if (c >= 0x20 && c <= 0x7e) out += s[i];
    else out += "\\u" + c.toString(16).padStart(4, "0");
  }
  return out + '"';
}

const files = walk(src_root, []);
const flat = [];
const tree = [];
let bytes = 0;
let tree_bytes = 0;

for (const p of files) {
  const text = readFileSync(p, "utf8");
  const label = relative(root, p).split("\\").join("/");
  bytes += text.length;
  flat.push(`w5 ${label} ${esc_case(text)}`);
  if (text.length <= 4096) {
    tree_bytes += text.length;
    tree.push(`p5 ${label} ${esc_case(text)}`);
  }
}

writeFileSync(out_path, flat.join("\n") + "\n");
writeFileSync(tree_out_path, tree.join("\n") + "\n");
process.stdout.write(
  `flat: ${files.length} files, ${bytes} chars\n` +
    `tree: ${tree.length} files, ${tree_bytes} chars\n` +
    `-> ${relative(root, out_path)}\n`,
);
