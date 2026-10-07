import fs from 'node:fs';
import p from 'node:path';

const snake = (s) =>
  s
    .replace(/([a-z0-9])([A-Z])/g, '$1_$2')
    .replace(/([A-Z]+)([A-Z][a-z])/g, '$1_$2')
    .toLowerCase();

const walk = (d, out = []) => {
  for (const e of fs.readdirSync(d, { withFileTypes: true })) {
    const f = p.join(d, e.name);
    if (e.isDirectory()) walk(f, out);
    else out.push(f);
  }
  return out;
};

const native = new Set(
  walk('native/lfw').map((f) => p.basename(f).replace(/\.(h|cpp)$/, ''))
);
const runtime = (src) => {
  const code = src
    .replace(/\/\*[\s\S]*?\*\//g, '')
    .split('\n')
    .map((l) => l.replace(/\/\/.*$/, '').trim())
    .filter(Boolean)
    .filter(
      (l) =>
        !/^import\s+type\b/.test(l) &&
        !/^export\s+type\b/.test(l) &&
        !/^(export\s+)?(declare\s+)?interface\b/.test(l) &&
        !/^export\s*\*/.test(l) &&
        !/^export\s*\{[^}]*\}\s*from/.test(l) &&
        !/^(import|export)\s*\{[^}]*\}\s*from\s*['"]/.test(l) &&
        !/^(type|interface)\s/.test(l) &&
        !/^[A-Za-z_$][\w$]*(\??:|\s*\?)\s*[\w<>{}\[\]|,'". ]*;?$/.test(l)
    );
  return code.filter((l) =>
    /[=(]|=>|\bclass\b|\bfunction\b|\benum\b|\breturn\b|\bsuper\b/.test(l)
  );
};

const ts = walk('src/LFW').filter((f) => f.endsWith('.ts'));
const rows = [];
for (const f of ts) {
  const src = fs.readFileSync(f, 'utf8');
  const n = snake(p.basename(f, '.ts'));
  if (native.has(n) || native.has(n.replace(/s$/, '')) || native.has('i_' + n)) continue;
  const code = runtime(src);
  if (code.length < 12) continue;
  rows.push({ f, code: code.length, lines: src.split('\n').length });
}
rows.sort((a, b) => b.code - a.code);
console.log('候选(含运行时语句且无同名端口):', rows.length);
const limit = Number(process.argv[2]) || 30;
for (const r of rows.slice(0, limit))
  console.log(String(r.code).padStart(5), String(r.lines).padStart(5), r.f);

