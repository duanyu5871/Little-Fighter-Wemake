import { inflateSync } from "fflate";

export interface IByteSource {
  readonly size: number;
  read(offset: number, length: number): Promise<Uint8Array>;
}

export function blob_source(blob: Blob): IByteSource {
  return {
    size: blob.size,
    async read(offset: number, length: number): Promise<Uint8Array> {
      return new Uint8Array(await blob.slice(offset, offset + length).arrayBuffer());
    },
  };
}

export function bytes_source(data: Uint8Array): IByteSource {
  return {
    size: data.byteLength,
    async read(offset: number, length: number): Promise<Uint8Array> {
      return data.slice(offset, Math.min(offset + length, data.byteLength));
    },
  };
}

export interface IZipEntry {
  name: string;
  flags: number;
  method: number;
  crc: number;
  csize: number;
  usize: number;
  lho: number;
}

const EOCD_SIG = 0x06054b50;
const CD_SIG = 0x02014b50;
const LFH_SIG = 0x04034b50;
const MAX_EOCD_SEARCH = 22 + 0xffff;

const UTF8 = new TextDecoder();

export class LazyZipReader {
  static async open(source: IByteSource): Promise<LazyZipReader> {
    if (source.size < 22) throw new Error("[LazyZip] too small to be a zip");
    const tail_len = Math.min(source.size, MAX_EOCD_SEARCH);
    const tail = await source.read(source.size - tail_len, tail_len);
    const dv = new DataView(tail.buffer, tail.byteOffset, tail.byteLength);
    let eocd = -1;
    for (let i = tail.byteLength - 22; i >= 0; --i) {
      if (dv.getUint32(i, true) !== EOCD_SIG) continue;
      if (dv.getUint16(i + 20, true) !== tail.byteLength - i - 22) continue;
      eocd = i;
      break;
    }
    if (eocd < 0) throw new Error("[LazyZip] end of central directory not found");
    const count = dv.getUint16(eocd + 10, true);
    const cd_size = dv.getUint32(eocd + 12, true);
    const cd_off = dv.getUint32(eocd + 16, true);
    if (count === 0xffff || cd_size === 0xffffffff || cd_off === 0xffffffff)
      throw new Error("[LazyZip] zip64 is not supported");

    const cd = await source.read(cd_off, cd_size);
    const cdv = new DataView(cd.buffer, cd.byteOffset, cd.byteLength);
    const entries: IZipEntry[] = [];
    for (let p = 0; p + 46 <= cd.byteLength;) {
      if (cdv.getUint32(p, true) !== CD_SIG)
        throw new Error(`[LazyZip] bad central directory record @${p}`);
      const flags = cdv.getUint16(p + 8, true);
      const method = cdv.getUint16(p + 10, true);
      const crc = cdv.getUint32(p + 16, true);
      const csize = cdv.getUint32(p + 20, true);
      const usize = cdv.getUint32(p + 24, true);
      const name_len = cdv.getUint16(p + 28, true);
      const extra_len = cdv.getUint16(p + 30, true);
      const comment_len = cdv.getUint16(p + 32, true);
      const lho = cdv.getUint32(p + 42, true);
      const name = UTF8.decode(cd.subarray(p + 46, p + 46 + name_len));
      entries.push({ name, flags, method, crc, csize, usize, lho });
      p += 46 + name_len + extra_len + comment_len;
    }
    if (entries.length !== count)
      throw new Error(`[LazyZip] entry count mismatch: ${entries.length} != ${count}`);
    return new LazyZipReader(source, entries);
  }

  protected _index: Map<string, IZipEntry> | null = null;

  protected constructor(
    readonly source: IByteSource,
    readonly entries: readonly IZipEntry[],
  ) { }

  find(name: string): IZipEntry | undefined {
    if (!this._index) {
      this._index = new Map();
      for (const e of this.entries) this._index.set(e.name, e);
    }
    return this._index.get(name);
  }

  async read(entry: IZipEntry): Promise<Uint8Array> {
    const head = await this.source.read(entry.lho, 30);
    const dv = new DataView(head.buffer, head.byteOffset, head.byteLength);
    if (dv.getUint32(0, true) !== LFH_SIG)
      throw new Error(`[LazyZip] bad local file header: ${entry.name}`);
    const data_off = entry.lho + 30 + dv.getUint16(26, true) + dv.getUint16(28, true);
    const raw = await this.source.read(data_off, entry.csize);
    if (entry.method === 0) return raw;
    if (entry.method === 8) return inflateSync(raw);
    throw new Error(`[LazyZip] unsupported compression method ${entry.method}: ${entry.name}`);
  }

  async read_all(): Promise<Uint8Array> {
    return this.source.read(0, this.source.size);
  }
}
