import { Ditto } from "../../../../src/LFW/ditto/Instance";
import { PlayerInfo } from "../../../../src/LFW/PlayerInfo";
import { __JSON5 } from "../../../../src/DittoImpl/JSON5";

import { parseValue, readCaseLines, renderValue, splitWs } from "./trace_util";

type DataSpec = { kind: "falsy" } | { kind: "bytes"; bytes: number[] } | { kind: "other"; value: unknown };
type BlobSpec = { kind: "falsy" } | { kind: "bytes"; bytes: number[] } | { kind: "throws" };

type Entry = {
  missing?: boolean;
  getThrew?: boolean;
  delThrew?: boolean;
  data: DataSpec;
  blob: BlobSpec;
};

const entries = new Map<string, Entry>();
const players = new Map<string, PlayerInfo>();
const loaded = new Map<string, boolean>();
const log: string[] = [];

function entry(pid: string): Entry {
  let e = entries.get(pid);
  if (e === undefined) entries.set(pid, (e = { data: { kind: "falsy" }, blob: { kind: "falsy" } }));
  return e;
}

function bytes_of(v: unknown): number[] {
  if (Array.isArray(v)) return v.map((x) => Number(x) & 0xff);
  return Array.from(new TextEncoder().encode(String(v)));
}

class FakeBlob {
  constructor(private readonly spec: BlobSpec) {}
  async arrayBuffer(): Promise<ArrayBuffer> {
    if (this.spec.kind === "bytes") return new Uint8Array(this.spec.bytes).buffer;
    throw new Error("blob failed");
  }
}

function install_ditto(): void {
  const key_of = (pid: string) => "player_info_" + pid;
  Ditto.setup({
    JSON5: __JSON5,
    warn: (...args: unknown[]) => {
      log.push(`warn:${String(args[0])}`);
    },
    Cache: {
      async get(name: string) {
        log.push(`get:${name}`);
        for (const [pid, e] of entries) {
          if (key_of(pid) !== name) continue;
          if (e.getThrew) throw new Error("get failed");
          if (e.missing) return undefined;
          const data =
            e.data.kind === "bytes"
              ? new Uint8Array(e.data.bytes)
              : e.data.kind === "other"
                ? e.data.value
                : undefined;
          const cache: Record<string, unknown> = { name, data, create_date: 0, id: 1, version: 0, type: "PlayerInfo" };
          if (e.data.kind === "falsy" && e.blob.kind !== "falsy") cache.blob = new FakeBlob(e.blob);
          return cache as never;
        }
        return undefined;
      },
      async del(...names: string[]) {
        for (const name of names) {
          log.push(`del:${name}`);
          for (const [pid, e] of entries) {
            if (key_of(pid) === name && e.delThrew) throw new Error("del failed");
          }
        }
      },
      async put(data: never) {
        const d = data as unknown as { name: string; type: string; version: number; data?: Uint8Array };
        log.push(`put:${d.name}|${d.type}|${d.version}|${Array.from(d.data ?? []).join(",")}`);
      },
      async list() {
        return undefined;
      },
      async forget() {
        return 0;
      },
    },
  } as never);
}

function watch(pi: PlayerInfo, pid: string): void {
  const cb = pi.callbacks;
  cb.on("on_name_changed", (...args: unknown[]) => {
    log.push(`cb:name:${args.slice(0, 2).map(renderValue).join("|")}`);
  });
  cb.on("on_ctrl_changed", (...args: unknown[]) => {
    log.push(`cb:ctrl:${args.slice(0, 2).map(renderValue).join("|")}`);
  });
  cb.on("on_is_com_changed", (...args: unknown[]) => {
    log.push(`cb:is_com:${args.slice(0, 1).map(renderValue).join("|")}`);
  });
  cb.on("on_key_changed", (...args: unknown[]) => {
    log.push(`cb:key:${args.slice(0, 3).map(renderValue).join("|")}`);
  });
  void pid;
}

function info_of(pi: PlayerInfo): unknown {
  return (pi as unknown as { _info: unknown })._info;
}

