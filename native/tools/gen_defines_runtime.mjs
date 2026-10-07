#!/usr/bin/env node

import { execFileSync } from "node:child_process";
import { mkdirSync, writeFileSync } from "node:fs";
import { createRequire } from "node:module";
import { dirname, join, relative, resolve } from "node:path";
import { fileURLToPath } from "node:url";

const here = dirname(fileURLToPath(import.meta.url));
const root = resolve(here, "..", "..");
const src_defines = resolve(root, "src/LFW/defines");
const gen_dir = resolve(root, "native/build/gen");
const out_h = resolve(root, "native/lfw/defines/runtime_gen.h");
const out_cpp = resolve(root, "native/lfw/defines/runtime_gen.cpp");
const out_ts = resolve(root, "native/tests/differential/subjects/gen/defines_runtime.ts");

const REL = relative(gen_dir, src_defines).split("\\").join("/");
const CHUNK_LIMIT = 8000;

const TOP_LEVEL = [
  ["EMPTY_FRAME_INFO", "EMPTY_FRAME_INFO", "EMPTY_FRAME_INFO"],
  ["GONE_FRAME_INFO", "GONE_FRAME_INFO", "GONE_FRAME_INFO"],
  ["ENTITY_PRIORITY_MAP", "ENTITY_PRIORITY_MAP", "EntityEnum"],
  ["CONFLICTS_KEY_MAP", "CONFLICTS_KEY_MAP", "GameKey"],
  ["DifficultyList", "DifficultyList", "Difficulty"],
  ["DifficultyNames", "DifficultyNames", "Difficulty"],
  ["DifficultyDescriptions", "DifficultyDescriptions", "Difficulty"],
  ["HIT_FLAG_NAME_MAP", "HIT_FLAG_NAME_MAP", "HitFlag"],
  ["HIT_FLAG_DESC_MAP", "HIT_FLAG_DESC_MAP", "HitFlag"],
  ["HitFlagDescriptions", "HitFlagDescriptions", "HitFlag"],
  ["ALL_HIT_FLAG", "ALL_HIT_FLAG", "HitFlag"],
  ["WpointKindDescriptions", "WpointKindDescriptions", "WpointKind"],
  ["BdyKindDescriptions", "BdyKindDescriptions", "BdyKind"],
  ["OLD_BDY_KIND_GOTO_MIN", "OLD_BDY_KIND_GOTO_MIN", "BdyKind"],
  ["OLD_BDY_KIND_GOTO_MAX", "OLD_BDY_KIND_GOTO_MAX", "BdyKind"],
  ["FRAME_BEHAVIOR_LABEL_MAP", "FRAME_BEHAVIOR_LABEL_MAP", "FrameBehavior"],
  ["StateEnumNames", "StateEnumNames", "StateEnum"],
];

const NEW_FUNCS = [
  ["bg_info_new", "bg_info_new()", "IBgInfo"],
  ["bg_layer_info_new", "bg_layer_info_new()", "IBgLayerInfo"],
  ["bg_data_new", "bg_data_new()", "IBgData"],
  ["armor_Info_new", "armor_Info_new()", "IArmorInfo"],
  ["bdy_info_new", "bdy_info_new()", "IBdyInfo"],
  ["bpoint_info_new", "bpoint_info_new()", "IBpointInfo"],
  ["chase_info_new", "chase_info_new()", "IChaseInfo"],
  ["cpoint_new", "cpoint_new()", "ICpoint"],
  ["dat_index_new", "dat_index_new()", "IDatIndex"],
  ["dialog_info_new", "dialog_info_new()", "IDialogInfo"],
  ["drink_info_new", "drink_info_new()", "IDrinkInfo"],
  ["entity_data_new", "entity_data_new()", "IEntityData"],
  ["entity_info_new", "entity_info_new()", "IEntityInfo"],
  ["frame_indexes_new", "frame_indexes_new()", "IFrameIndexes"],
  ["frame_info_new", "frame_info_new()", "IFrameInfo"],
  ["frame_model_new", "frame_model_new()", "IFrameModel"],
  ["frame_pic_new", "frame_pic_new()", "IFramePic"],
  ["itr_info_new", "itr_info_new()", "IItrInfo"],
  ["model_info_new", "model_info_new()", "IModelInfo"],
  ["next_frame_new", "next_frame_new()", "INextFrame"],
  ["opoint_info_new", "opoint_info_new()", "IOpointInfo"],
  ["opoint_multi_new", "opoint_multi_new()", "IOpointMulti"],
  ["picture_info_new", "picture_info_new()", "IPictureInfo"],
  ["sound_play_info_new", "sound_play_info_new()", "IStagePhaseInfo"],
  ["stage_info_new", "stage_info_new()", "IStageInfo"],
  ["stage_object_info_new", "stage_object_info_new()", "IStageObjectInfo"],
  ["stage_phase_info_new", "stage_phase_info_new()", "IStagePhaseInfo"],
  ["terrain_info_new", "terrain_info_new()", "ITerrainInfo"],
  ["wpoint_info_new", "wpoint_info_new()", "IWpointInfo"],
];

