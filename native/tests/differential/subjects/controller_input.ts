import { ControllerKeyStatus } from "../../../../src/LFW/controller/ControllerKeyStatus";
import { ControllerResult } from "../../../../src/LFW/controller/ControllerResult";
import type { KeyStatus } from "../../../../src/LFW/controller/KeyStatus";
import { AGK, CONFLICTS_KEY_MAP, GKLabels } from "../../../../src/LFW/defines/GameKey";

import { parseJsStringLiteral, parseValue, readCaseLines, renderValue, splitWs } from "./trace_util";

const out: string[] = [];

const ORDER = ["L", "R", "U", "D", "d", "j", "a"];

type Ctrl = { time: number; world: { dataset: { key_hit_duration: number } } };

function flag(b: boolean): string {
  return b ? "b1" : "b0";
}

function slotOf(inst: ControllerKeyStatus, key: string): KeyStatus | undefined {
  switch (key) {
    case "L": return inst.L;
    case "R": return inst.R;
    case "U": return inst.U;
    case "D": return inst.D;
    case "d": return inst.d;
    case "j": return inst.j;
    case "a": return inst.a;
    default: return undefined;
  }
}

function fieldsText(r: ControllerResult): string {
  const result = r.result === undefined ? "-" : renderValue(r.result);
  return `${renderValue(r.time)} ${renderValue(r.keys)} ${renderValue(r.kind)} ${result}`;
}

