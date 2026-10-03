import { handle_ball_frozen } from "../../../../src/LFW/collision/handle_ball_frozen";
import { EntityEnum } from "../../../../src/LFW/defines";
import { parseValue, readCaseLines, renderValue, splitWs, num } from "./trace_util";

const out: string[] = [];
const log: string[] = [];

const state = {
  itr: undefined as Record<string, unknown> | undefined,
};

class Fake {
  _id: string;
  _group: unknown = undefined;
  _state: unknown = undefined;
  _type: unknown = EntityEnum.Fighter;
  _face: unknown = 1;
  _pos = [0, 0, 0];
  _frame: Record<string, unknown> = { centerx: 0, centery: 0, width: 0, height: 0 };
  _spawn_ok = true;

  constructor(id: string) {
    this._id = id;
  }

  get id(): string {
    return this._id;
  }
  get group(): unknown {
    return this._group;
  }
  get state(): unknown {
    return this._state;
  }
  get facing(): unknown {
    return this._face;
  }
  get position(): { x: number; y: number; z: number } {
    return { x: this._pos[0]!, y: this._pos[1]!, z: this._pos[2]! };
  }
  get frame(): Record<string, unknown> {
    return this._frame;
  }
  get data(): Record<string, unknown> {
    return { type: this._type };
  }
  spawn(opoint: unknown, _owner: unknown, face: unknown): unknown {
    const o = opoint as Record<string, unknown>;
    const action = o.action as Record<string, unknown> | undefined;
    log.push(
      `${this._id}:spawn:${renderValue(o.oid)}:${renderValue(o.kind)}:${renderValue(o.x)}:${renderValue(
        o.y,
      )}:${renderValue(o.z)}:${renderValue(action?.id)}:${renderValue(face)}`,
    );
    return this._spawn_ok ? {} : undefined;
  }
  enter_frame(info: unknown): void {
    log.push(`${this._id}:enter_frame:${renderValue((info as Record<string, unknown>).id)}`);
  }
}

const a = new Fake("A");
const v = new Fake("V");

function sideText(f: Fake): string {
  return (
    `${f._id}.group=${renderValue(f._group)} ${f._id}.state=${renderValue(f._state)} ` +
    `${f._id}.type=${renderValue(f._type)} ${f._id}.face=${renderValue(f._face)} ` +
    `${f._id}.pos=${renderValue(f._pos[0])}/${renderValue(f._pos[1])}/${renderValue(f._pos[2])} ` +
    `${f._id}.spawn=${f._spawn_ok ? "1" : "0"}`
  );
}

function stateText(): string {
  return `${sideText(a)} ${sideText(v)}`;
}

function walkSide(f: Fake, field: string, tok: string[], i: number): number {
  const idx = [i];
  const bump = (): string => {
    const t = tok[idx[0]!]!;
    idx[0] = idx[0]! + 1;
    return t;
  };
  if (field === "group") f._group = parseValue(tok, idx);
  else if (field === "state") f._state = parseValue(tok, idx);
  else if (field === "type") f._type = parseValue(tok, idx);
  else if (field === "face") f._face = parseValue(tok, idx);
  else if (field === "posx") f._pos[0] = num(bump());
  else if (field === "posy") f._pos[1] = num(bump());
  else if (field === "posz") f._pos[2] = num(bump());
  else if (field === "frame") f._frame = parseValue(tok, idx) as Record<string, unknown>;
  else if (field === "spawn") f._spawn_ok = bump() === "1";
  else {
    process.stderr.write(`unknown side field '${field}'\n`);
    process.exit(2);
  }
  return idx[0]!;
}

function main(): void {
  const casePath = process.argv[2];
  if (!casePath) {
    process.stderr.write("usage: lfw_trace_collision_ball_frozen.mjs <case-file>\n");
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
      const hit = handle_ball_frozen(a as never, v as never, (state.itr ?? {}) as never);
      out.push(`run hit || ${log.join(",")} | ${stateText()} | ret=${hit ? "1" : "0"}`);
      continue;
    }
    process.stderr.write(`unknown op '${op}'\n`);
    process.exit(2);
  }

  process.stdout.write(out.join("\n") + "\n");
}

main();
