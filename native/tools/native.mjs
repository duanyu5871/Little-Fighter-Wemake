#!/usr/bin/env node

import { execFileSync, spawnSync } from "node:child_process";
import { existsSync, mkdirSync, writeFileSync } from "node:fs";
import { dirname, join, resolve } from "node:path";
import { fileURLToPath } from "node:url";

const HERE = dirname(fileURLToPath(import.meta.url));
const NATIVE = resolve(HERE, "..");
const BUILD_DIR = join(NATIVE, "build", "msvc-x64");

const GEN_DIR = join(NATIVE, "build", "gen");
const CMD_FILE = join(GEN_DIR, "native-cmd.cmd");

const C = {
  red: (s) => `\x1b[31m${s}\x1b[0m`,
  green: (s) => `\x1b[32m${s}\x1b[0m`,
  dim: (s) => `\x1b[2m${s}\x1b[0m`,
  bold: (s) => `\x1b[1m${s}\x1b[0m`,
};

function findVs() {
  const pf86 = process.env["ProgramFiles(x86)"] || "C:\\Program Files (x86)";
  const vswhere = join(pf86, "Microsoft Visual Studio", "Installer", "vswhere.exe");
  if (!existsSync(vswhere)) return null;
  try {
    const out = execFileSync(vswhere, ["-latest", "-products", "*", "-property", "installationPath"], {
      encoding: "utf8",
    }).trim();
    return out || null;
  } catch {
    return null;
  }
}

function hasOnPath(exe) {
  return spawnSync("where", [exe], { stdio: "ignore" }).status === 0;
}

function runInVsEnv(cmd) {
  const vs = findVs();
  if (!vs) {
    console.error(C.red("Visual Studio not found (vswhere returned nothing)"));
    return 1;
  }

  const vcvars = join(vs, "VC", "Auxiliary", "Build", "vcvars64.bat");
  if (!existsSync(vcvars)) {
    console.error(C.red(`missing ${vcvars}`));
    return 1;
  }

  const lines = ["@echo off", `call "${vcvars}" >nul`];

  const extra = [];
  const cmakeBin = join(vs, "Common7", "IDE", "CommonExtensions", "Microsoft", "CMake", "CMake", "bin");
  const ninjaBin = join(vs, "Common7", "IDE", "CommonExtensions", "Microsoft", "CMake", "Ninja");
  if (!hasOnPath("cmake") && existsSync(cmakeBin)) extra.push(cmakeBin);
  if (!hasOnPath("ninja") && existsSync(ninjaBin)) extra.push(ninjaBin);
  if (extra.length) lines.push(`set "PATH=${extra.join(";")};%PATH%"`);

  lines.push(cmd, "exit /b %ERRORLEVEL%");

  mkdirSync(GEN_DIR, { recursive: true });
  writeFileSync(CMD_FILE, lines.join("\r\n") + "\r\n");
  console.log(C.dim(`  generated ${CMD_FILE}`));

  return spawnSync("cmd.exe", ["/d", "/c", CMD_FILE], { stdio: "inherit" }).status ?? 1;
}

const BIN = join(BUILD_DIR, "bin");

function step(name, fn) {
  console.log("");
  console.log(C.bold(`> ${name}`));
  const code = fn();
  if (code !== 0) {
    console.error(C.red(`x ${name} failed (exit ${code})`));
    process.exit(code);
  }
  console.log(C.green(`v ${name}`));
}

function cmdConfigure() {
  if (existsSync(BUILD_DIR)) {
    console.log(C.dim(`  ${BUILD_DIR} exists, skipping`));
    return 0;
  }
  return runInVsEnv(`cmake -S "${NATIVE}" -B "${BUILD_DIR}" -G Ninja -DCMAKE_BUILD_TYPE=Release`);
}

function cmdBuild() {
  const code = runInVsEnv(`cmake --build "${BUILD_DIR}"`);
  if (code !== 0) return code;
  console.log(C.dim(`  -> ${BIN}`));
  return 0;
}

function cmdLint() {
  return spawnSync(process.execPath, [join(HERE, "check_lfw_cpp_includes.mjs")], { stdio: "inherit" }).status ?? 1;
}

function cmdCoverage() {
  return (
    spawnSync(process.execPath, [join(HERE, "check_defines_coverage.mjs")], { stdio: "inherit" }).status ?? 1
  );
}

function cmdTest() {
  const r = spawnSync(
    process.execPath,
    [join(NATIVE, "tests", "differential", "run.mjs"), ...process.argv.slice(3)],
    { stdio: "inherit" },
  );
  return r.status ?? 1;
}

const table = {
  configure: () => step("configure", cmdConfigure),
  build: () => step("build", cmdBuild),
  lint: () => step("lint", cmdLint),
  coverage: () => step("coverage", cmdCoverage),
  test: () => step("test", cmdTest),
  all: () => {
    step("configure", cmdConfigure);
    step("build", cmdBuild);
    step("lint", cmdLint);
    step("coverage", cmdCoverage);
    step("test", cmdTest);
    console.log("");
    console.log(C.green(C.bold("all passed")));
    return 0;
  },
};

const SUB = process.argv[2] ?? "all";
if (!table[SUB]) {
  console.error(`unknown subcommand '${SUB}', available: ${Object.keys(table).join(" | ")}`);
  process.exit(2);
}

process.exit(table[SUB]());
