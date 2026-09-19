import { readFileSync, readdirSync, statSync, writeFileSync } from 'fs';
import { join, relative, sep } from 'path';
import JSZip from 'jszip';

const STORED_EXT = /\.(zip|7z|gz|png|jpe?g|gif|webp|wav|mp3|ico)$/i;

export async function zip_dir(src_dir, dst_path) {
  const zip = new JSZip();
  const walk = (dir) => {
    for (const name of readdirSync(dir)) {
      const full = join(dir, name);
      const stat = statSync(full);
      if (stat.isDirectory()) {
        walk(full);
        continue;
      }
      const rel = relative(src_dir, full).split(sep).join('/');
      zip.file(rel, readFileSync(full), {
        date: stat.mtime,
        createFolders: false,
        compression: STORED_EXT.test(rel) ? 'STORE' : 'DEFLATE',
        compressionOptions: { level: 9 },
      });
    }
  };
  walk(src_dir);
  const buf = await zip.generateAsync({
    type: 'nodebuffer',
    platform: 'UNIX',
    compression: 'DEFLATE',
    compressionOptions: { level: 9 },
  });
  writeFileSync(dst_path, buf);
  return Object.keys(zip.files).length;
}
