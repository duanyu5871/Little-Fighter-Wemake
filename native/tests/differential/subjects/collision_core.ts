import {
  collision_clone,
  collision_from_snapshot,
  collision_get,
  collision_new,
  collision_test,
  collision_to_snapshot,
} from "../../../../src/LFW/collision/Collision";
import { collisions_keeper } from "../../../../src/LFW/collision/CollisionKeeper";
import { Ditto } from "../../../../src/LFW/ditto";
import { parseJsStringLiteral, parseValue, readCaseLines, renderValue, splitWs } from "./trace_util";

const out: string[] = [];
const log: string[] = [];

function num(v: unknown): string {
  return renderValue(v);
}
function flag(b: boolean): string {
  return b ? "b1" : "b0";
}

const state = {
  dataset: undefined as unknown,
  itr: undefined as unknown,
  bdy: undefined as unknown,
  is_ally: false,
  v_rest: false,
  dev: false,
  load_ok: false,
  load_names: undefined as unknown,
  pool: undefined as unknown,
  pool_ok: true,
  pool_handlers: 0,
  itr_index: 0,
  bdy_index: 0,
  id_seq: 0,
};

class FakeActor {
  id = "";
  px = 0;
  py = 0;
  pz = 0;
  data_id = "";
  data_type = 0;
  frame: unknown = undefined;
  prefabs: unknown = undefined;
  bear: unknown = undefined;
  marks_flag = false;
  dropping = false;
  arest: unknown = undefined;
  has_catcher = false;
  hurtable: unknown = undefined;
  invul: unknown = undefined;
  bot_ignore: unknown = undefined;
  team: unknown = undefined;
  emitter: unknown = undefined;
  spawn_time: unknown = undefined;
  bot = false;

  get position() {
    return { x: this.px, y: this.py, z: this.pz };
  }
  get data() {
    return { id: this.data_id, type: this.data_type, itr_prefabs: this.prefabs };
  }
  get bearer() {
    return this.bear === undefined
      ? undefined
      : { frame: { wpoint: { attacking: this.bear } } };
  }
  get marks() {
    return { has: (_k: unknown) => this.marks_flag };
  }
  get catcher() {
    return this.has_catcher ? { frame: { cpoint: { hurtable: this.hurtable } } } : undefined;
  }
  get invulnerable() {
    return this.invul;
  }
  get ctrl() {
    return this.bot ? { __is_bot_ctrl__: true } : {};
  }
  is_ally(_v: unknown): boolean {
    log.push(`is_ally:${flag(state.is_ally)}`);
    return state.is_ally;
  }
  get_v_rest(aid: string): boolean {
    log.push(`get_v_rest:${aid}:${flag(state.v_rest)}`);
    return state.v_rest;
  }
  world = worldFake;
  lfw = lfwFake;
}

function cubeOf(frame: unknown): Record<string, number> {
  const arr = (frame as { __cube?: unknown } | undefined)?.__cube as number[] | undefined;
  return {
    left: arr?.[0] ?? 0,
    right: arr?.[1] ?? 0,
    bottom: arr?.[2] ?? 0,
    top: arr?.[3] ?? 0,
    near: arr?.[4] ?? 0,
    far: arr?.[5] ?? 0,
  };
}

const worldFake = {
  get dataset() {
    return state.dataset;
  },
  get_bounding: (_e: unknown, frame: unknown, _box: unknown) => cubeOf(frame),
  find_entity: (id: string) => {
    log.push(`find_entity:${id}`);
    if (id === a.id) return a;
    if (id === v.id) return v;
    return undefined;
  },
};

const lfwFake = {
  get dataset() {
    return state.dataset;
  },
  get dev() {
    return state.dev;
  },
  get new_id() {
    state.id_seq += 1;
    log.push("new_id");
    return `N${state.id_seq}`;
  },
  acquire_collision: () => {
    log.push("acquire");
    if (!state.pool_ok) return undefined;
    const o = { ...((state.pool as object | undefined) ?? {}) } as Record<string, unknown>;
    const hs: string[] = [];
    for (let i = 0; i < state.pool_handlers; ++i) hs.push(`h${i}`);
    o.handlers = hs;
    return o;
  },
  datas: {
    find_object: (id: string) => {
      log.push(`find_object:${id}`);
      if (id === a.data_id) return (a.frame as { __owner?: unknown } | undefined)?.__owner;
      if (id === v.data_id) return (v.frame as { __owner?: unknown } | undefined)?.__owner;
      return undefined;
    },
  },
  world: worldFake,
};

