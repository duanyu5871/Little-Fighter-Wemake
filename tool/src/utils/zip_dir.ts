import fs from "fs/promises";
import { join, relative } from "path";
import { zipSync, type Zippable } from "fflate";

const FIXED_MTIME = new Date(2024, 0, 1, 0, 0, 0);
const FIXED_ATTRS = 0o100644 << 16;
const FIXED_OS = 3;
const STORE_EXTS = new Set([
  "png", "jpg", "jpeg", "gif", "webp", "avif",
  "mp3", "m4a", "aac", "ogg", "oga", "opus", "flac",
  "webm", "mp4", "zip",
]);

async function walk(dir: string, out: string[]): Promise<void> {
  const entries = await fs.readdir(dir, { withFileTypes: true });
  entries.sort((a, b) => (a.name < b.name ? -1 : a.name > b.name ? 1 : 0));
  for (const entry of entries) {
    const path = join(dir, entry.name);
    if (entry.isDirectory()) await walk(path, out);
    else if (entry.isFile()) out.push(path);
  }
}

function file_level(name: string): 0 | 9 {
  const ext = name.split(".").pop()?.toLowerCase() ?? "";
  return STORE_EXTS.has(ext) ? 0 : 9;
}

export async function zip_entries(
  entries: { file: string; name: string }[],
  zip_path: string,
): Promise<void> {
  const zippable: Zippable = {};
  for (const { file, name } of entries) {
    zippable[name] = [
      new Uint8Array(await fs.readFile(file)),
      { mtime: FIXED_MTIME, attrs: FIXED_ATTRS, os: FIXED_OS, level: file_level(name) },
    ];
  }
  await fs.writeFile(zip_path, zipSync(zippable));
}

export async function zip_dir(src_dir: string, zip_path: string): Promise<void> {
  const files: string[] = [];
  await walk(src_dir, files);
  await zip_entries(
    files.map((file) => ({
      file,
      name: relative(src_dir, file).replace(/\\/g, "/"),
    })),
    zip_path,
  );
}
