import { strToU8, unzipSync, zipSync } from "fflate";
import { blob_source, bytes_source, LazyZipReader } from "../DittoImpl/LazyZip";

const UTF8 = new TextDecoder();

const bytes_eq = (a: Uint8Array, b: Uint8Array): boolean =>
  a.byteLength === b.byteLength && a.every((v, i) => v === b[i]);

test("lazy zip: 只读中央目录，按需读条目", async () => {
  const big = new Uint8Array(1 << 20);
  for (let i = 0; i < big.length; ++i) big[i] = i & 0xff;
  const zip = zipSync({
    "a.txt": strToU8("hello 世界"),
    "b/big.bin": [big, { level: 0 }],
  });
  const reads: [number, number][] = [];
  const src = blob_source(new Blob([zip]));
  const counted = {
    size: src.size,
    read(offset: number, length: number) {
      reads.push([offset, length]);
      return src.read(offset, length);
    },
  };
  const reader = await LazyZipReader.open(counted);
  expect(reader.entries.map((e) => e.name)).toEqual(["a.txt", "b/big.bin"]);
  const opened = reads.reduce((n, [, l]) => n + l, 0);
  expect(opened).toBeLessThan(zip.length / 2);

  reads.length = 0;
  const a = reader.find("a.txt")!;
  expect(UTF8.decode(await reader.read(a))).toBe("hello 世界");
  expect(reads.reduce((n, [, l]) => n + l, 0)).toBeLessThan(4096);

  const b = reader.find("b/big.bin")!;
  expect(bytes_eq(await reader.read(b), big)).toBe(true);
});

test("lazy zip: bytes_source 与重复读取", async () => {
  const zip = zipSync({ "x.json": strToU8('{"a":1}') });
  const reader = await LazyZipReader.open(bytes_source(zip));
  const e = reader.find("x.json")!;
  expect(UTF8.decode(await reader.read(e))).toBe('{"a":1}');
  expect(UTF8.decode(await reader.read(e))).toBe('{"a":1}');
});

test("__Zip: set/blob 往返", async () => {
  const { __Zip } = await import("../DittoImpl/Zip");
  const zip = zipSync({ "a.txt": strToU8("a"), "b/c.txt": strToU8("c") });
  const z = await __Zip.read_buf("t.zip", zip);
  expect(await z.file("a.txt")!.text()).toBe("a");
  z.set("a.txt", "A2");
  z.set("new.txt", "N");
  expect(await z.file("a.txt")!.text()).toBe("A2");
  expect(await z.file("new.txt")!.text()).toBe("N");
  const out = unzipSync(await z.blob());
  expect(UTF8.decode(out["a.txt"]!)).toBe("A2");
  expect(UTF8.decode(out["b/c.txt"]!)).toBe("c");
  expect(UTF8.decode(out["new.txt"]!)).toBe("N");
});

test("__Zip: 无覆盖时 blob 原样返回", async () => {
  const { __Zip } = await import("../DittoImpl/Zip");
  const zip = zipSync({ "a.txt": strToU8("a") });
  const z = await __Zip.read_buf("t.zip", zip);
  expect(await z.blob()).toEqual(zip);
});
