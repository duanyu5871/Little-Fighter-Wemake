import { BaseController } from "../../../../src/LFW/controller/BaseController";

import { parseJsStringLiteral, parseValue, readCaseLines, renderValue, splitWs } from "./trace_util";

const out: string[] = [];
const log: string[] = [];

function flag(b: boolean): string {
  return b ? "b1" : "b0";
}

function resultText(r: unknown): string {
  return r === undefined ? "-" : renderValue(r);
}

function fieldsTextOf(r: { time: number; keys: string; kind: string; result?: unknown }): string {
  return (
    `${renderValue(r.time)} ${renderValue(r.keys)} ${renderValue(r.kind)} ${resultText(r.result)}`
  );
}

function main(): void {
  const casePath = process.argv[2];
  if (!casePath) {
    process.stderr.write("usage: lfw_trace_base_controller.mjs <case-file>\n");
    process.exit(2);
  }

  const world: Record<string, unknown> = {
    dataset: { key_hit_duration: 0, double_click_interval: 0 },
    etc: (x: number, y: number, z: number, e: string) =>
      log.push(`etc:${renderValue(x)},${renderValue(y)},${renderValue(z)},${e}`),
    team_come: (t: string, x: number, y: number, z: number) =>
      log.push(`come:${t},${renderValue(x)},${renderValue(y)},${renderValue(z)}`),
    team_stay: (t: string) => log.push(`stay:${t}`),
    team_move: (t: string) => log.push(`move:${t}`),
    team_follow: () => log.push("follow"),
  };
  const entity: Record<string, unknown> = {
    facing: 1,
    hp: 1,
    team: "",
    position: { x: 0, y: 0, z: 0 },
    world,
    lfw: { players: new Map() },
    frame: {
      state: 0,
      hold: undefined,
      hit: undefined,
      key_down: undefined,
      key_up: undefined,
      __seq_map: undefined,
    },
    data: {
      pre_hitkeys: undefined,
      post_hitkeys: undefined,
      __pre_hitkeys_map: undefined,
      __post_hitkeys_map: undefined,
    },
    transforms: undefined,
    get_next_frame: (nf: unknown) => (nf === "none" ? undefined : nf),
  };
  const dataset = world.dataset as { key_hit_duration: number; double_click_interval: number };
  const frame = entity.frame as Record<string, unknown>;
  const data = entity.data as Record<string, unknown>;

  let ctl = new BaseController("p", entity as never);

  const keysOf = (t: string[], i: number): string[] => {
    const ks: string[] = [];
    for (; i < t.length; ++i) ks.push(parseJsStringLiteral(t[i]!));
    return ks;
  };

  const keysText = (): string => `${String((ctl as never as { _key_list: string })._key_list)}/${ctl.key_list}`;

  const updateText = (): string => {
    const r = ctl.result;
    return (
      `${renderValue(ctl.time)} ${renderValue(ctl.LR)} ${renderValue(ctl.UD)} ${renderValue(ctl.jd)} ` +
      `${keysText()} ${renderValue(r.time)} ${renderValue(r.keys)} ${renderValue(r.kind)} ` +
      `${resultText(r.result)} ${renderValue(ctl.keys.to_snapshot())}`
    );
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
      if (sub === "hit_dur") {
        dataset.key_hit_duration = Number(t[i++]!);
      } else if (sub === "dbl_int") {
        dataset.double_click_interval = Number(t[i++]!);
      } else if (sub === "facing") {
        entity.facing = Number(t[i++]!);
      } else if (sub === "alive") {
        entity.hp = t[i++] === "1" ? 1 : 0;
      } else if (sub === "human") {
        (ctl as never as Record<string, unknown>).__is_human_ctrl__ = t[i++] === "1";
      } else if (sub === "bot") {
        (ctl as never as Record<string, unknown>).__is_bot_ctrl__ = t[i++] === "1";
      } else if (sub === "team") {
        entity.team = parseJsStringLiteral(t[i++]!);
      } else if (sub === "pos") {
        entity.position = { x: Number(t[i++]!), y: Number(t[i++]!), z: Number(t[i++]!) };
      } else if (sub === "fstate") {
        frame.state = Number(t[i++]!);
      } else {
        const idx = [i];
        const v = parseValue(t, idx);
        i = idx[0]!;
        const asSeqMap = (x: unknown): unknown =>
          x === undefined ? undefined : new Map(Object.entries(x as Record<string, unknown>));
        if (sub === "pre") {
          data.pre_hitkeys = v;
        } else if (sub === "post") {
          data.post_hitkeys = v;
        } else if (sub === "hf") {
          frame.hit = v;
        } else if (sub === "hl") {
          frame.hold = v;
        } else if (sub === "kd") {
          frame.key_down = v;
        } else if (sub === "ku") {
          frame.key_up = v;
        } else if (sub === "seq") {
          frame.__seq_map = asSeqMap(v);
        } else if (sub === "tpre" || sub === "tpost") {
          if (!entity.transforms) entity.transforms = [{}];
          const tf = (entity.transforms as Record<string, unknown>[])[0]!;
          if (sub === "tpre") tf.__pre_hitkeys_map = asSeqMap(v);
          else tf.__post_hitkeys_map = asSeqMap(v);
        } else if (sub === "dpre") {
          data.__pre_hitkeys_map = asSeqMap(v);
        } else if (sub === "dpost") {
          data.__post_hitkeys_map = asSeqMap(v);
        } else {
          process.stderr.write(`unknown env '${sub}'\n`);
          process.exit(2);
        }
      }
      out.push(`env ${sub}`);
      continue;
    }

    if (op === "ctl") {
      const sub = t[i++]!;
      if (sub === "new") {
        ctl = new BaseController("p", entity as never);
      } else if (sub === "reset") {
        ctl.reset("p", entity as never);
        out.push(
          `ctl reset ${fieldsTextOf(ctl.result)} ${renderValue(ctl.keys.to_snapshot())} ` +
            `${renderValue(ctl.queue.length)} ${renderValue(ctl.dbc.to_snapshot())}`,
        );
        continue;
      } else if (
        sub === "start" || sub === "hold" || sub === "end" || sub === "db_hit" ||
        sub === "click" || sub === "dbl_click" || sub === "kd" || sub === "ku" || sub === "ck"
      ) {
        const ks = keysOf(t, i);
        i = t.length;
        (ctl as never as Record<string, (...a: never[]) => unknown>)[sub]!(...(ks as never[]));
      } else if (sub === "update") {
        ctl.update();
        out.push(`ctl update ${updateText()}`);
        continue;
      } else if (sub === "tst") {
        const type = parseJsStringLiteral(t[i++]!);
        const key = parseJsStringLiteral(t[i++]!);
        out.push(`ctl tst ${type} ${key} ${flag(ctl.tst(type as never, key as never))}`);
        continue;
      } else if (sub === "flags") {
        const key = parseJsStringLiteral(t[i++]!);
        out.push(
          `ctl flags ${key} ${flag(ctl.is_start(key as never))} ${flag(ctl.is_hit(key as never))} ` +
            `${flag(ctl.is_hold(key as never))} ${flag(ctl.is_end(key as never))}`,
        );
        continue;
      } else if (sub === "dbhit") {
        const key = parseJsStringLiteral(t[i++]!);
        out.push(
          `ctl dbhit ${key} ${flag(ctl.is_db_hit(key as never))} ${renderValue(ctl.dbc.to_snapshot())}`,
        );
        continue;
      } else if (
        sub === "lr" || sub === "rl" || sub === "ud" || sub === "du" || sub === "jd" || sub === "dj"
      ) {
        const v =
          sub === "lr" ? ctl.LR : sub === "rl" ? ctl.RL : sub === "ud" ? ctl.UD :
          sub === "du" ? ctl.DU : sub === "jd" ? ctl.jd : ctl.dj;
        out.push(`ctl ${sub} ${renderValue(v)}`);
        continue;
      } else if (sub === "envdump") {
        const c = ctl as never as Record<string, unknown>;
        out.push(
          `ctl envdump ${renderValue(dataset.key_hit_duration)} ` +
            `${renderValue(dataset.double_click_interval)} ${renderValue(entity.facing)} ` +
            `${renderValue(frame.state)} ${flag(c.__is_human_ctrl__ === true)} ` +
            `${flag(c.__is_bot_ctrl__ === true)} ${flag(Number(entity.hp) !== 0)}`,
        );
        continue;
      } else if (sub === "q") {
        out.push(`ctl q ${renderValue(ctl.queue.length)}`);
        continue;
      } else if (sub === "keys") {
        out.push(`ctl keys ${renderValue(ctl.keys.to_snapshot())}`);
        continue;
      } else if (sub === "dbs") {
        out.push(`ctl dbs ${renderValue(ctl.dbc.to_snapshot())}`);
        continue;
      } else if (sub === "kraw") {
        out.push(`ctl kraw ${keysText()}`);
        continue;
      } else if (sub === "seqtest") {
        const s = parseJsStringLiteral(t[i++]!);
        out.push(`ctl seqtest ${s} ${flag(ctl.sequence_keys_test(s))}`);
        continue;
      } else if (sub === "sametest") {
        const s = parseJsStringLiteral(t[i++]!);
        out.push(`ctl sametest ${s} ${flag(ctl.sametime_keys_test(s))}`);
        continue;
      } else if (sub === "log") {
        out.push(`ctl log${log.map((e) => ` ${e}`).join("")}`);
        log.length = 0;
        continue;
      } else {
        process.stderr.write(`unknown ctl sub '${sub}'\n`);
        process.exit(2);
      }
      if (i !== t.length) {
        process.stderr.write(`unexpected trailing token '${t[i]}': ${raw}\n`);
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