const a = new FakeActor();
const v = new FakeActor();

(collisions_keeper as unknown as { load_handlers: (c: never) => boolean }).load_handlers = ((
  c: { handlers: string[] },
) => {
  log.push("load_handlers");
  c.handlers.length = 0;
  const names = state.load_names as unknown[] | undefined;
  if (Array.isArray(names)) for (const n of names) c.handlers.push(String(n));
  return state.load_ok;
}) as never;

(Ditto as unknown as { Log: (...args: unknown[]) => void }).Log = (...args: unknown[]) => {
  log.push("log:" + args.map((x) => String(x)).join(" "));
};

const slots: unknown[] = [undefined, undefined];
const cur = { i: 0 };
let snap: Record<string, unknown> = {};

function cubeText(v: unknown): string {
  const o = v as Record<string, unknown> | undefined;
  return `${num(o?.left)},${num(o?.right)},${num(o?.bottom)},${num(o?.top)},${num(o?.near)},${num(o?.far)}`;
}

function stateText(c: Record<string, unknown>): string {
  const hs = Array.isArray(c.handlers) ? (c.handlers as unknown[]).map(String) : [];
  let s = "";
  s += `id=${String(c.id ?? "")}`;
  s += ` aid=${String(c.aid ?? "")}`;
  s += ` vid=${String(c.vid ?? "")}`;
  s += ` adata=${String(c.adata_id ?? "")}`;
  s += ` vdata=${String(c.vdata_id ?? "")}`;
  s += ` aframe=${String(c.aframe_id ?? "")}`;
  s += ` bframe=${String(c.bframe_id ?? "")}`;
  s += ` ith=${num(c.itr_index)}`;
  s += ` bdy=${num(c.bdy_index)}`;
  s += ` ax=${num(c.ax)}`;
  s += ` ay=${num(c.ay)}`;
  s += ` az=${num(c.az)}`;
  s += ` vx=${num(c.vx)}`;
  s += ` vy=${num(c.vy)}`;
  s += ` vz=${num(c.vz)}`;
  s += ` dx=${num(c.dx)}`;
  s += ` dy=${num(c.dy)}`;
  s += ` dz=${num(c.dz)}`;
  s += ` md=${num(c.m_distance)}`;
  s += ` rest=${num(c.rest)}`;
  s += ` prio=${num(c.priority)}`;
  s += ` inj=${num(c.injury)}`;
  s += ` inj_r=${num(c.injury_r)}`;
  s += ` rinj=${num(c.real_injury)}`;
  s += ` rinj_r=${num(c.real_injury_r)}`;
  s += ` nh=${num(hs.length)}`;
  s += ` hs=${hs.join(",")}`;
  s += ` acl=${cubeText(c.a_cube)}`;
  s += ` bcl=${cubeText(c.b_cube)}`;
  return s;
}

function snapText(s: Record<string, unknown>): string {
  return (
    `aid=${String(s.aid ?? "")}` +
    ` vid=${String(s.vid ?? "")}` +
    ` adata=${String(s.adata_id ?? "")}` +
    ` vdata=${String(s.vdata_id ?? "")}` +
    ` aframe=${String(s.aframe_id ?? "")}` +
    ` bframe=${String(s.bframe_id ?? "")}` +
    ` ith=${num(s.itr_index)}` +
    ` bdy=${num(s.bdy_index)}` +
    ` ax=${num(s.ax)}` +
    ` ay=${num(s.ay)}` +
    ` az=${num(s.az)}` +
    ` vx=${num(s.vx)}` +
    ` vy=${num(s.vy)}` +
    ` vz=${num(s.vz)}` +
    ` dx=${num(s.dx)}` +
    ` dy=${num(s.dy)}` +
    ` dz=${num(s.dz)}` +
    ` md=${num(s.m_distance)}` +
    ` rest=${num(s.rest)}`
  );
}

function opText(): string {
  return log.join(",");
}

