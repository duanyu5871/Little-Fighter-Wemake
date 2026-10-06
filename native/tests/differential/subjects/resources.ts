// `Resources`（`src/LFW/Resources.ts`）的 TS 侧台面（数据包部分复用 `ZipMgr`）。
//
// 用例：`cases/resources/all.txt`。op：
//   zip    <zid> <name>                        新建假数据包
//   zfile  <zid> <path-token> miss | hit <name-token>   脚本化 `zip.file(path)`
//   zval   <zid> <path-token> <method> <value> 命中对象的读取方法返回什么（method：json / text /
//                                             blob_url / array_buffer / image_bitmap）
//   zfail  <zid> <path-token> <method> <msg>   该方法 reject（msg 是值字面量）
//   add    <zid>                               `zip_mgr.add({ zip, info: 空 info })`
//   netval <method> <value> <hit>              宿主 Importer 的方法返回 `[value, hit]`
//   netfail <method> <msg>                     宿主 Importer 的方法 reject
//   xmlparse <value> | null | fail <msg>       `Ditto.XML.parse` 的返回（`null` / `u` ⇒ 触发「解析失败」）
//   rjson <path-token> [1|0] / rres <path-token> <1|0> / rimg <path-token> <1|0> /
//   rabuf <path-token> <1|0> / rxml <path-token> [1|0]
//                                              调 `Resources.import_*`，打 data / file / origin
import { Resources } from "../../../../src/LFW/Resources";
import { ZipMgr, type ILoadedZip } from "../../../../src/LFW/ZipMgr";
import { Ditto } from "../../../../src/LFW/ditto/Instance";
import type { IZip } from "../../../../src/LFW/ditto/zip/IZip";
import type { IZipObject } from "../../../../src/LFW/ditto/zip/IZipObject";

import { keyOf, parseValue, readCaseLines, renderValue, splitWs } from "./trace_util";

const log: string[] = [];

function fail(msg: string): never {
  process.stderr.write(msg + "\n");
  process.exit(2);
}

type MethodName = "json" | "text" | "blob_url" | "array_buffer" | "image_bitmap";
const METHODS: MethodName[] = ["json", "text", "blob_url", "array_buffer", "image_bitmap"];

function method_of(name: string): MethodName {
  for (const m of METHODS) if (m === name) return m;
  return fail(`bad method '${name}'`);
}

function bytes_of(v: unknown): number[] {
  return Array.isArray(v) ? v.map((x) => Number(x) & 0xff) : [];
}

type FileScript = { name: string; values: Map<MethodName, unknown>; fails: Map<MethodName, string> };
type NetScript = { value: unknown; hit: unknown };

const zips = new Map<string, FakeZip>();
const file_scripts = new Map<string, FileScript>();
const zip_mgr = new ZipMgr();
const net = new Map<MethodName, NetScript>();
const net_fails = new Map<MethodName, string>();
let xml_result: unknown = undefined;
let xml_fails: string | undefined = undefined;
let resources: Resources;

function script_key(zid: string, path: string): string {
  return `${zid}\u0000${path}`;
}

function file_script(zid: string, path: string): FileScript {
  const key = script_key(zid, path);
  const found = file_scripts.get(key);
  if (found !== undefined) return found;
  const s: FileScript = { name: path, values: new Map(), fails: new Map() };
  file_scripts.set(key, s);
  const zip = zips.get(zid) ?? fail(`unknown zip '${zid}'`);
  zip.files.set(path, s);
  return s;
}

class FakeZipObject implements IZipObject {
  constructor(private readonly script: FileScript) {}
  get name(): string {
    return this.script.name;
  }
  private result(m: MethodName): unknown {
    const message = this.script.fails.get(m);
    if (message !== undefined) throw new Error(message);
    if (!this.script.values.has(m)) throw new Error(`unscripted ${m}`);
    const v = this.script.values.get(m);
    return m === "array_buffer" ? new Uint8Array(bytes_of(v)).buffer : v;
  }
  async text(): Promise<string> {
    return this.result("text") as string;
  }
  async json<T = any>(): Promise<T> {
    return this.result("json") as T;
  }
  async blob(): Promise<Uint8Array> {
    return new Uint8Array();
  }
  async blob_url(): Promise<string> {
    return this.result("blob_url") as string;
  }
  async array_buffer(): Promise<ArrayBuffer> {
    return this.result("array_buffer") as ArrayBuffer;
  }
  async uint8_array(): Promise<Uint8Array> {
    return new Uint8Array();
  }
  async image_bitmap(): Promise<ImageBitmap> {
    return this.result("image_bitmap") as ImageBitmap;
  }
}

class FakeZip implements IZip {
  readonly files = new Map<string, FileScript>();
  readonly md5 = "";
  constructor(readonly name: string) {}
  file(path: string): IZipObject | null {
    log.push(`call:${this.name}|${path}`);
    const script = this.files.get(path);
    return script === undefined ? null : new FakeZipObject(script);
  }
  set(_path: string, _data: string | Uint8Array | ArrayBuffer): void {
    void _path;
    void _data;
  }
  async blob(): Promise<Uint8Array> {
    return new Uint8Array();
  }
}

