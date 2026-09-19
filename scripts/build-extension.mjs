/**
 * Chrome Extension 构建脚本
 * 用法: node scripts/build-extension.mjs [--zip]
 * 
 * 输出到 dist-extension/，不影响 web app 的 dist/ 目录。
 * 流程:
 * 1. 清除旧 dist-extension
 * 2. 运行 vite build（输出到临时目录）
 * 3. 复制构建产物到 dist-extension
 * 4. 移除 web app manifest.json (PWA)
 * 5. 复制 Chrome extension manifest.json 并同步版本号
 * 6. 可选: 打包为 .zip
 */

import { execSync } from 'child_process';
import { copyFileSync, cpSync, existsSync, mkdirSync, readFileSync, rmSync, writeFileSync } from 'fs';
import { dirname, join, resolve } from 'path';
import { fileURLToPath } from 'url';
import { zip_dir } from './zip-dir.mjs';

const __dirname = dirname(fileURLToPath(import.meta.url));
const ROOT = resolve(__dirname, '..');
const WEB_DIST = join(ROOT, 'dist');                // web app 构建输出（保留）
const EXT_DIST = join(ROOT, 'dist-extension');       // 扩展构建输出
const EXT_SRC  = join(ROOT, 'extension');            // 扩展源文件

const pkg = JSON.parse(readFileSync(join(ROOT, 'package.json'), 'utf-8'));

// Step 1: Clean extension dist
console.log('[ext] Cleaning dist-extension...');
if (existsSync(EXT_DIST)) rmSync(EXT_DIST, { recursive: true, force: true });
mkdirSync(EXT_DIST, { recursive: true });

// Step 2: Run vite build (uses existing web build config)
console.log('[ext] Running vite build...');
execSync('npx vite build', { cwd: ROOT, stdio: 'inherit' });

// Step 3: Copy web build output to extension dist
console.log('[ext] Copying build output to dist-extension...');
cpSync(WEB_DIST, EXT_DIST, { recursive: true });

// Step 4: Remove web app manifest (PWA) — replaced by Chrome extension manifest
const pwamanifest = join(EXT_DIST, 'manifest.json');
if (existsSync(pwamanifest)) {
  console.log('[ext] Replacing PWA manifest.json with Chrome extension manifest...');
  rmSync(pwamanifest);
}

// Remove large files not needed in extension
for (const f of ['lfw.full.zip']) {
  const fp = join(EXT_DIST, f);
  if (existsSync(fp)) {
    console.log(`[ext] Removing unnecessary ${f}...`);
    rmSync(fp);
  }
}

// Step 5: Copy Chrome extension manifest + background.js + icons
console.log('[ext] Copying extension files...');
const extManifestSrc = join(EXT_SRC, 'manifest.json');
copyFileSync(extManifestSrc, join(EXT_DIST, 'manifest.json'));

const bgSrc = join(EXT_SRC, 'background.js');
if (existsSync(bgSrc)) {
  copyFileSync(bgSrc, join(EXT_DIST, 'background.js'));
}

// Copy PNG icons if present
for (const icon of ['icon16.png', 'icon48.png', 'icon128.png', 'icon256.png']) {
  const src = join(EXT_SRC, icon);
  if (existsSync(src)) copyFileSync(src, join(EXT_DIST, icon));
}
const manifestOut = JSON.parse(readFileSync(join(EXT_DIST, 'manifest.json'), 'utf-8'));
manifestOut.version = pkg.version;
writeFileSync(join(EXT_DIST, 'manifest.json'), JSON.stringify(manifestOut, null, 2));

const indexHtmlPath = join(EXT_DIST, 'index.html');
if (existsSync(indexHtmlPath)) {
  const html = readFileSync(indexHtmlPath, 'utf-8');
  const patched = html.replace(/[ \t]*<link[^>]*rel="manifest"[^>]*>\r?\n?/i, '');
  if (patched !== html) {
    writeFileSync(indexHtmlPath, patched);
    console.log('[ext] Removed PWA <link rel="manifest"> from index.html');
  }
}

// Step 6: Validate
const requiredFiles = ['index.html'];
for (const f of requiredFiles) {
  if (!existsSync(join(EXT_DIST, f))) {
    console.warn(`[ext] WARNING: "${f}" not found in dist-extension!`);
  }
}

const manifestRefs = [
  ...Object.values(manifestOut.icons ?? {}),
  ...Object.values(manifestOut.action?.default_icon ?? {}),
  manifestOut.background?.service_worker,
];
for (const f of new Set(manifestRefs.filter(v => typeof v === 'string'))) {
  if (!existsSync(join(EXT_DIST, f))) {
    console.warn(`[ext] WARNING: manifest 引用的文件缺失: ${f}`);
  }
}

console.log(`[ext] ✅ Extension built to: ${EXT_DIST}`);
console.log('[ext] → Load unpacked at chrome://extensions');
console.log(`[ext] → Web app 仍在: ${WEB_DIST}`);

// Optional: zip for Chrome Web Store
if (process.argv.includes('--zip')) {
  const ZIP_PATH = join(ROOT, `release/lfw-extension-v${pkg.version}.zip`);
  mkdirSync(dirname(ZIP_PATH), { recursive: true });
  const count = await zip_dir(EXT_DIST, ZIP_PATH);
  console.log(`[ext] ✅ Zip: ${ZIP_PATH} (${count} files)`);
}
