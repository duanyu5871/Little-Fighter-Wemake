import { WorldDataset } from "../../../../src/LFW/WorldDataset";
import { parseValue, readCaseLines, renderValue, splitWs } from "./trace_util";

const out: string[] = [];
const log: string[] = [];

const r = (v: unknown): string => renderValue(v);

let pure = false;
let hook_key: string | null = null;
let ds: WorldDataset | undefined = undefined;

// `make_private_properties` stores each trapped field under a `_$_`-prefixed backup
// property and wires per-field hooks through `on_<key>_change` properties; those
// callback slots and backups are not part of the user level field set the port
// models, so they are filtered out of `Object.keys`.
const is_internal = (k: string): boolean => k.startsWith("on_") || k.startsWith("_$_");

const keys_of = (v: unknown): string =>
  Object.keys(v as object)
    .filter((k) => !is_internal(k))
    .join(",");

const tracked_of = (v: unknown, key: string): boolean => {
  const d = Object.getOwnPropertyDescriptor(v as object, key);
  return !!d && typeof d.set === "function";
};

function bind(): void {
  if (!ds) return;
  ds.on_dataset_change = (k: string, curr: unknown, prev: unknown): void => {
    log.push(`dataset_change:${k}:${r(curr)}:${r(prev)}`);
  };
  if (hook_key !== null) {
    (ds as unknown as Record<string, unknown>)[`on_${hook_key}_change`] = (
      v: unknown,
      prev: unknown,
    ): void => {
      log.push(`field_change:${hook_key}:${r(v)}:${r(prev)}`);
    };
  }
}

function main(): void {
  const casePath = process.argv[2];
  if (!casePath) {
    process.stderr.write("usage: lfw_trace_world_dataset.mjs <case-file>\n");
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
      if (sub === "pure") pure = Boolean(parseValue(t, idx));
      else if (sub === "hook") hook_key = String(parseValue(t, idx));
      else {
        process.stderr.write(`unknown env '${sub}'\n`);
        process.exit(2);
      }
      out.push(`env ${sub}`);
      continue;
    }
    if (op === "run") {
      const what = t[i++]!;
      if (what === "make") {
        ds = new WorldDataset(pure);
        bind();
        out.push(`run make || pure=${r(pure)} keys=${keys_of(ds)}`);
      } else if (what === "default") {
        const a = WorldDataset.DEFAULT;
        const b = WorldDataset.DEFAULT;
        ds = b;
        bind();
        out.push(`run default || same=${a === b ? "b1" : "b0"} keys=${keys_of(b)}`);
      } else if (what === "keys") {
        out.push(`run keys || ${keys_of(ds)}`);
      } else if (what === "dump") {
        out.push(`run dump || ${r(ds!.dump_dataset())}`);
      } else if (what === "get") {
        const key = String(parseValue(t, [i]));
        out.push(`run get ${key} || ${log.join(",")} | v=${r((ds as never)[key as never])}`);
      } else if (what === "has") {
        const key = String(parseValue(t, [i]));
        out.push(
          `run has ${key} || has=${Object.prototype.hasOwnProperty.call(ds, key) ? "b1" : "b0"}`,
        );
      } else if (what === "tracked") {
        const key = String(parseValue(t, [i]));
        out.push(`run tracked ${key} || tracked=${tracked_of(ds, key) ? "b1" : "b0"}`);
      } else if (what === "set") {
        const idx = [i];
        const key = String(parseValue(t, idx));
        const value = parseValue(t, idx);
        (ds as never)[key as never] = value as never;
        out.push(`run set ${key} ${r(value)} || ${log.join(",")}`);
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
