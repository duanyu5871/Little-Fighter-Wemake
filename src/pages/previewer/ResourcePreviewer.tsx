import { useEffect, useMemo, useState } from "react";
import json5 from "json5";
import format_xml from "xml-formatter";
import type { LFW } from "@/LFW";
import { usePreviewer } from "./ctx";
import { load_image } from "./load_image";
import { ModelPreview } from "./ModelPreview";
import csses from "./styles.module.scss";

type TKind = "image" | "model" | "audio" | "text" | "binary";

const IMG_RE = /\.(png|jpe?g|gif|bmp|webp|svg)$/i;
const MODEL_RE = /\.(glb|gltf)$/i;
const AUDIO_RE = /\.(mp3|wav|ogg|oga|m4a|aac|flac|opus)$/i;
const AUDIO_MIME: Record<string, string> = {
  mp3: "audio/mpeg", wav: "audio/wav", ogg: "audio/ogg", oga: "audio/ogg",
  m4a: "audio/mp4", aac: "audio/aac", flac: "audio/flac", opus: "audio/ogg",
};
const KIND_LABEL: Record<TKind, string> = { image: "图片", model: "模型", audio: "音频", text: "文本", binary: "二进制" };

/** 文本显示上限（再大就不铺到 DOM 里了） */
const MAX_TEXT = 400_000;
/** 二进制预览只看开头这么多个字节 */
const HEX_PREVIEW = 256;

interface IContent {
  kind: TKind;
  /** 实际命中的路径（可能是 @Nx 变体） */
  path: string;
  size?: number;
  /** 图片（与 load_image 的结果同语义） */
  url?: string;
  scale?: number;
  w?: number;
  h?: number;
  /** 音频 */
  audio_url?: string;
  mime?: string;
  /** 文本 */
  text?: string;
  /** 文本的格式化版本（json5 / xml 解析成功才有，解析失败回退原文） */
  pretty?: string;
  truncated?: boolean;
  /** 二进制 */
  hex?: string;
}

/**
 * 内容嗅探：扩展名靠不住（模组里什么都有），直接看开头的字节。
 *
 * 有 NUL 或控制字符比例较高就当二进制。
 */
function looks_like_text(bytes: Uint8Array): boolean {
  const n = Math.min(bytes.length, 4096);
  if (!n) return true;
  let bad = 0;
  for (let i = 0; i < n; ++i) {
    const b = bytes[i];
    if (b === 0) return false;
    if (b < 9 || (b > 13 && b < 32)) ++bad;
  }
  return bad / n < 0.02;
}

/** 扩展名不是图片但内容是图片（模组里见过 .bin 贴图） */
function is_image_bytes(b: Uint8Array): boolean {
  if (b.length < 12) return false;
  if (b[0] === 0x89 && b[1] === 0x50 && b[2] === 0x4e && b[3] === 0x47) return true; // PNG
  if (b[0] === 0xff && b[1] === 0xd8) return true; // JPEG
  if (b[0] === 0x47 && b[1] === 0x49 && b[2] === 0x46) return true; // GIF
  if (b[0] === 0x42 && b[1] === 0x4d) return true; // BMP
  const tag = (i: number) => String.fromCharCode(b[i], b[i + 1], b[i + 2], b[i + 3]);
  return tag(0) === "RIFF" && tag(8) === "WEBP";
}

/** glTF / GLB 的魔数（GLB 容器头就是 "glTF"） */
function is_model_bytes(b: Uint8Array): boolean {
  return b.length >= 4 && b[0] === 0x67 && b[1] === 0x6c && b[2] === 0x54 && b[3] === 0x46;
}

/** json5 / xml 美化：解析失败就返回 undefined，界面回退到文件原文 */
function try_pretty(name: string, text: string): string | undefined {
  try {
    // quote 用双引号，和工具链写出的数据文件（tool/src/utils/write_obj_file.ts）保持一致
    if (/\.json5?$/i.test(name)) return json5.stringify(json5.parse(text), { space: 2, quote: '"' });
    if (/\.xml$/i.test(name)) return format_xml(text);
  } catch (e) {
    console.warn("[previewer] 格式化失败，按原文显示", e);
  }
  return void 0;
}

