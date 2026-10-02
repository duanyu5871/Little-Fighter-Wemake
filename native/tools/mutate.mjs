#!/usr/bin/env node

import { execFileSync } from "node:child_process";
import { existsSync, mkdirSync, readFileSync, rmSync, writeFileSync } from "node:fs";
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
const badAnchors = [];
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
  if (count !== 1) badAnchors.push(`[${m.note}] anchor occurs ${count} times in ${m.file}`);
}
if (badAnchors.length) {
  for (const line of badAnchors) process.stderr.write(`${line}\n`);
  process.stderr.write(`${badAnchors.length} bad anchor(s)\n`);
  process.exit(2);
}

for (const m of mutations) {
  if (!m.file.replace(/\\/g, "/").startsWith("native/lfw/")) {
    process.stderr.write(
      `mutation '[${m.note}]' targets ${m.file}; only native/lfw/** may be mutated ` +
        `(the TS side is reused across the run)\n`,
    );
    process.exit(2);
  }
}

function restoreAll() {
  for (const [file, text] of originals) writeFileSync(file, text);
}

function backupDir() {
  return resolve(root, "native", "build", "mutate-backup");
}

function backupOriginals() {
  const dir = backupDir();
  mkdirSync(dir, { recursive: true });
  const files = [];
  let index = 0;
  for (const [file, text] of originals) {
    const name = `f${index++}`;
    writeFileSync(resolve(dir, name), text, "utf8");
    files.push({ file, name });
  }
  writeFileSync(resolve(dir, "manifest.json"), JSON.stringify(files), "utf8");
}

function recoverFromInterruptedRun() {
  const dir = backupDir();
  const manifest = resolve(dir, "manifest.json");
  if (!existsSync(manifest)) return false;
  const files = JSON.parse(readFileSync(manifest, "utf8"));
  for (const f of files) writeFileSync(f.file, readFileSync(resolve(dir, f.name), "utf8"));
  rmSync(dir, { recursive: true, force: true });
  return true;
}

if (recoverFromInterruptedRun()) {
  process.stdout.write(
    "NOTE: recovered from an interrupted run (sources restored, rebuild required)\n",
  );
  run(["build"]);
}

const baseline = run(["test", subject]);
if (!baseline.ok) {
  process.stderr.write(`baseline already failing for '${subject}':\n${baseline.out}\n`);
  process.exit(1);
}

backupOriginals();

const rows = [];
let killed = 0;
let survived = 0;
let compileError = 0;
const startedAt = Date.now();

for (const m of mutations) {
  const file = resolve(root, m.file);
  const text = originals.get(file);
  writeFileSync(file, text.replace(m.from, m.to));

  const t0 = Date.now();
  const built = run(["build", subject]);
  if (built.ok) {
    const tested = run(["test", subject, "--reuse-ts"]);
    rows.push({ note: m.note, survived: tested.ok, ms: Date.now() - t0 });
    if (tested.ok) ++survived;
    else ++killed;
  } else {
    rows.push({ note: m.note, survived: false, compileError: true, ms: Date.now() - t0 });
    ++compileError;
  }
  process.stdout.write(`.${survived ? "|" : compileError ? "e" : ""}`);

  restoreAll();
}
process.stdout.write("\n");

restoreAll();
rmSync(backupDir(), { recursive: true, force: true });
run(["build"]);

const width = Math.max(...rows.map((r) => r.note.length));
for (const r of rows) {
  const status = r.compileError ? "compile-error" : r.survived ? "SURVIVED" : "killed";
  process.stdout.write(`${r.note.padEnd(width)}  ${status}\n`);
}
const totalMs = Date.now() - startedAt;
const slowest = [...rows].sort((a, b) => (b.ms ?? 0) - (a.ms ?? 0))[0];
process.stdout.write(
  `\n${killed} killed, ${survived} survived, ${compileError} compile-error (of ${rows.length})\n` +
    `total ${(totalMs / 1000).toFixed(1)}s, ${(totalMs / rows.length).toFixed(0)} ms/mutation` +
    (slowest ? `, slowest '${slowest.note}' ${slowest.ms} ms\n` : "\n"),
);

process.exit(survived === 0 && compileError === 0 ? 0 : 1);
