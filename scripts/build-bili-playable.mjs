import { execSync } from "node:child_process";
import { cpSync, existsSync, mkdirSync, mkdtempSync, readdirSync, rmSync, statSync } from "node:fs";
import { tmpdir } from "node:os";
import { join, relative } from "node:path";
import { zip } from "compressing";
import { APP_NAME, DIST, ICON, ROOT, check_build_tools, dir_size, electron_version, fail, quote, read_pkg, set_prefix, stage_app, stage_extra, stage_game, step, walk } from "./desktop-stage.mjs";

set_prefix("[build-playable]");
const FLAGS = new Set(process.argv.slice(2));
const NO_BUILD = FLAGS.has("--no-build");
const NO_ZIP = FLAGS.has("--no-zip");
const NO_CONVERTERS = FLAGS.has("--no-converters");
const KEEP = FLAGS.has("--keep");

const pkg = read_pkg();
const AUTHOR = pkg.author?.name ?? "Gim";
const ELECTRON_VERSION = electron_version(pkg);
if (!ELECTRON_VERSION) fail("package.json 里没有 electron 版本（先 npm i -D electron）");
check_build_tools();
const NAME = `${APP_NAME}_${pkg.version}`;

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

stage_game(GAME);
stage_app(APP, pkg);

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

stage_extra(STAGE, { converters: !NO_CONVERTERS });

const files = walk(STAGE);
const bad_names = files.filter((f) => /[^\x00-\x7F]/.test(relative(STAGE, f)));
if (bad_names.length) {
  for (const f of bad_names) console.error(`  ${relative(STAGE, f)}`);
  fail("包内存在非 ASCII 文件名（B站要求不能有）");
}
const total = dir_size(STAGE);
const total_mb = total / 1024 / 1024;
step(`包内容: ${files.length} 个路径，解包 ${total_mb.toFixed(1)} MB`);

const RELEASE = join(ROOT, "release");
const OUT_STAGE = join(RELEASE, NAME);
mkdirSync(RELEASE, { recursive: true });
mkdirSync(OUT_STAGE, { recursive: true });
try {
  for (const name of readdirSync(OUT_STAGE))
    rmSync(join(OUT_STAGE, name), { recursive: true, force: true, maxRetries: 3, retryDelay: 300 });
} catch {
  fail(`release/${NAME} 目录被占用（可能正在运行里面的 start.exe，或用资源管理器打开了它），请关闭后重试`);
}
for (const name of readdirSync(STAGE)) cpSync(join(STAGE, name), join(OUT_STAGE, name), { recursive: true });
step(`解包目录 -> release/${NAME}`);

if (!NO_ZIP) {
  const out_zip = join(RELEASE, `${NAME}.zip`);
  step(`压缩 -> release/${NAME}.zip`);
  await zip.compressDir(OUT_STAGE, out_zip, { ignoreBase: true });
  const zip_mb = statSync(out_zip).size / 1024 / 1024;
  step(`完成: release/${NAME}.zip（${zip_mb.toFixed(1)} MB）`);
  if (zip_mb > 500) fail(`超过 B站 500MB 上限（${zip_mb.toFixed(1)} MB），需要用 --no-converters 或删减资源`);
}

if (KEEP) step(`保留构建目录: ${BUILD}`);
else rmSync(BUILD, { recursive: true, force: true });
