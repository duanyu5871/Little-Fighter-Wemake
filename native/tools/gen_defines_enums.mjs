#!/usr/bin/env node

import { mkdirSync, readdirSync, readFileSync, statSync, writeFileSync } from "node:fs";
import { dirname, join, relative, resolve } from "node:path";
import { fileURLToPath } from "node:url";

const here = dirname(fileURLToPath(import.meta.url));
const root = resolve(here, "..", "..");
const src_root = resolve(root, "src/LFW/defines");
const out_root = resolve(root, "native/lfw/defines");

const SKIP_ENUMS = new Set(["BinOp"]);

function snake(name) {
  return name
    .replace(/([a-z0-9])([A-Z])/g, "$1_$2")
    .replace(/([A-Z]+)([A-Z][a-z])/g, "$1_$2")
    .toLowerCase();
}

function stripComments(src) {
  let out = "";
  let i = 0;
  while (i < src.length) {
    const c = src[i];
    if (c === '"' || c === "'" || c === "`") {
      const quote = c;
      out += c;
      i++;
      while (i < src.length) {
        if (src[i] === "\\") {
          out += src[i] + (src[i + 1] ?? "");
          i += 2;
          continue;
        }
        out += src[i];
        if (src[i] === quote) {
          i++;
          break;
        }
        i++;
      }
      continue;
    }
    if (c === "/" && src[i + 1] === "/") {
      while (i < src.length && src[i] !== "\n") i++;
      continue;
    }
    if (c === "/" && src[i + 1] === "*") {
      i += 2;
      while (i < src.length && !(src[i] === "*" && src[i + 1] === "/")) i++;
      i += 2;
      continue;
    }
    out += c;
    i++;
  }
  return out;
}

