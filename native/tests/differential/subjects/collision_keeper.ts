import { collisions_keeper } from "../../../../src/LFW/collision/CollisionKeeper";
import { parseValue, readCaseLines, splitWs, num } from "./trace_util";

const out: string[] = [];

interface NamedFn {
  name?: string;
}

const collision = {
  attacker: { data: { type: 0 }, state: 0 },
  victim: { data: { type: 0 }, state: 0 },
  itr: undefined as Record<string, unknown> | undefined,
  bdy: undefined as Record<string, unknown> | undefined,
  handlers: [] as NamedFn[],
};

function handlersText(): string {
  return `[${collision.handlers.map((f) => String(f?.name)).join(",")}]`;
}

function seedHandlers(v: unknown): void {
  const items = Array.isArray(v) ? v : [];
  collision.handlers = items.map((s) => ({ name: String(s) })) as NamedFn[];
}

function main(): void {
  const casePath = process.argv[2];
  if (!casePath) {
    process.stderr.write("usage: lfw_trace_collision_keeper.mjs <case-file>\n");
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
      if (sub === "a" || sub === "v") {
        const field = t[i++]!;
        const n = num(t[i++]!);
        const side = sub === "a" ? collision.attacker : collision.victim;
        if (field === "type") side.data.type = n;
        else if (field === "state") side.state = n;
        else {
          process.stderr.write(`unknown side field '${field}'\n`);
          process.exit(2);
        }
        if (i !== t.length) {
          process.stderr.write(`trailing tokens after side field '${field}'\n`);
          process.exit(2);
        }
      } else if (sub === "itr") {
        collision.itr = parseValue(t, idx) as Record<string, unknown>;
      } else if (sub === "bdy") {
        collision.bdy = parseValue(t, idx) as Record<string, unknown>;
      } else if (sub === "handlers") {
        seedHandlers(parseValue(t, idx));
      } else {
        process.stderr.write(`unknown env '${sub}'\n`);
        process.exit(2);
      }
      out.push(`env ${sub}`);
      continue;
    }
    if (op === "run") {
      const ret = collisions_keeper.load_handlers(collision as never);
      out.push(`run load || handlers=${handlersText()} | ret=${ret ? "1" : "0"}`);
      continue;
    }
    process.stderr.write(`unknown op '${op}'\n`);
    process.exit(2);
  }

  process.stdout.write(out.join("\n") + "\n");
}

main();