function flatModuleName(s) {
  return "M_" + s.replace(/[^A-Za-z0-9]/g, "_");
}

const topImports = [...new Set([...TOP_LEVEL.map((x) => x[2]), ...NEW_FUNCS.map((x) => x[2])])];

const entry = [];
entry.push(`import * as M_defines from "${REL}/defines";`);
for (const m of topImports) entry.push(`import * as ${flatModuleName(m)} from "${REL}/${m}";`);
entry.push("");
entry.push("function plain(v: unknown): unknown {");
entry.push("  if (v instanceof Map) {");
entry.push("    const o: Record<string, unknown> = {};");
entry.push("    for (const [k, x] of v) o[String(k)] = plain(x);");
entry.push("    return o;");
entry.push("  }");
entry.push("  if (Array.isArray(v)) return v.map((x) => (x === undefined ? null : plain(x)));");
entry.push("  if (v !== null && typeof v === 'object') {");
entry.push("    const src = v as Record<string, unknown>;");
entry.push("    const o: Record<string, unknown> = {};");
entry.push("    for (const k of Object.keys(src)) {");
entry.push("      if (src[k] === undefined) continue;");
entry.push("      o[k] = plain(src[k]);");
entry.push("    }");
entry.push("    return o;");
entry.push("  }");
entry.push("  return v;");
entry.push("}");
entry.push("");
entry.push("function json5ify(v: unknown): string {");
entry.push("  if (typeof v === 'number') {");
entry.push("    if (Object.is(v, -0)) return '-0';");
entry.push("    if (!Number.isFinite(v)) {");
entry.push("      return v === Infinity ? 'Infinity' : v === -Infinity ? '-Infinity' : 'NaN';");
entry.push("    }");
entry.push("    return String(v);");
entry.push("  }");
entry.push("  if (typeof v === 'string') return JSON.stringify(v);");
entry.push("  if (v === null || typeof v === 'boolean') return String(v);");
entry.push("  if (Array.isArray(v)) return '[' + v.map((x) => json5ify(x)).join(',') + ']';");
entry.push("  const o = v as Record<string, unknown>;");
entry.push("  return '{' + Object.keys(o).map((k) => JSON.stringify(k) + ':' + json5ify(o[k])).join(',') + '}';");
entry.push("}");
entry.push("");
entry.push("const out: { name: string; json: string; kind: string }[] = [];");
entry.push("const funcs: { name: string; json: string }[] = [];");
entry.push("const skipped: string[] = [];");
entry.push("function add(name: string, v: unknown, kind: string) {");
entry.push("  out.push({ name, json: json5ify(plain(v)), kind });");
entry.push("}");
entry.push("function addf(name: string, v: unknown) {");
entry.push("  funcs.push({ name, json: json5ify(plain(v)) });");
entry.push("}");
entry.push("");
entry.push("for (const k of Object.keys(M_defines.Defines).sort()) {");
entry.push("  const v = (M_defines.Defines as Record<string, unknown>)[k];");
entry.push("  const t = typeof v;");
entry.push("  if (t === 'function') { skipped.push(`Defines.${k} (function)`); continue; }");
entry.push("  if (v === undefined) { skipped.push(`Defines.${k} (undefined)`); continue; }");
entry.push("  if (t !== 'number' && t !== 'string' && t !== 'boolean' && v === null) {");
entry.push("    skipped.push(`Defines.${k} (null)`);");
entry.push("    continue;");
entry.push("  }");
entry.push("  if (t === 'symbol' || t === 'bigint') { skipped.push(`Defines.${k} (${t})`); continue; }");
entry.push("  add(`Defines.${k}`, v, t);");
entry.push("}");
for (const [name, expr, mod] of TOP_LEVEL) {
  entry.push(`add(${JSON.stringify(name)}, ${flatModuleName(mod)}.${expr}, 'top');`);
}
for (const [name, expr, mod] of NEW_FUNCS) {
  entry.push(`addf(${JSON.stringify(name)}, ${flatModuleName(mod)}.${expr});`);
}
entry.push("console.log(JSON.stringify({ out, skipped, funcs }));");

