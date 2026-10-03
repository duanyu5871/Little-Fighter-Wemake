import { handle_healing } from "../../../../src/LFW/collision/handle_healing";
import { parseValue, readCaseLines, renderValue, splitWs } from "./trace_util";

const out: string[] = [];
const log: string[] = [];

const state = {
  itr: undefined as Record<string, unknown> | undefined,
};

let granted: Record<string, unknown> | undefined = undefined;
let grantedAttacker: unknown = undefined;

class Fake {
  _id: string;
  _dataset: Record<string, unknown> = {};

  constructor(id: string) {
    this._id = id;
  }

  get id(): string {
    return this._id;
  }
  dataset(key: string): unknown {
    return this._dataset[key];
  }
  buffs_set(_k: string, _b: unknown): void {}
  buffs_delete(_k: string): void {}
}

const a = new Fake("A");
const v = new Fake("V");

const buffFake = (id: string): Record<string, unknown> => {
  const buf: Record<string, unknown> = { id, lifetime: 0, duration: 0, level: 0 };
  buf.set_attacker = (x: unknown) => {
    grantedAttacker = (x as { id?: unknown } | undefined)?.id;
    return undefined;
  };
  buf.set_victim = (_x: unknown) => undefined;
  buf.mount = () => undefined;
  granted = buf;
  return buf;
};

const worldFake = {
  buffs: { get: (_id: string) => undefined as unknown },
  find_entity: (id: string) => {
    if (id === "A") return a;
    if (id === "V") return v;
    return undefined;
  },
};

const lfwFake = {
  factory: {
    create_buff: (kind: string, _lfw: unknown, id: string) => {
      log.push(`create_buff:${kind}:${id}`);
      return buffFake(id);
    },
  },
};

for (const f of [a, v]) {
  (f as unknown as Record<string, unknown>).world = worldFake;
  (f as unknown as Record<string, unknown>).lfw = lfwFake;
}

const collision = {
  attacker: a,
  victim: v,
  get itr(): Record<string, unknown> {
    return (state.itr ?? {}) as Record<string, unknown>;
  },
};

function sideText(f: Fake): string {
  return `${f._id}.dataset=${renderValue(f._dataset)}`;
}

function stateText(): string {
  let s = `${sideText(a)} ${sideText(v)} buff=`;
  if (granted === undefined) {
    s += "none";
  } else {
    s +=
      `${String(granted.id)}/${renderValue(granted.lifetime)}/${renderValue(granted.duration)}/` +
      `${renderValue(granted.level)}/${grantedAttacker === undefined ? "" : String(grantedAttacker)}`;
  }
  return s;
}

function walkSide(f: Fake, field: string, tok: string[], i: number): number {
  const idx = [i];
  if (field === "dataset") f._dataset = parseValue(tok, idx) as Record<string, unknown>;
  else {
    process.stderr.write(`unknown side field '${field}'\n`);
    process.exit(2);
  }
  return idx[0]!;
}

function main(): void {
  const casePath = process.argv[2];
  if (!casePath) {
    process.stderr.write("usage: lfw_trace_collision_healing.mjs <case-file>\n");
    process.exit(2);
  }

  for (const raw of readCaseLines(casePath)) {
    const t = splitWs(raw);
    if (t.length === 0) continue;
    const op = t[0]!;
    let i = 1;
    log.length = 0;
    granted = undefined;
    grantedAttacker = undefined;
    if (op === "env") {
      const sub = t[i++]!;
      const idx = [i];
      if (sub === "itr") {
        state.itr = parseValue(t, idx) as Record<string, unknown>;
      } else if (sub === "a" || sub === "v") {
        const f = sub === "a" ? a : v;
        const field = t[i++]!;
        i = walkSide(f, field, t, i);
        if (i !== t.length) {
          process.stderr.write(`trailing tokens after side field '${field}'\n`);
          process.exit(2);
        }
      } else {
        process.stderr.write(`unknown env '${sub}'\n`);
        process.exit(2);
      }
      out.push(`env ${sub}`);
      continue;
    }
    if (op === "run") {
      handle_healing(collision as never);
      out.push(`run heal || ${log.join(",")} | ${stateText()}`);
      continue;
    }
    process.stderr.write(`unknown op '${op}'\n`);
    process.exit(2);
  }

  process.stdout.write(out.join("\n") + "\n");
}

main();
