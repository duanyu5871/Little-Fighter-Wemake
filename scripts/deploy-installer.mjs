import { createHash } from 'crypto';
import { createReadStream, existsSync, readFileSync, statSync } from 'fs';
import { dirname, join, resolve } from 'path';
import { fileURLToPath } from 'url';
import json5 from 'json5';
import OSS from 'ali-oss';

const ROOT = resolve(dirname(fileURLToPath(import.meta.url)), '..');
const args = process.argv.slice(2);
const REMOTE_DIR = 'desktop';

function arg_value(key) {
  const idx = args.indexOf(key);
  return idx >= 0 ? args[idx + 1] : void 0;
}

const scheme = arg_value('--scheme') ?? 'oss';
const dry_run = args.includes('--dry');

function fail(msg) {
  console.error(`[installer] ${msg}`);
  process.exit(1);
}

const dir = join(ROOT, 'release', 'installer');
if (!existsSync(dir)) fail('找不到 release/installer，先执行 npm run build:installer');

const yml_file = join(dir, 'latest.yml');
if (!existsSync(yml_file)) fail('release/installer 里没有 latest.yml');

const yml = readFileSync(yml_file, 'utf-8');
const pick = (re) => `${yml.match(re)?.[1] ?? ''}`.trim();
const version = pick(/^version:\s*(.+)$/m);
const exe_name = pick(/^path:\s*(.+)$/m);
const sha512 = pick(/^sha512:\s*(.+)$/m);
if (!version || !exe_name || !sha512) fail('latest.yml 内容不完整（缺少 version / path / sha512）');

const exe_file = join(dir, exe_name);
if (!existsSync(exe_file)) fail(`latest.yml 指向的安装包不在目录里：${exe_name}`);

const blockmap_name = `${exe_name}.blockmap`;
const has_blockmap = existsSync(join(dir, blockmap_name));

function sha512_base64(file) {
  return new Promise((ok, ng) => {
    const hash = createHash('sha512');
    createReadStream(file)
      .on('data', (chunk) => hash.update(chunk))
      .on('error', ng)
      .on('end', () => ok(hash.digest('base64')));
  });
}

const real_sha512 = await sha512_base64(exe_file);
if (real_sha512 !== sha512)
  fail(`校验失败：${exe_name} 的 sha512 与 latest.yml 不一致（安装包和 latest.yml 不是同一次构建）`);

const pkg = JSON.parse(readFileSync(join(ROOT, 'package.json'), 'utf-8'));
if (pkg.version !== version)
  console.warn(`[installer] 注意：安装包是 v${version}，但 package.json 是 v${pkg.version}`);

const size_mb = (statSync(exe_file).size / 1024 / 1024).toFixed(1);
console.log(`[installer] 待上传: v${version}（${exe_name}，${size_mb} MB）${has_blockmap ? '，含差分 blockmap' : '，没有 blockmap（客户端将整包下载）'}`);

const upload_names = [exe_name];
if (has_blockmap) upload_names.push(blockmap_name);

if (dry_run) {
  for (const name of [...upload_names, 'latest.yml']) console.log(`[installer] --dry：跳过上传 ${REMOTE_DIR}/${name}`);
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
  timeout: 300000,
});

for (const name of upload_names) {
  let step = 10;
  await oss.multipartUpload(`${REMOTE_DIR}/${name}`, join(dir, name), {
    partSize: 8 * 1024 * 1024,
    parallel: 4,
    progress: (p) => {
      const pct = Math.min(100, Math.round(p <= 1 ? p * 100 : p));
      if (pct >= step) {
        step = pct + 10;
        console.log(`[installer] 上传中 ${pct}% ${name}`);
      }
    },
  });
  console.log(`[installer] 已上传 ${REMOTE_DIR}/${name}`);
}
await oss.put(`${REMOTE_DIR}/latest.yml`, yml_file, { headers: { 'Cache-Control': 'no-cache' } });
console.log(`[installer] 已上传 ${REMOTE_DIR}/latest.yml: v${version}`);
console.log(`[installer] 客户端检查地址：https://lf.gim.ink/${REMOTE_DIR}/latest.yml`);