function to_hex(bytes: Uint8Array, max: number): string {
  const n = Math.min(bytes.length, max);
  const lines: string[] = [];
  for (let i = 0; i < n; i += 16) {
    const row = bytes.subarray(i, Math.min(i + 16, n));
    const hex = Array.from(row, (b) => b.toString(16).padStart(2, "0")).join(" ");
    const ascii = Array.from(row, (b) => (b >= 0x20 && b < 0x7f ? String.fromCharCode(b) : ".")).join("");
    lines.push(`${i.toString(16).padStart(6, "0")}  ${hex.padEnd(47)}  ${ascii}`);
  }
  return lines.join("\n");
}

function fmt_size(n: number | undefined): string {
  if (n === void 0) return "-";
  if (n < 1024) return `${n} B`;
  if (n < 1024 * 1024) return `${(n / 1024).toFixed(1)} KB`;
  return `${(n / 1024 / 1024).toFixed(2)} MB`;
}

/** 按类型读出一个资源的内容（图片继续走 load_image，其它类型自己判） */
async function load_content(lfw: LFW, path: string): Promise<IContent> {
  const as_image = async (): Promise<IContent> => {
    const it = await load_image(lfw, path);
    return {
      kind: "image",
      url: it.img.src,
      path: it.path,
      scale: it.scale,
      w: it.img.naturalWidth,
      h: it.img.naturalHeight,
    };
  };
  // 图片走既有缓存（含 @Nx 追踪与倍数信息）
  if (IMG_RE.test(path)) return as_image();

  const file = lfw.zips.find([path], true)[0]?.file;
  if (!file) throw new Error("文件不存在：" + path);
  const bytes = await file.uint8_array();
  if (is_image_bytes(bytes)) return as_image();
  if (AUDIO_RE.test(path)) {
    const ext = path.slice(path.lastIndexOf(".") + 1).toLowerCase();
    return {
      kind: "audio",
      path: file.name,
      size: bytes.length,
      audio_url: await file.blob_url(),
      mime: AUDIO_MIME[ext],
    };
  }
  // 模型要在文本之前判：GLB 里带着一大块 JSON 块
  if (MODEL_RE.test(path) || is_model_bytes(bytes)) {
    return { kind: "model", path: file.name, size: bytes.length };
  }
  if (looks_like_text(bytes)) {
    const all = new TextDecoder("utf-8").decode(bytes);
    return {
      kind: "text",
      path: file.name,
      size: bytes.length,
      text: all.slice(0, MAX_TEXT),
      pretty: try_pretty(file.name, all)?.slice(0, MAX_TEXT),
      truncated: all.length > MAX_TEXT,
    };
  }
  return { kind: "binary", path: file.name, size: bytes.length, hex: to_hex(bytes, HEX_PREVIEW) };
}

interface INode {
  name: string;
  /** 完整路径（目录是前缀，文件是整条） */
  path: string;
  dir: boolean;
  /** 目录下（含各级子目录）的文件数 */
  count: number;
  children: INode[];
}

interface IRow extends INode {
  depth: number;
  open: boolean;
}

/** 把扁平的路径列表折成一棵树（目录在前，同类按名字排序） */
function build_tree(files: string[]): INode[] {
  const root: INode = { name: "", path: "", dir: true, count: 0, children: [] };
  const dirs = new Map<string, INode>([["", root]]);
  for (const f of files) {
    const segs = f.split("/");
    let prefix = "";
    let cur = root;
    for (let i = 0; i < segs.length - 1; ++i) {
      prefix = `${prefix}${prefix ? "/" : ""}${segs[i]}`;
      let it = dirs.get(prefix);
      if (!it) {
        it = { name: segs[i], path: prefix, dir: true, count: 0, children: [] };
        dirs.set(prefix, it);
        cur.children.push(it);
      }
      cur = it;
    }
    cur.children.push({ name: segs[segs.length - 1], path: f, dir: false, count: 0, children: [] });
    // 逐级累加文件数（根节点不显示，顺手加了也无所谓）
    for (let p = cur.path; ; ) {
      const it = dirs.get(p);
      if (!it) break;
      ++it.count;
      if (!p) break;
      const i = p.lastIndexOf("/");
      p = i < 0 ? "" : p.slice(0, i);
    }
  }
  const sort = (list: INode[]) => {
    list.sort((a, b) => (a.dir === b.dir ? (a.name < b.name ? -1 : a.name > b.name ? 1 : 0) : a.dir ? -1 : 1));
    for (const it of list) if (it.dir) sort(it.children);
  };
  sort(root.children);
  return root.children;
}

