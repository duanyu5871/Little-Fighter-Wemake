import { Buff } from "../../../../src/LFW/buff/Buff";
import { grant_buff } from "../../../../src/LFW/buff/grant_buff";

import { parseJsStringLiteral, parseValue, readCaseLines, renderValue, splitWs } from "./trace_util";

const out: string[] = [];
const log: string[] = [];
const entities = new Map<string, Record<string, unknown>>();
let entitySeq = 0;
const state: Record<string, unknown> = { data: undefined, create_entity_ok: true, create_buff_ok: true };

function flag(b: boolean): string {
  return b ? "b1" : "b0";
}
function num(d: number): string {
  return renderValue(d);
}

function makeEntity(id: string): Record<string, unknown> {
  const e: Record<string, unknown> = {
    id,
    lfw,
    world,
    position: { x: 0, y: 0, z: 0 },
    frame: { centery: 0, height: 0, pic: { h: 0 } },
    exists: true,
    buffs: new Map<string, unknown>(),
    add_v_rest: () => undefined,
    pick: () => undefined,
    set_position(x: number, y: number, z: number) {
      (this as { position: { x: number; y: number; z: number } }).position = { x, y, z };
      log.push(`set_position:${id}:${num(x)},${num(y)},${num(z)}`);
    },
    set_frame(info: unknown) {
      log.push(`set_frame:${id}:${renderValue(info)}`);
    },
    enter_frame_by_id(fid: string) {
      log.push(`enter_frame:${fid}`);
    },
    attach(on: boolean) {
      log.push(`attach:${flag(on)}`);
    },
  };
  const buffs = e.buffs as Map<string, unknown>;
  const origSet = buffs.set.bind(buffs);
  buffs.set = ((k: string, v: unknown) => {
    log.push(`buffs_set:${id}:${k}`);
    return origSet(k, v);
  }) as never;
  const origDel = buffs.delete.bind(buffs);
  buffs.delete = ((k: string) => {
    log.push(`buffs_delete:${id}:${k}`);
    return origDel(k);
  }) as never;
  for (const key of ["outline_alpha", "outline_width", "outline_color"]) {
    Object.defineProperty(e, key, {
      set(v: unknown) {
        log.push(`${key === "outline_color" ? "outline_color" : key}:${
          key === "outline_color" ? renderValue(v) : num(Number(v))
        }`);
      },
      configurable: true,
    });
  }
  return e;
}

function ensure(id: string): Record<string, unknown> {
  let e = entities.get(id);
  if (!e) {
    e = makeEntity(id);
    entities.set(id, e);
  }
  return e;
}

function findEntity(id: string): Record<string, unknown> | undefined {
  const e = entities.get(id);
  if (!e || e.exists === false) return undefined;
  return e;
}

const made = new Map<string, Buff<unknown>>();

const world = {
  dataset: {},
  get buffs() {
    return {
      set: (id: string, b: unknown) => {
        log.push(`world_buffs_set:${id}`);
        made.set(id, b as Buff<unknown>);
      },
      get: (id: string) => {
        const found = made.has(id);
        log.push(`world_buffs_get:${id}:${flag(found)}`);
        return made.get(id);
      },
    };
  },
  find_entity: (id: string) => findEntity(id),
};

class TestBuff extends Buff<unknown> {
  hooks = "none";
  _oid = "";
  _fid = "0";
  get effect_oid(): string {
    return this._oid;
  }
  get effect_frame_id(): string {
    return this._fid;
  }
}

const lfw: Record<string, unknown> = {
  world,
  datas: {
    find: (oid: string) => {
      log.push(`find_data:${oid}`);
      return state.data;
    },
  },
  factory: {
    create_entity: (_world: unknown, _data: unknown) => {
      if (state.create_entity_ok !== true) return undefined;
      const id = `E${++entitySeq}`;
      log.push(`create_entity:${id}`);
      return ensure(id);
    },
    create_buff: (kind: string, _lfw: unknown, id: string) => {
      const ok = state.create_buff_ok === true;
      log.push(`create_buff:${kind}:${id}:${flag(ok)}`);
      if (!ok) return undefined;
      const b = new TestBuff(lfw as never, id, kind);
      made.set(id, b);
      return b;
    },
  },
};

