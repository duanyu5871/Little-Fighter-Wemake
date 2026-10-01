#!/usr/bin/env node

import { execFileSync } from "node:child_process";
import { existsSync, readFileSync, writeFileSync } from "node:fs";
import { dirname, resolve } from "node:path";
import { fileURLToPath, pathToFileURL } from "node:url";

const here = dirname(fileURLToPath(import.meta.url));
const root = resolve(here, "..", "..");
const nativeScript = resolve(root, "native", "tools", "native.mjs");

function run(args) {
  try {
    execFileSync(process.execPath, [nativeScript, ...args], {
      cwd: root,
      stdio: ["ignore", "pipe", "pipe"],
    });
    return { ok: true, out: "" };
  } catch (e) {
    const out = `${e.stdout ?? ""}${e.stderr ?? ""}`.toString();
    const tail = out.trimEnd().split(/\r?\n/).slice(-40).join("\n");
    return { ok: false, out: tail };
  }
}

const specPath = process.argv[2];
if (!specPath) {
  process.stderr.write("usage: mutate.mjs <mutations.mjs>\n");
  process.exit(2);
}

const spec = (await import(pathToFileURL(resolve(process.cwd(), specPath)).href)).default;
const { subject, mutations } = spec;

const originals = new Map();
for (const m of mutations) {
  const file = resolve(root, m.file);
  if (!originals.has(file)) {
    if (!existsSync(file)) {
      process.stderr.write(`missing file: ${m.file}\n`);
      process.exit(2);
    }
    originals.set(file, readFileSync(file, "utf8"));
  }
  const count = originals.get(file).split(m.from).length - 1;
  if (count !== 1) {
    process.stderr.write(`[${m.note}] anchor occurs ${count} times in ${m.file}\n`);
    process.exit(2);
  }
}

function restoreAll() {
  for (const [file, text] of originals) writeFileSync(file, text);
}

const baseline = run(["test", subject]);
if (!baseline.ok) {
  process.stderr.write(`baseline already failing for '${subject}':\n${baseline.out}\n`);
  process.exit(1);
}

const rows = [];
let killed = 0;

for (const m of mutations) {
  const file = resolve(root, m.file);
  const text = originals.get(file);
  writeFileSync(file, text.replace(m.from, m.to));

  const built = run(["build"]);
  if (built.ok) {
    const tested = run(["test", subject]);
    rows.push({ note: m.note, survived: tested.ok });
    if (!tested.ok) ++killed;
  } else {
    rows.push({ note: m.note, survived: false, compileError: true });
    ++killed;
  }

  restoreAll();
}

run(["build"]);

const width = Math.max(...rows.map((r) => r.note.length));
for (const r of rows) {
  const status = r.compileError ? "compile-error" : r.survived ? "SURVIVED" : "killed";
  process.stdout.write(`${r.note.padEnd(width)}  ${status}\n`);
}
process.stdout.write(`\n${killed}/${rows.length} killed\n`);

process.exit(killed === rows.length ? 0 : 1);
