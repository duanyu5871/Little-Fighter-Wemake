#!/usr/bin/env node

import { execFileSync, spawnSync } from "node:child_process";
import { existsSync, mkdirSync, readFileSync, statSync, writeFileSync } from "node:fs";
import { dirname, join, resolve } from "node:path";
import { fileURLToPath } from "node:url";

const HERE = dirname(fileURLToPath(import.meta.url));
const NATIVE = resolve(HERE, "..");
const BUILD_DIR = join(NATIVE, "build", "msvc-x64");

const GEN_DIR = join(NATIVE, "build", "gen");

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

let hasOnPathMemo = new Map();
function hasOnPath(exe) {
  if (!hasOnPathMemo.has(exe)) {
    hasOnPathMemo.set(exe, spawnSync("where", [exe], { stdio: "ignore" }).status === 0);
  }
  return hasOnPathMemo.get(exe);
}

const VSENV_CACHE = join(GEN_DIR, "vsenv.json");
let vsenvMemo = null;

function vsTools() {
  if (vsenvMemo && vsenvMemo.tools) return vsenvMemo.tools;
  const vs = findVs();
  if (!vs) return null;
  const vcvars = join(vs, "VC", "Auxiliary", "Build", "vcvars64.bat");
  if (!existsSync(vcvars)) return null;
  const cmakeBin = join(vs, "Common7", "IDE", "CommonExtensions", "Microsoft", "CMake", "CMake", "bin");
  const ninjaBin = join(vs, "Common7", "IDE", "CommonExtensions", "Microsoft", "CMake", "Ninja");
  const extra = [];
  let cmakeExe = "cmake";
  if (!hasOnPath("cmake")) {
    cmakeExe = join(cmakeBin, "cmake.exe");
    extra.push(cmakeBin);
  }
  if (!hasOnPath("ninja") && existsSync(ninjaBin)) extra.push(ninjaBin);
  return { vcvars, cmakeExe, extra };
}

function vsEnv() {
  if (vsenvMemo && vsenvMemo.env) return vsenvMemo.env;
  const tools = vsTools();
  if (!tools) return process.env;

  const mtimeMs = statSync(tools.vcvars).mtimeMs;
  if (existsSync(VSENV_CACHE)) {
    try {
      const cached = JSON.parse(readFileSync(VSENV_CACHE, "utf8"));
      if (cached.vcvars === tools.vcvars && cached.mtimeMs === mtimeMs) {
        vsenvMemo = { tools, env: applyExtra(cached.env, tools.extra) };
        return vsenvMemo.env;
      }
    } catch {
      /* regenerate below */
    }
  }

  mkdirSync(GEN_DIR, { recursive: true });
  const capture = join(GEN_DIR, "capture-env.cmd");
  writeFileSync(capture, `@echo off\r\ncall "${tools.vcvars}" >nul\r\nset\r\nexit /b 0\r\n`);
  const out = execFileSync("cmd.exe", ["/d", "/c", capture], {
    encoding: "utf8",
    maxBuffer: 64 * 1024 * 1024,
  });
  const env = { ...process.env };
  for (const line of out.split(/\r?\n/)) {
    const i = line.indexOf("=");
    if (i <= 0) continue;
    env[line.slice(0, i)] = line.slice(i + 1);
  }
  writeFileSync(VSENV_CACHE, JSON.stringify({ vcvars: tools.vcvars, mtimeMs, env }));
  vsenvMemo = { tools, env: applyExtra(env, tools.extra) };
  return vsenvMemo.env;
}

function applyExtra(env, extra) {
  if (!extra.length) return env;
  const key = Object.keys(env).find((k) => k.toUpperCase() === "PATH") ?? "Path";
  return { ...env, [key]: `${extra.join(";")};${env[key] ?? ""}` };
}

function runCmake(args) {
  const tools = vsTools();
  if (!tools) {
    console.error(C.red("Visual Studio not found (vswhere returned nothing)"));
    return 1;
  }
  return spawnSync(tools.cmakeExe, args, { stdio: "inherit", env: vsEnv() }).status ?? 1;
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
  const stamp = join(BUILD_DIR, "compile_commands.json");
  if (existsSync(BUILD_DIR) && existsSync(stamp)) {
    console.log(C.dim(`  ${BUILD_DIR} exists, skipping`));
    return 0;
  }
  return runCmake([
    "-S",
    NATIVE,
    "-B",
    BUILD_DIR,
    "-G",
    "Ninja",
    "-DCMAKE_BUILD_TYPE=Release",
    "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
  ]);
}

function cmdBuild() {
  const subject = process.argv[3];
  const args = ["--build", BUILD_DIR];
  if (subject) args.push("--target", `lfw_trace_${subject}`);
  const code = runCmake(args);
  if (code !== 0) return code;
  console.log(C.dim(`  -> ${BIN}${subject ? ` (target lfw_trace_${subject})` : ""}`));
  return 0;
}

function cmdLint() {
  const tools = ["check_lfw_cpp_includes.mjs", "check_mutation_anchors.mjs"];
  for (const tool of tools) {
    const code = spawnSync(process.execPath, [join(HERE, tool)], { stdio: "inherit" }).status ?? 1;
    if (code !== 0) return code;
  }
  return 0;
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
