import "./shared/patch_date";

import { Randoming } from "../../../../src/LFW/helper/Randoming";
import { spawn_ice_piece, ice_piece_opoints } from "../../../../src/LFW/state/spawn_ice_piece";
import { MersenneTwister } from "../../../../src/LFW/utils/math/MersenneTwister";

import { parseJsStringLiteral, parseValue, readCaseLines, renderValue, splitWs } from "./trace_util";

type Any = never;
type Rec = Record<string, unknown>;

const GEN_KEYS = ["__gen_dvx", "__gen_dvy", "__gen_x", "__gen_y"];

const out: string[] = [];

function strip(p: unknown): unknown {
  const o = p as Rec;
  for (const k of GEN_KEYS) delete o[k];
  return o;
}

function main(): void {
  const casePath = process.argv[2];
  if (!casePath) {
    process.stderr.write("usage: lfw_trace_mt_random.mjs <case-file>\n");
    process.exit(2);
  }

  const mts = new Map<string, MersenneTwister>();
  const randoms = new Map<string, Randoming<unknown>>();
  const ents = new Map<string, unknown>();

  const need = <T>(reg: Map<string, T>, name: string, what: string): T => {
    if (!reg.has(name)) {
      process.stderr.write(`unknown ${what} '${name}'\n`);
      process.exit(2);
    }
    return reg.get(name) as T;
  };

  const mtLabel = (mt: MersenneTwister): string => {
    for (const [k, v] of mts) if (v === mt) return k;
    if (mt === Randoming.mt) return "default";
    return "?";
  };

  const mtArg = (tok: string): MersenneTwister | undefined =>
    tok === "-" ? undefined : need(mts, parseJsStringLiteral(tok), "mt");

  const rqDump = (name: string, r: Randoming<unknown>): string => {
    const p = r as unknown as Rec;
    return (
      `rn ${name} dump name=${String(p["name"])} mt=${mtLabel(p["mt"] as MersenneTwister)}` +
      ` dup=${renderValue(p["duplicate"])} taken=${renderValue(p["taken"])}` +
      ` src=${renderValue(p["_src"])} cur=${renderValue(p["cur"])}`
    );
  };

  for (const raw of readCaseLines(casePath)) {
    const t = splitWs(raw);
    if (t.length === 0) continue;
    const op = t[0]!;

    if (op === "ip") {
      if (t.length < 2) {
        process.stderr.write(`too few operands: ${raw}\n`);
        process.exit(2);
      }
      const sub = t[1]!;
      if (sub === "table") {
        out.push(`ip table ${renderValue(ice_piece_opoints.map((v) => strip(v)))}`);
        if (t.length !== 2) {
          process.stderr.write(`trailing token(s): ${raw}\n`);
          process.exit(2);
        }
        continue;
      }
      if (sub === "spawn") {
        if (t.length < 4) {
          process.stderr.write(`too few operands: ${raw}\n`);
          process.exit(2);
        }
        const name = parseJsStringLiteral(t[2]!);
        const idx = [3];
        const id = parseValue(t, idx) as string;
        if (idx[0]! !== t.length) {
          process.stderr.write(`trailing token(s): ${raw}\n`);
          process.exit(2);
        }
        out.push(`ip ${name} ${renderValue(strip(spawn_ice_piece(id)))}`);
        continue;
      }
      if (sub === "dvx" || sub === "dvy" || sub === "x" || sub === "y") {
        if (t.length < 4) {
          process.stderr.write(`too few operands: ${raw}\n`);
          process.exit(2);
        }
        const mt = mtArg(t[2]!) as MersenneTwister;
        const ent = need(ents, parseJsStringLiteral(t[3]!), "entity");
        const e = { ...(ent as Rec), lfw: { mt } };
        const proto = spawn_ice_piece("0") as unknown as Rec;
        const gen = proto[`__gen_${sub}`] as { get: (v: unknown) => unknown };
        const value = gen.get(e);
        if (t.length !== 4) {
          process.stderr.write(`trailing token(s): ${raw}\n`);
          process.exit(2);
        }
        out.push(`ip ${sub} ${renderValue(value)}`);
        continue;
      }
      process.stderr.write(`unknown ip sub '${sub}'\n`);
      process.exit(2);
    }

    if (t.length < 3) {
      process.stderr.write(`too few operands: ${raw}\n`);
      process.exit(2);
    }
    const sub = t[1]!;
    const name = parseJsStringLiteral(t[2]!);
    const idx = [3];

    if (op === "mt") {
      if (sub !== "new") {
        process.stderr.write(`unknown mt sub '${sub}'\n`);
        process.exit(2);
      }
      mts.set(name, new MersenneTwister(parseValue(t, idx) as number));
      out.push(`mt ${name}`);
    } else if (op === "ent") {
      if (sub === "put") ents.set(name, parseValue(t, idx));
      else if (sub === "del") ents.delete(name);
      else {
        process.stderr.write(`unknown ent sub '${sub}'\n`);
        process.exit(2);
      }
      out.push(`ent ${name}`);
    } else if (op === "rn") {
      if (sub === "new" || sub === "create") {
        const mt = mtArg(t[idx[0]!++]!);
        const src = parseValue(t, idx) as unknown[];
        let dup: unknown = false;
        if (sub === "create" || idx[0]! < t.length) dup = parseValue(t, idx);
        const r =
          sub === "new"
            ? new Randoming<unknown>(name, src, mt, dup as boolean)
            : Randoming.create(name, src, mt, dup as boolean);
        randoms.set(name, r);
        out.push(rqDump(name, r));
      } else {
        const r = need(randoms, name, "randoming");
        if (sub === "src") {
          r.set_src(parseValue(t, idx) as unknown[]);
          out.push(`rn ${name} set_src ${renderValue((r as unknown as Rec)["cur"])}`);
        } else if (sub === "get") {
          out.push(`rn ${name} get ${renderValue(r.get())}`);
        } else if (sub === "dump") {
          out.push(rqDump(name, r));
        } else {
          process.stderr.write(`unknown rn sub '${sub}'\n`);
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

  process.stdout.write(out.map((s) => `${s}\n`).join(""));
}

main() as Any;
