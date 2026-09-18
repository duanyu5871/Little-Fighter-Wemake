import fs from "fs/promises";
import { conf } from "../conf";
import { exec_cmd, exec_cmd_capture } from "./exec_cmd";
import { find_real_cmd } from "./find_real_cmd";

export async function optimize_png(src_path: string, dst_path: string): Promise<boolean> {
  const { MAGICK_CMD } = conf();
  const real_cmd = MAGICK_CMD ? find_real_cmd(MAGICK_CMD) : "";
  if (real_cmd) {
    const tmp_path = dst_path + ".opt";
    try {
      await fs.rm(tmp_path, { force: true });
      await exec_cmd(
        real_cmd,
        src_path,
        "-strip",
        "-define",
        "png:compression-level=9",
        "-define",
        "png:exclude-chunk=time",
        tmp_path,
      );
      const metric = await exec_cmd_capture(real_cmd, "compare", "-metric", "AE", src_path, tmp_path, "null:");
      const diff = parseFloat(metric);
      const src_size = (await fs.stat(src_path)).size;
      const tmp_size = (await fs.stat(tmp_path)).size;
      if (diff === 0 && tmp_size < src_size) {
        await fs.rm(dst_path, { force: true });
        await fs.rename(tmp_path, dst_path);
        return true;
      }
    } catch (e) {
      console.error(e);
    } finally {
      await fs.rm(tmp_path, { force: true }).catch(() => void 0);
    }
  }
  await fs.copyFile(src_path, dst_path);
  return false;
}
