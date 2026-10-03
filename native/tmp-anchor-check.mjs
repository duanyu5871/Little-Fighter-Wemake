import { readFileSync } from "node:fs";

const subjects = process.argv.slice(2);
const cache = new Map();
let bad = 0;
for (const subject of subjects) {
  const spec = (await import(`./tests/differential/mutations/${subject}.mjs`)).default;
  spec.mutations.forEach((m, i) => {
    if (!cache.has(m.file)) cache.set(m.file, readFileSync(m.file, "utf8"));
    const src = cache.get(m.file);
    const n = src.split(m.from).length - 1;
    const same = m.from === m.to;
    if (n !== 1 || same) {
      bad++;
      console.log(`[${subject}] #${i} count=${n} same=${same} :: ${m.note}`);
      console.log(`   from=${JSON.stringify(m.from).slice(0, 140)}`);
    }
  });
}
console.log(bad === 0 ? "all anchors ok" : `${bad} bad anchors`);
process.exit(bad === 0 ? 0 : 1);