function main(): void {
  const casePath = process.argv[2];
  if (!casePath) {
    process.stderr.write("usage: lfw_trace_controller_input.mjs <case-file>\n");
    process.exit(2);
  }

  const ks = new Map<string, ControllerKeyStatus>();
  const ctrls = new Map<string, Ctrl>();
  const res = new Map<string, ControllerResult>();
  const owner = {
    entity: { get_next_frame: (nf: unknown) => (nf === "none" ? undefined : nf) },
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

    if (op === "ks") {
      const sub = t[i++]!;
      if (sub === "new") {
        const name = parseJsStringLiteral(t[i++]!);
        const ctrl: Ctrl = { time: 0, world: { dataset: { key_hit_duration: 0 } } };
        ctrls.set(name, ctrl);
        const inst = new ControllerKeyStatus(ctrl as never);
        ks.set(name, inst);
        out.push(`ks ${name} ${renderValue(inst.to_snapshot())}`);
      } else {
        const name = parseJsStringLiteral(t[i++]!);
        const inst = ks.get(name);
        if (!inst) {
          process.stderr.write(`unknown ks '${name}'\n`);
          process.exit(2);
        }
        if (sub === "hit") {
          const key = parseJsStringLiteral(t[i++]!);
          let tv: unknown;
          if (t[i] === "-") {
            tv = undefined;
            i++;
          } else {
            const idx = [i];
            tv = parseValue(t, idx);
            i = idx[0]!;
          }
          const time = Number(t[i++]!);
          const p = slotOf(inst, key);
          if (!p) {
            process.stderr.write(`unknown key '${key}'\n`);
            process.exit(2);
          }
          ctrls.get(name)!.time = time;
          p.hit(tv as never, time);
        } else if (sub === "end") {
          const key = parseJsStringLiteral(t[i++]!);
          const time = Number(t[i++]!);
          const p = slotOf(inst, key);
          if (!p) {
            process.stderr.write(`unknown key '${key}'\n`);
            process.exit(2);
          }
          ctrls.get(name)!.time = time;
          p.end(time);
        } else if (sub === "reset") {
          inst.reset();
        } else if (sub === "load") {
          const idx = [i];
          const v = parseValue(t, idx);
          i = idx[0]!;
          inst.from_snapshot(v as never);
        } else if (sub === "slot") {
          const key = parseJsStringLiteral(t[i++]!);
          out.push(`ks ${name} slot ${key} ${flag(slotOf(inst, key) !== undefined)}`);
          continue;
        } else if (sub === "use") {
          const key = parseJsStringLiteral(t[i++]!);
          const p = slotOf(inst, key);
          if (!p) {
            process.stderr.write(`unknown key '${key}'\n`);
            process.exit(2);
          }
          out.push(`ks ${name} use ${renderValue(p.use())} ${renderValue(inst.to_snapshot())}`);
          continue;
        } else if (sub === "flags") {
          const key = parseJsStringLiteral(t[i++]!);
          const time = Number(t[i++]!);
          const dur = Number(t[i++]!);
          const p = slotOf(inst, key);
          if (!p) {
            process.stderr.write(`unknown key '${key}'\n`);
            process.exit(2);
          }
          const ctrl = ctrls.get(name)!;
          ctrl.time = time;
          ctrl.world.dataset.key_hit_duration = dur;
          out.push(
            `ks ${name} flags ${flag(p.is_start(time))} ${flag(p.is_hit(time, dur))} ` +
              `${flag(p.is_hld(time, dur))} ${flag(p.is_end())}`,
          );
          continue;
        } else if (sub === "raw") {
          let s = "";
          for (const key of ORDER) {
            const p = slotOf(inst, key);
            s += " ";
            if (!p) {
              s += "-";
            } else {
              const snap = p.to_snapshot();
              s +=
                `${renderValue(p.key)}|${renderValue(snap[0])}/` +
                `${renderValue(snap[1])}/${renderValue(snap[2])}`;
            }
          }
          out.push(`ks ${name} raw${s}`);
          continue;
        } else if (sub !== "snap") {
          process.stderr.write(`unknown ks sub '${sub}'\n`);
          process.exit(2);
        }
        out.push(`ks ${name} ${renderValue(inst.to_snapshot())}`);
      }
    } else if (op === "gktable") {
      const what = t[i++]!;
      if (what === "labels") {
        let s = "";
        for (const [k, v] of Object.entries(GKLabels)) s += ` ${k}=${v}`;
        out.push(`gktable labels${s}`);
      } else if (what === "label") {
        const key = parseJsStringLiteral(t[i++]!);
        const label = (GKLabels as Record<string, string | undefined>)[key];
        out.push(`gktable label ${key} ${label === undefined ? "-" : label}`);
      } else if (what === "agk") {
        let s = "";
        for (const k of AGK) s += ` ${k}`;
        out.push(`gktable agk${s}`);
      } else if (what === "conflicts") {
        let s = "";
        for (const [k, v] of Object.entries(CONFLICTS_KEY_MAP)) s += ` ${k}=${v === undefined ? "-" : v}`;
        out.push(`gktable conflicts${s}`);
      } else if (what === "conflict") {
        const key = parseJsStringLiteral(t[i++]!);
        const c = (CONFLICTS_KEY_MAP as Record<string, string | undefined>)[key];
        out.push(`gktable conflict ${key} ${c === undefined ? "-" : c}`);
      } else {
        process.stderr.write(`unknown gktable '${what}'\n`);
        process.exit(2);
      }
    } else if (op === "res") {
      const sub = t[i++]!;
      if (sub === "new") {
        const name = parseJsStringLiteral(t[i++]!);
        const r = new ControllerResult(owner as never);
        res.set(name, r);
        out.push(`res ${name} snap ${fieldsText(r)}`);
      } else {
        const name = parseJsStringLiteral(t[i++]!);
        const r = res.get(name);
        if (!r) {
          process.stderr.write(`unknown res '${name}'\n`);
          process.exit(2);
        }
        if (sub === "fire" || sub === "fire2") {
          const idx = [i];
          const v = parseValue(t, idx);
          i = idx[0]!;
          const time = Number(t[i++]!);
          const keys = parseJsStringLiteral(t[i++]!);
          const kind = parseJsStringLiteral(t[i++]!);
          const ok =
            sub === "fire"
              ? r.fire(v as never, time, keys, kind as never)
              : r.fire2(v as never, time, keys, kind as never);
          out.push(`res ${name} ${flag(ok)} ${fieldsText(r)}`);
          continue;
        } else if (sub === "clear") {
          r.clear();
        } else if (sub !== "snap") {
          process.stderr.write(`unknown res sub '${sub}'\n`);
          process.exit(2);
        }
        out.push(`res ${name} snap ${fieldsText(r)}`);
      }
    } else {
      process.stderr.write(`unknown op '${op}'\n`);
      process.exit(2);
    }

    if (i !== t.length) {
      process.stderr.write(`unexpected trailing token '${t[i]}': ${raw}\n`);
      process.exit(2);
    }
  }

  process.stdout.write(out.join("\n") + "\n");
}

main();