/** 一个路径沿途的所有父目录 */
function ancestors_of(path: string): string[] {
  const segs = path.split("/");
  const out: string[] = [];
  for (let i = 0, p = ""; i < segs.length - 1; ++i) {
    p = `${p}${p ? "/" : ""}${segs[i]}`;
    out.push(p);
  }
  return out;
}

export function ResourcePreviewer() {
  const { lfw } = usePreviewer();
  const [keyword, set_keyword] = useState("");
  const [path, set_path] = useState("");
  const [content, set_content] = useState<IContent>();
  const [normalize, set_normalize] = useState(true);
  const [pretty, set_pretty] = useState(true);
  const [error, set_error] = useState<string>();
  const [zoom, set_zoom] = useState(1);
  /** 用户手动改过展开状态的目录（没记过的按默认规则算） */
  const [toggled, set_toggled] = useState<Record<string, boolean>>({});

  // 数据包里的全部文件（不再只列图片）
  const files = useMemo(() => {
    if (!lfw) return [];
    const set = new Set<string>();
    for (const zip of lfw.zips.zips) {
      for (const key in zip.files) set.add(key);
    }
    return [...set].sort();
  }, [lfw]);

  const tree = useMemo(() => build_tree(files), [files]);

  /** 当前选中文件沿途的目录默认展开 */
  const auto_open = useMemo(() => new Set(ancestors_of(path)), [path]);

  // 树拍平成行：第一层目录默认展开，深层跟着选中项展开；搜索时全部展开并剪掉没命中的目录
  const rows = useMemo(() => {
    const kw = keyword.trim().toLowerCase();
    const out: IRow[] = [];
    const walk = (list: INode[], depth: number) => {
      for (const it of list) {
        if (!it.dir) {
          if (kw && !it.path.toLowerCase().includes(kw)) continue;
          out.push({ ...it, depth, open: false });
          continue;
        }
        const before = out.length;
        const open = kw ? true : toggled[it.path] ?? (depth === 0 || auto_open.has(it.path));
        out.push({ ...it, depth, open });
        if (!open) continue;
        walk(it.children, depth + 1);
        // 搜索时这个目录一个命中都没有，整段撤掉
        if (kw && out.length === before + 1) out.length = before;
      }
    };
    walk(tree, 0);
    return out;
  }, [tree, keyword, toggled, auto_open]);

  const all_dirs = useMemo(() => {
    const out: string[] = [];
    const walk = (list: INode[]) => {
      for (const it of list) if (it.dir) { out.push(it.path); walk(it.children); }
    };
    walk(tree);
    return out;
  }, [tree]);

  const set_all_open = (open: boolean) => {
    const next: Record<string, boolean> = {};
    for (const d of all_dirs) next[d] = open;
    set_toggled(next);
  };

  /** 选中文件，同时把它沿途被手动收起的目录重新展开 */
  const select = (v: string) => {
    set_path(v);
    set_toggled((prev) => {
      let next: Record<string, boolean> | undefined;
      for (const d of ancestors_of(v)) {
        if (prev[d] === false) {
          next ??= { ...prev };
          delete next[d];
        }
      }
      return next ?? prev;
    });
  };

  useEffect(() => {
    if (path || !files.length) return;
    set_path(files[0]);
  }, [files, path]);

  useEffect(() => {
    if (!lfw || !path) return;
    let cancelled = false;
    set_error(undefined);
    set_content(undefined);
    set_zoom(1);
    load_content(lfw, path)
      .then((it) => { if (!cancelled) set_content(it); })
      .catch((e) => { if (!cancelled) set_error("" + e); });
    return () => { cancelled = true; };
  }, [lfw, path]);

  const raw_w = content?.w ?? 0;
  const raw_h = content?.h ?? 0;
  const pic_scale = content?.scale ?? 1;
  const dw = normalize ? raw_w / pic_scale : raw_w;
  const dh = normalize ? raw_h / pic_scale : raw_h;
  const is_image = content?.kind === "image";

  return (
    <>
      <div className={csses.stage}>
        <div className={`${csses.img_box}${is_image ? " " + csses.checker : ""}`}>
          {content?.kind === "image" && (
            <img
              className={csses.img_el}
              src={content.url}
              alt={content.path}
              style={{ width: dw, height: dh, transform: `scale(${zoom})` }}
              draggable={false}
            />
          )}
          {content?.kind === "audio" && (
            <div className={csses.center_col}>
              <audio className={csses.audio} controls src={content.audio_url} />
              <div className={csses.muted}>{fmt_size(content.size)}{content.mime ? ` · ${content.mime}` : ""}</div>
            </div>
          )}
          {content?.kind === "model" && lfw && (
            <ModelPreview key={content.path} lfw={lfw} path={content.path} />
          )}
          {content?.kind === "text" && (
            <pre className={csses.text_view}>
              {(pretty && content.pretty) || content.text}{content.truncated ? "\n\n…（已截断，只显示前 400 KB）" : ""}
            </pre>
          )}
          {content?.kind === "binary" && (
            <pre className={csses.text_view}>
              {`${fmt_size(content.size)}（仅预览前 ${HEX_PREVIEW} 字节）\n\n${content.hex}`}
            </pre>
          )}
          {!content && <div className={csses.center_text}>{error ?? (path ? "加载中…" : "未选择资源")}</div>}
        </div>
        {is_image ? (
          <div className={csses.cam_row}>
            <span className={csses.label}>缩放</span>
            <input
              type="range"
              min={0.25}
              max={8}
              step={0.25}
              value={zoom}
              onChange={(e) => set_zoom(Number(e.target.value))}
            />
            <span className={csses.muted}>{zoom.toFixed(2)}x</span>
            <button className={csses.btn} onClick={() => set_zoom(1)}>1:1</button>
            <label className={csses.check}>
              <input
                type="checkbox"
                checked={normalize}
                onChange={(e) => set_normalize(e.target.checked)}
              />
              倍数归一
            </label>
            <div className={csses.spacer} />
            <div className={csses.muted}>
              {content
                ? `${Math.round(dw)} × ${Math.round(dh)}${pic_scale > 1 ? ` · 原图 ${raw_w} × ${raw_h} @${pic_scale}x` : ""}`
                : "-"}
            </div>
          </div>
        ) : (
          <div className={csses.cam_row}>
            <span className={csses.label}>类型</span>
            <span className={csses.muted}>{content ? KIND_LABEL[content.kind] : "-"}</span>
            {content?.kind === "text" && content.pretty !== void 0 && (
              <label className={csses.check}>
                <input
                  type="checkbox"
                  checked={pretty}
                  onChange={(e) => set_pretty(e.target.checked)}
                />
                格式化
              </label>
            )}
            <div className={csses.spacer} />
            <div className={csses.muted}>{content ? fmt_size(content.size) : "-"}</div>
          </div>
        )}
        <div className={csses.toolbar}>
          <div className={csses.muted}>{content?.path ?? path}</div>
        </div>
      </div>
      <div className={csses.side}>
        <div className={`${csses.section} ${csses.section_fill}`}>
          <div className={csses.section_title}>资源（{files.length} 个文件）</div>
          <div className={csses.search_row}>
            <input
              className={csses.search}
              placeholder="搜索路径"
              value={keyword}
              onChange={(e) => set_keyword(e.target.value)}
            />
          </div>
          <div className={csses.tree_tools}>
            <button className={csses.mini_btn} onClick={() => set_all_open(true)}>展开全部</button>
            <button className={csses.mini_btn} onClick={() => set_all_open(false)}>收起全部</button>
            <div className={csses.spacer} />
            <span className={csses.muted}>{rows.length} 项</span>
          </div>
          <div className={`${csses.bg_list} ${csses.bg_list_fill}`}>
            {rows.map((r) =>
              r.dir ? (
                <button
                  key={`d:${r.path}`}
                  className={`${csses.bg_item} ${csses.bg_item_dir}`}
                  style={{ paddingLeft: 8 + r.depth * 12 }}
                  title={r.path}
                  onClick={() => set_toggled((prev) => ({ ...prev, [r.path]: !r.open }))}
                >
                  <span className={csses.tree_arrow}>{r.open ? "▾" : "▸"}</span>
                  <span className={csses.bg_name}>{r.name}</span>
                  <span className={csses.tree_count}>{r.count}</span>
                </button>
              ) : (
                <button
                  key={`f:${r.path}`}
                  className={`${csses.bg_item}${r.path === path ? " " + csses.bg_item_active : ""}`}
                  style={{ paddingLeft: 8 + r.depth * 12 + 16 }}
                  title={r.path}
                  onClick={() => select(r.path)}
                >
                  <span className={csses.bg_name}>{r.name}</span>
                </button>
              )
            )}
          </div>
        </div>
      </div>
    </>
  );
}
