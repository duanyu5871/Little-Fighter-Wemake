import { createRequire } from "node:module";

const require = createRequire(import.meta.url);
const util = require("../../node_modules/json5/lib/util.js");
const JSON5 = require("../../node_modules/json5/lib/index.js");

for (const c of ["$", "_", "a", "0", "\u200c", "\u200d", "\u00e9", "\uD835\uDC00"]) {
  process.stdout.write(
    `${JSON.stringify(c)} start=${util.isIdStartChar(c)} continue=${util.isIdContinueChar(c)}\n`,
  );
}

process.stdout.write("--- keys ---\n");
for (const k of ["a", "_a", "$a", "a1", "", "1a", "a-b", "\u00e9", "a\u200c", "\uD835\uDC00", "a\uD835\uDC00"]) {
  process.stdout.write(`${JSON.stringify(k)} -> ${JSON.stringify(JSON5.stringify({ [k]: 1 }))}\n`);
}

process.stdout.write("--- strings ---\n");
const strings = [
  "a",
  "a'b",
  'a"b',
  "a'b\"c",
  "'",
  '"',
  "''",
  '""',
  "\u0001",
  "\u0008",
  "\u000b",
  "x\u2028y",
  "a\u0000b",
  "a\u00001b",
  "\u00e9",
  "\uD83D\uDE00",
];
for (const s of strings) {
  process.stdout.write(`${JSON.stringify(s)} -> ${JSON.stringify(JSON5.stringify(s))}\n`);
}

process.stdout.write("--- numbers ---\n");
for (const n of [0, -0, NaN, Infinity, -Infinity, 1e21, 1e-7, 123456789012345678901234567890]) {
  process.stdout.write(`${Object.is(n, -0) ? "-0" : String(n)} -> ${JSON.stringify(JSON5.stringify(n))}\n`);
}