let buff = new TestBuff(lfw as never, "", "k");

function logText(): string {
  let s = "";
  for (const e of log) s += ` ${e}`;
  return s;
}

function buffTextOf(b: Buff<unknown>): string {
  const victims = [...b.victims];
  const atk = b.attacter;
  return (
    ` id=${b.id} lvl=${num(b.level)} ` +
    `mounted=${flag(Boolean((b as never as { _mounted: boolean })._mounted))} ` +
    `lifetime=${num(b.lifetime)} duration=${num(b.duration)} ticks=${num(b.ticks)} ` +
    `dead=${flag(b.dead)} aid=${renderValue(b.to_snapshot().attacker_id)} ` +
    `atk=${renderValue(atk ? atk.id : undefined)} ` +
    `nfx=${num(((b as never as { _effects: Map<string, unknown> })._effects ?? new Map()).size)} ` +
    `victims=[${victims.join(",")}]`
  );
}

function buffText(): string {
  return buffTextOf(buff);
}

function main(): void {
  const casePath = process.argv[2];
  if (!casePath) {
    process.stderr.write("usage: lfw_trace_buff.mjs <case-file>\n");
    process.exit(2);
  }

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
      if (sub === "entity") {
        const rawId = parseJsStringLiteral(t[i++]!);
        const id = rawId === "@" ? `E${entitySeq}` : rawId;
        const ok = t[i++] === "1";
        ensure(id).exists = ok;
        out.push(`env entity ${id} ${flag(ok)}`);
      } else if (sub === "pos") {
        const e = ensure(parseJsStringLiteral(t[i++]!));
        e.position = { x: Number(t[i++]!), y: Number(t[i++]!), z: Number(t[i++]!) };
        out.push("env pos");
      } else if (sub === "frame") {
        const e = ensure(parseJsStringLiteral(t[i++]!));
        e.frame = {
          centery: Number(t[i++]!),
          height: Number(t[i++]!),
          pic: { h: Number(t[i++]!) },
        };
        out.push("env frame");
      } else if (sub === "data") {
        const idx = [i];
        state.data = parseValue(t, idx);
        i = idx[0]!;
        out.push("env data");
      } else if (sub === "create_entity_ok") {
        state.create_entity_ok = t[i++] === "1";
        out.push("env create_entity_ok");
      } else if (sub === "create_buff_ok") {
        state.create_buff_ok = t[i++] === "1";
        out.push("env create_buff_ok");
      } else {
        process.stderr.write(`unknown env '${sub}'\n`);
        process.exit(2);
      }
      continue;
    }
    if (op === "buff") {
      const sub = t[i++]!;
      if (sub === "new") {
        const id = parseJsStringLiteral(t[i++]!);
        const idx = [i];
        const kind = parseValue(t, idx);
        i = idx[0]!;
        buff = new TestBuff(lfw as never, id, kind as never);
        out.push(`buff new${buffText()}`);
        continue;
      } else if (sub === "hook") {
        buff.hooks = t[i++]!;
        const which = buff.hooks;
        (buff as never as Record<string, unknown>).on_update =
          which === "update"
            ? (a: { id: string } | undefined, v: { id: string } | undefined) =>
                log.push(`on_update:${a ? a.id : "-"}:${v ? v.id : "-"}`)
            : undefined;
        (buff as never as Record<string, unknown>).on_tick =
          which === "tick"
            ? (a: { id: string } | undefined, v: { id: string } | undefined) =>
                log.push(`on_tick:${a ? a.id : "-"}:${v ? v.id : "-"}`)
            : undefined;
        (buff as never as Record<string, unknown>).on_end =
          which === "end"
            ? (a: { id: string } | undefined, v: { id: string } | undefined) =>
                log.push(`on_end:${a ? a.id : "-"}:${v ? v.id : "-"}`)
            : undefined;
        out.push(`buff hook ${which}`);
        continue;
      } else if (sub === "oid") {
        buff._oid = parseJsStringLiteral(t[i++]!);
        out.push(`buff oid ${buff._oid}`);
        continue;
      } else if (sub === "fid") {
        buff._fid = parseJsStringLiteral(t[i++]!);
        out.push(`buff fid ${buff._fid}`);
        continue;
      } else if (sub === "level") {
        buff.level = Number(t[i++]!);
      } else if (sub === "lifetime") {
        buff.lifetime = Number(t[i++]!);
      } else if (sub === "duration") {
        buff.duration = Number(t[i++]!);
      } else if (sub === "ticks") {
        buff.ticks = Number(t[i++]!);
      } else if (sub === "attacker_id") {
        buff.set_attacker(parseJsStringLiteral(t[i++]!) as never);
      } else if (sub === "attacker_entity") {
        buff.set_attacker(ensure(parseJsStringLiteral(t[i++]!)) as never);
      } else if (sub === "victim") {
        buff.set_victim(ensure(parseJsStringLiteral(t[i++]!)) as never);
      } else if (sub === "add_victim") {
        buff.add_victim(ensure(parseJsStringLiteral(t[i++]!)) as never);
      } else if (sub === "del_victim") {
        buff.del_victim(ensure(parseJsStringLiteral(t[i++]!)) as never);
      } else if (sub === "del_id") {
        const r = (buff as never as { _del(id: string): boolean })._del(parseJsStringLiteral(t[i++]!));
        out.push(`buff del_id ${flag(r)}`);
        continue;
      } else if (sub === "reset") {
        buff.reset(parseJsStringLiteral(t[i++]!));
      } else if (sub === "mount") {
        buff.mount();
      } else if (sub === "unmount") {
        buff.unmount();
      } else if (sub === "update") {
        buff.update(Number(t[i++]!));
      } else if (sub === "place_center") {
        const v = ensure(parseJsStringLiteral(t[i++]!));
        (buff as never as { place_effect_center(e: unknown, v: unknown): void }).place_effect_center(v, v);
      } else if (sub === "show") {
        const v = ensure(parseJsStringLiteral(t[i++]!));
        (buff as never as { show_effect(v: unknown): void }).show_effect(v);
      } else if (sub === "clear") {
        (buff as never as { clear_effects(): void }).clear_effects();
      } else if (sub === "upd_fx") {
        (buff as never as { update_effects(): void }).update_effects();
      } else if (sub === "del_fx") {
        (buff as never as { del_effect(id: string): void }).del_effect(parseJsStringLiteral(t[i++]!));
      } else if (sub === "use") {
        const id = parseJsStringLiteral(t[i++]!);
        const b = made.get(id);
        if (b) buff = b as TestBuff;
        out.push(`buff use ${id}`);
        continue;
      } else if (sub === "snap") {
        out.push(`buff snap ${renderValue(buff.to_snapshot())}`);
        continue;
      } else if (sub === "read") {
        const idx = [i];
        const v = parseValue(t, idx);
        i = idx[0]!;
        buff.read_snapshot(v as never);
      } else if (sub === "log") {
        out.push(`buff log${logText()}`);
        log.length = 0;
        continue;
      } else {
        process.stderr.write(`unknown buff sub '${sub}'\n`);
        process.exit(2);
      }
      if (i !== t.length) {
        process.stderr.write(`unexpected trailing token: ${raw}\n`);
        process.exit(2);
      }
      out.push(`buff ${sub}${buffText()} |${logText()}`);
      log.length = 0;
      continue;
    }
    if (op === "grant") {
      const kind = parseJsStringLiteral(t[i++]!);
      const aid = parseJsStringLiteral(t[i++]!);
      const vid = parseJsStringLiteral(t[i++]!);
      const dur = Number(t[i++]!);
      const b = grant_buff(kind, findEntity(aid) as never, ensure(vid) as never, dur);
      out.push(`grant ${kind} ${b ? b.id : "-"}${b ? buffTextOf(b) : ""} |${logText()}`);
      log.length = 0;
      continue;
    }
    process.stderr.write(`unknown op '${op}'\n`);
    process.exit(2);
  }

  process.stdout.write(out.join("\n") + "\n");
}

main();
