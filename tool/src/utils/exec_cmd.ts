import { spawn } from "child_process";

export async function exec_cmd(cmd: string, ...args: string[]) {
  await new Promise((resolve, reject) => {
    const temp = spawn(cmd, args).on("exit", resolve).on("error", reject);
    temp.stderr.on("data", (buf: Buffer) =>
      console.error("[stderr]: ", buf.toString()),
    );
  });
}

export async function exec_cmd_capture(cmd: string, ...args: string[]): Promise<string> {
  return new Promise((resolve, reject) => {
    const temp = spawn(cmd, args);
    let out = "";
    temp.stdout.on("data", (buf: Buffer) => (out += buf.toString()));
    temp.stderr.on("data", (buf: Buffer) => (out += buf.toString()));
    temp.on("exit", () => resolve(out));
    temp.on("error", reject);
  });
}
