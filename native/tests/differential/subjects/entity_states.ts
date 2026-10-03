import "../../../../src/LFW/entity/Entity";
import { ENTITY_STATES } from "../../../../src/LFW/state/ENTITY_STATES";
import { CharacterState_Running } from "../../../../src/LFW/state/CharacterState_Running";
import { parseValue, readCaseLines, renderValue, splitWs, num } from "./trace_util";

const out: string[] = [];

const r = (v: unknown): string => renderValue(v);

const name_of = (v: unknown): string =>
  v === undefined || v === null
    ? "none"
    : (v as { constructor: { name: string } }).constructor.name;

const state_of = (v: unknown): string =>
  v === undefined || v === null ? "none" : r((v as { state: unknown }).state);

let type: unknown = undefined;
let code = 0;

function main(): void {
  const casePath = process.argv[2];
  if (!casePath) {
    process.stderr.write("usage: lfw_trace_entity_states.mjs <case-file>\n");
    process.exit(2);
  }

  for (const raw of readCaseLines(casePath)) {
    const t = splitWs(raw);
    if (t.length === 0) continue;
    const op = t[0]!;
    let i = 1;
    if (op === "env") {
      const sub = t[i++]!;
      const idx = [i];
      if (sub === "type") type = parseValue(t, idx);
      else if (sub === "code") code = num(t[i++]!);
      else {
        process.stderr.write(`unknown env '${sub}'\n`);
        process.exit(2);
      }
      out.push(`env ${sub}`);
      continue;
    }
    if (op === "run") {
      const what = t[i++]!;
      if (what === "size") {
        out.push(`run size || n=${ENTITY_STATES.map.size}`);
      } else if (what === "dump") {
        const rows: string[] = [];
        for (const [k, v] of ENTITY_STATES.map) {
          rows.push(`k=${r(k)} cls=${name_of(v)} s=${state_of(v)}`);
        }
        out.push(`run dump ||\n${rows.join("\n")}`);
      } else if (what === "head") {
        const rows: string[] = [];
        let n = 0;
        for (const [k, v] of ENTITY_STATES.map) {
          if (++n > 2) break;
          rows.push(`k=${r(k)} cls=${name_of(v)} s=${state_of(v)}`);
        }
        out.push(`run head ||\n${rows.join("\n")}`);
      } else if (what === "tail") {
        const all = [...ENTITY_STATES.map];
        const rows: string[] = [];
        for (const [k, v] of all.slice(-2)) {
          rows.push(`k=${r(k)} cls=${name_of(v)} s=${state_of(v)}`);
        }
        out.push(`run tail ||\n${rows.join("\n")}`);
      } else if (what === "has") {
        const key = parseValue(t, [i]);
        out.push(`run has ${r(key)} || has=${ENTITY_STATES.map.has(key) ? "b1" : "b0"}`);
      } else if (what === "get") {
        const key = parseValue(t, [i]);
        const hit = ENTITY_STATES.get(key);
        out.push(`run get ${r(key)} || r=${name_of(hit)} s=${state_of(hit)}`);
      } else if (what === "fallback") {
        const hit = ENTITY_STATES.fallback(type as never, code);
        out.push(`run fallback || r=${name_of(hit)} s=${state_of(hit)}`);
      } else if (what === "fallback2") {
        const a = ENTITY_STATES.fallback(type as never, code);
        const b = ENTITY_STATES.fallback(type as never, code);
        out.push(
          `run fallback2 || same=${a === b ? "b1" : "b0"} r=${name_of(b)} s=${state_of(b)}`,
        );
      } else if (what === "setdup") {
        ENTITY_STATES.set(0, new CharacterState_Running() as never);
        out.push(`run setdup ||`);
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
