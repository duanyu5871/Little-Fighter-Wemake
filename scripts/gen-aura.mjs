// 生成红书/绿书光环特效贴图: node scripts/gen-aura.mjs
// 输出: lf2s/origin/extra_data/sprite/aura_red.png / aura_green.png (512x512, 4x4 cells of 128)
// 预览: temp/aura_preview.png
import { mkdirSync, writeFileSync } from "node:fs";
import { join, dirname } from "node:path";
import { fileURLToPath } from "node:url";
import { deflateSync } from "node:zlib";

const ROOT = join(dirname(fileURLToPath(import.meta.url)), "..");
const OUT_DIR = join(ROOT, "lf2s/origin/extra_data/sprite");
const PREVIEW_PATH = join(ROOT, "temp/aura_preview.png");

const CELL = 128;
const FRAMES = 16;
const COLS = 4;
const CX = 64;
const CY = 64;
const R0 = 22;
const ALPHA_GAIN = 0.95;
const TAU = Math.PI * 2;

const CRC_TABLE = (() => {
  const t = new Uint32Array(256);
  for (let n = 0; n < 256; n++) {
    let c = n;
    for (let k = 0; k < 8; k++) c = c & 1 ? 0xedb88320 ^ (c >>> 1) : c >>> 1;
    t[n] = c >>> 0;
  }
  return t;
})();

function crc32(buf) {
  let c = 0xffffffff;
  for (let i = 0; i < buf.length; i++) c = CRC_TABLE[(c ^ buf[i]) & 0xff] ^ (c >>> 8);
  return (c ^ 0xffffffff) >>> 0;
}

function chunk(type, data) {
  const out = Buffer.alloc(12 + data.length);
  out.writeUInt32BE(data.length, 0);
  out.write(type, 4, "latin1");
  data.copy(out, 8);
  out.writeUInt32BE(crc32(out.subarray(4, 8 + data.length)), 8 + data.length);
  return out;
}

function encode_png(width, height, pixels) {
  const stride = width * 4;
  const raw = Buffer.alloc((stride + 1) * height);
  for (let y = 0; y < height; y++) {
    raw[y * (stride + 1)] = 0;
    Buffer.from(pixels.buffer, pixels.byteOffset + y * stride, stride)
      .copy(raw, y * (stride + 1) + 1);
  }
  const ihdr = Buffer.alloc(13);
  ihdr.writeUInt32BE(width, 0);
  ihdr.writeUInt32BE(height, 4);
  ihdr[8] = 8;
  ihdr[9] = 6;
  return Buffer.concat([
    Buffer.from([0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a]),
    chunk("IHDR", ihdr),
    chunk("IDAT", deflateSync(raw, { level: 9 })),
    chunk("IEND", Buffer.alloc(0)),
  ]);
}

function mulberry32(seed) {
  let a = seed >>> 0;
  return () => {
    a = (a + 0x6d2b79f5) >>> 0;
    let t = a;
    t = Math.imul(t ^ (t >>> 15), t | 1);
    t ^= t + Math.imul(t ^ (t >>> 7), t | 61);
    return ((t ^ (t >>> 14)) >>> 0) / 4294967296;
  };
}

const PALETTES = {
  red: {
    core: [1.0, 0.25, 0.12],
    hot: [1.0, 0.78, 0.55],
    glow: [0.6, 0.05, 0.0],
    fill: [1.0, 0.3, 0.15],
    spark: [1.0, 0.5, 0.3],
  },
  green: {
    core: [0.18, 1.0, 0.45],
    hot: [0.72, 1.0, 0.85],
    glow: [0.0, 0.5, 0.22],
    fill: [0.22, 1.0, 0.52],
    spark: [0.45, 1.0, 0.62],
  },
};

const ARC_COUNT = 5;
const SPARKS = (() => {
  const rnd = mulberry32(20260921);
  const list = [];
  for (let j = 0; j < 12; j++) {
    list.push({
      ang: rnd() * TAU,
      r_off: (rnd() * 2 - 1) * 5,
      size: 1.3 + rnd() * 0.9,
      phase: rnd() * TAU,
    });
  }
  return list;
})();

