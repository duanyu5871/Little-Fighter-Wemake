import { useEffect, useMemo, useState } from "react";
import type { LFW } from "@/LFW";
import { usePreviewer } from "./ctx";
import { load_image } from "./load_image";
import csses from "./styles.module.scss";

type TKind = "image" | "audio" | "text" | "binary";

const IMG_RE = /\.(png|jpe?g|gif|bmp|webp|svg)$/i;
const AUDIO_RE = /\.(mp3|wav|ogg|oga|m4a|aac|flac|opus)$/i;
const AUDIO_MIME: Record<string, string> = {
  mp3: "audio/mpeg", wav: "audio/wav", ogg: "audio/ogg", oga: "audio/ogg",
  m4a: "audio/mp4", aac: "audio/aac", flac: "audio/flac", opus: "audio/ogg",
};
const KIND_LABEL: Record<TKind, string> = { image: "图片", audio: "音频", text: "文本", binary: "二进制" };

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
  if (looks_like_text(bytes)) {
    const all = new TextDecoder("utf-8").decode(bytes);
    return {
      kind: "text",
      path: file.name,
      size: bytes.length,
      text: all.slice(0, MAX_TEXT),
      truncated: all.length > MAX_TEXT,
    };
  }
  return { kind: "binary", path: file.name, size: bytes.length, hex: to_hex(bytes, HEX_PREVIEW) };
}

export function ResourcePreviewer() {
  const { lfw } = usePreviewer();
  const [keyword, set_keyword] = useState("");
  const [path, set_path] = useState("");
  const [content, set_content] = useState<IContent>();
  const [normalize, set_normalize] = useState(true);
  const [error, set_error] = useState<string>();
  const [zoom, set_zoom] = useState(1);

  // 数据包里的全部文件（不再只列图片）
  const files = useMemo(() => {
    if (!lfw) return [];
    const set = new Set<string>();
    for (const zip of lfw.zips.zips) {
      for (const key in zip.files) set.add(key);
    }
    return [...set].sort();
  }, [lfw]);

  const shown = useMemo(() => {
    const kw = keyword.trim().toLowerCase();
    if (!kw) return files;
    return files.filter((v) => v.toLowerCase().includes(kw));
  }, [files, keyword]);

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
          {content?.kind === "text" && (
            <pre className={csses.text_view}>
              {content.text}{content.truncated ? "\n\n…（已截断，只显示前 400 KB）" : ""}
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
          <div className={csses.section_title}>资源（{shown.length}/{files.length}）</div>
          <div className={csses.search_row}>
            <input
              className={csses.search}
              placeholder="搜索路径"
              value={keyword}
              onChange={(e) => set_keyword(e.target.value)}
            />
          </div>
          <div className={csses.bg_list}>
            {shown.map((v) => (
              <button
                key={v}
                className={`${csses.bg_item}${v === path ? " " + csses.bg_item_active : ""}`}
                onClick={() => set_path(v)}
              >
                <span className={csses.bg_name}>{v}</span>
              </button>
            ))}
          </div>
        </div>
      </div>
    </>
  );
}
