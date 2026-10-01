/**
 * 逐文件检查 src/LFW 下的每个 .ts 是否只使用标准 ECMAScript。
 *
 *   node scripts/check-lfw-esonly.mjs [--all]
 *
 * 三趟检查，任一不通过就以非 0 退出：
 *   TS   用 src/LFW/tsconfig.json（lib=ES2022、types=[]）编译，取逐文件诊断
 *        —— 宿主 API 的静态引用在这里暴露（window/fetch/performance/setTimeout/...）
 *   AST  复用 eslint.config.js 里 src/LFW/** 的 no-restricted-* 规则
 *        —— eval / new Function / globalThis 这类动态逃逸在这里暴露
 *   RT   把 src/LFW/index.ts 打成 IIFE，丢进「只有 ECMAScript 内建」的 vm 沙箱求值，
 *        再用 esbuild metafile 确认每个文件都在模块图里（即其顶层代码确实跑过）
 *        —— 依赖层面的宿主调用在这里暴露
 *
 * 脚本自带探针自检（故意用 fetch），确保检查器真的抓得住。
 */
import { readdirSync } from "node:fs";
import { createRequire } from "node:module";
import path from "node:path";
import { fileURLToPath } from "node:url";
import vm from "node:vm";

const REPO = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "..");
process.chdir(REPO);

const SHOW_ALL = process.argv.includes("--all");
const SRC_DIR = "src/LFW";
const ENTRY = `${SRC_DIR}/index.ts`;
const SKIP_DIRS = new Set(["node_modules", "dist"]);
const to_posix = (p) => p.split(path.sep).join("/");
const req = (from, name) => createRequire(new URL(from, import.meta.url))(name);
const push = (map, key, value) => map.set(key, [...(map.get(key) ?? []), value]);

const ts = req("../src/LFW/package.json", "typescript");
const esbuild = req("../src/LFW/package.json", "esbuild");
const { ESLint } = req("../package.json", "eslint");

function list_files(dir) {
  const out = [];
  for (const entry of readdirSync(dir, { withFileTypes: true })) {
    const full = path.join(dir, entry.name);
    if (entry.isDirectory()) {
      if (!SKIP_DIRS.has(entry.name)) out.push(...list_files(full));
    } else if (entry.name.endsWith(".ts") && !entry.name.endsWith(".test.ts")) {
      out.push(to_posix(full));
    }
  }
  return out;
}

/** TS：lib=ES2022 + types=[] 下的逐文件诊断 */
function check_types() {
  const base = path.join(REPO, SRC_DIR);
  const raw = ts.readConfigFile(path.join(base, "tsconfig.json"), ts.sys.readFile);
  const parsed = ts.parseJsonConfigFileContent(raw.config, ts.sys, base);
  const program = ts.createProgram(parsed.fileNames, parsed.options);
  const problems = new Map();
  for (const d of ts.getPreEmitDiagnostics(program)) {
    if (!d.file || d.start === undefined) continue;
    const file = to_posix(path.relative(REPO, d.file.fileName));
    const at = d.file.getLineAndCharacterOfPosition(d.start);
    push(problems, file, `${at.line + 1}:${at.character + 1} ${ts.flattenDiagnosticMessageText(d.messageText, " ")}`);
  }
  return problems;
}

/** AST：eslint 的 no-restricted-* 命中 */
async function check_restricted() {
  const eslint = new ESLint({ cwd: REPO });
  const results = await eslint.lintFiles([`${SRC_DIR}/**/*.ts`]);
  const problems = new Map();
  for (const r of results) {
    const hits = r.messages.filter((m) => m.ruleId?.startsWith("no-restricted"));
    if (!hits.length) continue;
    const file = to_posix(path.relative(REPO, r.filePath));
    for (const m of hits) push(problems, file, `${m.line}:${m.column} ${m.message}`);
  }
  return problems;
}

/** RT：整包在裸沙箱求值 + 模块图覆盖 */
function check_runtime() {
  const built = esbuild.buildSync({
    entryPoints: [ENTRY],
    bundle: true,
    format: "iife",
    globalName: "LFWCore",
    target: "es2022",
    write: false,
    metafile: true,
    logLevel: "warning",
  });
  const covered = new Set();
  for (const input of Object.keys(built.metafile.inputs)) {
    const rel = to_posix(path.isAbsolute(input) ? path.relative(REPO, input) : input);
    if (rel.startsWith(`${SRC_DIR}/`)) covered.add(rel);
  }
  const sandbox = {};
  vm.createContext(sandbox);
  try {
    vm.runInContext(built.outputFiles[0].text, sandbox, { filename: ENTRY });
    return { covered, error: null };
  } catch (e) {
    return { covered, error: `${e.name}: ${e.message}` };
  }
}

/** 自检：探针（用 fetch）在同一个沙箱里必须报 ReferenceError */
function probe_works() {
  const built = esbuild.buildSync({
    stdin: { contents: "export const probe = fetch.length;", resolveDir: REPO, loader: "ts" },
    bundle: true,
    format: "iife",
    globalName: "Probe",
    target: "es2022",
    write: false,
    logLevel: "warning",
  });
  const sandbox = {};
  vm.createContext(sandbox);
  try {
    vm.runInContext(built.outputFiles[0].text, sandbox, { filename: "probe" });
    return false;
  } catch (e) {
    return e?.name === "ReferenceError";
  }
}

const files = list_files(SRC_DIR).sort();
const types = check_types();
const restricted = await check_restricted();
const runtime = check_runtime();
const probe = probe_works();

let clean = 0;
const failures = [];
const warnings = [];
const ok_lines = [];
for (const file of files) {
  const issues = [
    ...(types.get(file) ?? []).map((m) => `TS  ${m}`),
    ...(restricted.get(file) ?? []).map((m) => `AST ${m}`),
  ];
  if (issues.length) {
    failures.push(`FAIL ${file}`, ...issues.map((i) => `     ${i}`));
    continue;
  }
  // .d.ts 不产生运行时代码，不参与模块图覆盖。
  // 不在模块图内 = 死代码，其顶层本就不会在真实运行时执行，故只警告不算违规。
  if (!file.endsWith(".d.ts") && !runtime.covered.has(file)) {
    warnings.push(`WARN ${file}  不在 ${ENTRY} 的模块图内，顶层求值未被裸沙箱覆盖`);
    continue;
  }
  clean++;
  ok_lines.push(`OK   ${file}`);
}

const ok = probe && !runtime.error && failures.length === 0;
console.log(`自检(探针)     : ${probe ? "✓ fetch 被沙箱拦下" : "✗ 检查器失效"}`);
console.log(`裸沙箱求值     : ${runtime.error ? `✗ ${runtime.error}` : "✓ 未触碰宿主全局"}`);
console.log(`模块图覆盖     : ${runtime.covered.size} / ${files.length}`);
console.log(`结果           : ${clean} 干净 / ${failures.length} 不通过 / ${warnings.length} 警告`);
if (failures.length) console.log(`\n${failures.join("\n")}`);
if (warnings.length) console.log(`\n${warnings.join("\n")}`);
if (SHOW_ALL) console.log(`\n${ok_lines.join("\n")}`);
if (!ok) process.exitCode = 1;
