import { execSync } from "node:child_process";
import { copyFileSync, existsSync, mkdirSync, mkdtempSync, readFileSync, readdirSync, rmSync, statSync, writeFileSync } from "node:fs";
import { tmpdir } from "node:os";
import { join } from "node:path";
import json5 from "json5";
import { APP_NAME, DIST, ICON, ROOT, check_build_tools, electron_version, fail, read_pkg, set_prefix, stage_app, stage_extra, stage_game, step } from "./desktop-stage.mjs";

set_prefix("[build-installer]");

const FLAGS = new Set(process.argv.slice(2));
const NO_BUILD = FLAGS.has("--no-build");
const NO_CONVERTERS = FLAGS.has("--no-converters");
const KEEP = FLAGS.has("--keep");

function read_json5(path) {
  if (!existsSync(path)) return {};
  try {
    return json5.parse(readFileSync(path, "utf8"));
  } catch {
    fail(`json5 配置解析失败: ${path}`);
  }
}

const MIRRORS = {
  ...(read_json5(join(ROOT, "deployer.config.json5")).installer ?? {}),
  ...(read_json5(join(ROOT, "deployer.private.json5")).installer ?? {}),
};
for (const name of ["ELECTRON_BUILDER_BINARIES_MIRROR", "ELECTRON_MIRROR"]) {
  if (!process.env[name] && MIRRORS[name]) process.env[name] = MIRRORS[name];
}

const pkg = read_pkg();
const ELECTRON_VERSION = electron_version(pkg);
if (!ELECTRON_VERSION) fail("package.json 里没有 electron 版本（先 npm i -D electron）");
const BUILDER_CLI = join(ROOT, "node_modules", "electron-builder", "out", "cli", "cli.js");
if (!existsSync(BUILDER_CLI)) fail("缺少 electron-builder（先执行 npm i -D electron-builder）");
check_build_tools();

function icon_256(path) {
  if (!existsSync(path)) return false;
  const buf = readFileSync(path);
  if (buf.length < 6 || buf.readUInt16LE(2) !== 1) return false;
  const count = buf.readUInt16LE(4);
  for (let i = 0; i < count; i++) {
    const w = buf[6 + i * 16] || 256;
    const h = buf[7 + i * 16] || 256;
    if (w >= 256 && h >= 256) return true;
  }
  return false;
}

if (!NO_BUILD) {
  step("构建游戏（vite build）");
  execSync("npx vite build", { cwd: ROOT, stdio: "inherit" });
}

if (!existsSync(DIST)) fail(`找不到 ${DIST}，请先执行 npx vite build`);

const BUILD = mkdtempSync(join(tmpdir(), "lfwm-installer-"));
const APP = join(BUILD, "app");
const GAME = join(BUILD, "game");
const EXTRA = join(BUILD, "extra");
const OUT = join(BUILD, "out");
const BUILD_RES = join(BUILD, "build");
mkdirSync(APP, { recursive: true });
mkdirSync(EXTRA, { recursive: true });
mkdirSync(BUILD_RES, { recursive: true });
step(`准备临时目录: ${BUILD}`);

writeFileSync(join(BUILD, "package.json"), `${JSON.stringify({
  name: "little-fighter-wemake-installer",
  version: pkg.version,
  private: true,
}, null, 2)}\n`);

stage_game(GAME);
stage_app(APP, pkg, { updater: true });
stage_extra(EXTRA, { converters: !NO_CONVERTERS });

const win = { target: ["nsis"], executableName: "start", signExecutable: false };
if (icon_256(ICON)) win.icon = ICON;
else step("favicon.ico 不含 256x256 图像，安装包将使用默认图标（可另备一个 ≥256 的 ico）");
step("代码签名: 已禁用（不使用任何证书）");

const config = {
  appId: "ink.gim.lfwm",
  productName: APP_NAME,
  copyright: `Copyright (c) ${new Date().getFullYear()} ${pkg.author?.name ?? "Gim"}`,
  artifactName: "${productName}_${version}_Setup.${ext}",
  electronVersion: ELECTRON_VERSION,
  directories: { app: APP, output: OUT, buildResources: BUILD_RES },
  files: ["**/*"],
  asar: false,
  npmRebuild: false,
  electronLanguages: ["en-US", "zh-CN", "zh-TW"],
  extraResources: [{ from: GAME, to: "game" }],
  extraFiles: [{ from: EXTRA, to: "." }],
  win,
  nsis: {
    oneClick: false,
    perMachine: false,
    allowToChangeInstallationDirectory: true,
    runAfterFinish: true,
    deleteAppDataOnUninstall: false,
    shortcutName: APP_NAME,
  },
  publish: [{ provider: "generic", url: "https://lf.gim.ink/desktop/" }],
};
const config_path = join(BUILD, "electron-builder.json");
writeFileSync(config_path, `${JSON.stringify(config, null, 2)}\n`);

step("打包安装包（electron-builder nsis）");
try {
  execSync(`node "${BUILDER_CLI}" --config "${config_path}" --win nsis --x64 --publish never`, {
    cwd: BUILD,
    stdio: "inherit",
    env: { ...process.env, CSC_LINK: "", WIN_CSC_LINK: "", CSC_KEY_PASSWORD: "", WIN_CSC_KEY_PASSWORD: "" },
  });
} catch {
  fail("electron-builder 构建失败（若卡在下载工具，可在 deployer.config.json5 的 installer 段配置镜像，或设置 ELECTRON_BUILDER_BINARIES_MIRROR / ELECTRON_MIRROR 环境变量）");
}

const artifacts = readdirSync(OUT).filter((name) => statSync(join(OUT, name)).isFile() && (/\.(exe|blockmap)$/i.test(name) || name === "latest.yml"));
if (!artifacts.some((name) => /\.exe$/i.test(name))) fail("构建产物里找不到安装包（.exe）");

const INSTALLER_DIR = join(ROOT, "release", "installer");
mkdirSync(INSTALLER_DIR, { recursive: true });
try {
  for (const name of readdirSync(INSTALLER_DIR))
    rmSync(join(INSTALLER_DIR, name), { recursive: true, force: true, maxRetries: 3, retryDelay: 300 });
} catch {
  fail("release/installer 目录被占用，请关闭里面的程序后重试");
}
let total = 0;
for (const name of artifacts) {
  copyFileSync(join(OUT, name), join(INSTALLER_DIR, name));
  total += statSync(join(INSTALLER_DIR, name)).size;
  step(`产物 -> release/installer/${name}`);
}
step(`完成: release/installer/（${artifacts.length} 个文件，${(total / 1024 / 1024).toFixed(1)} MB）`);
step("上传整个 release/installer 到 https://lf.gim.ink/desktop/（latest.yml 里的文件名要与安装包一致）");

if (KEEP) step(`保留构建目录: ${BUILD}`);
else rmSync(BUILD, { recursive: true, force: true });
