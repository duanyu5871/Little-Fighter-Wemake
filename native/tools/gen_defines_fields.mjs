#!/usr/bin/env node

import { execFileSync } from "node:child_process";
import { mkdirSync, readdirSync, readFileSync, statSync, writeFileSync } from "node:fs";
import { createRequire } from "node:module";
import { dirname, join, relative, resolve } from "node:path";
import { fileURLToPath } from "node:url";

const here = dirname(fileURLToPath(import.meta.url));
const root = resolve(here, "..", "..");
const src_root = resolve(root, "src/LFW/defines");
const gen_dir = resolve(root, "native/build/gen");
const out_header = resolve(root, "native/lfw/defines/fields_gen.h");
const out_ts = resolve(root, "native/tests/differential/subjects/gen/defines_fields.ts");

const REL_FROM_GEN = relative(gen_dir, src_root).split("\\").join("/");
const REL_FROM_SUBJECT_GEN = relative(
  resolve(root, "native/tests/differential/subjects/gen"),
  src_root,
)
  .split("\\")
  .join("/");

function walk(dir, out) {
  for (const n of readdirSync(dir).sort()) {
    const p = join(dir, n);
    if (statSync(p).isDirectory()) walk(p, out);
    else if (n.endsWith(".ts")) out.push(p);
  }
  return out;
}

const modules = walk(src_root, []).filter((f) => {
  const base = f.split(/[\\/]/).pop();
  if (base === "index.ts") return false;
  return readFileSync(f, "utf8").includes("_fields");
});

mkdirSync(gen_dir, { recursive: true });
mkdirSync(dirname(out_ts), { recursive: true });

const modPath = (f) => relative(src_root, f).replace(/\.ts$/, "").split("\\").join("/");

const tsLines = [];
tsLines.push("const mods: [string, Record<string, unknown>][] = [");
for (const f of modules) {
  tsLines.push(`  [${JSON.stringify(modPath(f))}, M_${modPath(f).replace(/[^A-Za-z0-9]/g, "_")}],`);
}
tsLines.push("];");
tsLines.push("");
tsLines.push("function plain(v: unknown): unknown {");
tsLines.push("  if (v instanceof Map) {");
tsLines.push("    const o: Record<string, unknown> = {};");
tsLines.push("    for (const [k, x] of v) o[String(k)] = plain(x);");
tsLines.push("    return o;");
tsLines.push("  }");
tsLines.push("  if (Array.isArray(v)) return v.map(plain);");
tsLines.push("  if (v !== null && typeof v === 'object') {");
tsLines.push("    const src = v as Record<string, unknown>;");
tsLines.push("    const o: Record<string, unknown> = {};");
tsLines.push("    for (const k of Object.keys(src)) o[k] = plain(src[k]);");
tsLines.push("    return o;");
tsLines.push("  }");
tsLines.push("  return v;");
tsLines.push("}");
tsLines.push("");
tsLines.push("const out: { name: string; value: unknown; module: string }[] = [];");
tsLines.push("for (const [path, m] of mods) {");
tsLines.push("  for (const k of Object.keys(m).sort()) {");
tsLines.push("    if (!k.endsWith('_fields')) continue;");
tsLines.push("    out.push({ name: k, value: plain(m[k]), module: path });");
tsLines.push("  }");
tsLines.push("}");
tsLines.push("console.log(JSON.stringify(out));");

const entryLines = [];
for (const f of modules) {
  entryLines.push(
    `import * as M_${modPath(f).replace(/[^A-Za-z0-9]/g, "_")} from "${REL_FROM_GEN}/${modPath(f)}";`,
  );
}
entryLines.push(...tsLines);

const entry = join(gen_dir, "defines_fields_dump.ts");
writeFileSync(entry, entryLines.join("\n"));

const requireFromLfw = createRequire(join(root, "src", "LFW", "package.json"));
const esbuild = requireFromLfw("esbuild");
const bundle = join(gen_dir, "defines_fields_dump.mjs");
esbuild.buildSync({
  entryPoints: [entry],
  bundle: true,
  format: "esm",
  platform: "node",
  target: "node20",
  outfile: bundle,
  logLevel: "warning",
});

