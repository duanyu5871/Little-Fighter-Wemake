import { calc_itr_velocity } from "../../../../src/LFW/collision/calc_itr_velocity";
import { is_armor_work } from "../../../../src/LFW/collision/is_armor_work";
import { is_fall } from "../../../../src/LFW/collision/is_fall";
import { Entity } from "../../../../src/LFW/entity/Entity";

import { parseJsStringLiteral, parseValue, readCaseLines, renderValue, splitWs } from "./trace_util";

type Any = never;

const out: string[] = [];

const proto = Entity.prototype as unknown as Record<string, unknown>;
const datasetFn = proto["dataset"] as ((this: unknown, name: string) => unknown) | undefined;

function makeEntity(raw: unknown): Any {
  const e = { ...(raw as Record<string, unknown>) } as Record<string, unknown>;
  if (datasetFn) {
    e["dataset"] = function (this: unknown, name: string): unknown {
      return datasetFn.call(this, name);
    };
  }
  for (const key of ["weight", "state"]) {
    const d = Object.getOwnPropertyDescriptor(proto, key);
    if (d && d.get) {
      Object.defineProperty(e, key, { get: d.get, configurable: true, enumerable: d.enumerable });
    }
  }
  return e as Any;
}

function main(): void {
  const casePath = process.argv[2];
  if (!casePath) {
    process.stderr.write("usage: lfw_trace_collision_helpers.mjs <case-file>\n");
    process.exit(2);
  }

  const ents = new Map<string, Any>();
  const cols = new Map<string, Any>();

  const entOf = (name: string): Any => {
    if (!ents.has(name)) {
      process.stderr.write(`unknown entity '${name}'\n`);
      process.exit(2);
    }
    return ents.get(name) as Any;
  };

  const colOf = (name: string): Any => {
    if (!cols.has(name)) {
      process.stderr.write(`unknown collision '${name}'\n`);
      process.exit(2);
    }
    return cols.get(name) as Any;
  };

  for (const raw of readCaseLines(casePath)) {
    const t = splitWs(raw);
    if (t.length === 0) continue;
    const op = t[0]!;

    if (op === "ds") {
      if (t.length !== 3) {
        process.stderr.write(`ds needs <entity> <key>: ${raw}\n`);
        process.exit(2);
      }
      const eName = parseJsStringLiteral(t[1]!);
      const key = parseJsStringLiteral(t[2]!);
      out.push(`ds ${eName} ${key} ${renderValue((entOf(eName) as Any).dataset(key))}`);
      continue;
    }

    if (op === "ifall" || op === "armor" || op === "civ") {
      if (t.length !== 2) {
        process.stderr.write(`${op} needs <collision>: ${raw}\n`);
        process.exit(2);
      }
      const cName = parseJsStringLiteral(t[1]!);
      const c = colOf(cName);
      if (op === "ifall") out.push(`ifall ${cName} ${is_fall(c as Any) ? "b1" : "b0"}`);
      else if (op === "armor") out.push(`armor ${cName} ${is_armor_work(c as Any) ? "b1" : "b0"}`);
      else {
        const r = calc_itr_velocity(c as Any);
        out.push(`civ ${cName} ${renderValue(r[0])} ${renderValue(r[1])} ${renderValue(r[2])} ${renderValue(r[3])}`);
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
    } else if (op === "col") {
      if (sub === "mk") {
        const aName = parseJsStringLiteral(t[idx[0]!++]!);
        const vName = parseJsStringLiteral(t[idx[0]!++]!);
        const itr = parseValue(t, idx);
        const bframe = parseValue(t, idx);
        const aframe = parseValue(t, idx);
        cols.set(name, { attacker: entOf(aName), victim: entOf(vName), itr, bframe, aframe });
      } else if (sub === "del") {
        cols.delete(name);
      } else {
        process.stderr.write(`unknown col sub '${sub}'\n`);
        process.exit(2);
      }
      out.push(`col ${name}`);
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
