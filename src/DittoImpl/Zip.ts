import json5 from "json5";
import { zipSync, type Zippable } from "fflate";
import type { IDownloadedZip, IReadable, IZip, IZipDownloadOpts, IZipObject } from "../LFW/ditto";
import { download_resumable, forget_stored_download, get_stored_download } from "./download/download_resumable";
import { blob_source, bytes_source, LazyZipReader, type IZipEntry } from "./LazyZip";
import { md5_buf, md5_blob } from "./md5";
import { is_str } from "../LFW/utils/type_check";

const UTF8 = new TextDecoder();
const UTF8_ENC = new TextEncoder();

const to_bytes = (data: string | Uint8Array | ArrayBuffer): Uint8Array =>
  typeof data === "string" ? UTF8_ENC.encode(data)
    : data instanceof Uint8Array ? data
      : new Uint8Array(data);

export class ZipObject implements IZipObject {
  readonly name: string;
  protected reader: LazyZipReader | null;
  protected entry: IZipEntry | null;
  protected override: Uint8Array | null;

  constructor(
    name: string,
    reader: LazyZipReader | null,
    entry: IZipEntry | null,
    override: Uint8Array | null = null,
  ) {
    this.name = name;
    this.reader = reader;
    this.entry = entry;
    this.override = override;
  }
  protected async bytes(): Promise<Uint8Array> {
    if (this.override) return this.override;
    if (!this.reader || !this.entry)
      throw new Error(`[ZipObject] no data: ${this.name}`);
    return this.reader.read(this.entry);
  }
  async text(): Promise<string> {
    return UTF8.decode(await this.bytes());
  }
  async json(): Promise<any> {
    return this.text().then(json5.parse);
  }
  async blob(): Promise<Uint8Array> {
    return await this.bytes();
  }
  async blob_url(): Promise<string> {
    return URL.createObjectURL(new Blob([await this.array_buffer()]));
  }
  async array_buffer(): Promise<ArrayBuffer> {
    const b = await this.bytes();
    return b.buffer.slice(b.byteOffset, b.byteOffset + b.byteLength) as ArrayBuffer;
  }
  async uint8_array(): Promise<Uint8Array> {
    return await this.bytes();
  }
  async image_bitmap(): Promise<ImageBitmap> {
    const buf = await this.array_buffer();
    const ext = this.name.split('.').pop()?.toLowerCase();
    const mimeMap: Record<string, string> = {
      png: 'image/png',
      jpg: 'image/jpeg',
      jpeg: 'image/jpeg',
      bmp: 'image/bmp',
      webp: 'image/webp',
      gif: 'image/gif',
    };
    const mime = mimeMap[ext || ''] || 'image/png';
    return createImageBitmap(new Blob([buf], { type: mime }));
  }
}

export class __Zip implements IZip {
  static async read_file(file: IReadable): Promise<IZip> {
    const blob = (typeof (file as { slice?: unknown }).slice === "function" ? file : null) as unknown as Blob | null;
    if (blob)
      return new __Zip(file.name, await LazyZipReader.open(blob_source(blob)), await md5_blob(blob));
    const buf = new Uint8Array(await file.arrayBuffer());
    return new __Zip(file.name, await LazyZipReader.open(bytes_source(buf)), md5_buf(buf));
  }
  static async read_buf(name: string, buf: Uint8Array): Promise<IZip> {
    return new __Zip(name, await LazyZipReader.open(bytes_source(buf)), md5_buf(buf));
  }
  static async read_blob(name: string, blob: Blob, md5?: string): Promise<IZip> {
    return new __Zip(name, await LazyZipReader.open(blob_source(blob)), md5 ?? "");
  }
  static async get_stored(url: string, md5?: string): Promise<Blob | null> {
    return await get_stored_download(url, md5);
  }
  static async forget_stored(type: string, version: number): Promise<void> {
    await forget_stored_download(type, version);
  }
  static async download(
    url: string,
    on_progress: (progress: number, size: number) => void,
    opts?: IZipDownloadOpts,
  ): Promise<IDownloadedZip> {
    return await download_resumable(url, {
      md5: opts?.md5,
      aborted: opts?.aborted,
      type: opts?.type,
      version: opts?.version,
      on_progress,
    });
  }

  readonly name: string;
  readonly md5: string;
  protected readonly reader: LazyZipReader;
  protected readonly _overrides = new Map<string, Uint8Array>();
  private _files: { [key in string]?: ZipObject } | null = null;
  private _caches: { [key in string]?: ZipObject[] } = {};

  private constructor(name: string, reader: LazyZipReader, md5: string) {
    this.name = name;
    this.reader = reader;
    this.md5 = md5;
  }

  file(path: string): ZipObject | null;
  file(path: RegExp): ZipObject[];
  file(path: string | RegExp): ZipObject | null | ZipObject[] {
    const { files } = this;
    if (is_str(path)) return files[path] ?? null;
    const flags = [...path.flags].sort().join('');
    const k = path.source + '|' + flags;
    if (this._caches[k]) return this._caches[k];
    const ret: ZipObject[] = this._caches[k] = [];
    for (const key in files) {
      const file = files[key];
      if (!file || !path.test(key)) continue;
      ret.push(file)
    }
    return ret

  }
  set(path: string, data: string | Uint8Array | ArrayBuffer): void {
    const bytes = to_bytes(data);
    this._overrides.set(path, bytes);
    if (this._files)
      this._files[path] = new ZipObject(path, this.reader, this.reader.find(path) ?? null, bytes);
  }
  async blob(): Promise<Uint8Array> {
    if (!this._overrides.size) return this.reader.read_all();
    const zippable: Zippable = {};
    for (const e of this.reader.entries)
      zippable[e.name] = this._overrides.get(e.name) ?? await this.reader.read(e);
    for (const [name, data] of this._overrides)
      if (!(name in zippable)) zippable[name] = data;
    return zipSync(zippable);
  }
  get files(): { [key in string]?: ZipObject } {
    if (this._files) return this._files;
    const files: { [key in string]?: ZipObject } = {};
    for (const e of this.reader.entries)
      files[e.name] = new ZipObject(e.name, this.reader, e, this._overrides.get(e.name) ?? null);
    for (const [name, data] of this._overrides)
      if (!files[name]) files[name] = new ZipObject(name, null, null, data);
    return this._files = files;
  }
}