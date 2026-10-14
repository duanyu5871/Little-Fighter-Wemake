#!/usr/bin/env node

import { execFileSync, spawnSync } from "node:child_process";
import { existsSync, mkdirSync, readdirSync, readFileSync, statSync, writeFileSync } from "node:fs";
import { dirname, isAbsolute, join, resolve } from "node:path";
import { fileURLToPath } from "node:url";

const HERE = dirname(fileURLToPath(import.meta.url));
const NATIVE = resolve(HERE, "..");
const REPO = resolve(NATIVE, "..");
const BUILD_DIR = join(NATIVE, "build", "msvc-x64");

const GODOT_PROJECT = join(NATIVE, "host", "godot", "project");
const HOST_DATA = join(NATIVE, "build", "host-data");

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

// Godot 宿主（DESIGN §G1）：把 `lfw_core` + `host/` 编成一个 GDExtension。
// 两份仓库外的输入走环境变量/既有目录（见 native/README.md「Godot 宿主」）：
//   GODOT_CPP_DIR            godot-cpp 的检出目录（仓库外）
//   native/build/godot-api/  由本机 Godot 自己 dump 的 `extension_api.json`
// 产物：`native/host/godot/project/bin/lfw_godot.dll`（`project/bin/` 已忽略）。
function cmdGodot() {
  const cppDir = process.env.GODOT_CPP_DIR;
  if (!cppDir) {
    console.error(C.red("GODOT_CPP_DIR is not set (path to a godot-cpp checkout, outside this repo)"));
    return 2;
  }
  const apiFile = join(NATIVE, "build", "godot-api", "extension_api.json");
  if (!existsSync(apiFile)) {
    console.error(C.red(`missing ${apiFile}`));
    console.error("  run in that folder: <godot-console> --headless --dump-extension-api --dump-gdextension-interface");
    return 2;
  }
  const dir = join(NATIVE, "build", "godot");
  const code = runCmake([
    "-S",
    NATIVE,
    "-B",
    dir,
    "-G",
    "Ninja",
    "-DCMAKE_BUILD_TYPE=Release",
    "-DLFW_GODOT_HOST=ON",
    `-DLFW_GODOT_CPP_DIR=${cppDir}`,
    `-DGODOTCPP_CUSTOM_API_FILE=${apiFile}`,
    "-DGODOTCPP_TARGET=template_debug",
  ]);
  if (code !== 0) return code;
  const built = runCmake(["--build", dir, "--target", "lfw_godot"]);
  if (built !== 0) return built;
  console.log(C.dim(`  -> ${join(NATIVE, "host", "godot", "project", "bin", "lfw_godot.dll")}`));
  return 0;
}

function cmdTest() {
  const r = spawnSync(
    process.execPath,
    [join(NATIVE, "tests", "differential", "run.mjs"), ...process.argv.slice(3)],
    { stdio: "inherit" },
  );
  return r.status ?? 1;
}

// Godot 可执行文件：`GODOT_BIN` → PATH（`godot4`/`godot`）→ 桌面上的官方构建。
// 同版本优先 `_console` 变体（Windows 上只有它会打 stdout，`[godot]` 那几行才看得见）。
function findGodot() {
  const env = process.env.GODOT_BIN;
  if (env && existsSync(env)) return env;
  for (const name of ["godot4", "godot"]) {
    const r = spawnSync("where", [name], { encoding: "utf8" });
    if (r.status === 0) {
      const first = (r.stdout ?? "").split(/\r?\n/).find((l) => l.trim());
      if (first) return first.trim();
    }
  }
  const home = process.env.USERPROFILE ?? "";
  const found = [];
  for (const desk of [join(home, "Desktop"), join(home, "OneDrive", "Desktop")]) {
    if (!existsSync(desk)) continue;
    for (const f of readdirSync(desk)) {
      if (/^Godot_v[\d.]+-stable_win64(_console)?\.exe$/i.test(f)) found.push(join(desk, f));
    }
  }
  if (found.length === 0) return null;
  const ver = (p) => {
    const m = /Godot_v(\d+)\.(\d+)(?:\.(\d+))?/.exec(p);
    return m ? Number(m[1]) * 1e6 + Number(m[2]) * 1e3 + Number(m[3] ?? 0) : 0;
  };
  found.sort((a, b) => {
    if (ver(a) !== ver(b)) return ver(b) - ver(a);
    return (/_console\.exe$/i.test(b) ? 1 : 0) - (/_console\.exe$/i.test(a) ? 1 : 0);
  });
  return found[0];
}

// `npm run godot [-- <扩展自己的参数>]`：编（缺则编）→ 让 Godot 认工程（缺则 import）→ 跑。
// 数据路径由这里补成绝对路径 —— Godot 跑工程时会把 cwd 切到工程目录，相对路径会找错地方。
function cmdGodotRun() {
  const bin = join(GODOT_PROJECT, "bin", "lfw_godot.dll");
  if (!existsSync(bin)) {
    console.log(C.dim("  lfw_godot.dll 不在，先编一次（npm run native godot 的同一套）"));
    const built = cmdGodot();
    if (built !== 0) return built;
  }
  const godot = findGodot();
  if (!godot) {
    console.error(C.red("找不到 Godot 可执行文件；设 GODOT_BIN 或用 `npm run native godot` 只编不跑"));
    return 2;
  }
  if (!existsSync(join(GODOT_PROJECT, ".godot", "extension_list.cfg"))) {
    console.log(C.dim("  首次运行：让 Godot 认这个工程（--headless --import）"));
    const imp = spawnSync(godot, ["--headless", "--import", "--path", GODOT_PROJECT], {
      stdio: "inherit",
    });
    if (imp.status !== 0) return imp.status ?? 1;
  }

  const userArgs = process.argv.slice(3);
  const headless = userArgs.includes("--headless");
  const rest = userArgs.filter((a) => a !== "--headless");
  const has = (flag) => rest.includes(flag);

  const args = [];
  if (headless) args.push("--headless");
  args.push("--path", GODOT_PROJECT, "--");
  if (!has("--root")) args.push("--root", join(REPO, "dist"));
  if (!has("--zip")) {
    args.push("--zip", join(HOST_DATA, "prel"));
    const data = join(HOST_DATA, "data");
    if (existsSync(data)) args.push("--zip", data);
  }
  if (!has("--frames")) args.push("--frames", has("--screenshot") ? "200" : "900");
  if (!has("--delta")) args.push("--delta", "0.017");
  for (let i = 0; i < rest.length; i++) {
    const a = rest[i];
    // 存盘路径按「调用处」解成绝对路径：扩展拿到的 cwd 是工程目录
    if (a === "--screenshot" && rest[i + 1] && !isAbsolute(rest[i + 1])) {
      args.push(a, resolve(process.cwd(), rest[i + 1]));
      i += 1;
      continue;
    }
    args.push(a);
  }

  console.log(C.dim(`  ${godot}`));
  console.log(C.dim(`  ${args.join(" ")}`));
  const r = spawnSync(godot, args, { stdio: "inherit", cwd: REPO });
  return r.status ?? 1;
}

const table = {
  configure: () => step("configure", cmdConfigure),
  build: () => step("build", cmdBuild),
  lint: () => step("lint", cmdLint),
  coverage: () => step("coverage", cmdCoverage),
  test: () => step("test", cmdTest),
  godot: () => step("godot", cmdGodot),
  "godot-run": cmdGodotRun,
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
