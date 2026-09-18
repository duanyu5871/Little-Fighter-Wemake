import command_exists from "command-exists";
import { R_OK } from "constants";
import { accessSync } from "fs";
import { dirname, join } from "path";

const exact_map = new Map<string, string>()
const lax_map = new Map<string, string>()

function readable(p: string): boolean {
  try {
    accessSync(p, R_OK);
    return true;
  } catch (e) {
    return false;
  }
}

export function find_bundled_tool(name: string): string {
  const exe = process.platform === "win32" ? name + ".exe" : name;
  const plat = process.platform === "win32" ? "win" : process.platform === "darwin" ? "mac" : "linux";
  const arch = process.arch === "arm64" ? "arm64" : "x64";
  const dirs = [dirname(process.execPath)];
  if (typeof __dirname === "string") dirs.push(join(__dirname, "..", "exe", plat, arch));
  for (const dir of dirs) {
    for (const sub of ["tools/" + exe, exe]) {
      const p = join(dir, sub);
      if (readable(p)) return p;
    }
  }
  return "";
}

export function find_real_cmd(cmd: string, exact = false): string {
  const map = exact ? exact_map : lax_map;
  const real_cmd = map.get(cmd);
  if (real_cmd !== void 0) return real_cmd;

  if (!cmd.includes('/') && !cmd.includes('\\')) {
    const bundled = find_bundled_tool(cmd);
    if (bundled) {
      map.set(cmd, bundled);
      return bundled;
    }
  }

  // 找下是否存在命令。
  if (command_exists.sync(cmd)) {
    map.set(cmd, cmd);
    return cmd
  }

  const trys = [
    // cwd下找文件。
    cmd,
  ]
  if (!exact) {
    // exe目录下找。 
    trys.push(join(dirname(process.execPath), cmd))
  }

  while (1) {
    let current = trys.shift();
    if (!current) break;
    try {
      accessSync(current, R_OK);
      map.set(cmd, current);
      return current;
    } catch (e) { }
  }

  map.set(cmd, '');
  return '';
}