mkdirSync(gen_dir, { recursive: true });
mkdirSync(dirname(out_ts), { recursive: true });

const entryPath = join(gen_dir, "defines_runtime_dump.ts");
writeFileSync(entryPath, entry.join("\n"));

const requireFromLfw = createRequire(join(root, "src", "LFW", "package.json"));
const esbuild = requireFromLfw("esbuild");
const bundle = join(gen_dir, "defines_runtime_dump.mjs");
esbuild.buildSync({
  entryPoints: [entryPath],
  bundle: true,
  format: "esm",
  platform: "node",
  target: "node20",
  outfile: bundle,
  logLevel: "warning",
});

const json = execFileSync(process.execPath, [bundle], { encoding: "utf8", maxBuffer: 1 << 28 });
const { out: entries, skipped, funcs } = JSON.parse(json);

if (entries.length === 0) {
  console.error("gen_defines_runtime: no entries extracted");
  process.exit(1);
}

const sorted = [...entries].sort((a, b) => (a.name < b.name ? -1 : a.name > b.name ? 1 : 0));
const topExpr = new Map(TOP_LEVEL.map(([n, e, m]) => [n, `${flatModuleName(m)}.${e}`]));
for (const e of sorted) {
  if (e.kind === "top" && !topExpr.has(e.name)) {
    console.error(`gen_defines_runtime: missing expression for top-level entry ${e.name}`);
    process.exit(1);
  }
}

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

function splitChunks(text) {
  const units = [];
  for (const ch of text) {
    const u = ch.codePointAt(0);
    if (u > 0xffff) {
      const v = u - 0x10000;
      units.push(0xd800 + (v >> 10), 0xdc00 + (v & 0x3ff));
    } else {
      units.push(u);
    }
  }
  const chunks = [];
  for (let i = 0; i < units.length; i += CHUNK_LIMIT) chunks.push(units.slice(i, i + CHUNK_LIMIT));
  return chunks.map((c) => String.fromCharCode(...c));
}

const header = [];
header.push("#pragma once");
header.push("");
header.push("#include <string>");
header.push("#include <vector>");
header.push("");
header.push("#include \"lfw/core/value.h\"");
header.push("");
header.push("namespace lfw {");
header.push("");
header.push("struct DefinesRuntimeEntry {");
header.push("  std::u16string name;");
header.push("  std::u16string value_json5;");
header.push("  bool is_top_level;");
header.push("};");
header.push("");
header.push("const std::vector<DefinesRuntimeEntry>& defines_runtime_entries();");
header.push("");
for (const f of funcs) header.push(`Value ${f.name}();`);
header.push("");
header.push("}");
header.push("");
writeFileSync(out_h, header.join("\n"));