function resolve_frame(acc) {
  const out = new Uint8Array(CELL * CELL * 4);
  for (let i = 0; i < CELL * CELL; i++) {
    const pa = acc[i * 4 + 3];
    if (pa <= 0.002) continue;
    const inv = 1 / pa;
    out[i * 4] = Math.min(255, Math.round(acc[i * 4] * inv * 255));
    out[i * 4 + 1] = Math.min(255, Math.round(acc[i * 4 + 1] * inv * 255));
    out[i * 4 + 2] = Math.min(255, Math.round(acc[i * 4 + 2] * inv * 255));
    out[i * 4 + 3] = Math.round((1 - Math.exp(-pa * ALPHA_GAIN)) * 255);
  }
  return out;
}

function render_ring_frame(pal, f) {
  const acc = new Float32Array(CELL * CELL * 4);
  const phase = (f / FRAMES) * TAU;
  const gain = 0.93 + 0.08 * Math.sin(phase);
  const R = R0 + 0.5 * Math.sin(phase * 2 + 1.2);
  const spark_pos = SPARKS.map((s) => {
    const ang = s.ang + 0.12 * Math.sin(phase + s.phase);
    const sr = R + s.r_off + 1.2 * Math.sin(phase * 2 + s.phase * 1.7);
    return {
      x: Math.cos(ang) * sr,
      y: Math.sin(ang) * sr,
      size: s.size * (0.75 + 0.45 * Math.sin(phase * 3 + s.phase * 2.2)),
      bright: 0.55 + 0.45 * Math.sin(phase * 2.6 + s.phase * 3.1),
      phase: s.phase,
    };
  });
  const arcs = [];
  for (let k = 0; k < ARC_COUNT; k++) {
    arcs.push({
      base: (k / ARC_COUNT) * TAU + 0.1 * Math.sin(phase + k * 2.1),
      bright: 0.55 + 0.45 * Math.sin(phase * 2 + k * 1.7),
    });
  }

  for (let y = 0; y < CELL; y++) {
    for (let x = 0; x < CELL; x++) {
      const dx = x + 0.5 - CX;
      const dy = y + 0.5 - CY;
      const r = Math.hypot(dx, dy);
      if (r > R + 16) continue;
      const ang = Math.atan2(dy, dx);
      let pr = 0, pg = 0, pb = 0, pa = 0;
      const add = (col, a) => {
        if (a <= 0.0004) return;
        pr += col[0] * a;
        pg += col[1] * a;
        pb += col[2] * a;
        pa += a;
      };
      {
        const t = Math.max(0, r - R) / 7;
        add(pal.glow, 0.65 * Math.exp(-t * t * 1.5) * gain);
      }
      if (r < R + 4) {
        const q = Math.max(0, 1 - r / (R + 2));
        add(pal.fill, 0.2 * q * q * gain);
      }
      {
        const t = (r - R) / 2.0;
        const band = Math.exp(-t * t);
        add(pal.core, 1.15 * band * gain);
        add(pal.hot, 0.9 * band * band * band * gain);
        add([1, 1, 1], 0.85 * band * band * band * band * gain);
      }
      {
        const t = (r - R * 0.62) / 1.6;
        add(pal.hot, 0.22 * Math.exp(-t * t) * gain);
      }
      for (const arc of arcs) {
        let d = Math.abs(ang - arc.base) % TAU;
        if (d > Math.PI) d = TAU - d;
        const span = 0.24;
        if (d >= span) continue;
        const w = 1 - (d / span) * (d / span);
        const t = (r - R) / 2.0;
        add(pal.hot, 0.9 * w * w * arc.bright * Math.exp(-t * t) * gain);
        add([1, 1, 1], 0.4 * w * w * arc.bright * Math.exp(-t * t) * gain);
      }
      for (const s of spark_pos) {
        const sd = Math.hypot(dx - s.x, dy - s.y);
        const t = sd / (s.size * 1.6);
        add(pal.spark, 0.9 * Math.exp(-t * t) * s.bright);
        const tc = sd / (s.size * 0.9);
        add([1, 1, 1], 0.6 * Math.exp(-tc * tc) * s.bright);
      }
      const i = (y * CELL + x) * 4;
      acc[i] = pr;
      acc[i + 1] = pg;
      acc[i + 2] = pb;
      acc[i + 3] = pa;
    }
  }

  return resolve_frame(acc);
}

