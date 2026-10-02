import { Summary } from "../../../../src/LFW/entity/Summary";
import { SummaryMgr, summary_mgr } from "../../../../src/LFW/entity/SummaryMgr";

import {
  esc,
  parseJsStringLiteral,
  parseValue,
  readCaseLines,
  renderValue,
  splitWs,
} from "./trace_util";

type Any = never;

const out: string[] = [];

const snap = (s: Summary): string =>
  `id=${esc(s.id)} dmg=${renderValue(s.damage_sum)} kill=${renderValue(s.kill_sum)}` +
  ` pick=${renderValue(s.picking_sum)} hp=${renderValue(s.hp_lost)} mp=${renderValue(s.mp_usage)}`;

const itemsOf = (prefix: string, m: SummaryMgr): string => {
  let line = `${prefix} items`;
  const map = (m as unknown as { _items: Map<string, Summary> })._items;
  for (const id of map.keys()) line += ` ${id}`;
  const graves = (m as unknown as { _graves: Summary[] })._graves;
  return line + ` graves=${graves.length}`;
};

function main(): void {
  const casePath = process.argv[2];
  if (!casePath) {
    process.stderr.write("usage: lfw_trace_summary_helpers.mjs <case-file>\n");
    process.exit(2);
  }

  const ents = new Map<string, Any>();
  const mgrs = new Map<string, SummaryMgr>();

  const entOf = (name: string): Any => {
    if (!ents.has(name)) {
      process.stderr.write(`unknown entity '${name}'\n`);
      process.exit(2);
    }
    return ents.get(name) as Any;
  };

  const mgrOf = (name: string): SummaryMgr => {
    if (!mgrs.has(name)) {
      process.stderr.write(`unknown manager '${name}'\n`);
      process.exit(2);
    }
    return mgrs.get(name)!;
  };

  for (const raw of readCaseLines(casePath)) {
    const t = splitWs(raw);
    if (t.length === 0) continue;
    const op = t[0]!;

    if (op === "sg") {
      if (t.length !== 2 || t[1] !== "items") {
        process.stderr.write(`sg takes only 'items': ${raw}\n`);
        process.exit(2);
      }
      out.push(itemsOf("sg", summary_mgr));
      continue;
    }

    if (op === "su") {
      if (t.length < 4) {
        process.stderr.write(`too few operands: ${raw}\n`);
        process.exit(2);
      }
      const sub = t[1]!;
      const mgrName = parseJsStringLiteral(t[2]!);
      const id = parseJsStringLiteral(t[3]!);
      const idx = [4];
      const mgr = mgrOf(mgrName);
      if (sub === "on") {
        const event = parseJsStringLiteral(t[idx[0]!++]!);
        const sum = mgr.get(id);
        sum.callbacks.on(event as Any, ((value: unknown, prev: unknown) => {
          out.push(`cb ${mgrName} ${id} ${event} v=${renderValue(value)} o=${renderValue(prev)}`);
        }) as Any);
        out.push(`su ${mgrName} ${id} on ${event}`);
      } else {
        const sum = mgr.get(id);
        if (sub === "set") {
          const field = parseJsStringLiteral(t[idx[0]!++]!);
          const v = parseValue(t, idx);
          (sum as unknown as Record<string, unknown>)[field] = v;
        } else if (sub === "reset") {
          sum.reset(id);
        } else if (sub === "rel") {
          sum.release();
        } else if (sub !== "snap") {
          process.stderr.write(`unknown su sub '${sub}'\n`);
          process.exit(2);
        }
        out.push(`su ${mgrName} ${id} ${snap(sum)}`);
      }
      if (idx[0]! !== t.length) {
        process.stderr.write(`trailing token(s): ${raw}\n`);
        process.exit(2);
      }
      continue;
    }

    if (t.length < 3) {
      process.stderr.write(`too few operands: ${raw}\n`);
      process.exit(2);
    }
    const sub = t[1]!;
    const name = parseJsStringLiteral(t[2]!);
    const idx = [3];

    if (op === "ent") {
      if (sub === "put") ents.set(name, parseValue(t, idx));
      else if (sub === "del") ents.delete(name);
      else {
        process.stderr.write(`unknown ent sub '${sub}'\n`);
        process.exit(2);
      }
      out.push(`ent ${name}`);
    } else if (op === "sm") {
      if (sub === "new") {
        mgrs.set(name, new SummaryMgr());
      } else {
        const mgr = mgrOf(name);
        if (sub === "get") {
          const id = parseJsStringLiteral(t[idx[0]!++]!);
          out.push(`sm ${name} get ${id} ${snap(mgr.get(id))}`);
          if (idx[0]! !== t.length) {
            process.stderr.write(`trailing token(s): ${raw}\n`);
            process.exit(2);
          }
          continue;
        } else if (sub === "items") {
          out.push(itemsOf(`sm ${name}`, mgr));
        } else if (sub === "release") {
          mgr.release(parseJsStringLiteral(t[idx[0]!++]!));
          out.push(itemsOf(`sm ${name}`, mgr));
        } else if (sub === "clear") {
          mgr.clear();
          out.push(itemsOf(`sm ${name}`, mgr));
        } else if (sub === "dmg") {
          const aName = parseJsStringLiteral(t[idx[0]!++]!);
          const value = parseValue(t, idx);
          mgr.add_damage_sum(entOf(aName) as Any, value as Any);
          out.push(itemsOf(`sm ${name}`, mgr));
        } else if (sub === "kill") {
          const aName = parseJsStringLiteral(t[idx[0]!++]!);
          const value = idx[0]! < t.length ? parseValue(t, idx) : undefined;
          mgr.add_kill_sum(entOf(aName) as Any, value as Any);
          out.push(itemsOf(`sm ${name}`, mgr));
        } else if (sub === "apply") {
          const aName = parseJsStringLiteral(t[idx[0]!++]!);
          const injury = parseValue(t, idx);
          const vName = parseJsStringLiteral(t[idx[0]!++]!);
          const prevHp = parseValue(t, idx);
          mgr.apply_damage(entOf(aName) as Any, injury as Any, entOf(vName) as Any, prevHp as Any);
          out.push(itemsOf(`sm ${name}`, mgr));
        } else {
          process.stderr.write(`unknown sm sub '${sub}'\n`);
          process.exit(2);
        }
      }
    } else {
      process.stderr.write(`unknown op '${op}'\n`);
      process.exit(2);
    }

    if (idx[0]! !== t.length) {
      process.stderr.write(`trailing token(s): ${raw}\n`);
      process.exit(2);
    }
  }

  process.stdout.write(out.join("\n") + "\n");
}

main();
