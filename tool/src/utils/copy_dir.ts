import fs from "fs/promises";
import path from "path";
import { optimize_png } from "./optimize_png";
import { track_output } from "./output_tracker";

export async function copy_dir(src_dir_path: string, dst_dir_path: string) {
  const file_names = await fs.readdir(src_dir_path);
  await fs.mkdir(dst_dir_path).catch((_) => 0);
  for (const file_name of file_names) {
    const src_path = path.join(src_dir_path, file_name);
    const dst_path = path.join(dst_dir_path, file_name);
    const stat = await fs.stat(src_path);
    if (stat.isFile()) {
      track_output(dst_path);
      if (/\.png$/i.test(file_name)) await optimize_png(src_path, dst_path);
      else await fs.copyFile(src_path, dst_path).catch((e) => console.error(e));
    } else if (stat.isDirectory()) {
      await copy_dir(src_path, dst_path);
    }
  }
}
