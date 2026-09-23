import { execFileSync, execSync } from "node:child_process";
import { copyFileSync, cpSync, existsSync, mkdirSync, mkdtempSync, readFileSync, readdirSync, rmSync, statSync, writeFileSync } from "node:fs";
import { tmpdir } from "node:os";
import { dirname, join, relative, resolve } from "node:path";
import { fileURLToPath } from "node:url";
import { zip } from "compressing";

const ROOT = resolve(dirname(fileURLToPath(import.meta.url)), "..");
const FLAGS = new Set(process.argv.slice(2));
const NO_BUILD = FLAGS.has("--no-build");
const NO_ZIP = FLAGS.has("--no-zip");
const NO_CONVERTERS = FLAGS.has("--no-converters");
const KEEP = FLAGS.has("--keep");

const pkg = JSON.parse(readFileSync(join(ROOT, "package.json"), "utf8"));
const AUTHOR = pkg.author?.name ?? "Gim";
const ELECTRON_VERSION = String(pkg.devDependencies?.electron ?? pkg.dependencies?.electron ?? "").replace(/^[^\d]*/, "");
if (!ELECTRON_VERSION) fail("package.json 里没有 electron 版本（先 npm i -D electron）");
if (!existsSync(join(ROOT, "node_modules", "esbuild", "bin", "esbuild")))
  fail("根目录缺少 esbuild（先执行 npm i）");
if (![join(ROOT, "node_modules", "ws"), join(ROOT, "desktop", "node_modules", "ws")].some((p) => existsSync(p)))
  fail("找不到 ws（根目录执行 npm i，或在 desktop 目录执行 npm i）");
const APP_NAME = "Little Fighter Wemake";
const NAME = `${APP_NAME}_${pkg.version}`;
const DIST = join(ROOT, "dist");
const BRIDGE = join(ROOT, "desktop");
const APP_SRC = join(BRIDGE, "app");
const ICON = join(ROOT, "public", "favicon.ico");
const CREATE_REQUIRE_BANNER = 'import { createRequire } from "node:module"; const require = createRequire(import.meta.url);';

const CONFIG_TEMPLATE = `{
  // B站互动玩法（开放平台）应用密钥：创作者服务中心 ▶ 我的项目 ▶ 项目详情
  app_id: "",
  access_key: "",
  access_key_secret: "",

  // 可选：本地调试用直播间号（填了不填 code 也能先用 web 模式收弹幕）
  // room: "",

  score_weights: { kills: 10, spawns: 1, cheers: 1, deads: 0, damages: 0 },
}
`;

const TOOL_CONSOLE_CMD = `@echo off
cd /d "%~dp0.."
echo Little Fighter Wemake - data tool
echo.
"%~dp0..\\start.exe" --tool help
echo.
echo Examples:
echo   start.exe --tool make-data-zip -c conf.json5
echo   start.exe --tool make-data
echo.
echo Working directory: %CD%
`;

const README_TEXT = `Little Fighter Wemake 桌面客户端

怎么用
- 直接双击 start.exe 就是游戏本体；不接直播时以单机模式运行
- 接直播弹幕：在直播姬 / 幻星互动里启动本玩法即可，身份码由平台通过 start.exe code=xxxx 传入
- 本地调试：在本目录打开 cmd，执行  start.exe code=你的主播身份码，或 start.exe --room 12345（web 模式，免密钥）
- 拖拽转换：把 LF2 目录（或 conf 文件）拖到 start.exe 上，会自动开数据工具控制台并按拖入路径开始转换

窗口
- 没有系统标题栏：按住画面上方那条半透明区域可以拖动窗口，双击该区域可最大化/还原
- 右上角按钮依次是：最小化、最大化/还原、全屏、关闭
- 关闭窗口即整个程序退出（一个直播间同时只能开启一个玩法）

配置
- config.json5 里的 app_id / access_key / access_key_secret 是该玩法的开平应用密钥
- 游戏画面、弹幕桥、联机服务器存档（ranks/）、战绩存档（scores.json）、运行日志（logs.txt）都在本目录
- 命令行参数（均可用环境变量或 config.json5 代替）
  code / --room / --port / --game-port / --host / --server / --server-port / --server-lan / --tool / --user-data / --debug / --devtools

托盘（任务栏右下角图标）
- 开启/关闭联机服务器：默认只监听本机 127.0.0.1:8080
- 勾选「允许局域网连接」后，同一网络下的其他人可以用「复制联机地址」得到的地址连你
- 打开数据工具（命令行）：在本目录开一个控制台窗口（tools\lfwm-console.cmd），里面会直接列出全部命令，
  光标已经停在本目录，直接敲  start.exe --tool make-data-zip -c conf.json5  就可以跑（无需另装 Node）

转换器
- 数据转换用的 ffmpeg 与 magick 已随包附带（tools\ 目录），数据工具会优先用它们，命令行里不用再装
- 想用自己那一份：在数据工具的配置里改 FFMPEG_CMD / MAGICK_CMD，或把 tools\ 删掉改用 PATH 里的
`;

