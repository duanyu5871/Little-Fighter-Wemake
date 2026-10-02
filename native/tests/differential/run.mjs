#!/usr/bin/env node

import { execFileSync } from "node:child_process";
import { existsSync, mkdirSync, readdirSync, readFileSync, statSync, writeFileSync } from "node:fs";
import { createRequire } from "node:module";
import { basename, dirname, join, resolve } from "node:path";
import { fileURLToPath } from "node:url";

const HERE = dirname(fileURLToPath(import.meta.url));
const NATIVE = resolve(HERE, "..", "..");
const ROOT = resolve(NATIVE, "..");

const SUBJECTS_DIR = join(HERE, "subjects");
const CASES_DIR = join(HERE, "cases");
const GEN = join(NATIVE, "build", "gen");
const BIN_DIR = join(NATIVE, "build", "msvc-x64", "bin");

const EXE_SUFFIX = process.platform === "win32" ? ".exe" : "";
const MAX = 256 * 1024 * 1024;

const C = {
  red: (s) => `\x1b[31m${s}\x1b[0m`,
  green: (s) => `\x1b[32m${s}\x1b[0m`,
  yellow: (s) => `\x1b[33m${s}\x1b[0m`,
  dim: (s) => `\x1b[2m${s}\x1b[0m`,
};

function discoverSubjects() {
  if (!existsSync(SUBJECTS_DIR)) return [];
  return readdirSync(SUBJECTS_DIR)
    .filter((f) => f.endsWith(".ts"))
    .map((f) => basename(f, ".ts"))
    .filter((n) => n !== "trace_util")
    .sort();
}

function discoverCases(subject) {
  const dir = join(CASES_DIR, subject);
  if (!existsSync(dir)) return [];
  return readdirSync(dir)
    .filter((f) => f.endsWith(".txt"))
    .sort()
    .map((f) => join(dir, f));
}

function buildTs(subject) {
  const requireFromLfw = createRequire(join(ROOT, "src", "LFW", "package.json"));
  const esbuild = requireFromLfw("esbuild");

  mkdirSync(GEN, { recursive: true });
  const outfile = join(GEN, `trace_${subject}.mjs`);
  esbuild.buildSync({
    entryPoints: [join(SUBJECTS_DIR, `${subject}.ts`)],
    bundle: true,
    format: "esm",
    platform: "node",
    target: "node20",
    outfile,
    logLevel: "warning",
  });
  return outfile;
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
  const argv = process.argv.slice(2);
  const flags = new Set(argv.filter((a) => a.startsWith("--")));
  const rest = argv.filter((a) => !a.startsWith("--"));
  const wantedSubject = rest[0];
  const wantedCase = rest[1];
  const reuseTs = flags.has("--reuse-ts");

  const all = discoverSubjects().filter((s) => !wantedSubject || s === wantedSubject);
  if (all.length === 0) {
    console.error(C.red(wantedSubject ? `no subject named ${wantedSubject}` : "no subjects found"));
    process.exit(2);
  }

  let passed = 0;
  let failed = 0;
  let skipped = 0;

  for (const subject of all) {
    const exe = join(BIN_DIR, `lfw_trace_${subject}${EXE_SUFFIX}`);
    if (!existsSync(exe)) {
      console.error(C.red(`x ${subject}: missing ${exe} (run: node native/tools/native.mjs build)`));
      failed++;
      continue;
    }

    const cases = discoverCases(subject).filter(
      (p) => !wantedCase || basename(p, ".txt") === wantedCase,
    );
    if (cases.length === 0) {
      console.log(C.yellow(`! ${subject}: no cases`));
      skipped++;
      continue;
    }

    let bundle = null;
    const bundleOnce = () => (bundle ??= buildTs(subject));
    const subjectTs = join(SUBJECTS_DIR, `${subject}.ts`);

    for (const casePath of cases) {
      const caseName = basename(casePath, ".txt");
      const label = `${subject}/${caseName}`;
      const stem = `${subject}.${caseName}`;
      const tsCache = join(GEN, `trace.${stem}.ts.txt`);

      let cppOut;
      let tsOut;
      try {
        cppOut = execFileSync(exe, [casePath], { encoding: "utf8", maxBuffer: MAX });
      } catch (e) {
        console.error(C.red(`x ${label}: C++ failed\n${e.stderr ?? e.message}`));
        failed++;
        continue;
      }

      const caseMtime = statSync(casePath).mtimeMs;
      const subjectMtime = existsSync(subjectTs) ? statSync(subjectTs).mtimeMs : 0;
      const cacheFresh =
        reuseTs &&
        existsSync(tsCache) &&
        statSync(tsCache).mtimeMs >= caseMtime &&
        statSync(tsCache).mtimeMs >= subjectMtime;

      if (cacheFresh) {
        tsOut = readFileSync(tsCache, "utf8");
      } else {
        try {
          tsOut = execFileSync(process.execPath, [bundleOnce(), casePath], {
            encoding: "utf8",
            maxBuffer: MAX,
          });
        } catch (e) {
          console.error(C.red(`x ${label}: TS failed\n${e.stderr ?? e.message}`));
          failed++;
          continue;
        }
      }

      writeFileSync(join(GEN, `trace.${stem}.cpp.txt`), norm(cppOut));
      if (!cacheFresh) writeFileSync(tsCache, norm(tsOut));

      const d = diff(cppOut, tsOut);
      if (d === null) {
        const lines = norm(cppOut).split("\n").length - 1;
        console.log(C.green(`v ${label}`) + C.dim(`  ${lines} lines match`));
        passed++;
      } else {
        console.log(C.red(`x ${label}`) + C.yellow(` <- drift`));
        console.log(d);
        console.log(C.dim(`  artifacts: ${join(GEN, `trace.${stem}.{cpp,ts}.txt`)}`));
        failed++;
      }
    }
  }

  console.log("");
  console.log(
    failed === 0
      ? C.green(`differential: ${passed}/${passed + skipped} passed`)
      : C.red(`differential: ${failed} failed, ${passed} passed`),
  );
  process.exit(failed === 0 ? 0 : 1);
}

main();
