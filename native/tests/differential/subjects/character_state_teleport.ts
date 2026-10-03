import "../../../../src/LFW/entity/Entity";
import { CharacterState_Teleport2FarthestAlly } from "../../../../src/LFW/state/CharacterState_Teleport2FarthestAlly";
import { CharacterState_Teleport2NearestEnemy } from "../../../../src/LFW/state/CharacterState_Teleport2NearestEnemy";
import { parseValue, readCaseLines, renderValue, splitWs, num } from "./trace_util";

const out: string[] = [];
const log: string[] = [];

const r = (v: unknown): string => renderValue(v);

class RefEnt {
  data: { type: number };
  hp: number;
  position: { x: number; y: number; z: number };
  _ally: boolean;

  constructor(fighter: boolean, ally: boolean, hp: number, x: number, z: number) {
    this.data = { type: fighter ? 8 : 4 };
    this.hp = hp;
    this.position = { x, y: 0, z };
    this._ally = ally;
  }
  is_ally(_m: unknown): boolean {
    return this._ally;
  }
}

class FakeEnt {
  _id: string;
  _facing: unknown = undefined;
  _px = 0;
  _py = 0;
  _pz = 0;
  _segment: unknown = undefined;
  _gy = 0;
  _refs: { id: string; ent: unknown }[] = [];
  buffs = {
    set: (k: string, _v: unknown) => {
      log.push(`${this._id}:buffs_set:${k}`);
      return this.buffs;
    },
    delete: (k: string) => {
      log.push(`${this._id}:buffs_delete:${k}`);
      return true;
    },
  };

  constructor(id: string) {
    this._id = id;
  }

  get id(): string {
    return this._id;
  }
  get data(): { type: number } {
    return { type: 8 };
  }
  get hp(): number {
    return 1;
  }
  get facing(): unknown {
    return this._facing;
  }
  get position(): { x: number; y: number; z: number } {
    return { x: this._px, y: this._py, z: this._pz };
  }
  get world(): { entities: unknown[]; ground: { segment: unknown; y: unknown } } {
    const self = this;
    return {
      entities: self._refs.map((x) => x.ent),
      ground: {
        segment: (x: number, z: number) => {
          log.push(`${self._id}:ground_segment:${r(x)}:${r(z)}`);
          return self._segment;
        },
        y: (seg: unknown, x: number, z: number) => {
          log.push(`${self._id}:ground_y:${r(seg)}:${r(x)}:${r(z)}`);
          return self._gy;
        },
      },
    };
  }

  set_position(x: number, y: number, z: number): void {
    this._px = x;
    this._py = y;
    this._pz = z;
  }
  set_frame(info: unknown): void {
    log.push(`${this._id}:set_frame:${r((info as Record<string, unknown>)?.id)}`);
  }
  enter_frame(frame: unknown): void {
    log.push(`${this._id}:enter_frame:${r(frame)}`);
  }
  enter_frame_by_id(id: unknown): void {
    log.push(`${this._id}:enter_frame_by_id:${r(id)}`);
  }
  attach(on: boolean): void {
    log.push(`${this._id}:attach:${on ? "1" : "0"}`);
  }
  set outline_alpha(v: unknown) {
    log.push(`${this._id}:outline_alpha:${r(v)}`);
  }
  set outline_width(v: unknown) {
    log.push(`${this._id}:outline_width:${r(v)}`);
  }
  set outline_color(v: unknown) {
    log.push(`${this._id}:outline_color:${r(v)}`);
  }
}

const ents: FakeEnt[] = [];
let victim: FakeEnt | undefined = undefined;
let cls = "nearest";
let state: unknown = 0;
let stateObj: {
  state: unknown;
  enter?: (e: unknown, f: unknown) => void;
} | undefined = undefined;

function findEnt(id: string): FakeEnt | undefined {
  return ents.find((e) => e._id === id);
}

function ent(id: string): FakeEnt {
  const e = findEnt(id);
  if (e) return e;
  const created = new FakeEnt(id);
  ents.push(created);
  return created;
}

function stateText(): string {
  const v = victim!;
  return (
    `id=${v._id} pos=[${r(v._px)}:${r(v._py)}:${r(v._pz)}] ` +
    `face=${r(v._facing)} seg=${r(v._segment)} gy=${r(v._gy)}`
  );
}

function main(): void {
  const casePath = process.argv[2];
  if (!casePath) {
    process.stderr.write("usage: lfw_trace_character_state_teleport.mjs <case-file>\n");
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
      if (sub === "cls") cls = String(parseValue(t, idx));
      else if (sub === "state") state = parseValue(t, idx);
      else if (sub === "victim") victim = ent(String(parseValue(t, idx)));
      else if (sub === "facing") {
        if (victim) victim._facing = parseValue(t, idx);
      } else if (sub === "seg") {
        if (victim) victim._segment = parseValue(t, idx);
      } else if (sub === "gy") {
        if (victim) victim._gy = num(t[i++]!);
      } else if (sub === "pos") {
        const v = parseValue(t, idx) as Record<string, unknown>;
        if (victim) {
          victim._px = Number(v.x);
          victim._py = Number(v.y);
          victim._pz = Number(v.z);
        }
      } else if (sub === "ent") {
        const v = parseValue(t, idx) as Record<string, unknown>;
        if (victim) {
          const id = String(v.id);
          const ref: unknown =
            id === victim._id
              ? victim
              : new RefEnt(
                  Boolean(v.fighter),
                  Boolean(v.ally),
                  Number(v.hp),
                  Number(v.x),
                  Number(v.z),
                );
          const at = victim._refs.findIndex((x) => x.id === id);
          if (at >= 0) victim._refs[at] = { id, ent: ref };
          else victim._refs.push({ id, ent: ref });
        }
      } else {
        process.stderr.write(`unknown env '${sub}'\n`);
        process.exit(2);
      }
      out.push(`env ${sub}`);
      continue;
    }
    if (op === "run") {
      const what = t[i++]!;
      if (what === "make") {
        stateObj =
          cls === "farthest"
            ? new CharacterState_Teleport2FarthestAlly(state as never)
            : new CharacterState_Teleport2NearestEnemy(state as never);
        out.push(`run make || ${log.join(",")} | ${stateText()}`);
      } else if (what === "default") {
        stateObj =
          cls === "farthest"
            ? new CharacterState_Teleport2FarthestAlly()
            : new CharacterState_Teleport2NearestEnemy();
        out.push(
          `run default || ${log.join(",")} | state=${r(stateObj.state)} | ${stateText()}`,
        );
      } else if (what === "enter") {
        const fn = stateObj!.enter;
        if (fn) fn(victim as never, undefined as never);
        out.push(`run enter || ${log.join(",")} | ${stateText()}`);
      } else {
        process.stderr.write(`unknown run '${what}'\n`);
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