function step(msg) {
  console.log(`[build-playable] ${msg}`);
}

function fail(msg) {
  console.error(`[build-playable] ${msg}`);
  process.exit(1);
}

function quote(v) {
  return /[\s"]/.test(v) ? `"${v.replace(/"/g, '\\"')}"` : v;
}

function run_esbuild(entry, outfile, format, extra = []) {
  execFileSync(process.execPath, [
    join(ROOT, "node_modules", "esbuild", "bin", "esbuild"),
    entry,
    "--bundle",
    "--platform=node",
    `--format=${format}`,
    "--target=node22",
    ...extra,
    `--outfile=${outfile}`,
  ], { stdio: "inherit" });
}

function walk(dir, out = []) {
  for (const name of readdirSync(dir)) {
    const full = join(dir, name);
    out.push(full);
    if (statSync(full).isDirectory()) walk(full, out);
  }
  return out;
}

function dir_size(dir) {
  return walk(dir).reduce((sum, f) => sum + (statSync(f).isFile() ? statSync(f).size : 0), 0);
}

function find_converter(cmd, env_key) {
  const override = process.env[env_key];
  if (override) {
    const p = resolve(ROOT, override);
    if (existsSync(p)) return p;
    fail(`${env_key}=${override} 找不到文件`);
  }
  try {
    return execFileSync("where", [cmd], { encoding: "utf8", stdio: ["ignore", "pipe", "ignore"] })
      .split(/\r?\n/).map((s) => s.trim()).filter(Boolean)[0] ?? "";
  } catch {
    return "";
  }
}

if (!NO_BUILD) {
  step("构建游戏（vite build）");
  execSync("npx vite build", { cwd: ROOT, stdio: "inherit" });
}

if (!existsSync(DIST)) fail(`找不到 ${DIST}，请先执行 npx vite build`);

const BUILD = mkdtempSync(join(tmpdir(), "lfwm-playable-"));
const APP = join(BUILD, "app");
const GAME = join(BUILD, "game");
mkdirSync(APP, { recursive: true });
step(`准备临时目录: ${BUILD}`);

cpSync(DIST, GAME, { recursive: true });
rmSync(join(GAME, "lfw.full.zip"), { force: true });

writeFileSync(join(APP, "package.json"), `${JSON.stringify({
  name: "little-fighter-wemake",
  productName: APP_NAME,
  version: pkg.version,
  description: "Little Fighter Wemake desktop client",
  author: AUTHOR,
  license: "UNLICENSED",
  private: true,
  type: "module",
  main: "main.mjs",
}, null, 2)}\n`);
copyFileSync(join(APP_SRC, "preload.cjs"), join(APP, "preload.cjs"));

step("打包弹幕桥（esbuild bridge）");
run_esbuild(join(BRIDGE, "index.mjs"), join(APP, "bridge.bundle.mjs"), "esm", [`--banner:js=${CREATE_REQUIRE_BANNER}`]);
step("打包主进程（esbuild main）");
run_esbuild(join(APP_SRC, "main.mjs"), join(APP, "main.mjs"), "esm", ["--external:electron"]);
step("打包内置联机服务器（esbuild server）");
run_esbuild(join(ROOT, "server", "src", "index.ts"), join(APP, "server.bundle.cjs"), "cjs");
step("打包数据工具（esbuild tool）");
run_esbuild(join(ROOT, "tool", "src", "index.ts"), join(APP, "tool.bundle.cjs"), "cjs");
copyFileSync(ICON, join(APP, "icon.ico"));
if (!existsSync(join(APP, "main.mjs")) || !existsSync(join(APP, "bridge.bundle.mjs")))
  fail("主进程/弹幕桥打包失败");
if (!existsSync(join(APP, "server.bundle.cjs")) || !existsSync(join(APP, "tool.bundle.cjs")))
  fail("服务器/数据工具打包失败");

const PACKAGER_ARGS = [
  "@electron/packager", APP, APP_NAME,
  "--platform=win32", "--arch=x64",
  `--out=${join(BUILD, "out")}`,
  `--electron-version=${ELECTRON_VERSION}`,
  "--overwrite", "--no-asar", "--quiet",
  "--executable-name=start",
  `--app-version=${pkg.version}`,
  `--app-copyright=Copyright (c) ${new Date().getFullYear()} ${AUTHOR}`,
  `--extra-resource=${GAME}`,
];
if (existsSync(ICON)) PACKAGER_ARGS.push(`--icon=${ICON}`);
step("打包 Electron 应用（@electron/packager）");
execSync(`npx ${PACKAGER_ARGS.map(quote).join(" ")}`, { cwd: ROOT, stdio: "inherit" });

const OUT_DIR = join(BUILD, "out");
const STAGE = (() => {
  for (const name of readdirSync(OUT_DIR)) {
    const dir = join(OUT_DIR, name);
    if (statSync(dir).isDirectory() && existsSync(join(dir, "start.exe"))) return dir;
  }
  return join(OUT_DIR, `${APP_NAME}-win32-x64`);
})();
if (!existsSync(join(STAGE, "start.exe"))) fail(`打包失败：找不到 ${join(STAGE, "start.exe")}`);

const KEEP_LOCALES = new Set(["en-US.pak", "zh-CN.pak", "zh-TW.pak"]);
const locales_dir = join(STAGE, "locales");
let removed_locales = 0;
if (existsSync(locales_dir)) {
  for (const name of readdirSync(locales_dir)) {
    if (KEEP_LOCALES.has(name)) continue;
    rmSync(join(locales_dir, name), { force: true });
    ++removed_locales;
  }
}
if (removed_locales) step(`精简语言包: 保留 ${[...KEEP_LOCALES].join(" / ")}，移除 ${removed_locales} 个`);

const TOOLS = join(STAGE, "tools");
mkdirSync(TOOLS, { recursive: true });
writeFileSync(join(TOOLS, "lfwm-console.cmd"), TOOL_CONSOLE_CMD.replace(/\n/g, "\r\n"));

if (!NO_CONVERTERS) {
  const ffmpeg = find_converter("ffmpeg", "FFMPEG_PATH");
  if (!ffmpeg) fail("找不到 ffmpeg（可用 FFMPEG_PATH=<路径> 指定，或加 --no-converters 不打进包里）");
  const magick = find_converter("magick", "MAGICK_PATH");
  if (!magick) fail("找不到 magick（可用 MAGICK_PATH=<路径> 指定，或加 --no-converters 不打进包里）");
  mkdirSync(TOOLS, { recursive: true });
  cpSync(ffmpeg, join(TOOLS, "ffmpeg.exe"));
  const im_dir = dirname(magick);
  const IM_SKIP = new Set(["unins000.exe", "unins000.dat", "uninstall", "www", "index.html", "ImageMagick.ico"]);
  for (const name of readdirSync(im_dir)) {
    if (IM_SKIP.has(name)) continue;
    cpSync(join(im_dir, name), join(TOOLS, name), { recursive: true });
  }
  step(`内置转换器: ffmpeg（${relative(ROOT, ffmpeg)}）+ magick（${im_dir}），tools 目录 ${(dir_size(TOOLS) / 1024 / 1024).toFixed(1)} MB`);
}

const conf_src = process.env.PLAYABLE_CONFIG
  ? resolve(ROOT, process.env.PLAYABLE_CONFIG)
  : [join(BRIDGE, "config.json5"), join(BRIDGE, "config.json")].find((p) => existsSync(p));
if (conf_src && existsSync(conf_src)) {
  copyFileSync(conf_src, join(STAGE, "config.json5"));
  const raw = readFileSync(conf_src, "utf8");
  const ready = /app_id\s*:\s*["']\S/.test(raw) && /access_key?\s*:\s*["']\S/.test(raw);
  step(`内置配置文件: ${relative(ROOT, conf_src)}${ready ? "" : "（app_id/access_key 看起来还是空的，上传前记得补）"}`);
} else {
  writeFileSync(join(STAGE, "config.json5"), CONFIG_TEMPLATE);
  step("未找到 desktop/config.json5，已放入模板（上传前请填写应用密钥）");
}
writeFileSync(join(STAGE, "readme.txt"), README_TEXT);

const files = walk(STAGE);
const bad_names = files.filter((f) => /[^\x00-\x7F]/.test(relative(STAGE, f)));
if (bad_names.length) {
  for (const f of bad_names) console.error(`  ${relative(STAGE, f)}`);
  fail("包内存在非 ASCII 文件名（B站要求不能有）");
}
const total = dir_size(STAGE);
const total_mb = total / 1024 / 1024;
step(`包内容: ${files.length} 个路径，解包 ${total_mb.toFixed(1)} MB`);

if (!NO_ZIP) {
  mkdirSync(join(ROOT, "release"), { recursive: true });
  const out_zip = join(ROOT, "release", `${NAME}.zip`);
  step(`压缩 -> release/${NAME}.zip`);
  await zip.compressDir(STAGE, out_zip, { ignoreBase: true });
  const zip_mb = statSync(out_zip).size / 1024 / 1024;
  step(`完成: release/${NAME}.zip（${zip_mb.toFixed(1)} MB）`);
  if (zip_mb > 500) fail(`超过 B站 500MB 上限（${zip_mb.toFixed(1)} MB），需要用 --no-converters 或删减资源`);
}

if (KEEP) step(`保留构建目录: ${BUILD}`);
else rmSync(BUILD, { recursive: true, force: true });
