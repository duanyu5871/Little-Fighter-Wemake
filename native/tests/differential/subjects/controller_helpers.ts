import { DoubleClick } from "../../../../src/LFW/controller/DoubleClick";
import { SeqKeys } from "../../../../src/LFW/controller/SeqKeys";
import { KeyStatus } from "../../../../src/LFW/controller/KeyStatus";
import { ControllerDoubleClicks } from "../../../../src/LFW/controller/ControllerDoubleClicks";

import { parseJsStringLiteral, parseValue, readCaseLines, renderValue, splitWs } from "./trace_util";

const out: string[] = [];

type Any = never;

type Ctrl = { time: number; world: { dataset: { key_hit_duration: number } } };

function flag(b: boolean): string {
  return b ? "b1" : "b0";
}

function main(): void {
  const casePath = process.argv[2];
  if (!casePath) {
    process.stderr.write("usage: lfw_trace_controller_helpers.mjs <case-file>\n");
    process.exit(2);
  }

  const dc = new Map<string, DoubleClick<unknown>>();
  const sk = new Map<string, SeqKeys<unknown>>();
  const ks = new Map<string, KeyStatus>();
  const ctrl = new Map<string, Ctrl>();
  const cdc = new Map<string, ControllerDoubleClicks>();

  for (const raw of readCaseLines(casePath)) {
    const t = splitWs(raw);
    if (t.length === 0) continue;
    const op = t[0]!;
    if (t.length < 3) {
      process.stderr.write(`too few operands: ${raw}\n`);
      process.exit(2);
    }
    const sub = t[1]!;
    const name = parseJsStringLiteral(t[2]!);
    const idx = [3];

    if (op === "dc") {
      if (sub === "new") {
        dc.set(name, new DoubleClick<unknown>(parseValue(t, idx) as never));
      } else {
        const inst = dc.get(name);
        if (!inst) {
          process.stderr.write(`unknown dc '${name}'\n`);
          process.exit(2);
        }
        if (sub === "press") {
          const time = Number(t[idx[0]!++]);
          const interval = Number(t[idx[0]!++]);
          inst.press(time, parseValue(t, idx) as never, interval);
        } else if (sub === "step") inst.step();
        else if (sub === "reset") inst.reset();
        else if (sub === "load") inst.from_snapshot(parseValue(t, idx) as Any);
        else if (sub !== "snap") {
          process.stderr.write(`unknown dc sub '${sub}'\n`);
          process.exit(2);
        }
      }
      out.push(`dc ${name} ${renderValue(dc.get(name)!.to_snapshot())}`);
      continue;
    }

    if (op === "sk") {
      if (sub === "new") {
        const keys = String(parseValue(t, idx));
        sk.set(name, new SeqKeys<unknown>(keys, parseValue(t, idx) as never));
      } else {
        const inst = sk.get(name);
        if (!inst) {
          process.stderr.write(`unknown sk '${name}'\n`);
          process.exit(2);
        }
        if (sub === "press") inst.press(String(parseValue(t, idx)));
        else if (sub === "reset") inst.reset();
        else if (sub === "load") inst.from_snapshot(parseValue(t, idx) as Any);
        else if (sub !== "snap") {
          process.stderr.write(`unknown sk sub '${sub}'\n`);
          process.exit(2);
        }
      }
      out.push(`sk ${name} ${renderValue(sk.get(name)!.to_snapshot())}`);
      continue;
    }

    if (op === "ks") {
      if (sub === "new") {
        const c: Ctrl = { time: 0, world: { dataset: { key_hit_duration: 0 } } };
        ctrl.set(name, c);
        ks.set(name, new KeyStatus(c as never, parseValue(t, idx) as never));
      } else {
        const inst = ks.get(name);
        const c = ctrl.get(name)!;
        if (!inst) {
          process.stderr.write(`unknown ks '${name}'\n`);
          process.exit(2);
        }
        if (sub === "hit") {
          const time = Number(t[idx[0]!++]!);
          const arg = idx[0]! < t.length ? (parseValue(t, idx) as never) : undefined;
          c.time = time;
          inst.hit(arg);
        } else if (sub === "end") {
          c.time = Number(t[idx[0]!++]!);
          inst.end();
        } else if (sub === "reset") inst.reset();
        else if (sub === "load") inst.from_snapshot(parseValue(t, idx) as Any);
        else if (sub === "use") {
          const back = inst.use();
          out.push(`ks ${name} use ${renderValue(back)} ${renderValue(inst.to_snapshot())}`);
          continue;
        } else if (sub === "query") {
          const time = Number(t[idx[0]!++]!);
          const duration = Number(t[idx[0]!++]!);
          c.time = time;
          c.world.dataset.key_hit_duration = duration;
          out.push(
            `ks ${name} query ${flag(inst.is_start())} ${flag(inst.is_hit())} ` +
              `${flag(inst.is_hld())} ${flag(inst.is_end())}`,
          );
          continue;
        } else if (sub !== "snap") {
          process.stderr.write(`unknown ks sub '${sub}'\n`);
          process.exit(2);
        }
      }
      out.push(`ks ${name} ${renderValue(ks.get(name)!.to_snapshot())}`);
      continue;
    }

    if (op === "cdc") {
      if (sub === "new") {
        cdc.set(name, new ControllerDoubleClicks({} as never));
      } else {
        const inst = cdc.get(name);
        if (!inst) {
          process.stderr.write(`unknown cdc '${name}'\n`);
          process.exit(2);
        }
        if (sub === "reset") inst.reset();
        else if (sub === "load") inst.from_snapshot(parseValue(t, idx) as Any);
        else if (sub === "press") {
          const slot = String(parseValue(t, idx));
          const time = Number(t[idx[0]!++]!);
          const interval = Number(t[idx[0]!++]!);
          const data = parseValue(t, idx);
          const target = (inst as unknown as Record<string, DoubleClick<unknown>>)[slot];
          target!.press(time, data as never, interval);
        } else if (sub !== "snap") {
          process.stderr.write(`unknown cdc sub '${sub}'\n`);
          process.exit(2);
        }
      }
      out.push(`cdc ${name} ${renderValue(cdc.get(name)!.to_snapshot())}`);
      continue;
    }

    process.stderr.write(`unknown op '${op}'\n`);
    process.exit(2);
  }

  process.stdout.write(out.join("\n") + (out.length ? "\n" : ""));
}

main();