function seg_dist(px, py, ax, ay, bx, by) {
  const vx = bx - ax;
  const vy = by - ay;
  const wx = px - ax;
  const wy = py - ay;
  const l2 = vx * vx + vy * vy;
  let t = l2 ? (wx * vx + wy * vy) / l2 : 0;
  t = Math.max(0, Math.min(1, t));
  return Math.hypot(px - (ax + vx * t), py - (ay + vy * t));
}

function make_bolts(f) {
  const rnd = mulberry32(9000 + f * 131);
  const bolts = [];
  const count = 3 + Math.floor(rnd() * 2);
  const specs = [];
  for (let i = 0; i < 2; i++) specs.push([30 + rnd() * 14, 1.0 + rnd() * 0.4]);
  for (let i = 0; i < count; i++) specs.push([10 + rnd() * 12, 0.45 + rnd() * 0.3]);
  for (const [len, th] of specs) {
    const base = rnd() * TAU;
    const dirx = Math.cos(base);
    const diry = Math.sin(base);
    const perpx = -diry;
    const perpy = dirx;
    const n = Math.max(5, Math.round(len / 4));
    const amp = 1.6 + rnd() * 0.8;
    const r0 = 2 + rnd() * 2;
    const pts = [[dirx * r0, diry * r0]];
    for (let k = 1; k < n; k++) {
      const r = r0 + (len * k) / n;
      const off = (rnd() * 2 - 1) * amp;
      pts.push([dirx * r + perpx * off, diry * r + perpy * off]);
    }
    pts.push([dirx * (r0 + len), diry * (r0 + len)]);
    bolts.push({ pts, th });
  }
  return bolts;
}

function render_discharge_frame(pal, f) {
  const acc = new Float32Array(CELL * CELL * 4);
  const phase = (f / FRAMES) * TAU;
  const gain = 0.9 + 0.1 * Math.sin(phase * 2 + 0.7);
  const env = 0.75 + 0.25 * Math.sin(phase);
  const bolts = make_bolts(f).map((b) => ({
    pts: b.pts.map(([x, y]) => [x * env, y * env]),
    th: b.th,
  }));
  for (let y = 0; y < CELL; y++) {
    for (let x = 0; x < CELL; x++) {
      const dx = x + 0.5 - CX;
      const dy = y + 0.5 - CY;
      const r = Math.hypot(dx, dy);
      if (r > 48) continue;
      let pr = 0, pg = 0, pb = 0, pa = 0;
      const add = (col, a) => {
        if (a <= 0.0004) return;
        pr += col[0] * a;
        pg += col[1] * a;
        pb += col[2] * a;
        pa += a;
      };
      {
        const t = r / (3.5 + 0.8 * Math.sin(phase * 2));
        add(pal.hot, 1.0 * Math.exp(-t * t) * gain);
      }
      {
        const t = r / 2.6;
        add([1, 1, 1], 1.6 * Math.exp(-t * t) * gain);
      }
      {
        const t = r / 9;
        add(pal.core, 0.45 * Math.exp(-t * t) * gain * (1 - 0.6 * Math.exp(-((r / 2.8) ** 2))));
      }
      for (const bolt of bolts) {
        const nseg = bolt.pts.length - 1;
        for (let k = 0; k < nseg; k++) {
          const [ax, ay] = bolt.pts[k];
          const [bx, by] = bolt.pts[k + 1];
          const d = seg_dist(dx, dy, ax, ay, bx, by);
          const th = bolt.th * gain * (1 - 0.6 * (k / nseg));
          const wg = Math.exp(-((d / 0.5) ** 2));
          add(pal.glow, 0.35 * Math.exp(-((d / 2.4) ** 2)) * th);
          add(pal.core, 0.5 * Math.exp(-((d / 1.1) ** 2)) * th * (1 - 0.85 * wg));
          add(pal.hot, 0.7 * Math.exp(-((d / 0.6) ** 2)) * th * (1 - 0.6 * wg));
          add([1, 1, 1], 1.5 * wg * th);
        }
        const [tx, ty] = bolt.pts[bolt.pts.length - 1];
        const td = Math.hypot(dx - tx, dy - ty);
        add(pal.spark, 0.6 * Math.exp(-((td / 1.6) ** 2)) * bolt.th * gain);
        add([1, 1, 1], 0.35 * Math.exp(-((td / 0.8) ** 2)) * bolt.th * gain);
      }
      const i = (y * CELL + x) * 4;
      acc[i] = pr;
      acc[i + 1] = pg;
      acc[i + 2] = pb;
      acc[i + 3] = pa;
    }
  }
  return resolve_frame(acc);
}