async function main(): Promise<void> {
  const casePath = process.argv[2];
  if (!casePath) {
    process.stderr.write("usage: lfw_trace_player_info.mjs <case-file>\n");
    process.exit(2);
  }
  install_ditto();

  for (const raw of readCaseLines(casePath)) {
    const t = splitWs(raw);
    if (t.length === 0) continue;
    const op = t[0]!;
    const i = [1];
    const next = (): string => t[i[0]!++]!;
    const arg = (): unknown => parseValue(t, i);

    if (op.startsWith("cache_")) {
      const pid = next();
      const e = entry(pid);
      if (op === "cache_ok") e.data = { kind: "bytes", bytes: bytes_of((arg() as never) ?? "") };
      else if (op === "cache_bytes") e.data = { kind: "bytes", bytes: bytes_of(arg()) };
      else if (op === "cache_other") e.data = { kind: "other", value: arg() };
      else if (op === "cache_nulldata") e.data = { kind: "falsy" };
      else if (op === "cache_blob") e.blob = { kind: "bytes", bytes: bytes_of((arg() as never) ?? "") };
      else if (op === "cache_blobbytes") e.blob = { kind: "bytes", bytes: bytes_of(arg()) };
      else if (op === "cache_blobfail") e.blob = { kind: "throws" };
      else if (op === "cache_blobother") {
        arg();
        e.blob = { kind: "throws" };
      } else if (op === "cache_missing") e.missing = true;
      else if (op === "cache_getfail") e.getThrew = true;
      else if (op === "cache_delfail") e.delThrew = true;
      else {
        process.stderr.write(`unknown op '${op}'\n`);
        process.exit(2);
      }
    } else if (op === "new") {
      const pid = next();
      const name = i[0]! < t.length ? arg() : undefined;
      const local = i[0]! < t.length ? arg() : undefined;
      const mine = i[0]! < t.length ? arg() : undefined;
      const pi = new PlayerInfo(pid, name as never, local as never, mine as never);
      players.set(pid, pi);
      watch(pi, pid);
      loaded.set(pid, await pi.loaded);
      log.push(`new:${pid}`);
    } else {
      const pid = next();
      const pi = players.get(pid)!;
      if (op === "dump") {
        const loaded_v = loaded.get(pid);
        log.push(
          `dump:${pid}` +
            `|info=${renderValue(info_of(pi) as never)}` +
            `|name=${renderValue(pi.name as never)}` +
            `|ctrl=${renderValue(pi.ctrl as never)}` +
            `|local=${renderValue(pi.local as never)}` +
            `|mine=${renderValue(pi.mine as never)}` +
            `|is_com=${pi.is_com ? 1 : 0}` +
            `|loaded=${loaded_v ? 1 : 0}` +
            `|fighter=${pi.fighter ? 1 : 0}`,
        );
      } else if (op === "reload") {
        log.push(`reload:${pid}:${(await pi.load()) ? 1 : 0}`);
      } else if (op === "save") {
        await pi.save();
        log.push(`save:${pid}`);
      } else if (op === "setname") {
        pi.set_name(arg() as never, Boolean(arg()));
      } else if (op === "setctrl") {
        pi.set_ctrl(arg() as never, Boolean(arg()));
      } else if (op === "setiscom") {
        pi.set_is_com(Boolean(arg()), Boolean(arg()));
      } else if (op === "setkey") {
        const k = arg();
        const v = arg();
        const emit = Boolean(arg());
        try {
          pi.set_key(k as never, v as never, emit);
          log.push(`setkey:${pid}:ok`);
        } catch {
          log.push(`setkey:${pid}:throw`);
        }
      } else if (op === "getkey") {
        try {
          log.push(`getkey:${pid}:${renderValue(pi.get_key(arg() as never))}`);
        } catch {
          log.push(`getkey:${pid}:throw`);
        }
      } else if (op === "setfighter") {
        pi.fighter = arg() ? ({} as never) : null;
      } else {
        process.stderr.write(`unknown op '${op}'\n`);
        process.exit(2);
      }
    }

    if (i[0] !== t.length) {
      process.stderr.write(`trailing token(s): ${raw}\n`);
      process.exit(2);
    }
    while (log.length) process.stdout.write(log.shift() + "\n");
  }
}

void main();
