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
const KEEP = FLAGS.has("--keep");

const pkg = JSON.parse(readFileSync(join(ROOT, "package.json"), "utf8"));
const AUTHOR = pkg.author?.name ?? "Gim";
const ELECTRON_VERSION = String(pkg.devDependencies?.electron ?? pkg.dependencies?.electron ?? "").replace(/^[^\d]*/, "");
if (!ELECTRON_VERSION) fail("package.json 里没有 electron 版本（先 npm i -D electron）");
const APP_NAME = "Little Fighter Wemake";
const NAME = `${APP_NAME}_${pkg.version}`;
const DIST = join(ROOT, "dist");
const BRIDGE = join(ROOT, "danmu-bridge");
const APP_SRC = join(BRIDGE, "app");
const ICON = join(ROOT, "tool", "icon.ico");

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

const README_TEXT = `Little Fighter Wemake 桌面客户端

怎么用
- 直接双击 start.exe 就是游戏本体；不接直播时以单机模式运行
- 接直播弹幕：在直播姬 / 幻星互动里启动本玩法即可，身份码由平台通过 start.exe code=xxxx 传入
- 本地调试：在本目录打开 cmd，执行  start.exe code=你的主播身份码，或 start.exe --room 12345（web 模式，免密钥）

窗口
- 没有系统标题栏：按住画面上方那条半透明区域可以拖动窗口，双击该区域可最大化/还原
- 右上角按钮依次是：最小化、最大化/还原、全屏、关闭
- 关闭窗口即整个程序退出（一个直播间同时只能开启一个玩法）

配置
- config.json5 里的 app_id / access_key / access_key_secret 是该玩法的开平应用密钥
- 游戏画面、弹幕桥、战绩存档（scores.json）、运行日志（logs.txt）都在本目录
- 命令行参数（均可用环境变量或 config.json5 代替）
  code / --room / --port / --game-port / --host / --debug / --devtools
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

function run_bun(args) {
  for (const cmd of ["bun.exe", "bun"]) {
    try {
      return execFileSync(cmd, args, { stdio: "inherit" });
    } catch (e) {
      if (e.code !== "ENOENT") throw e;
    }
  }
  return execSync(["bun", ...args.map(quote)].join(" "), { stdio: "inherit" });
}

function walk(dir, out = []) {
  for (const name of readdirSync(dir)) {
    const full = join(dir, name);
    out.push(full);
    if (statSync(full).isDirectory()) walk(full, out);
  }
  return out;
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

step("打包弹幕桥（bun build bridge）");
run_bun(["build", join(BRIDGE, "index.mjs"), "--target=node", "--format=esm", "--outfile", join(APP, "bridge.bundle.mjs")]);
step("打包主进程（bun build main）");
run_bun(["build", join(APP_SRC, "main.mjs"), "--target=node", "--format=esm", "--external", "electron", "--outfile", join(APP, "main.mjs")]);
if (!existsSync(join(APP, "main.mjs")) || !existsSync(join(APP, "bridge.bundle.mjs")))
  fail("主进程/弹幕桥打包失败");

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
  step("未找到 danmu-bridge/config.json5，已放入模板（上传前请填写应用密钥）");
}
writeFileSync(join(STAGE, "readme.txt"), README_TEXT);

const files = walk(STAGE);
const bad_names = files.filter((f) => /[^\x00-\x7F]/.test(relative(STAGE, f)));
if (bad_names.length) {
  for (const f of bad_names) console.error(`  ${relative(STAGE, f)}`);
  fail("包内存在非 ASCII 文件名（B站要求不能有）");
}
const total = files.reduce((sum, f) => sum + (statSync(f).isFile() ? statSync(f).size : 0), 0);
const total_mb = total / 1024 / 1024;
step(`包内容: ${files.length} 个路径，${total_mb.toFixed(1)} MB`);
if (total_mb > 500) fail("超过 B站 500MB 上限");

if (!NO_ZIP) {
  mkdirSync(join(ROOT, "release"), { recursive: true });
  const out_zip = join(ROOT, "release", `${NAME}.zip`);
  step(`压缩 -> release/${NAME}.zip`);
  await zip.compressDir(STAGE, out_zip, { ignoreBase: true });
  step(`完成: release/${NAME}.zip（${(statSync(out_zip).size / 1024 / 1024).toFixed(1)} MB）`);
}

if (KEEP) step(`保留构建目录: ${BUILD}`);
else rmSync(BUILD, { recursive: true, force: true });