function walkActor(t: FakeActor, field: string, tok: string[], idx: number[]): void {
  let i = idx[0]!;
  const take = (): string => {
    if (i >= tok.length) {
      process.stderr.write(`actor field '${field}' missing operand: ${tok.join(" ")}\n`);
      process.exit(2);
    }
    return tok[i++]!;
  };
  if (field === "id") {
    t.id = parseJsStringLiteral(take());
  } else if (field === "pos") {
    t.px = Number(parseValue(tok, (idx = [i])));
    i = idx[0]!;
    t.py = Number(parseValue(tok, (idx = [i])));
    i = idx[0]!;
    t.pz = Number(parseValue(tok, (idx = [i])));
    i = idx[0]!;
  } else if (field === "data_id") {
    t.data_id = parseJsStringLiteral(take());
  } else if (field === "data_type") {
    t.data_type = Number(parseValue(tok, (idx = [i])));
    i = idx[0]!;
  } else if (field === "frame") {
    t.frame = parseValue(tok, [i]);
    i = tok.indexOf(" ", i) === -1 ? i + valueLen(tok, i) : i;
    i = idx[0]!;
  } else if (field === "prefabs") {
    t.prefabs = parseValue(tok, (idx = [i]));
    i = idx[0]!;
  } else if (field === "bear") {
    t.bear = parseValue(tok, (idx = [i]));
    i = idx[0]!;
  } else if (field === "marks") {
    t.marks_flag = take() === "1";
  } else if (field === "dropping") {
    t.dropping = take() === "1";
  } else if (field === "arest") {
    t.arest = parseValue(tok, (idx = [i]));
  } else if (field === "catcher") {
    t.has_catcher = take() === "1";
  } else if (field === "hurtable") {
    t.hurtable = parseValue(tok, (idx = [i]));
  } else if (field === "invul") {
    t.invul = parseValue(tok, (idx = [i]));
  } else if (field === "bot_ignore") {
    t.bot_ignore = parseValue(tok, (idx = [i]));
  } else if (field === "team") {
    t.team = parseValue(tok, (idx = [i]));
  } else if (field === "emitter") {
    t.emitter = parseValue(tok, (idx = [i]));
  } else if (field === "spawn") {
    t.spawn_time = parseValue(tok, (idx = [i]));
  } else if (field === "bot") {
    t.bot = take() === "1";
  } else {
    process.stderr.write(`unknown actor field '${field}'\n`);
    process.exit(2);
  }
}

function valueLen(_tok: string[], _i: number): number {
  return 0;
}

function walkSnap(field: string, tok: string[], idx: number[]): void {
  const cur: number[] = [idx[0]!];
  const str = (): string => {
    const v = parseJsStringLiteral(tok[cur[0]!]!);
    cur[0]! += 1;
    return v;
  };
  const n = (): number => Number(parseValue(tok, cur));
  if (field === "aid") snap.aid = str();
  else if (field === "vid") snap.vid = str();
  else if (field === "adata_id") snap.adata_id = str();
  else if (field === "vdata_id") snap.vdata_id = str();
  else if (field === "aframe_id") snap.aframe_id = str();
  else if (field === "bframe_id") snap.bframe_id = str();
  else if (field === "itr_index") snap.itr_index = n();
  else if (field === "bdy_index") snap.bdy_index = n();
  else if (field === "ax") snap.ax = n();
  else if (field === "ay") snap.ay = n();
  else if (field === "az") snap.az = n();
  else if (field === "vx") snap.vx = n();
  else if (field === "vy") snap.vy = n();
  else if (field === "vz") snap.vz = n();
  else if (field === "dx") snap.dx = n();
  else if (field === "dy") snap.dy = n();
  else if (field === "dz") snap.dz = n();
  else if (field === "m_distance") snap.m_distance = n();
  else if (field === "rest") snap.rest = n();
  else {
    process.stderr.write(`unknown snap field '${field}'\n`);
    process.exit(2);
  }
}

function current(): Record<string, unknown> | undefined {
  return slots[cur.i] as Record<string, unknown> | undefined;
}

function wrapTester(o: unknown): unknown {
  const obj = o as Record<string, unknown> | undefined;
  if (!obj) return o;
  const t = obj.__tester as Record<string, unknown> | undefined;
  if (!t) return o;
  obj.__tester = {
    run: (_c: unknown) => {
      log.push(`tester_run:${flag(t.ret === true)}`);
      return t.ret === true;
    },
    debug: () => {
      log.push("debug_get");
      return t.debug;
    },
  };
  return o;
}

