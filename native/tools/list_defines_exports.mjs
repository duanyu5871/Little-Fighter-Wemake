import { readdirSync, readFileSync, statSync } from "node:fs";
import { join, relative } from "node:path";

const ROOT = process.cwd();
const DIR = join(ROOT, "src", "LFW", "defines");

function walk(dir, out = []) {
  for (const name of readdirSync(dir)) {
    const p = join(dir, name);
    if (statSync(p).isDirectory()) walk(p, out);
    else if (name.endsWith(".ts")) out.push(p);
  }
  return out;
}

function stripCommentsAndStrings(src) {
  let out = "";
  let i = 0;
  while (i < src.length) {
    const c = src[i];
    if (c === "/" && src[i + 1] === "/") {
      while (i < src.length && src[i] !== "\n") i++;
    } else if (c === "/" && src[i + 1] === "*") {
      i += 2;
      while (i < src.length && !(src[i] === "*" && src[i + 1] === "/")) i++;
      i += 2;
    } else if (c === '"' || c === "'" || c === "`") {
      const q = c;
      i++;
      while (i < src.length && src[i] !== q) {
        if (src[i] === "\\") i++;
        i++;
      }
      i++;
      out += q === "`" ? "`" : "``";
    } else {
      out += c;
      i++;
    }
  }
  return out;
}

const KIND_PATTERNS = [
  ["function", /\bexport\s+(?:async\s+)?function\s+([A-Za-z_$][\w$]*)/g],
  ["arrow", /\bexport\s+const\s+([A-Za-z_$][\w$]*)\s*(?::[^=]*)?=\s*(?:async\s*)?(?:\([^)]*\)|[A-Za-z_$][\w$]*)\s*=>/g],
  ["fnExpr", /\bexport\s+const\s+([A-Za-z_$][\w$]*)\s*(?::[^=]*)?=\s*function\b/g],
  ["class", /\bexport\s+(?:abstract\s+)?class\s+([A-Za-z_$][\w$]*)/g],
  ["enum", /\bexport\s+(?:const\s+)?enum\s+([A-Za-z_$][\w$]*)/g],
  ["interface", /\bexport\s+interface\s+([A-Za-z_$][\w$]*)/g],
  ["type", /\bexport\s+type\s+([A-Za-z_$][\w$]*)/g],
  ["constValue", /\bexport\s+const\s+([A-Za-z_$][\w$]*)\s*(?::[^=]*)?=\s*(?!.*=>)/g],
  ["reExport", /\bexport\s+\{/g],
  ["exportStar", /\bexport\s+\*\s+from/g],
];

const files = walk(DIR).sort();
const buckets = new Map();
let totalLines = 0;

for (const f of files) {
  const raw = readFileSync(f, "utf8");
  const src = stripCommentsAndStrings(raw);
  const lines = raw.split("\n").length;
  totalLines += lines;
  const rel = relative(ROOT, f).replace(/\\/g, "/");
  const found = [];
  for (const [kind, re] of KIND_PATTERNS) {
    re.lastIndex = 0;
    let m;
    while ((m = re.exec(src)) !== null) {
      found.push({ kind, name: m[1] ?? "{}" });
    }
  }
  for (const item of found) {
    if (!buckets.has(item.kind)) buckets.set(item.kind, []);
    buckets.get(item.kind).push(`${rel}:${item.name}`);
  }
}

console.log(`files: ${files.length}, lines: ${totalLines}`);
const ORDER = ["function", "arrow", "fnExpr", "class", "constValue", "reExport", "exportStar", "enum", "interface", "type"];
const skipKnown = (n) => /_fields$/.test(n);
for (const kind of ORDER) {
  const items = buckets.get(kind) ?? [];
  if (items.length === 0) continue;
  if (kind === "interface" || kind === "type" || kind === "enum") {
    console.log(`\n${kind}: ${items.length}`);
    continue;
  }
  const shown = items.filter((it) => !skipKnown(it.split(":").pop()));
  console.log(`\n${kind}: ${items.length} (skipped ${items.length - shown.length} *_fields)`);
  for (const it of shown) console.log(`  ${it}`);
}