const json = execFileSync(process.execPath, [bundle], { encoding: "utf8", maxBuffer: 1 << 28 });
const tables = JSON.parse(json);

function cxxU16(text) {
  let out = 'u"';
  for (const ch of text) {
    const u = ch.codePointAt(0);
    if (u === 0x22) out += '\\"';
    else if (u === 0x5c) out += "\\\\";
    else if (u < 0x20) out += "\\u" + u.toString(16).padStart(4, "0");
    else if (u <= 0x7e) out += ch;
    else if (u <= 0xffff) out += "\\u" + u.toString(16).padStart(4, "0");
    else {
      const v = u - 0x10000;
      out += "\\u" + (0xd800 + (v >> 10)).toString(16).padStart(4, "0");
      out += "\\u" + (0xdc00 + (v & 0x3ff)).toString(16).padStart(4, "0");
    }
  }
  return out + '"';
}

function rawU16(text, delim) {
  let d = delim;
  while (text.includes(`)${d}"`)) d += "x";
  return `uR"${d}(${text})${d}"`;
}

const CHUNK = 8000;

function chunkText(s) {
  const parts = [];
  let i = 0;
  while (i < s.length) {
    let end = Math.min(i + CHUNK, s.length);
    const c = s.charCodeAt(end - 1);
    if (c >= 0xd800 && c <= 0xdbff && end < s.length) end -= 1;
    parts.push(s.slice(i, end));
    i = end;
  }
  return parts;
}

function rawLiteralFor(text) {
  let d = "J";
  while (text.includes(`)${d}"`)) d += "x";
  return chunkText(text)
    .map((c) => `uR"${d}(${c})${d}"`)
    .join("\n      ");
}

function snake(name) {
  return name
    .replace(/([a-z0-9])([A-Z])/g, "$1_$2")
    .replace(/([A-Z]+)([A-Z][a-z])/g, "$1_$2")
    .toLowerCase();
}

const header = [];
header.push("#pragma once");
header.push("");
header.push("#include <vector>");
header.push("");
header.push('#include "lfw/core/json5.h"');
header.push('#include "lfw/core/value.h"');
header.push("");
header.push("namespace lfw {");
header.push("");

for (const t of tables) {
  const text = JSON.stringify(t.value);
  header.push(`inline const Value& ${snake(t.name)}() {`);
  header.push(`  static const Value v = json5_parse(`);
  header.push(`      ${rawLiteralFor(text)}).value;`);
  header.push("  return v;");
  header.push("}");
  header.push("");
}

header.push("struct FieldTableRef {");
header.push("  const char16_t* name;");
header.push("  const Value& (*get)();");
header.push("};");
header.push("");
header.push("inline const std::vector<FieldTableRef>& all_field_table_refs() {");
header.push("  static const std::vector<FieldTableRef> t = {");
for (const t of tables) {
  header.push(`    {${cxxU16(t.name)}, &${snake(t.name)}},`);
}
header.push("  };");
header.push("  return t;");
header.push("}");
header.push("");
header.push("}");
header.push("");
writeFileSync(out_header, header.join("\n"));

const tsOut = [];
const aliases = new Map();
let idx = 0;
for (const f of modules) {
  const name = `F${idx++}`;
  aliases.set(modPath(f), name);
  tsOut.push(`import * as ${name} from "${REL_FROM_SUBJECT_GEN}/${modPath(f)}";`);
}
tsOut.push("");
tsOut.push("export const FIELD_TABLES: { name: string; value: unknown }[] = [");
for (const t of tables) {
  tsOut.push(`  { name: ${JSON.stringify(t.name)}, value: ${aliases.get(t.module)}.${t.name} },`);
}
tsOut.push("];");
tsOut.push("");
writeFileSync(out_ts, tsOut.join("\n"));

process.stdout.write(`modules=${modules.length} tables=${tables.length}\n`);
process.stdout.write(`-> ${relative(root, out_header)}\n-> ${relative(root, out_ts)}\n`);