const ARROWS = (() => {
  const rnd = mulberry32(77003);
  const list = [];
  for (let j = 0; j < 7; j++) {
    list.push({
      x: (rnd() * 2 - 1) * 42,
      off: rnd(),
      size: 9 + rnd() * 9,
      phase: rnd() * TAU,
      sway: 1 + rnd() * 3,
      y0: (rnd() * 2 - 1) * 4,
    });
  }
  return list;
})();

function render_arrows_frame(pal, f) {
  const acc = new Float32Array(CELL * CELL * 4);
  const phase = (f / FRAMES) * TAU;
  const gain = 0.92 + 0.08 * Math.sin(phase * 2 + 0.4);
  const parts = ARROWS.map((a) => {
    const t = (f / FRAMES + a.off) % 1;
    const env = Math.sin(Math.PI * t);
    const h = a.size * (0.55 + 0.45 * env) * gain;
    return {
      px: a.x + a.sway * Math.sin(phase + a.phase),
      py: 46 - 92 * t + a.y0,
      h,
      w: h * 0.32,
      alpha: env ** 0.8,
      twinkle: 0.8 + 0.2 * Math.sin(phase * 3 + a.phase * 2),
    };
  });
  for (let y = 0; y < CELL; y++) {
    for (let x = 0; x < CELL; x++) {
      let pr = 0, pg = 0, pb = 0, pa = 0;
      const add = (col, a) => {
        if (a <= 0.0004) return;
        pr += col[0] * a;
        pg += col[1] * a;
        pb += col[2] * a;
        pa += a;
      };
      for (const p of parts) {
        const dx = x + 0.5 - CX - p.px;
        const dy = y + 0.5 - CY - p.py;
        const h2 = p.h / 2;
        const w2 = p.w / 2;
        if (dy < -h2 - 9 || dy > h2 + 9 || Math.abs(dx) > w2 + 9) continue;
        const a = p.alpha * p.twinkle;
        const tc = Math.max(0, Math.min(1, (dy + h2) / p.h));
        const d = Math.min(w2 * tc - Math.abs(dx), dy + h2, h2 - dy);
        const inside = 1 / (1 + Math.exp(-d * 2.6));
        const gx = dx / (w2 + 3.5);
        const gy = dy / (h2 + 3.5);
        add(pal.glow, 0.6 * a * (1 - inside) * Math.exp(-(gx * gx + gy * gy) * 2.2));
        if (d < -4) continue;
        add(pal.core, 1.0 * a * inside * (1 - inside * inside * 0.7));
        add(pal.hot, 0.8 * a * inside * inside * (1 - inside * inside * 0.5));
        const wx = dx / (w2 * 1.0);
        const wy = dy / (h2 * 0.7);
        add([1, 1, 1], 2.6 * a * inside * Math.exp(-(wx * wx + wy * wy) * 0.85));
      }
      const i = (y * CELL + x) * 4;
      acc[i] = pr;
      acc[i + 1] = pg;
      acc[i + 2] = pb;
      acc[i + 3] = pa;
    }
  }
  return resolve_frame(acc);
}

