import { collisions_keeper } from "../../../../src/LFW/collision/CollisionKeeper";
import { collision_action_handlers } from "../../../../src/LFW/entity/collision_action_handlers";
import { Ditto } from "../../../../src/LFW/ditto";
import { parseValue, readCaseLines, renderValue, splitWs, num } from "./trace_util";

const out: string[] = [];
const log: string[] = [];

function flat(msg: string): string {
  return msg.replace(/\n/g, "\\n");
}

function clean(value: unknown): unknown {
  return JSON.parse(JSON.stringify(value));
}

Ditto.debug = ((msg: unknown) => {
  log.push(`dbg:${flat(String(msg))}`);
}) as never;

const handlers = collision_action_handlers as unknown as Record<string, unknown>;
for (const key of Object.keys(handlers)) {
  handlers[key] = (action: unknown, _c: unknown) => {
    log.push(`action:${key}:${renderValue(clean(action))}`);
  };
}

interface Fake {
  id: string;
  data: Record<string, unknown>;
  state: number;
}

const a: Fake = { id: "A", data: { type: 8 }, state: 0 };
const v: Fake = { id: "V", data: { type: 8 }, state: 0 };

(v as unknown as Record<string, unknown>).collided_list = {
  length: 0,
  push: (_x: unknown) => {
    log.push(`v_collided:${v.id}`);
    return 1;
  },
};
(a as unknown as Record<string, unknown>).collision_list = {
  length: 0,
  push: (_x: unknown) => {
    log.push(`a_collision:${a.id}`);
    return 1;
  },
};
(v as unknown as Record<string, unknown>).play_sound = (sounds: unknown) => {
  log.push(`sound:${renderValue(sounds)}`);
};

const collision = {
  attacker: a,
  victim: v,
  itr: undefined as Record<string, unknown> | undefined,
  bdy: undefined as Record<string, unknown> | undefined,
  handlers: [] as (() => void)[],
};

function makeStub(name: string): () => void {
  const fn = (): void => {
    log.push(`handler:${name}`);
  };
  Object.defineProperty(fn, "name", { value: name });
  return fn;
}

function seedHandlers(value: unknown): void {
  const names = Array.isArray(value) ? value.map((s) => String(s)) : [];
  collision.handlers = names.map(makeStub);
}

function attachTesters(value: unknown): Record<string, unknown> | undefined {
  const obj = value as Record<string, unknown> | undefined;
  if (!obj) return obj;
  const actions = obj.actions;
  if (!Array.isArray(actions)) return obj;
  for (const raw of actions) {
    const action = raw as Record<string, unknown>;
    const tester = action?.tester as Record<string, unknown> | undefined;
    if (!tester || typeof tester !== "object") continue;
    const r = tester.r;
    action.tester = {
      r,
      run: (_c: unknown) => {
        log.push(`tester:${renderValue(r)}`);
        return r;
      },
    };
  }
  return obj;
}

function main(): void {
  const casePath = process.argv[2];
  if (!casePath) {
    process.stderr.write("usage: lfw_trace_collision_keeper_handle.mjs <case-file>\n");
    process.exit(2);
  }

  for (const raw of readCaseLines(casePath)) {
    const t = splitWs(raw);
    if (t.length === 0) continue;
    const op = t[0]!;
    let i = 1;
    log.length = 0;
    if (op === "env") {
      const sub = t[i++]!;
      const idx = [i];
      if (sub === "dev") {
        Ditto.DEV = t[i++] === "1";
        if (i !== t.length) {
          process.stderr.write("trailing tokens after dev\n");
          process.exit(2);
        }
      } else if (sub === "data") {
        v.data = { type: v.data.type, base: parseValue(t, idx) };
      } else if (sub === "a" || sub === "v") {
        const field = t[i++]!;
        const side = sub === "a" ? a : v;
        if (field === "id") side.id = String(parseValue(t, idx));
        else if (field === "type") side.data.type = num(t[i++]!);
        else if (field === "state") side.state = num(t[i++]!);
        else {
          process.stderr.write(`unknown side field '${field}'\n`);
          process.exit(2);
        }
        if (i !== t.length) {
          process.stderr.write(`trailing tokens after side field '${field}'\n`);
          process.exit(2);
        }
      } else if (sub === "itr") {
        collision.itr = attachTesters(parseValue(t, idx)) as Record<string, unknown>;
      } else if (sub === "bdy") {
        collision.bdy = attachTesters(parseValue(t, idx)) as Record<string, unknown>;
      } else if (sub === "handlers") {
        seedHandlers(parseValue(t, idx));
      } else {
        process.stderr.write(`unknown env '${sub}'\n`);
        process.exit(2);
      }
      out.push(`env ${sub}`);
      continue;
    }
    if (op === "run") {
      collisions_keeper.handle(collision as never);
      out.push(`run hunt || ${log.join(",")}`);
      continue;
    }
    process.stderr.write(`unknown op '${op}'\n`);
    process.exit(2);
  }

  process.stdout.write(out.join("\n") + "\n");
}

main();
