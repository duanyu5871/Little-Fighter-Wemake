import { writeFileSync } from "node:fs";
import { createRequire } from "node:module";

const require = createRequire(import.meta.url);
const unicode = require("../../node_modules/json5/lib/unicode.js");

const OUT = "native/lfw/core/json5_unicode.cpp";

function probe(re) {
  const ranges = [];
  let start = -1;
  for (let cp = 0; cp <= 0x10ffff; cp++) {
    const hit = re.test(String.fromCodePoint(cp));
    if (hit && start < 0) start = cp;
    else if (!hit && start >= 0) {
      ranges.push([start, cp - 1]);
      start = -1;
    }
  }
  if (start >= 0) ranges.push([start, 0x10ffff]);
  return ranges;
}

const hex = (v) => "0x" + v.toString(16);

const tables = [
  ["kSpaceSeparator", probe(unicode.Space_Separator)],
  ["kIdStart", probe(unicode.ID_Start)],
  ["kIdContinue", probe(unicode.ID_Continue)],
];

const lines = ['#include "json5_unicode.h"', "", "namespace lfw {", ""];

for (const [name, ranges] of tables) {
  lines.push(`const CpRange ${name}[] = {`);
  let row = "  ";
  for (const [lo, hi] of ranges) {
    const item = `{${hex(lo)}, ${hex(hi)}}, `;
    if (row.length + item.length > 96) {
      lines.push(row.replace(/ $/, ""));
      row = "  ";
    }
    row += item;
  }
  if (row.trim().length) lines.push(row.replace(/ $/, ""));
  lines.push("};");
  lines.push(`const size_t ${name}Size = sizeof(${name}) / sizeof(CpRange);`);
  lines.push("");
}

lines.push("}");
lines.push("");

writeFileSync(OUT, lines.join("\n"));
console.error(
  `generated ${OUT}: ` + tables.map(([n, r]) => `${n}=${r.length}`).join(" "),
);