function main(): void {
  const casePath = process.argv[2];
  if (!casePath) {
    process.stderr.write("usage: lfw_trace_collision_core.mjs <case-file>\n");
    process.exit(2);
  }

  for (const raw of readCaseLines(casePath)) {
    const t = splitWs(raw);
    if (t.length === 0) continue;
    const op = t[0]!;
    let i = 1;
    log.length = 0;
    if (op === "env") {
      if (t.length < 3) {
        process.stderr.write(`too few operands: ${raw}\n`);
        process.exit(2);
      }
      const sub = t[i++]!;
      if (sub === "dataset") {
        state.dataset = parseValue(t, [i]);
      } else if (sub === "a" || sub === "v") {
        const field = t[i++]!;
        walkActor(sub === "a" ? a : v, field, t, [i]);
      } else if (sub === "ally") state.is_ally = t[i++] === "1";
      else if (sub === "vrest") state.v_rest = t[i++] === "1";
      else if (sub === "dev") state.dev = t[i++] === "1";
      else if (sub === "load_ok") state.load_ok = t[i++] === "1";
      else if (sub === "load_names" || sub === "itr" || sub === "bdy" || sub === "pool") {
        const idx = [i];
        const val = parseValue(t, idx);
        if (sub === "load_names") state.load_names = val;
        else if (sub === "itr") state.itr = wrapTester(val);
        else if (sub === "bdy") state.bdy = wrapTester(val);
        else state.pool = val;
      } else if (sub === "pool_ok") state.pool_ok = t[i++] === "1";
      else if (sub === "pool_handlers") state.pool_handlers = Number(t[i++]!);
      else if (sub === "idx") {
        state.itr_index = Number(t[i++]!);
        state.bdy_index = Number(t[i++]!);
      } else if (sub === "new_id_base") state.id_seq = Number(t[i++]!);
      else {
        process.stderr.write(`unknown env '${sub}'\n`);
        process.exit(2);
      }
      out.push(`env ${sub}`);
      continue;
    }
    if (op === "slot") {
      cur.i = Number(t[i++]!);
      out.push(`slot ${cur.i}`);
      continue;
    }
    if (op === "new") {
      const inits = {
        attacker: a,
        victim: v,
        itr: state.itr,
        bdy: state.bdy,
        aframe: a.frame,
        bframe: v.frame,
        itr_index: state.itr_index,
        bdy_index: state.bdy_index,
      };
      const c = collision_new(inits as never) as unknown as Record<string, unknown>;
      slots[cur.i] = c;
      out.push(`new ${stateText(c)}`);
      continue;
    }
    if (op === "get") {
      const c = collision_get(a as never, v as never) as unknown as
        | Record<string, unknown>
        | null;
      if (!c) {
        out.push(`get no || ${opText()}`);
      } else {
        slots[cur.i] = c;
        out.push(`get yes ${stateText(c)} || ${opText()}`);
      }
      continue;
    }
    if (op === "test") {
      const c = current();
      if (!c) {
        out.push("test no-slot");
        continue;
      }
      const r = collision_test(c as never);
      out.push(`test ${flag(r)} || ${opText()}`);
      continue;
    }
    if (op === "snap") {
      const c = current();
      if (!c) {
        out.push("snap no-slot");
        continue;
      }
      snap = collision_to_snapshot(c as never) as unknown as Record<string, unknown>;
      out.push(`snap ${snapText(snap)}`);
      continue;
    }
    if (op === "snapset") {
      const field = t[i++]!;
      walkSnap(field, t, [i]);
      out.push(`snapset ${field}`);
      continue;
    }
    if (op === "from_snap") {
      const c = collision_from_snapshot(
        lfwFake as never,
        snap as never,
      ) as unknown as Record<string, unknown> | null;
      if (!c) {
        out.push(`from_snap no || ${opText()}`);
      } else {
        slots[cur.i] = c;
        out.push(`from_snap yes ${stateText(c)} || ${opText()}`);
      }
      continue;
    }
    if (op === "clone") {
      const c = current();
      if (!c) {
        out.push("clone no-slot");
        continue;
      }
      const src = cur.i;
      cur.i = src === 0 ? 1 : 0;
      const cl = collision_clone(c as never) as unknown as Record<string, unknown>;
      slots[cur.i] = cl;
      out.push(`clone ${stateText(cl)}`);
      continue;
    }
    if (op === "state") {
      const c = current();
      if (!c) {
        out.push("state no-slot");
        continue;
      }
      out.push(`state ${stateText(c)}`);
      continue;
    }
    process.stderr.write(`unknown op '${op}'\n`);
    process.exit(2);
  }

  process.stdout.write(out.join("\n") + "\n");
}

main();