const cpp = [];
cpp.push("#include \"lfw/defines/runtime_gen.h\"");
cpp.push("");
cpp.push("#include <optional>");
cpp.push("");
cpp.push("#include \"lfw/core/json5.h\"");
cpp.push("");
cpp.push("namespace lfw {");
cpp.push("namespace {");
cpp.push("");
for (let i = 0; i < sorted.length; i++) {
  const chunks = splitChunks(sorted[i].json);
  if (chunks.length === 1) {
    cpp.push(`const char16_t kJson${i}[] = ${cxxU16(chunks[0])};`);
  } else {
    const parts = chunks.map((c) => cxxU16(c)).join(" ");
    cpp.push(`const char16_t kJson${i}[] = ${parts};`);
  }
}
cpp.push("");
cpp.push("struct Raw {");
cpp.push("  const char16_t* name;");
cpp.push("  const char16_t* json;");
cpp.push("  bool top;");
cpp.push("};");
cpp.push("");
cpp.push("const Raw kRaw[] = {");
for (let i = 0; i < sorted.length; i++) {
  const name = cxxU16(sorted[i].name);
  const top = sorted[i].kind === "top" ? "true" : "false";
  cpp.push(`    {${name}, kJson${i}, ${top}},`);
}
cpp.push("};");
cpp.push("");
cpp.push("std::vector<DefinesRuntimeEntry> build() {");
cpp.push("  std::vector<DefinesRuntimeEntry> out;");
cpp.push("  out.reserve(sizeof(kRaw) / sizeof(kRaw[0]));");
cpp.push("  for (const Raw& r : kRaw) out.push_back({r.name, r.json, r.top});");
cpp.push("  return out;");
cpp.push("}");
cpp.push("");
cpp.push("}");
cpp.push("");
cpp.push("const std::vector<DefinesRuntimeEntry>& defines_runtime_entries() {");
cpp.push("  static const std::vector<DefinesRuntimeEntry> kEntries = build();");
cpp.push("  return kEntries;");
cpp.push("}");
cpp.push("");
for (let i = 0; i < funcs.length; i++) {
  const chunks = splitChunks(funcs[i].json);
  if (chunks.length === 1) {
    cpp.push(`const char16_t kNew${i}[] = ${cxxU16(chunks[0])};`);
  } else {
    cpp.push(`const char16_t kNew${i}[] = ${chunks.map((c) => cxxU16(c)).join(" ")};`);
  }
}
cpp.push("");
for (let i = 0; i < funcs.length; i++) {
  cpp.push(`Value ${funcs[i].name}() {`);
  cpp.push(`  const Json5Result r = json5_parse(kNew${i});`);
  cpp.push("  return r.ok ? r.value : Value();");
  cpp.push("}");
  cpp.push("");
}
cpp.push("}");
cpp.push("");
writeFileSync(out_cpp, cpp.join("\n"));

const ts = [];
ts.push(`import * as M_defines from "${relative(dirname(out_ts), src_defines).split("\\").join("/")}/defines";`);
for (const m of topImports) {
  ts.push(
    `import * as ${flatModuleName(m)} from "${relative(dirname(out_ts), src_defines).split("\\").join("/")}/${m}";`,
  );
}
ts.push("");
ts.push("export const definesRuntimeEntries: { name: string; value: unknown; isTopLevel: boolean }[] = [");
for (const e of sorted) {
  const top = topExpr.get(e.name);
  const expr = top ?? `M_defines.Defines[${JSON.stringify(e.name.slice("Defines.".length))}]`;
  ts.push(`  { name: ${JSON.stringify(e.name)}, value: ${expr}, isTopLevel: ${e.kind === "top"} },`);
}
ts.push("];");
ts.push("");
writeFileSync(out_ts, ts.join("\n"));

console.log(`defines_runtime: ${entries.length} entries (${entries.filter((e) => e.kind === "top").length} top-level), ${funcs.length} new-funcs`);console.log(`  header: ${relative(root, out_h)}`);
console.log(`  source: ${relative(root, out_cpp)}`);
console.log(`  ts:     ${relative(root, out_ts)}`);
const nonNull = entries.filter((e) => e.kind !== "top" && e.kind !== "object");
console.log(`  scalar entries: ${nonNull.length}`);
if (skipped.length) {
  console.log(`  skipped ${skipped.length}:`);
  for (const s of skipped) console.log(`    ${s}`);
}
