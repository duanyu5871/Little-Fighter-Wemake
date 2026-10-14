#!/usr/bin/env node
// 编 WASM 宿主（步行骨架）。等价于手打：
//   call %USERPROFILE%\emsdk\emsdk_env.bat
//   cmake --preset wasm
//   cmake --build build\wasm --target lfw_wasm -j <n>
// 单独一个脚本的理由见 DESIGN W12 的「未定」：`native/tools/native.mjs` 把 `BUILD_DIR` 写死成
// `build/msvc-x64`，MSVC 那条链不该为了 wasm 改；这里专门找 emsdk + VS 自带的 cmake/ninja。
//
// 用法：node native/web/build-wasm.mjs [--configure] [-j 16]
import { execFileSync, spawnSync } from "node:child_process";
import { existsSync, rmSync, writeFileSync } from "node:fs";
import { cpus, tmpdir } from "node:os";
import { dirname, join, resolve } from "node:path";
import { fileURLToPath } from "node:url";

const HERE = dirname(fileURLToPath(import.meta.url));
const NATIVE = resolve(HERE, "..");
const BUILD_DIR = join(NATIVE, "build", "wasm");

const argv = process.argv.slice(2);
const jobs = argv.includes("-j") ? argv[argv.indexOf("-j") + 1] : String(cpus().length);
const configure = argv.includes("--configure") || !existsSync(join(BUILD_DIR, "build.ninja"));

function findVs() {
  const pf86 = process.env["ProgramFiles(x86)"] || "C:\\Program Files (x86)";
  const vswhere = join(pf86, "Microsoft Visual Studio", "Installer", "vswhere.exe");
  if (!existsSync(vswhere)) return null;
  try {
    return (
      execFileSync(vswhere, ["-latest", "-products", "*", "-property", "installationPath"], {
        encoding: "utf8",
      }).trim() || null
    );
  } catch {
    return null;
  }
}

const emsdk = process.env.EMSDK || join(process.env.USERPROFILE ?? "", "emsdk");
if (!existsSync(join(emsdk, "emsdk_env.bat"))) {
  console.error(`emsdk not found at ${emsdk} (set EMSDK or install emsdk)`);
  process.exit(1);
}

const vs = findVs();
const cmake = vs
  ? join(vs, "Common7", "IDE", "CommonExtensions", "Microsoft", "CMake", "CMake", "bin", "cmake.exe")
  : "cmake";
const ninjaDir = vs
  ? join(vs, "Common7", "IDE", "CommonExtensions", "Microsoft", "CMake", "Ninja")
  : null;

const steps = [
  `@echo off`,
  `call "${emsdk}\\emsdk_env.bat" >nul`,
  `set "PATH=${ninjaDir ? ninjaDir + ";" : ""}%PATH%"`,
  `cd /d "${NATIVE}"`,
];
if (configure) steps.push(`"${cmake}" --preset wasm`);
steps.push(`"${cmake}" --build "${BUILD_DIR}" --target lfw_wasm -j ${jobs}`);

// 走一个临时 `.cmd`：直接把带引号的 `call "…\emsdk_env.bat"` 塞进 `cmd /c` 会被 Node 的
// 参数转义成 `\"…\"`，cmd 不认（踩过一次）。
const script = join(tmpdir(), "lfw-wasm-build.cmd");
writeFileSync(script, `${steps.join("\r\n")}\r\n`);
console.log(steps.slice(1).join("\n"));
const code = spawnSync("cmd.exe", ["/d", "/c", script], { stdio: "inherit" }).status;
rmSync(script, { force: true });
if (code !== 0) {
  console.error(`wasm build failed (exit ${code})`);
  process.exit(code ?? 1);
}
console.log(`-> ${join(BUILD_DIR, "bin", "lfw_wasm.mjs")} (+ lfw_wasm.wasm)`);
