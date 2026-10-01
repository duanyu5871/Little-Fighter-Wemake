import { Callbacks } from "../../../../src/LFW/base/Callbacks";
import type { FSM as FSMType, IState } from "../../../../src/LFW/base/FSM";
import { FSM } from "../../../../src/LFW/base/FSM";
import { Ditto } from "../../../../src/LFW/ditto/Instance";

import { esc, parseValue, readCaseLines, renderValue, splitWs } from "./trace_util";

const out: string[] = [];
const line = (...p: (string | number)[]): string => p.map((x) => String(x)).join(" ");

const cb = new Callbacks<any>();
const byId = new Map<string, any>();
let reemitKey = "";

let fsm: FSMType<string, IState<string>> | undefined;
let snap: ReturnType<FSMType<string, IState<string>>["to_snapshot"]> | undefined;
let selfAddId = "";
let selfAddKey = "";

function keyValue(tok: string): string | number {
  return /^-?\d+(\.\d+)?$/.test(tok) ? Number(tok) : tok;
}

Ditto.warn = (msg: string) => {
  out.push("WARN " + esc(msg));
};

function makeListener(id: string, keys: string[], once: boolean): unknown {
  const o: Record<string, unknown> = {};
  for (const k of keys) {
    o[k] = (...args: unknown[]) => {
      out.push(line("L", id, k, ...args.map(renderValue)));
      if (reemitKey !== "" && reemitKey === k) cb.call(k);
      if (selfAddKey !== "" && selfAddKey === k) {
        cb.once(k, () => {
          out.push("LX " + id);
        });
      }
    };
  }
  if (once) o.once = true;
  return o;
}

function keyText(s: IState<string> | undefined): string {
  return s === undefined ? "-" : esc(String(s.key));
}

function fsmDump(tag: string): void {
  out.push(
    line(
      tag,
      fsm === undefined ? "-" : esc(fsm.name),
      keyText(fsm?.state),
      keyText(fsm?.prev_state),
      renderValue(fsm?.time ?? 0),
      renderValue(fsm?.state_time ?? 0),
    ),
  );
}

function main(): void {
  const casePath = process.argv[2];
  if (!casePath) {
    process.stderr.write("usage: lfw_trace_base.mjs <case-file>\n");
    process.exit(2);
  }

  for (const raw of readCaseLines(casePath)) {
    const t = splitWs(raw);
    if (t.length === 0) continue;
    const op = t[0]!;
    const idx = [1];

    if (op === "add") {
      const id = t[idx[0]!++]!;
      const keys: string[] = [];
      while (idx[0]! < t.length) keys.push(t[idx[0]!++]!);
      const l = makeListener(id, keys, false);
      byId.set(id, l);
      cb.add(l);
    } else if (op === "once") {
      const id = t[idx[0]!++]!;
      const key = t[idx[0]!++]!;
      const l = makeListener(id, [key], true);
      byId.set(id, l);
      cb.add(l);
    } else if (op === "on") {
      const id = t[idx[0]!++]!;
      const key = t[idx[0]!++]!;
      const l = makeListener(id, [key], false);
      byId.set(id, l);
      cb.add(l);
    } else if (op === "del") {
      const id = t[idx[0]!++]!;
      const l = byId.get(id);
      if (l !== undefined) cb.del(l);
    } else if (op === "clear") {
      cb.clear();
    } else if (op === "call") {
      const key = t[idx[0]!++]!;
      const args: unknown[] = [];
      while (idx[0]! < t.length) args.push(parseValue(t, idx));
      cb.call(key, ...(args as never[]));
    } else if (op === "keys") {
      out.push(line("KS", ...Array.from((cb as any)._map.keys()).map((k: string) => esc(k))));
    } else if (op === "count") {
      const key = t[idx[0]!++]!;
      out.push(line("CNT", key, (cb as any)._map.get(key)?._set.size ?? 0));
    } else if (op === "pend") {
      const key = t[idx[0]!++]!;
      out.push(line("PEND", key, (cb as any)._map.get(key)?._pendings.length ?? 0));
    } else if (op === "loop") {
      reemitKey = t[idx[0]!++]!;
    } else if (op === "selfadd") {
      selfAddId = t[idx[0]!++]!;
      selfAddKey = t[idx[0]!++]!;
    } else if (op === "stoploop") {
      reemitKey = "";
    } else if (op === "fsm") {
      const sub = t[idx[0]!++]!;
      if (sub === "init") {
        fsm = new FSM<string, IState<string>>(t[idx[0]!++]!);
        fsm.log = (msg: string) => {
          out.push("LOG " + esc(msg));
        };
      } else if (sub === "state") {
        const key = t[idx[0]!++]!;
        const name = t[idx[0]!++]!;
        const target = t[idx[0]!++]!;
        const k = keyValue(key);
        fsm!.add({
          key: k,
          name: name === "~" ? undefined : name,
          update: target === "~" ? undefined : () => target,
          enter: () => {
            out.push("E " + key);
          },
          leave: () => {
            out.push("LV " + key);
          },
        } as IState<string>);
      } else if (sub === "use") {
        fsm!.use(keyValue(t[idx[0]!++]!) as never);
      } else if (sub === "reset") {
        fsm!.reset(keyValue(t[idx[0]!++]!) as never);
      } else if (sub === "update") {
        const dt = Number(t[idx[0]!++]!);
        const times = Number(t[idx[0]!++]!);
        for (let k = 0; k < times; k++) fsm!.update(dt);
      } else if (sub === "listen") {
        const id = t[idx[0]!++]!;
        fsm!.callbacks.on("on_state_changed", (f: FSMType<string, IState<string>>) => {
          out.push(line("S", id, esc(f.name), keyText(f.state)));
        });
      } else if (sub === "dump") {
        fsmDump("F");
      } else if (sub === "snap") {
        snap = fsm!.to_snapshot();
        fsmDump("SN");
      } else if (sub === "restore") {
        if (snap !== undefined) fsm!.from_snapshot(snap);
        fsmDump("F");
      } else {
        process.stderr.write(`unknown fsm sub '${sub}'\n`);
        process.exit(2);
      }
    } else {
      process.stderr.write(`unknown op '${op}'\n`);
      process.exit(2);
    }
  }

  process.stdout.write(out.join("\n") + (out.length ? "\n" : ""));
}

main();