function findEnums(src) {
  const found = [];
  const re = /export\s+enum\s+(\w+)\s*\{/g;
  let m;
  while ((m = re.exec(src)) !== null) {
    const name = m[1];
    const index = m.index;
    let depth = 1;
    let i = re.lastIndex;
    const start = i;
    let inStr = null;
    while (i < src.length) {
      const c = src[i];
      if (inStr !== null) {
        if (c === "\\") {
          i += 2;
          continue;
        }
        if (c === inStr) inStr = null;
        i++;
        continue;
      }
      if (c === '"' || c === "'" || c === "`") {
        inStr = c;
        i++;
        continue;
      }
      if (c === "{") depth++;
      else if (c === "}") {
        depth--;
        if (depth === 0) {
          i++;
          break;
        }
      }
      i++;
    }
    found.push({ name, body: src.slice(start, i - 1), index });
    re.lastIndex = i;
  }
  return found;
}

function namespaceScopes(src) {
  const scopes = [];
  const re = /(?:export\s+)?namespace\s+([\w.]+)\s*\{/g;
  let m;
  while ((m = re.exec(src)) !== null) {
    let depth = 1;
    let i = re.lastIndex;
    let inStr = null;
    while (i < src.length) {
      const c = src[i];
      if (inStr !== null) {
        if (c === "\\") {
          i += 2;
          continue;
        }
        if (c === inStr) inStr = null;
        i++;
        continue;
      }
      if (c === '"' || c === "'" || c === "`") {
        inStr = c;
        i++;
        continue;
      }
      if (c === "{") depth++;
      else if (c === "}") {
        depth--;
        if (depth === 0) {
          i++;
          break;
        }
      }
      i++;
    }
    scopes.push({ name: m[1], start: m.index, end: i });
    re.lastIndex = i;
  }
  return scopes;
}

function qualifierFor(scopes, index) {
  const enclosing = scopes
    .filter((s) => s.start < index && index < s.end)
    .sort((a, b) => a.start - b.start);
  return enclosing.map((s) => s.name).join(".");
}

function splitMembers(body) {
  const members = [];
  let buf = "";
  let nested = 0;
  let inStr = null;
  for (let i = 0; i < body.length; i++) {
    const c = body[i];
    if (inStr !== null) {
      buf += c;
      if (c === "\\") {
        buf += body[++i] ?? "";
        continue;
      }
      if (c === inStr) inStr = null;
      continue;
    }
    if (c === '"' || c === "'" || c === "`") {
      inStr = c;
      buf += c;
      continue;
    }
    if (c === "(" || c === "{" || c === "[") nested++;
    if (c === ")" || c === "}" || c === "]") nested--;
    if (c === "," && nested === 0) {
      members.push(buf);
      buf = "";
      continue;
    }
    buf += c;
  }
  members.push(buf);
  return members.map((s) => s.trim()).filter((s) => s.length > 0);
}

function decodeStringLiteral(text) {
  const quote = text[0];
  if (quote !== '"' && quote !== "'") return null;
  let i = 1;
  const units = [];
  while (i < text.length) {
    const c = text[i];
    if (c === quote) break;
    if (c === "\\") {
      const e = text[i + 1];
      if (e === "n") units.push(10), (i += 2);
      else if (e === "r") units.push(13), (i += 2);
      else if (e === "t") units.push(9), (i += 2);
      else if (e === "0") units.push(0), (i += 2);
      else if (e === "b") units.push(8), (i += 2);
      else if (e === "f") units.push(12), (i += 2);
      else if (e === "v") units.push(11), (i += 2);
      else if (e === "u") {
        units.push(parseInt(text.slice(i + 2, i + 6), 16));
        i += 6;
      } else if (e === "x") {
        units.push(parseInt(text.slice(i + 2, i + 4), 16));
        i += 4;
      } else {
        units.push(e.charCodeAt(0));
        i += 2;
      }
      continue;
    }
    units.push(c.charCodeAt(0));
    i++;
  }
  return units;
}

function cxxU16(units) {
  let out = 'u"';
  for (const u of units) {
    if (u === 0x22) out += '\\"';
    else if (u === 0x5c) out += "\\\\";
    else if (u >= 0x20 && u <= 0x7e) out += String.fromCharCode(u);
    else out += "\\u" + u.toString(16).padStart(4, "0");
  }
  return out + '"';
}

function parseEnum(src, name, body) {
  const raw = splitMembers(body);
  const members = [];
  let next = 0;
  for (const item of raw) {
    const eq = item.indexOf("=");
    if (eq < 0) {
      members.push({ name: item.replace(/\s+/g, " ").trim(), value: next, auto: true });
      next += 1;
      continue;
    }
    const memberName = item.slice(0, eq).replace(/\s+/g, " ").trim();
    const valueText = item.slice(eq + 1).trim();
    const str = decodeStringLiteral(valueText);
    if (str !== null) {
      members.push({ name: memberName, text: str });
      continue;
    }
    const num = Number(valueText.replace(/_/g, ""));
    if (!Number.isFinite(num)) {
      return { name, error: `unsupported value for ${name}.${memberName}: ${valueText}` };
    }
    members.push({ name: memberName, value: num });
    next = num + 1;
  }
  const isText = members.every((m) => m.text !== undefined);
  return { name, members, isText };
}

const files = [];
const walk = (dir) => {
  for (const n of readdirSync(dir).sort()) {
    const p = join(dir, n);
    if (statSync(p).isDirectory()) walk(p);
    else if (n.endsWith(".ts")) files.push(p);
  }
};
walk(src_root);

mkdirSync(out_root, { recursive: true });

let emitted = 0;
let skipped = 0;
const generated = [];

for (const file of files) {
  const src = stripComments(readFileSync(file, "utf8"));
  const scopes = namespaceScopes(src);
  const found = findEnums(src)
    .filter((e) => !SKIP_ENUMS.has(e.name))
    .map((e) => ({ ...e, qualifier: qualifierFor(scopes, e.index) }));
  if (found.length === 0) continue;

  const parsed = found.map((e) => parseEnum(src, e.name, e.body));
  const bad = parsed.find((p) => p.error !== undefined);
  if (bad !== undefined) {
    process.stderr.write(`skip ${relative(root, file)}: ${bad.error}\n`);
    skipped += 1;
    continue;
  }

  const stem = snake(file.split(/[\\/]/).pop().replace(/\.ts$/, ""));
  const out = [];
  out.push("#pragma once");
  out.push("");
  out.push("#include <vector>");
  out.push("");
  out.push('#include "lfw/defines/enum_entries.h"');
  out.push("");
  out.push("namespace lfw {");

  for (const e of parsed) {
    out.push("");
    if (!e.isText) {
      out.push(`enum class ${e.name} : int {`);
      for (const m of e.members) out.push(`  ${m.name} = ${m.value},`);
      out.push("};");
      out.push("");
      out.push(`inline const std::vector<EnumNumberEntry>& ${snake(e.name)}_entries() {`);
      out.push(`  static const std::vector<EnumNumberEntry> e = {`);
      for (const m of e.members) {
        out.push(
          `    {${cxxU16([...m.name].map((c) => c.charCodeAt(0)))}, static_cast<double>(${e.name}::${m.name})},`,
        );
      }
      out.push("  };");
      out.push("  return e;");
      out.push("}");
      out.push("");
      const reverse = new Map();
      for (const m of e.members) reverse.set(m.value, m.name);
      const values = [...reverse.keys()].sort((a, b) => a - b);
      out.push(`inline const char16_t* ${snake(e.name)}_name_of(int v) {`);
      out.push("  switch (v) {");
      for (const v of values) {
        out.push(
          `    case static_cast<int>(${e.name}::${reverse.get(v)}): return ${cxxU16([...reverse.get(v)].map((c) => c.charCodeAt(0)))};`,
        );
      }
      out.push("  }");
      out.push("  return nullptr;");
      out.push("}");
    } else {
      out.push(`namespace ${snake(e.name)} {`);
      out.push("");
      for (const m of e.members) out.push(`inline constexpr const char16_t* k${m.name} = ${cxxU16(m.text)};`);
      out.push("");
      out.push("}");
      out.push("");
      out.push(`inline const std::vector<EnumTextEntry>& ${snake(e.name)}_entries() {`);
      out.push(`  static const std::vector<EnumTextEntry> e = {`);
      for (const m of e.members) {
        out.push(`    {${cxxU16([...m.name].map((c) => c.charCodeAt(0)))}, ${snake(e.name)}::k${m.name}},`);
      }
      out.push("  };");
      out.push("  return e;");
      out.push("}");
    }
  }

  out.push("");
  out.push("}");
  out.push("");

  const out_path = join(out_root, `${stem}.h`);
  writeFileSync(out_path, out.join("\n"));
  generated.push({
    stem,
    module: relative(src_root, file).replace(/\.ts$/, "").split("\\").join("/"),
    enums: parsed.map((p) => p.name),
    numberEnums: parsed.filter((p) => !p.isText).map((p) => p.name),
    textEnums: parsed.filter((p) => p.isText).map((p) => p.name),
    access: Object.fromEntries(parsed.map((p, k) => [p.name, found[k].qualifier])),
  });
  emitted += 1;
}

process.stdout.write(`generated ${emitted} headers, skipped ${skipped} files\n`);
for (const g of generated) process.stdout.write(`  ${g.stem}.h: ${g.enums.join(", ")}\n`);

const all = [];
all.push("#pragma once");
all.push("");
all.push("#include <vector>");
all.push("");
for (const g of generated) all.push(`#include "lfw/defines/${g.stem}.h"`);
all.push("");
all.push('#include "lfw/defines/enum_entries.h"');
all.push("");
all.push('#include "lfw/defines/all_enums_extra.h"');
all.push("");
all.push("namespace lfw {");
all.push("");
all.push("inline const std::vector<EnumNumberTableRef>& all_number_enum_tables() {");
all.push("  static const std::vector<EnumNumberTableRef> t = [] {");
all.push("    std::vector<EnumNumberTableRef> v = {");
for (const g of generated) {
  for (const e of g.numberEnums) {
    all.push(`      {${cxxU16([...e].map((c) => c.charCodeAt(0)))}, &${snake(e)}_entries(), &${snake(e)}_name_of},`);
  }
}
all.push("    };");
all.push("    append_extra_number_enum_tables(v);");
all.push("    return v;");
all.push("  }();");
all.push("  return t;");
all.push("}");
all.push("");
all.push("inline const std::vector<EnumTextTableRef>& all_text_enum_tables() {");
all.push("  static const std::vector<EnumTextTableRef> t = [] {");
all.push("    std::vector<EnumTextTableRef> v = {");
for (const g of generated) {
  for (const e of g.textEnums) {
    all.push(`      {${cxxU16([...e].map((c) => c.charCodeAt(0)))}, &${snake(e)}_entries()},`);
  }
}
all.push("    };");
all.push("    append_extra_text_enum_tables(v);");
all.push("    return v;");
all.push("  }();");
all.push("  return t;");
all.push("}");
all.push("");
all.push("}");
all.push("");
writeFileSync(join(out_root, "all_enums.h"), all.join("\n"));

const genDir = resolve(root, "native/tests/differential/subjects/gen");
mkdirSync(genDir, { recursive: true });
const ts = [];
const seen = new Set();
let modIndex = 0;
const alias = new Map();
for (const g of generated) {
  if (alias.has(g.module)) continue;
  const name = `M${modIndex++}`;
  alias.set(g.module, name);
  ts.push(`import * as ${name} from "../../../../../src/LFW/defines/${g.module}";`);
}
ts.push("import { EXTRA_ENUMS } from \"./defines_enums_extra\";");
ts.push("");
ts.push("export const ENUMS: { name: string; value: unknown }[] = [");
for (const g of generated) {
  for (const e of g.enums) {
    const q = g.access[e];
    const path = q.length > 0 ? `${alias.get(g.module)}.${q}.${e}` : `${alias.get(g.module)}.${e}`;
    ts.push(`  { name: "${e}", value: ${path} },`);
  }
}
ts.push("  ...EXTRA_ENUMS,");
ts.push("];");
ts.push("");
writeFileSync(join(genDir, "defines_enums.ts"), ts.join("\n"));
process.stdout.write(`all_enums.h + gen/defines_enums.ts written\n`);
