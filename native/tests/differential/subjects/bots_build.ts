import {
  bot_ball_cancelling,
  bot_ball_continuation,
  bot_chasing_action,
  bot_chasing_skill_action,
  bot_front_test,
  bot_ball_dfa,
  bot_ball_dfj,
  bot_explosion_dua,
  bot_explosion_duj,
  bot_idle_action,
  bot_uppercut_dua,
  bot_uppercut_duj,
  bot_uppercut_dva,
} from "../../../../src/LFW/dat_translator/bots";
import { frames } from "../../../../src/LFW/dat_translator/bots/frames";

import { parseValue, readCaseLines, renderValue, splitWs } from "./trace_util";

const out: string[] = [];

type AnyFn = (...args: unknown[]) => unknown;

function spreadArg(args: unknown[], n: number): unknown[] {
  const v = args[n];
  return Array.isArray(v) ? v : [];
}

function build(name: string, args: unknown[], edit: AnyFn | undefined): unknown {
  const S = (n: number): string => args[n] as string;
  const V = (n: number): never => args[n] as never;
  switch (name) {
    case "bot_ball_cancelling":
      return (bot_ball_cancelling as AnyFn)(S(0), V(1), ...spreadArg(args, 2)) as never;
    case "bot_ball_continuation":
      return (bot_ball_continuation as AnyFn)(S(0), V(1), V(2), ...spreadArg(args, 3)) as never;
    case "bot_chasing_action":
      return (bot_chasing_action as AnyFn)(S(0), V(1), V(2), V(3)) as never;
    case "bot_chasing_skill_action":
      return (bot_chasing_skill_action as AnyFn)(S(0), V(1), V(2), V(3)) as never;
    case "bot_front_test":
      return (bot_front_test as AnyFn)(S(0), V(1), V(2), V(3), V(4), V(5), V(6)) as never;
    case "bot_ball_dfa":
      return (bot_ball_dfa as AnyFn)(V(0), V(1), V(2), V(3), V(4)) as never;
    case "bot_ball_dfj":
      return (bot_ball_dfj as AnyFn)(V(0), V(1), V(2), V(3), V(4)) as never;
    case "bot_explosion_dua":
      return (bot_explosion_dua as AnyFn)(V(0), V(1), V(2), V(3), V(4)) as never;
    case "bot_explosion_duj":
      return (bot_explosion_duj as AnyFn)(V(0), V(1), V(2), V(3), V(4)) as never;
    case "bot_idle_action":
      return (bot_idle_action as AnyFn)(S(0), V(1), V(2), V(3)) as never;
    case "bot_uppercut_dua":
      return (bot_uppercut_dua as AnyFn)(V(0), V(1), V(2), V(3), V(4)) as never;
    case "bot_uppercut_duj":
      return (bot_uppercut_duj as AnyFn)(V(0), V(1), V(2), V(3), V(4)) as never;
    case "bot_uppercut_dva":
      return (bot_uppercut_dva as AnyFn)(V(0), V(1), V(2), V(3), V(4)) as never;
    default:
      return "unknown";
  }
}

function main(): void {
  const casePath = process.argv[2];
  if (!casePath) {
    process.stderr.write("usage: lfw_trace_bots_build.mjs <case-file>\n");
    process.exit(2);
  }

  const cannedEdit: AnyFn = (action, cond) => {
    const a = action as Record<string, unknown>;
    a.action_id = "Z";
    a.expression = (cond as { done: () => string }).done();
    return a;
  };

  for (const raw of readCaseLines(casePath)) {
    const t = splitWs(raw);
    if (t.length === 0) continue;
    const op = t[0]!;
    const i = [1];

    if (op === "fr") {
      out.push(`fr ${renderValue(frames)}`);
    } else if (op === "ba" || op === "bae") {
      const name = t[i[0]!++]!;
      const args: unknown[] = [];
      while (i[0]! < t.length) args.push(parseValue(t, i));
      const edit = op === "bae" ? cannedEdit : undefined;
      const made = build(name, args, edit);
      const value = typeof made === "function" ? (made as AnyFn)(edit) : made;
      out.push(`${op} ${name} ${renderValue(value)}`);
    } else {
      process.stderr.write(`unknown op '${op}'\n`);
      process.exit(2);
    }
  }

  process.stdout.write(out.join("\n") + (out.length ? "\n" : ""));
}

main();
