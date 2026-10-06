// `ZipMgr`（`src/LFW/ZipMgr.ts`）的 TS 侧台面。
//
// 用例：`cases/zip_mgr/all.txt`。op：
//   zip    <zid> <name>                 新建一个假数据包（`zip.name`）
//   zfile  <zid> <path> miss            让 `zip.file(path)` 返回 `null`
//   zfile  <zid> <path> hit <fname>     让 `zip.file(path)` 返回 `{ name: fname }`
//   info   <iid>                        新建一份空的 `IDataInfo`
//   imd5   <iid> u | z | s "md5"        设 / 删掉那份 info 的 `md5`（`u` = 没有这个键，`z` = `null`）
//   add    <zid> <iid>                  `zip_mgr.add({ zip, info })`
//   clear                               `zip_mgr.clear()`
//   dump                                打 `length` / `all` / `zips` / `md5s` / `data_infos`
//   find   <0|1> p <n> <path...>        `zip_mgr.find(paths, exact)`，逐条打 `origin` / `file.name` / `zip.name`
import { ZipMgr } from "../../../../src/LFW/ZipMgr";
import type { IDataInfo } from "../../../../src/LFW/defines/IDataInfo";
import type { IZip } from "../../../../src/LFW/ditto/zip/IZip";
import type { IZipObject } from "../../../../src/LFW/ditto/zip/IZipObject";

import { keyOf, readCaseLines, renderValue, splitWs } from "./trace_util";

const log: string[] = [];
const zips = new Map<string, FakeZip>();
const infos = new Map<string, IDataInfo>();
const zip_mgr = new ZipMgr();

function fail(msg: string): never {
  process.stderr.write(msg + "\n");
  process.exit(2);
}

class FakeZipObject {
  constructor(readonly name: string) {}
}

class FakeZip {
  readonly files = new Map<string, string>();
  constructor(readonly name: string) {}
  file(path: string): IZipObject | null {
    log.push(`call:${this.name}|${path}`);
    const file_name = this.files.get(path);
    if (file_name === undefined) return null;
    return new FakeZipObject(file_name) as never;
  }
}

function info_md5(info: IDataInfo): unknown {
  return (info as Record<string, unknown>).md5;
}

function name_of(zip: IZip): string {
  return (zip as unknown as { name: string }).name;
}

function dump(): void {
  const all = zip_mgr.all.map((v) => name_of(v.zip)).join(",");
  const zips_text = zip_mgr.zips.map((v) => name_of(v)).join(",");
  const infos = zip_mgr.data_infos;
  log.push(
    `dump|len=${zip_mgr.length}` +
      `|all=${all}` +
      `|zips=${zips_text}` +
      `|md5s=${renderValue(zip_mgr.md5s)}` +
      `|infos=${infos.length}:[${infos.map((v) => renderValue(info_md5(v))).join(",")}]`,
  );
}

function main(): void {
  const casePath = process.argv[2];
  if (!casePath) fail("usage: lfw_trace_zip_mgr.mjs <case-file>");

  for (const raw of readCaseLines(casePath)) {
    const t = splitWs(raw);
    if (t.length === 0) continue;
    const op = t[0]!;
    const i = [1];
    const next = (): string => t[i[0]!++]!;

    if (op === "zip") {
      const zid = next();
      zips.set(zid, new FakeZip(keyOf(next())));
    } else if (op === "zfile") {
      const zip = zips.get(next()) ?? fail("unknown zip");
      const path = keyOf(next());
      const kind = next();
      if (kind === "miss") zip.files.delete(path);
      else if (kind === "hit") zip.files.set(path, keyOf(next()));
      else fail(`bad zfile kind '${kind}'`);
    } else if (op === "info") {
      infos.set(next(), {});
    } else if (op === "imd5") {
      const info = infos.get(next()) ?? fail("unknown info");
      const kind = next();
      if (kind === "u") delete (info as Record<string, unknown>).md5;
      else if (kind === "z") (info as Record<string, unknown>).md5 = null;
      else if (kind === "s") (info as Record<string, unknown>).md5 = keyOf(next());
      else fail(`bad imd5 kind '${kind}'`);
    } else if (op === "add") {
      const zip = zips.get(next()) ?? fail("unknown zip");
      const info = infos.get(next()) ?? fail("unknown info");
      zip_mgr.add({ zip: zip as never, info });
    } else if (op === "clear") {
      zip_mgr.clear();
    } else if (op === "dump") {
      dump();
    } else if (op === "find") {
      const exact = next() === "1";
      if (next() !== "p") fail("find expects the 'p' marker before the path count");
      const n = Number(next());
      const paths: string[] = [];
      for (let j = 0; j < n; j++) paths.push(keyOf(next()));
      const res = zip_mgr.find(paths, exact);
      log.push(`find:n=${res.length}`);
      res.forEach((r, idx) => {
        const file = r.file as unknown as { name: string };
        log.push(`find:${idx}|${r.origin}|${file.name}|${name_of(r.zip)}`);
      });
    } else {
      fail(`unknown op '${op}'`);
    }

    if (i[0] !== t.length) fail(`trailing token(s): ${raw}`);
    while (log.length) process.stdout.write(log.shift() + "\n");
  }
}

main();
