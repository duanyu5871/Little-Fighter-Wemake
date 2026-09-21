import fs from "fs/promises";
import path from "path";
import { warn } from "./log";

const tracked = new Set<string>();

function norm(p: string): string {
  return p.replace(/\\/g, "/");
}

export function track_output(dst_path: string): void {
  tracked.add(norm(dst_path));
}

export function reset_output_tracker(): void {
  tracked.clear();
}

export async function remove_untracked_outputs(dir: string): Promise<void> {
  const dir_norm = norm(dir);
  async function walk(d: string): Promise<boolean> {
    const names = await fs.readdir(d).catch(() => [] as string[]);
    let empty = true;
    for (const name of names) {
      const p = path.join(d, name);
      const stat = await fs.stat(p).catch(() => void 0);
      if (!stat) continue;
      if (stat.isDirectory()) {
        if (!(await walk(p))) empty = false;
        continue;
      }
      if (tracked.has(norm(p))) {
        empty = false;
        continue;
      }
      await fs.rm(p, { force: true }).catch(() => void 0);
      warn("Remove untracked:", p);
    }
    if (empty && norm(d) !== dir_norm) {
      await fs.rmdir(d).catch(() => void 0);
      return true;
    }
    return empty;
  }
  await walk(dir);
}
