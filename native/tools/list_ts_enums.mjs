#!/usr/bin/env node

import { readdirSync, readFileSync, statSync } from "node:fs";
import { join, relative, resolve } from "node:path";
import { dirname } from "node:path";
import { fileURLToPath } from "node:url";

const here = dirname(fileURLToPath(import.meta.url));
const root = resolve(here, "..", "..");

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

function enumsOf(src) {
  const found = [];
  const re = /export\s+enum\s+(\w+)\s*\{/g;
  let m;
  while ((m = re.exec(src)) !== null) {
    const name = m[1];
    let depth = 1;
    let i = re.lastIndex;
    const start = i;
    while (i < src.length && depth > 0) {
      if (src[i] === "{") depth++;
      else if (src[i] === "}") depth--;
      i++;
    }
    const body = src.slice(start, i - 1);
    const members = [];
    let buf = "";
    let nested = 0;
    for (let k = 0; k < body.length; k++) {
      const c = body[k];
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
    const parsed = members
      .map((s) => s.trim())
      .filter((s) => s.length > 0)
      .map((s) => {
        const eq = s.indexOf("=");
        if (eq < 0) return { name: s.replace(/\s+/g, " "), value: "(auto)" };
        return {
          name: s.slice(0, eq).replace(/\s+/g, " ").trim(),
          value: s.slice(eq + 1).replace(/\s+/g, " ").trim(),
        };
      });
    found.push({ name, members: parsed });
  }
  return found;
}

const args = process.argv.slice(2);
const files = [];
if (args.length === 0) {
  const walk = (dir) => {
    for (const n of readdirSync(dir).sort()) {
      const p = join(dir, n);
      if (statSync(p).isDirectory()) walk(p);
      else if (n.endsWith(".ts")) files.push(p);
    }
  };
  walk(resolve(root, "src/LFW/defines"));
} else {
  for (const a of args) files.push(resolve(root, a));
}

for (const f of files) {
  const src = stripComments(readFileSync(f, "utf8"));
  const enums = enumsOf(src);
  if (enums.length === 0) continue;
  process.stdout.write(`== ${relative(root, f).split("\\").join("/")}\n`);
  for (const e of enums) {
    process.stdout.write(`   enum ${e.name}  (${e.members.length})\n`);
    for (const mem of e.members) process.stdout.write(`     ${mem.name} = ${mem.value}\n`);
  }
}