function build_sheet(pal, render) {
  const W = CELL * COLS;
  const H = CELL * (FRAMES / COLS);
  const pixels = new Uint8Array(W * H * 4);
  for (let f = 0; f < FRAMES; f++) {
    const cell = render(pal, f);
    const ox = (f % COLS) * CELL;
    const oy = ((f / COLS) | 0) * CELL;
    for (let y = 0; y < CELL; y++) {
      pixels.set(
        cell.subarray(y * CELL * 4, (y + 1) * CELL * 4),
        ((oy + y) * W + ox) * 4,
      );
    }
  }
  return pixels;
}

function over_pixel(dst, i, r, g, b, a) {
  const ia = 255 - a;
  dst[i] = Math.round(r * (a / 255) + dst[i] * (ia / 255));
  dst[i + 1] = Math.round(g * (a / 255) + dst[i + 1] * (ia / 255));
  dst[i + 2] = Math.round(b * (a / 255) + dst[i + 2] * (ia / 255));
  dst[i + 3] = 255;
}

function compose_preview(sheets) {
  const W = CELL * COLS;
  const H = CELL * (FRAMES / COLS) * sheets.length;
  const dst = new Uint8Array(W * H * 4);
  for (let i = 0; i < W * H; i++) {
    dst[i * 4] = 26;
    dst[i * 4 + 1] = 26;
    dst[i * 4 + 2] = 36;
    dst[i * 4 + 3] = 255;
  }
  sheets.forEach((sheet, idx) => {
    const oy = idx * (CELL * (FRAMES / COLS));
    for (let y = 0; y < CELL * (FRAMES / COLS); y++) {
      for (let x = 0; x < W; x++) {
        const si = (y * W + x) * 4;
        const a = sheet[si + 3];
        if (!a) continue;
        over_pixel(dst, ((oy + y) * W + x) * 4, sheet[si], sheet[si + 1], sheet[si + 2], a);
      }
    }
  });
  return dst;
}

mkdirSync(OUT_DIR, { recursive: true });
mkdirSync(dirname(PREVIEW_PATH), { recursive: true });

const sheets = {};
for (const name of ["red", "green"]) {
  const render = name === "green" ? render_discharge_frame : render_arrows_frame;
  sheets[name] = build_sheet(PALETTES[name], render);
  const buf = encode_png(CELL * COLS, CELL * (FRAMES / COLS), sheets[name]);
  const path = join(OUT_DIR, `aura_${name}.png`);
  writeFileSync(path, buf);
  console.log(`[aura] ${path} (${(buf.length / 1024).toFixed(1)} KB)`);
}
writeFileSync(PREVIEW_PATH, encode_png(CELL * COLS, CELL * (FRAMES / COLS) * 2, compose_preview([sheets.red, sheets.green])));
console.log(`[aura] ${PREVIEW_PATH}`);

function zoom_crop(sheet, w, x0, y0, cw, ch, scale) {
  const out_w = cw * scale;
  const out_h = ch * scale;
  const dst = new Uint8Array(out_w * out_h * 4);
  for (let i = 0; i < out_w * out_h; i++) {
    dst[i * 4] = 26;
    dst[i * 4 + 1] = 26;
    dst[i * 4 + 2] = 36;
    dst[i * 4 + 3] = 255;
  }
  for (let y = 0; y < out_h; y++) {
    for (let x = 0; x < out_w; x++) {
      const sx = x0 + ((x / scale) | 0);
      const sy = y0 + ((y / scale) | 0);
      const si = (sy * w + sx) * 4;
      const a = sheet[si + 3];
      if (!a) continue;
      over_pixel(dst, (y * out_w + x) * 4, sheet[si], sheet[si + 1], sheet[si + 2], a);
    }
  }
  return { w: out_w, h: out_h, pixels: dst };
}

const zc = zoom_crop(sheets.green, CELL * COLS, 0, 0, 256, 256, 3);
writeFileSync(join(ROOT, "temp/aura_zoom.png"), encode_png(zc.w, zc.h, zc.pixels));
console.log(`[aura] ${join(ROOT, "temp/aura_zoom.png")}`);
