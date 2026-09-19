import { existsSync, readFileSync, writeFileSync } from 'fs';
import { dirname, join, resolve } from 'path';
import { fileURLToPath } from 'url';
import json5 from 'json5';
import OSS from 'ali-oss';

const ROOT = resolve(dirname(fileURLToPath(import.meta.url)), '..');
const args = process.argv.slice(2);

function arg_value(key) {
  const idx = args.indexOf(key);
  return idx >= 0 ? args[idx + 1] : void 0;
}

const scheme = arg_value('--scheme') ?? 'oss';
const dry_run = args.includes('--dry');

function fail(msg) {
  console.error(`[latest] ${msg}`);
  process.exit(1);
}

const version_file = join(ROOT, 'dist', 'version.json');
if (!existsSync(version_file)) fail('找不到 dist/version.json，先跑一次 npm run build');

const info = JSON.parse(readFileSync(version_file, 'utf-8'));
if (typeof info?.version !== 'string' || !info.version) fail('dist/version.json 缺少 version');

const pkg = JSON.parse(readFileSync(join(ROOT, 'package.json'), 'utf-8'));
if (pkg.version !== info.version)
  console.warn(`[latest] 注意：dist 里是 v${info.version}，但 package.json 是 v${pkg.version}，dist 可能不是最新构建`);

const out_file = join(ROOT, 'latest.json');
writeFileSync(out_file, `${JSON.stringify(info, null, 2)}\n`, 'utf-8');
const short_commit = `${info.commit ?? ''}`.slice(0, 7) || 'unknown';
console.log(`[latest] 已生成 latest.json: v${info.version} (${short_commit}${info.dirty ? '-dirty' : ''})`);

if (dry_run) {
  console.log('[latest] --dry：跳过上传');
  process.exit(0);
}

const pub_conf = json5.parse(readFileSync(join(ROOT, 'deployer.config.json5'), 'utf-8'));
const pri_path = join(ROOT, 'deployer.private.json5');
if (!existsSync(pri_path)) fail('找不到 deployer.private.json5');
const pri_conf = json5.parse(readFileSync(pri_path, 'utf-8'));

const conf = { ...(pub_conf?.[scheme] ?? {}), ...(pri_conf?.[scheme] ?? {}) };
const { OSS_REGION, OSS_BUCKET, OSS_ACCESS_KEY_ID, OSS_ACCESS_KEY_SECRET } = conf;
if (!OSS_REGION || !OSS_BUCKET || !OSS_ACCESS_KEY_ID || !OSS_ACCESS_KEY_SECRET)
  fail(`配置方案 "${scheme}" 缺少 OSS 配置（OSS_REGION / OSS_BUCKET / OSS_ACCESS_KEY_ID / OSS_ACCESS_KEY_SECRET）`);

const oss = new OSS({
  region: OSS_REGION,
  bucket: OSS_BUCKET,
  accessKeyId: OSS_ACCESS_KEY_ID,
  accessKeySecret: OSS_ACCESS_KEY_SECRET,
});

await oss.put('latest.json', out_file, {
  headers: { 'Cache-Control': 'no-cache' },
});
console.log(`[latest] 已上传 latest.json: v${info.version}`);