function install_ditto(): void {
  const required = (m: MethodName): NetScript => {
    const f = net_fails.get(m);
    if (f !== undefined) throw new Error(f);
    const s = net.get(m);
    if (s === undefined) fail(`unscripted import_as_${m}`);
    return s;
  };
  Ditto.setup({
    XML: {
      parse(text: string) {
        log.push(`xml:${String(text)}`);
        if (xml_fails !== undefined) throw new Error(xml_fails);
        return xml_result as never;
      },
    },
    Importer: {
      async import_as_json(urls: string[]) {
        log.push(`imp:json|${urls.join(",")}`);
        const s = required("json");
        return [s.value, s.hit] as never;
      },
      async import_as_blob_url(urls: string[]) {
        log.push(`imp:blob_url|${urls.join(",")}`);
        const s = required("blob_url");
        return [s.value, s.hit] as never;
      },
      async import_as_array_buffer(urls: string[]) {
        log.push(`imp:array_buffer|${urls.join(",")}`);
        const s = required("array_buffer");
        return [new Uint8Array(bytes_of(s.value)).buffer, s.hit] as never;
      },
      async import_as_image_bitmap(urls: string[]) {
        log.push(`imp:image_bitmap|${urls.join(",")}`);
        const s = required("image_bitmap");
        return [s.value, s.hit] as never;
      },
      async import_as_text(urls: string[]) {
        log.push(`imp:text|${urls.join(",")}`);
        const s = required("text");
        return [s.value, s.hit] as never;
      },
    },
  } as never);
}

function data_render(v: unknown): string {
  if (v instanceof ArrayBuffer) return renderValue(Array.from(new Uint8Array(v)));
  return renderValue(v);
}

async function call(op: string, path: string, exact: boolean | undefined): Promise<void> {
  try {
    const r =
      op === "rjson"
        ? await resources.import_json(path, exact ?? true)
        : op === "rres"
          ? await resources.import_resource(path, exact as boolean)
          : op === "rimg"
            ? await resources.import_image_bitmap(path, exact as boolean)
            : op === "rabuf"
              ? await resources.import_array_buffer(path, exact as boolean)
              : await resources.import_xml(path, exact ?? true);
    log.push(
      `${op}:${path}|data=${data_render(r.data)}|file=${renderValue(r.file)}|origin=${renderValue(r.origin)}`,
    );
  } catch (e) {
    log.push(`${op}:${path}|throw:${String((e as Error).message)}`);
  }
}

async function main(): Promise<void> {
  const casePath = process.argv[2];
  if (!casePath) fail("usage: lfw_trace_resources.mjs <case-file>");
  install_ditto();
  resources = new Resources(zip_mgr);

  for (const raw of readCaseLines(casePath)) {
    const t = splitWs(raw);
    if (t.length === 0) continue;
    const op = t[0]!;
    const i = [1];
    const next = (): string => t[i[0]!++]!;
    const arg = (): unknown => parseValue(t, i);

    if (op === "zip") {
      const zid = next();
      zips.set(zid, new FakeZip(keyOf(next())));
    } else if (op === "zfile") {
      const zid = next();
      const path = keyOf(next());
      const kind = next();
      if (kind === "miss") {
        file_scripts.delete(script_key(zid, path));
        (zips.get(zid) ?? fail("unknown zip")).files.delete(path);
      } else if (kind === "hit") {
        file_script(zid, path).name = keyOf(next());
      } else {
        fail(`bad zfile kind '${kind}'`);
      }
    } else if (op === "zval") {
      const zid = next();
      const path = keyOf(next());
      const m = method_of(next());
      file_script(zid, path).values.set(m, arg());
    } else if (op === "zfail") {
      const zid = next();
      const path = keyOf(next());
      const m = method_of(next());
      file_script(zid, path).fails.set(m, String(arg()));
    } else if (op === "add") {
      const zip = zips.get(next()) ?? fail("unknown zip");
      zip_mgr.add({ zip, info: {} } as ILoadedZip);
    } else if (op === "netval") {
      const m = method_of(next());
      net_fails.delete(m);
      const value = arg();
      const hit = arg();
      net.set(m, { value, hit });
    } else if (op === "netfail") {
      const m = method_of(next());
      net_fails.set(m, String(arg()));
    } else if (op === "xmlparse") {
      const kind = next();
      if (kind === "fail") {
        xml_fails = String(arg());
      } else if (kind === "null") {
        xml_fails = undefined;
        xml_result = null;
      } else {
        i[0]! -= 1;
        xml_fails = undefined;
        xml_result = arg();
      }
    } else if (op === "rjson" || op === "rres" || op === "rimg" || op === "rabuf" || op === "rxml") {
      const path = keyOf(next());
      const exact = i[0]! < t.length ? next() === "1" : undefined;
      await call(op, path, exact);
    } else {
      fail(`unknown op '${op}'`);
    }

    if (i[0] !== t.length) fail(`trailing token(s): ${raw}`);
    while (log.length) process.stdout.write(log.shift() + "\n");
  }
}

void main();
