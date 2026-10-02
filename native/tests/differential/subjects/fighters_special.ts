import {
  make_fighter_data_bat,
  make_fighter_data_davis,
  make_fighter_data_deep,
  make_fighter_data_dennis,
  make_fighter_data_firen,
  make_fighter_data_firzen,
  make_fighter_data_freeze,
  make_fighter_data_henry,
  make_fighter_data_henter,
  make_fighter_data_jack,
  make_fighter_data_jan,
  make_fighter_data_john,
  make_fighter_data_julian,
  make_fighter_data_justin,
  make_fighter_data_knight,
  make_fighter_data_louis,
  make_fighter_data_louisex,
  make_fighter_data_mark,
  make_fighter_data_monk,
  make_fighter_data_rudolf,
  make_fighter_data_sorcerer,
  make_fighter_data_woody,
} from "../../../../src/LFW/dat_translator/fighters";

import { make_fighter_data_template } from "../../../../src/LFW/dat_translator/fighters/make_fighter_data_template";

import { make_fighter_special } from "../../../../src/LFW/dat_translator/make_fighter_special";

import { parseValue, readCaseLines, renderValue, splitWs } from "./trace_util";

const out: string[] = [];

type FighterFn = (data: never) => unknown;

const fighters: Record<string, FighterFn> = {
  bat: make_fighter_data_bat as never,
  davis: make_fighter_data_davis as never,
  deep: make_fighter_data_deep as never,
  dennis: make_fighter_data_dennis as never,
  firen: make_fighter_data_firen as never,
  firzen: make_fighter_data_firzen as never,
  freeze: make_fighter_data_freeze as never,
  henry: make_fighter_data_henry as never,
  henter: make_fighter_data_henter as never,
  jack: make_fighter_data_jack as never,
  jan: make_fighter_data_jan as never,
  john: make_fighter_data_john as never,
  julian: make_fighter_data_julian as never,
  justin: make_fighter_data_justin as never,
  knight: make_fighter_data_knight as never,
  louis: make_fighter_data_louis as never,
  louisex: make_fighter_data_louisex as never,
  mark: make_fighter_data_mark as never,
  monk: make_fighter_data_monk as never,
  rudolf: make_fighter_data_rudolf as never,
  sorcerer: make_fighter_data_sorcerer as never,
  template: make_fighter_data_template as never,
  woody: make_fighter_data_woody as never,
};

function main(): void {
  const casePath = process.argv[2];
  if (!casePath) {
    process.stderr.write("usage: lfw_trace_fighters_special.mjs <case-file>\n");
    process.exit(2);
  }

  for (const raw of readCaseLines(casePath)) {
    const t = splitWs(raw);
    if (t.length === 0) continue;
    const op = t[0]!;

    if (op === "fd") {
      const name = t[1];
      const fn = name ? fighters[name] : undefined;
      if (!name || !fn) {
        process.stderr.write(`unknown fighter '${name}'\n`);
        process.exit(2);
      }
      const idx = [2];
      const data = parseValue(t, idx);
      if (idx[0] !== t.length) {
        process.stderr.write(`fd trailing token(s): ${name}\n`);
        process.exit(2);
      }
      out.push(`fd ${name} ${renderValue(fn(data as never))}`);
      continue;
    }

    if (op === "fs") {
      const idx = [1];
      const data = parseValue(t, idx);
      if (idx[0] !== t.length) {
        process.stderr.write("fs trailing token(s)\n");
        process.exit(2);
      }
      out.push(`fs ${renderValue(make_fighter_special(data as never))}`);
      continue;
    }

    process.stderr.write(`unknown op '${op}'\n`);
    process.exit(2);
  }

  process.stdout.write(out.join("\n") + (out.length ? "\n" : ""));
}

main();
