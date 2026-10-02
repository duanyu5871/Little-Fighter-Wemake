import { handle_body_goto } from "../../../../src/LFW/collision/handle_body_goto";
import { handle_itr_kind_magic_flute } from "../../../../src/LFW/collision/handle_itr_kind_magic_flute";
import { handle_rest } from "../../../../src/LFW/collision/handle_rest";
import { handle_stiffness } from "../../../../src/LFW/collision/handle_stiffness";
import { handle_super_punch_me } from "../../../../src/LFW/collision/handle_super_punch_me";
import { handle_weapon_picked } from "../../../../src/LFW/collision/handle_weapon_picked";

import { parseJsStringLiteral, parseValue, readCaseLines, renderValue, splitWs } from "./trace_util";

const out: string[] = [];
const log: string[] = [];
const buffs = new Map<string, unknown>();

const state: Record<string, unknown> = {
  rest: 0,
  aid: "",
  vid: "",
  itr_motionless: undefined,
  motionless: undefined,
  shaking: undefined,
  arest: undefined,
  create_ok: true,
};

function flag(b: boolean): string {
  return b ? "b1" : "b0";
}

function main(): void {
  const casePath = process.argv[2];
  if (!casePath) {
    process.stderr.write("usage: lfw_trace_collision_handlers.mjs <case-file>\n");
    process.exit(2);
  }

  const victim: Record<string, unknown> = {
    id: "",
    add_v_rest: (c: { vid: string; rest: number }) => log.push(`add_v_rest:${c.vid}:${renderValue(c.rest)}`),
    set shaking(v: unknown) {
      state.shaking = v;
      log.push(`set_shaking:${renderValue(v)}`);
    },
  };
  const attacker: Record<string, unknown> = {
    id: "",
    pick: (v: { id: string }) => log.push(`pick:${String(attacker.id)}:${v.id}`),
    get itr_motionless() {
      log.push("get_itr_motionless");
      return state.itr_motionless;
    },
    set motionless(v: unknown) {
      state.motionless = v;
      log.push(`set_motionless:${renderValue(v)}`);
    },
    set arest(v: unknown) {
      state.arest = v;
      log.push(`set_arest:${renderValue(v)}`);
    },
  };
  const world: Record<string, unknown> = {
    dataset: {},
    buffs: {
      get: (id: string) => {
        const found = buffs.has(id);
        log.push(`buff_get:${id}:${flag(found)}`);
        return buffs.get(id);
      },
    },
    find_entity: (id: string) => undefined as unknown,
  };
  const lfw = {
    factory: {
      create_buff: (kind: string, _lfw: unknown, id: string) => {
        const ok = state.create_ok === true;
        log.push(`create:${kind}:${id}:${flag(ok)}`);
        if (!ok) return undefined;
        const buf = {
          set lifetime(_v: unknown) {
            log.push(`lifetime_zero:${id}`);
          },
          set_attacker: () => log.push(`set_attacker:${id}:${String(attacker.id)}`),
          set_victim: () => log.push(`set_victim:${id}:${String(victim.id)}`),
          mount: () => log.push(`mount:${id}`),
        };
        buffs.set(id, buf);
        return buf;
      },
    },
  };
  attacker.world = world;
  victim.world = world;
  const collision = {
    attacker,
    victim,
    world,
    lfw,
    itr: {} as Record<string, unknown>,
    rest: 0,
    aid: "",
    vid: "",
    get id() {
      return String(state.vid === "" ? "V" : state.vid);
    },
  };
  Object.defineProperty(collision, "id", { get: () => String(victim.id) });

  const stateText = (): string => {
    let s = "";
    for (const e of log) s += ` ${e}`;
    return (
      `${s} | motionless=${renderValue(state.motionless)} shaking=${renderValue(state.shaking)} ` +
      `arest=${renderValue(state.arest)} buffs=${renderValue(buffs.size)}`
    );
  };

  const run = (name: string): void => {
    log.length = 0;
    if (name === "super") {
      handle_super_punch_me(collision as never);
    } else if (name === "picked") {
      handle_weapon_picked(collision as never);
    } else if (name === "stiff") {
      handle_stiffness(collision as never);
    } else if (name === "goto") {
      handle_body_goto(collision as never);
    } else if (name === "rest") {
      handle_rest(collision as never);
    } else if (name === "flute") {
      handle_itr_kind_magic_flute(collision as never);
    } else {
      process.stderr.write(`unknown handler '${name}'\n`);
      process.exit(2);
    }
    out.push(`run ${name}${stateText()}`);
  };

  for (const raw of readCaseLines(casePath)) {
    const t = splitWs(raw);
    if (t.length === 0) continue;
    const op = t[0]!;
    if (t.length < 2) {
      process.stderr.write(`too few operands: ${raw}\n`);
      process.exit(2);
    }
    let i = 1;
    if (op === "env") {
      const sub = t[i++]!;
      if (sub === "rest") {
        collision.rest = Number(t[i++]!);
      } else if (sub === "aid") {
        collision.aid = parseJsStringLiteral(t[i++]!);
        attacker.id = collision.aid;
      } else if (sub === "vid") {
        collision.vid = parseJsStringLiteral(t[i++]!);
        victim.id = collision.vid;
      } else if (sub === "create_ok") {
        state.create_ok = t[i++] === "1";
      } else if (sub === "itr_motionless" || sub === "itr" || sub === "dataset") {
        const idx = [i];
        const v = parseValue(t, idx);
        i = idx[0]!;
        if (sub === "itr_motionless") state.itr_motionless = v;
        else if (sub === "itr") collision.itr = v as never;
        else world.dataset = v;
      } else {
        process.stderr.write(`unknown env '${sub}'\n`);
        process.exit(2);
      }
      out.push(`env ${sub}`);
      continue;
    }
    if (op === "run") {
      run(t[i++]!);
      if (i !== t.length) {
        process.stderr.write(`unexpected trailing token: ${raw}\n`);
        process.exit(2);
      }
      continue;
    }
    process.stderr.write(`unknown op '${op}'\n`);
    process.exit(2);
  }

  process.stdout.write(out.join("\n") + "\n");
}

main();
