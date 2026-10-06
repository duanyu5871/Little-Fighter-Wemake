import { I18N } from "../../../../src/LFW/I18N";
import { get_import_fallbacks } from "../../../../src/LFW/loader/get_import_fallbacks";

import { parseValue, readCaseLines, renderValue, splitWs } from "./trace_util";

const out: string[] = [];
const insts = new Map<string, I18N>();

function instOf(id: string): I18N {
  let v = insts.get(id);
  if (v === undefined) {
    v = new I18N();
    insts.set(id, v);
  }
  return v;
}

// `[TAG] …` 开头的才是要对比的错误文本（其它错误只记 `-`，两边都不比对）。
function msgOf(err: unknown): string {
  const msg = err instanceof Error ? err.message : String(err);
  return msg.startsWith("[") ? msg : "-";
}

// JS 的默认参数只对 `undefined` 生效（显式的 `u` 也一样）⇒ 缺省或 `u` 时取当前语言。
function langArg(it: I18N, tokens: string[], i: number[]): unknown {
  const v = i[0]! < tokens.length ? parseValue(tokens, i) : undefined;
  return v === undefined ? it.lang : v;
}

function main(): void {
  const casePath = process.argv[2];
  if (!casePath) {
    process.stderr.write("usage: lfw_trace_i18n.mjs <case-file>\n");
    process.exit(2);
  }

  for (const raw of readCaseLines(casePath)) {
    const t = splitWs(raw);
    if (t.length === 0) continue;
    const op = t[0]!;
    const i = [1];
    const next = (): string => t[i[0]!++]!;

    if (op === "gif") {
      const name = parseValue(t, i);
      try {
        const [fallbacks, suffix] = get_import_fallbacks(name as never);
        out.push(`gif ok ${renderValue(fallbacks)} suffix=${renderValue(suffix)}`);
      } catch (err) {
        out.push(`gif throw - suffix=- msg=${msgOf(err)}`);
      }
    } else if (op === "new") {
      const id = next();
      insts.set(id, new I18N());
      out.push(`new ${id}`);
    } else if (op === "add") {
      const id = next();
      const langs = parseValue(t, i);
      instOf(id).add(langs as never);
      out.push(`add ${id} ${renderValue(langs)}`);
    } else if (op === "lang") {
      const id = next();
      const v = parseValue(t, i);
      let msg = "-";
      try {
        instOf(id).set_lang(v as never);
      } catch (err) {
        msg = msgOf(err);
      }
      const it = instOf(id);
      out.push(`lang ${id} ${msg === "-" ? "ok" : "throw"} cur=${renderValue(it.lang)} msg=${msg}`);
    } else if (op === "alias" || op === "canonical") {
      const id = next();
      const it = instOf(id);
      const v = langArg(it, t, i);
      const got = op === "alias" ? it.alias(v as never) : it.canonical(v as never);
      out.push(`${op} ${id} ${renderValue(got)}`);
    } else if (op === "str" || op === "strs") {
      const id = next();
      const name = parseValue(t, i);
      const it = instOf(id);
      const lang = langArg(it, t, i);
      const got =
        op === "str"
          ? it.string(name as never, lang as never)
          : it.strings(name as never, lang as never);
      out.push(`${op} ${id} ${renderValue(got)}`);
    } else {
      process.stderr.write(`unknown op '${op}'\n`);
      process.exit(2);
    }

    if (i[0] !== t.length) {
      process.stderr.write(`trailing token(s): ${raw}\n`);
      process.exit(2);
    }
  }

  process.stdout.write(out.join("\n") + (out.length ? "\n" : ""));
}

main();
