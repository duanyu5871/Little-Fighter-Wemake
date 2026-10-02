import { make_bot_data_bat } from "../../../../src/LFW/dat_translator/bots/make_bot_data_bat";
import { make_bot_data_davis } from "../../../../src/LFW/dat_translator/bots/make_bot_data_davis";
import { make_bot_data_firen } from "../../../../src/LFW/dat_translator/bots/make_bot_data_firen";
import { make_bot_data_firzen } from "../../../../src/LFW/dat_translator/bots/make_bot_data_firzen";
import { make_bot_data_henry } from "../../../../src/LFW/dat_translator/bots/make_bot_data_henry";
import { make_bot_data_hunter } from "../../../../src/LFW/dat_translator/bots/make_bot_data_hunter";
import { make_bot_data_jack } from "../../../../src/LFW/dat_translator/bots/make_bot_data_jack";
import { make_bot_data_jan } from "../../../../src/LFW/dat_translator/bots/make_bot_data_jan";
import { make_bot_data_julian } from "../../../../src/LFW/dat_translator/bots/make_bot_data_julian";
import { make_bot_data_justin } from "../../../../src/LFW/dat_translator/bots/make_bot_data_justin";
import { make_bot_data_knight } from "../../../../src/LFW/dat_translator/bots/make_bot_data_knight";
import { make_bot_data_louis } from "../../../../src/LFW/dat_translator/bots/make_bot_data_louis";
import { make_bot_data_louisex } from "../../../../src/LFW/dat_translator/bots/make_bot_data_louisex";
import { make_bot_data_mark } from "../../../../src/LFW/dat_translator/bots/make_bot_data_mark";
import { make_bot_data_monk } from "../../../../src/LFW/dat_translator/bots/make_bot_data_monk";
import { make_bot_data_sorcerer } from "../../../../src/LFW/dat_translator/bots/make_bot_data_sorcerer";
import { make_bot_data_woody } from "../../../../src/LFW/dat_translator/bots/make_bot_data_woody";
import { BotMaker } from "../../../../src/LFW/dat_translator/bots/BotMaker";

import { readCaseLines, renderValue, splitWs } from "./trace_util";

const out: string[] = [];

type Maker = { bot: Record<string, unknown>; frames: unknown; states: unknown };

function makeBot(name: string): Maker | undefined {
  switch (name) {
    case "bat":
      return make_bot_data_bat() as never;
    case "davis":
      return make_bot_data_davis() as never;
    case "firen":
      return make_bot_data_firen() as never;
    case "firzen":
      return make_bot_data_firzen() as never;
    case "henry":
      return make_bot_data_henry() as never;
    case "hunter":
      return make_bot_data_hunter() as never;
    case "jack":
      return make_bot_data_jack() as never;
    case "jan":
      return make_bot_data_jan() as never;
    case "julian":
      return make_bot_data_julian() as never;
    case "justin":
      return make_bot_data_justin() as never;
    case "knight":
      return make_bot_data_knight() as never;
    case "louis":
      return make_bot_data_louis() as never;
    case "louisex":
      return make_bot_data_louisex() as never;
    case "mark":
      return make_bot_data_mark() as never;
    case "monk":
      return make_bot_data_monk() as never;
    case "sorcerer":
      return make_bot_data_sorcerer() as never;
    case "woody":
      return make_bot_data_woody() as never;
    default:
      return undefined;
  }
}

function main(): void {
  const casePath = process.argv[2];
  if (!casePath) {
    process.stderr.write("usage: lfw_trace_bots_data.mjs <case-file>\n");
    process.exit(2);
  }

  for (const raw of readCaseLines(casePath)) {
    const t = splitWs(raw);
    if (t.length === 0) continue;
    const op = t[0]!;
    if (op === "reg") {
      const oids = t.slice(1);
      const all = Array.from(BotMaker.makers.keys());
      const order = all.filter((k) => oids.includes(k)).join(",");
      let line = `reg order=${order}`;
      for (const oid of oids) {
        const fn = BotMaker.makers.get(oid);
        if (!fn) {
          line += ` ${oid}=missing`;
          continue;
        }
        line += ` ${oid}=${renderValue(fn().bot.id)}`;
      }
      out.push(line);
      continue;
    }

    const name = t[1];
    if (!name) {
      process.stderr.write("missing bot name\n");
      process.exit(2);
    }
    const m = makeBot(name);
    if (!m) {
      process.stderr.write(`unknown bot '${name}'\n`);
      process.exit(2);
    }

    if (op === "mb") {
      out.push(`mb ${name} ${renderValue(m.bot)}`);
    } else if (op === "mbf") {
      void m.frames;
      out.push(`mbf ${name} ${renderValue(m.bot)}`);
    } else if (op === "mbs") {
      void m.states;
      out.push(`mbs ${name} ${renderValue(m.bot)}`);
    } else if (op === "mbd") {
      out.push(`mbd ${name} ${renderValue(m.bot.dataset)}`);
    } else if (op === "mba") {
      out.push(`mba ${name} ${renderValue(m.bot.actions)}`);
    } else {
      process.stderr.write(`unknown op '${op}'\n`);
      process.exit(2);
    }
  }

  process.stdout.write(out.join("\n") + (out.length ? "\n" : ""));
}

main();
