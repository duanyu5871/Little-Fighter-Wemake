#!/usr/bin/env node

import { execFileSync } from "node:child_process";
import { existsSync, mkdirSync, readFileSync, readdirSync, writeFileSync } from "node:fs";
import { createRequire } from "node:module";
import { dirname, join, resolve, basename } from "node:path";
import { fileURLToPath } from "node:url";

const HERE = dirname(fileURLToPath(import.meta.url));
const NATIVE = resolve(HERE, "..", "..");
const ROOT = resolve(NATIVE, "..");

const GEN = join(NATIVE, "build", "gen");
const CASES_DIR = join(HERE, "cases");

const BIN_DIR = join(NATIVE, "build", "msvc-x64", "bin");
const CPP_EXE = join(BIN_DIR, process.platform === "win32" ? "lfw_mt_trace.exe" : "lfw_mt_trace");

const C = {
  red: (s) => `\x1b[31m${s}\x1b[0m`,
  green: (s) => `\x1b[32m${s}\x1b[0m`,
  yellow: (s) => `\x1b[33m${s}\x1b[0m`,
  dim: (s) => `\x1b[2m${s}\x1b[0m`,
};

function buildTsBundle() {
  const requireFromLfw = createRequire(join(ROOT, "src", "LFW", "package.json"));
  const esbuild = requireFromLfw("esbuild");

  mkdirSync(GEN, { recursive: true });
  const outfile = join(GEN, "mt_trace_ts.mjs");
  esbuild.buildSync({
    entryPoints: [join(HERE, "mt_trace.ts")],
    bundle: true,
    format: "esm",
    platform: "node",
    target: "node20",
    outfile,
    logLevel: "warning",
  });
  return outfile;
}

const MAX = 256 * 1024 * 1024;

function runTs(bundle, casePath) {
  return execFileSync(process.execPath, [bundle, casePath], { encoding: "utf8", maxBuffer: MAX });
}

function runCpp(casePath) {
  return execFileSync(CPP_EXE, [casePath], { encoding: "utf8", maxBuffer: MAX });
}

const norm = (s) => s.replace(/\r\n/g, "\n").replace(/\n+$/, "\n");

function diff(a, b) {
  const la = norm(a).split("\n");
  const lb = norm(b).split("\n");
  const n = Math.max(la.length, lb.length);

  for (let i = 0; i < n; i++) {
    if (la[i] === lb[i]) continue;
    const ctx = [];
    for (let k = Math.max(0, i - 3); k < i; k++) {
      ctx.push(C.dim(`  ${String(k + 1).padStart(6)}  ${la[k] ?? "<missing>"}`));
    }
    ctx.push(C.red(`  ${String(i + 1).padStart(6)}  C++: ${la[i] ?? "<missing>"}`));
    ctx.push(C.red(`  ${String(i + 1).padStart(6)}  TS : ${lb[i] ?? "<missing>"}`));
    return [
      `  first divergence at line ${i + 1} (total ${la.length} / ${lb.length})`,
      ...ctx,
    ].join("\n");
  }
  return null;
}

function main() {
  if (!existsSync(CPP_EXE)) {
    console.error(C.red(`missing C++ runner: ${CPP_EXE}`));
    console.error(`run: node native/tools/native.mjs build`);
    process.exit(2);
  }

  const filter = process.argv[2];
  const cases = readdirSync(CASES_DIR)
    .filter((f) => f.endsWith(".txt"))
    .filter((f) => !filter || basename(f, ".txt") === filter)
    .sort();

  if (cases.length === 0) {
    console.error(C.red(filter ? `no case named ${filter}` : "no cases found"));
    process.exit(2);
  }

  const bundle = buildTsBundle();

  let passed = 0;
  let failed = 0;

  for (const file of cases) {
    const name = basename(file, ".txt");
    const casePath = join(CASES_DIR, file);

    let cppOut;
    let tsOut;
    try {
      cppOut = runCpp(casePath);
    } catch (e) {
      console.error(C.red(`x ${name}: C++ failed\n${e.stderr ?? e.message}`));
      failed++;
      continue;
    }
    try {
      tsOut = runTs(bundle, casePath);
    } catch (e) {
      console.error(C.red(`x ${name}: TS failed\n${e.stderr ?? e.message}`));
      failed++;
      continue;
    }

    writeFileSync(join(GEN, `trace.${name}.cpp.txt`), norm(cppOut));
    writeFileSync(join(GEN, `trace.${name}.ts.txt`), norm(tsOut));

    const d = diff(cppOut, tsOut);
    if (d === null) {
      const lines = norm(cppOut).split("\n").length - 1;
      console.log(C.green(`v ${name}`) + C.dim(`  ${lines} lines match`));
      passed++;
    } else {
      console.log(C.red(`x ${name}`) + C.yellow(` <- drift`));
      console.log(d);
      console.log(C.dim(`  artifacts: ${join(GEN, `trace.${name}.{cpp,ts}.txt`)}`));
      failed++;
    }
  }

  console.log("");
  console.log(
    failed === 0
      ? C.green(`differential: ${passed}/${cases.length} passed`)
      : C.red(`differential: ${failed}/${cases.length} failed`),
  );
  process.exit(failed === 0 ? 0 : 1);
}

main();
