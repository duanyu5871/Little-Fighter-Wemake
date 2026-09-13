/**
 * 构建后拷贝 .env 与 server.config.json5 到 dist/cjs
 *
 * 仅当对应文件存在时执行拷贝（不存在则跳过），
 * 路径基于脚本自身位置解析，与运行时的当前目录无关（跨平台）。
 */
import { copyFileSync, existsSync, mkdirSync } from 'node:fs';
import { dirname, join } from 'node:path';
import { fileURLToPath } from 'node:url';

const script_dir = dirname(fileURLToPath(import.meta.url));
const server_dir = join(script_dir, '..');

const names = ['.env', 'server.config.json5'];
const dst_dir = join(server_dir, 'dist', 'cjs');
let copied = 0;

for (const name of names) {
  const src = join(server_dir, name);
  if (!existsSync(src)) continue;
  mkdirSync(dst_dir, { recursive: true });
  copyFileSync(src, join(dst_dir, name));
  console.log(`[copy-env] copied ${src} -> ${join(dst_dir, name)}`);
  copied++;
}

if (!copied) console.log('[copy-env] nothing to copy, skip');
