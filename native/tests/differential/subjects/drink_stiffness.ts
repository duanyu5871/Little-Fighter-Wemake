import { handle_stiffness } from "../../../../src/LFW/collision/handle_stiffness";
import { DrinkInfo } from "../../../../src/LFW/entity/DrinkInfo";
import { Entity } from "../../../../src/LFW/entity/Entity";

import { parseJsStringLiteral, parseValue, readCaseLines, renderValue, splitWs } from "./trace_util";

type Any = never;

const out: string[] = [];

const proto = Entity.prototype as unknown as Record<string, unknown>;

function makeEntity(raw: unknown): Any {
  const src = raw as Record<string, unknown>;
  const e = { ...src } as Record<string, unknown>;
  if ("data" in src) e["_data"] = src["data"];
  for (const key of ["data", "type", "itr_motionless", "weight", "state"]) {
    const d = Object.getOwnPropertyDescriptor(proto, key);
    if (!d) continue;
    if (d.get) {
      Object.defineProperty(e, key, { get: d.get, configurable: true, enumerable: d.enumerable });
    }
  }
  const ds = Object.getOwnPropertyDescriptor(proto, "dataset");
  if (ds && typeof ds.value === "function") {
    e["dataset"] = function (this: unknown, name: string): unknown {
      return (ds.value as (this: unknown, n: string) => unknown).call(this, name);
    };
  }
  return e as Any;
}

function main(): void {
  const casePath = process.argv[2];
  if (!casePath) {
    process.stderr.write("usage: lfw_trace_drink_stiffness.mjs <case-file>\n");
    process.exit(2);
  }

  const ents = new Map<string, Any>();
  const dis = new Map<string, DrinkInfo>();

  const entOf = (name: string): Any => {
    if (!ents.has(name)) {
      process.stderr.write(`unknown entity '${name}'\n`);
      process.exit(2);
    }
    return ents.get(name) as Any;
  };

  const diOf = (name: string): DrinkInfo => {
    if (!dis.has(name)) {
      process.stderr.write(`unknown drink '${name}'\n`);
      process.exit(2);
    }
    return dis.get(name)!;
  };

  for (const raw of readCaseLines(casePath)) {
    const t = splitWs(raw);
    if (t.length === 0) continue;
    const op = t[0]!;

    if (op === "stf") {
      if (t.length < 3) {
        process.stderr.write(`too few operands: ${raw}\n`);
        process.exit(2);
      }
      const eName = parseJsStringLiteral(t[1]!);
      const idx = [2];
      const itr = parseValue(t, idx);
      const attacker = entOf(eName);
      const victim = { shaking: 0 } as Record<string, unknown>;
      handle_stiffness({ itr, attacker, victim } as Any);
      out.push(`stf ${eName} ${renderValue(attacker.motionless)} ${renderValue(victim["shaking"])}`);
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
      if (sub === "put") ents.set(name, makeEntity(parseValue(t, idx)));
      else if (sub === "del") ents.delete(name);
      else {
        process.stderr.write(`unknown ent sub '${sub}'\n`);
        process.exit(2);
      }
      out.push(`ent ${name}`);
    } else if (op === "di") {
      if (sub === "new") {
        dis.set(name, new DrinkInfo(parseValue(t, idx) as Any));
        out.push(`di ${name} ${renderValue(diOf(name).to_snapshot())}`);
      } else {
        const d = diOf(name);
        if (sub === "set") {
          const field = parseJsStringLiteral(t[idx[0]!++]!);
          const v = parseValue(t, idx);
          (d as unknown as Record<string, unknown>)[field] = v;
          out.push(`di ${name} ${renderValue(d.to_snapshot())}`);
        } else if (sub === "load") {
          d.from_snapshot(parseValue(t, idx) as Any);
          out.push(`di ${name} ${renderValue(d.to_snapshot())}`);
        } else if (sub === "snap") {
          out.push(`di ${name} ${renderValue(d.to_snapshot())}`);
        } else if (sub === "empty") {
          out.push(
            `di ${name} empty ${d.hp_h_empty ? "b1" : "b0"} ${d.hp_r_empty ? "b1" : "b0"} ` +
              `${d.mp_h_empty ? "b1" : "b0"}`,
          );
        } else {
          process.stderr.write(`unknown di sub '${sub}'\n`);
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
