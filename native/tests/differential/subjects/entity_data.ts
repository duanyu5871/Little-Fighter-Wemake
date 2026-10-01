import { make_entity_data } from "../../../../src/LFW/dat_translator/make_entity_data";
import { make_frames_special } from "../../../../src/LFW/dat_translator/make_frames_special";
import { post_process_obj_data } from "../../../../src/LFW/dat_translator/post_process_obj_data";
import { traversal } from "../../../../src/LFW/utils/container_help/traversal";

import { parseValue, readCaseLines, renderValue, splitWs } from "./trace_util";

const out: string[] = [];
const objs = new Map<string, unknown>();

function render(v: unknown): string {
  return renderValue(v);
}

function objOf(id: string): Record<string, unknown> {
  const cur = objs.get(id);
  if (cur !== undefined && typeof cur === "object" && !Array.isArray(cur)) {
    return cur as Record<string, unknown>;
  }
  const o: Record<string, unknown> = {};
  objs.set(id, o);
  return o;
}

function fieldOf(id: string, key: string): unknown {
  const o = objs.get(id);
  if (o === undefined || o === null) return undefined;
  return (o as Record<string, unknown>)[key];
}

function valueOf(t: string[], i: number[]): unknown {
  return parseValue(t, i);
}

function collectTraversal(target: unknown): unknown[] {
  const acc: { k: string; v: unknown }[] = [];
  traversal(target as never, ((k: string, v: unknown) => {
    acc.push({ k, v });
  }) as never);
  return acc;
}

function main(): void {
  const casePath = process.argv[2];
  if (!casePath) {
    process.stderr.write("usage: lfw_trace_entity_data.mjs <case-file>\n");
    process.exit(2);
  }

  for (const raw of readCaseLines(casePath)) {
    const t = splitWs(raw);
    if (t.length === 0) continue;
    const op = t[0]!;
    const i = [1];
    const next = (): string => t[i[0]!++]!;

    if (op === "new") {
      objs.set(next(), {});
    } else if (op === "set") {
      const id = next();
      const key = next();
      objOf(id)[key] = valueOf(t, i);
    } else if (op === "setarr") {
      const id = next();
      const key = next();
      const arr: unknown[] = [];
      while (i[0]! < t.length) arr.push(valueOf(t, i));
      objOf(id)[key] = arr;
    } else if (op === "dump") {
      const id = next();
      out.push(`D ${id} ${render(objs.get(id))}`);
    } else if (op === "trav") {
      const id = next();
      out.push(`TRAV ${id} ${render(collectTraversal(objs.get(id)))}`);
    } else if (op === "travk") {
      const id = next();
      const key = next();
      out.push(`TRAVK ${id} ${key} ${render(collectTraversal(fieldOf(id, key)))}`);
    } else if (op === "mfs") {
      const id = next();
      const v = objOf(id);
      make_frames_special(v as never);
      out.push(`MFS ${id} ${render(v)}`);
    } else if (op === "med") {
      const id = next();
      const v = objOf(id);
      out.push(`MED ${id} ${render(make_entity_data(v as never))}`);
    } else if (op === "ppo") {
      const id = next();
      const v = objOf(id);
      post_process_obj_data(v as never);
      out.push(`PPO ${id} ${render(v)}`);
    } else {
      process.stderr.write(`unknown op '${op}'\n`);
      process.exit(2);
    }
  }

  process.stdout.write(out.join("\n") + (out.length ? "\n" : ""));
}

main();
