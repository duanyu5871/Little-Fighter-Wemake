// `lfw/ditto/xml`（IXMLElement / IXML 缝的 tool 实现）的 TS 侧台面。
//
// 用例：`cases/xml/*.txt`。op（两侧名目一致）：
//   new    <eid> <tag>                `xml.create(tag)`
//   fromstr <eid> <tag> <str>         `xml.from_string(str, tag)`
//   fromnum <eid> <tag> <num>         `xml.from_number(num, tag)`
//   frombool <eid> <tag> <0|1>        `xml.from_boolean(v, tag)`
//   fromarr <eid> <tag> <literal...>  `xml.from_array(arr, tag)`（literal 须是 `a …`）
//   fromobj <eid> <tag> <literal...>  `xml.from_object(obj, tag)`（literal 须是 `o …`）
//   bytag  <eid> <name> <alias>       alias = `child_by_tag(name)`
//   bytagi <eid> <name> <i> <alias>   alias = `children_by_tag(name)[i]`
//   text   <eid> <str>                `set_text`
//   attr   <eid> <name> <literal...>  `set_attr`
//   dattr  <eid> <name>               `del_attr`
//   sattr  <eid> <name> <literal...>  `set_arr_attr_soft`
//   ins    <eid> <cid> <idx|->        `insert`（`-` = `index` 省略）
//   rm     <eid> <cid>                `remove` → 打 `rm:true/false`
//   rmself <eid>                      `remove_self` → 打 `rmself:true/false`
//   rmall  <eid>                      `remove_all`
//   rd     <eid> <what> [args...]     读（下面每种 kind 打一行 `rd|<what>|<render>`）
//   dump   <eid>                      DFS 逐层打 `<depth>|<tag>|attrs|text`
//
// `rd` 的 kind：tag / text / parent / children / attrs / type / action / strop /
//   attr <name> / str_attr <name> / num_attr <name> / bool_attr <name> /
//   strs <name> / nums <name> / strssoft <name> / numssoft <name> /
//   asstr / asnum / asbool / asval / asarr / asobj /
//   asstr_or <str> / asnum_or <num> / asbool_or <0|1> /
//   getstr <name> / getnum <name> / getbool <name> /
//   getstr_or <name> <str> / getnum_or <name> <num> / getbool_or <name> <0|1> /
//   gstrarr <name> / gnumarr <name> / getobj <name> / cbt <name> / cbtall <name>
//
// xml 方言读写层（`src/LFW/dat_translator/xml/*`）的 op（数据对象放 `dv` 变量）：
//   dv     <dvar> [literal]           定义数据对象（缺省 undefined）
//   dset   <dvar> <key> <literal>     `obj[key] = v`（u 也会留键）
//   ddel   <dvar> <key>               `delete obj[key]`
//   ddump  <dvar>                     打 `ddump|<dvar>|<render>`
//   dvp    <dvar> <fn> <eid|-> [arg]  读口：`dvar = fn(el[, arg])`（eid `-` = undefined）
//   dvo    <dvar> <fn> <srcvar>       值助手：one_or_arr / non_empty
//   dvm    <dvar> <eid> <tag> <fn> [target]  `merge_by_tag`（target 给了就传）
//   wrv    <eid> <fn> <dvar> <tag>    写口：造元素存到 `<eid>`，打 `wrv|<eid>|<esc stringify>|u`
//   wrins  <parentEid> <fn> <dvar> <tag>  `xml_x_t_next_frame` 造一串挂到父上，打 count + 父串
// 读口 fn：xml_2_bdy / xml_2_itr / xml_2_armor_info / xml_2_chase / xml_2_bpoint /
//   xml_2_wpoint / xml_2_cpoint / xml_2_next_frame / xml_2_colli_action /
//   xml_2_t_next_frame <tag> / xml_2_qube [outvar|-] / xml_to_velocity_info [outvar|-] /
//   parse_rect_qube；写口 fn：xml_x_bdy / xml_x_itr / xml_x_armor_info / xml_x_chase /
//   xml_x_bpoint / xml_x_wpoint / xml_x_cpoint / xml_x_next_frame / xml_x_colli_action。
import { merge_by_tag } from "../../../../src/LFW/dat_translator/xml/merge_by_tag";
import { non_empty, one_or_arr } from "../../../../src/LFW/dat_translator/xml/one_or_arr";
import { parse_rect_qube } from "../../../../src/LFW/dat_translator/xml/parse_rect_qube";
import { xml_to_velocity_info } from "../../../../src/LFW/dat_translator/xml/xml_to_velocity_info";
import {
  xml_2_armor_info,
  xml_x_armor_info,
} from "../../../../src/LFW/dat_translator/xml/xml_x_armor_info";
import { xml_2_bdy, xml_x_bdy } from "../../../../src/LFW/dat_translator/xml/xml_x_bdy";
import { xml_2_bpoint, xml_x_bpoint } from "../../../../src/LFW/dat_translator/xml/xml_x_bpoint";
import { xml_2_chase, xml_x_chase } from "../../../../src/LFW/dat_translator/xml/xml_x_chase";
import {
  xml_2_colli_action,
  xml_x_colli_action,
} from "../../../../src/LFW/dat_translator/xml/xml_x_colli_action";
import { xml_2_cpoint, xml_x_cpoint } from "../../../../src/LFW/dat_translator/xml/xml_x_cpoint";
import { xml_2_itr, xml_x_itr } from "../../../../src/LFW/dat_translator/xml/xml_x_itr";
import {
  xml_2_next_frame,
  xml_2_t_next_frame,
  xml_x_next_frame,
  xml_x_t_next_frame,
} from "../../../../src/LFW/dat_translator/xml/xml_x_next_frame";
import { xml_2_qube } from "../../../../src/LFW/dat_translator/xml/xml_x_qube";
import { xml_2_wpoint, xml_x_wpoint } from "../../../../src/LFW/dat_translator/xml/xml_x_wpoint";
import type { IXMLElement } from "../../../../src/LFW/ditto/xml";
import { ToolXML, ToolXMLElement } from "../../../../tool/src/xml";

import { esc, keyOf, parseValue, readCaseLines, renderValue, splitWs } from "./trace_util";

const log: string[] = [];
const xml = new ToolXML();
const els = new Map<string, ToolXMLElement>();
const data = new Map<string, unknown>();

type ReaderFn = (el: IXMLElement, arg: string, hasArg: boolean) => unknown;

const readers: Record<string, ReaderFn> = {
  xml_2_bdy: (e) => xml_2_bdy(e),
  xml_2_itr: (e) => xml_2_itr(e),
  xml_2_armor_info: (e) => xml_2_armor_info(e),
  xml_2_chase: (e) => xml_2_chase(e),
  xml_2_bpoint: (e) => xml_2_bpoint(e),
  xml_2_wpoint: (e) => xml_2_wpoint(e),
  xml_2_cpoint: (e) => xml_2_cpoint(e),
  xml_2_next_frame: (e) => xml_2_next_frame(e),
  xml_2_colli_action: (e) => xml_2_colli_action(e),
  xml_2_t_next_frame: (e, arg) => xml_2_t_next_frame(e.children_by_tag(keyOf(arg))),
  xml_2_qube: (e, arg, hasArg) =>
    xml_2_qube(e, hasArg && arg !== "-" ? (data.get(arg) as never) : undefined),
  xml_to_velocity_info: (e, arg, hasArg) =>
    xml_to_velocity_info(e, hasArg && arg !== "-" ? (data.get(arg) as never) : undefined),
  parse_rect_qube: (e) => parse_rect_qube(e),
};

type ParserFn = (child: IXMLElement) => Record<string, unknown>;
type WriterFn = (xml: ToolXML, d: unknown, tag: string) => IXMLElement | undefined | null;

const parsers: Record<string, ParserFn> = {
  xml_2_bdy: (c) => xml_2_bdy(c) as never,
  xml_2_itr: (c) => xml_2_itr(c) as never,
  xml_2_chase: (c) => xml_2_chase(c) as never,
  xml_2_bpoint: (c) => xml_2_bpoint(c) as never,
  xml_2_wpoint: (c) => xml_2_wpoint(c) as never,
  xml_2_cpoint: (c) => xml_2_cpoint(c) as never,
  xml_2_next_frame: (c) => xml_2_next_frame(c) as never,
  xml_2_colli_action: (c) => xml_2_colli_action(c) as never,
};

const writers: Record<string, WriterFn> = {
  xml_x_bdy: (x, d, tag) => xml_x_bdy(x, d as never, tag),
  xml_x_itr: (x, d, tag) => xml_x_itr(x, d as never, tag),
  xml_x_armor_info: (x, d, tag) => xml_x_armor_info(x, d as never, tag),
  xml_x_chase: (x, d, tag) => xml_x_chase(x, d as never, tag),
  xml_x_bpoint: (x, d, tag) => xml_x_bpoint(x, d as never, tag),
  xml_x_wpoint: (x, d, tag) => xml_x_wpoint(x, d as never, tag),
  xml_x_cpoint: (x, d, tag) => xml_x_cpoint(x, d as never, tag),
  xml_x_next_frame: (x, d, tag) => xml_x_next_frame(x, d as never, tag),
  xml_x_colli_action: (x, d, tag) => xml_x_colli_action(x, d as never, tag),
};

function fail(msg: string): never {
  process.stderr.write(msg + "\n");
  process.exit(2);
}

function el(id: string): ToolXMLElement {
  const e = els.get(id);
  if (e === undefined) fail(`unknown element '${id}'`);
  return e;
}

function dump(node: ToolXMLElement, depth: number): void {
  const attrs = node.attrs.map((a) => `${esc(a.name)}=${esc(a.value)}`).join(",");
  log.push(
    `d|${depth}|${esc(node.tag)}|${node.attrs.length}|${attrs}|${esc(node.text)}`,
  );
  for (const child of node.children) dump(child, depth + 1);
}

function main(): void {
  const casePath = process.argv[2];
  if (!casePath) fail("usage: lfw_trace_xml.mjs <case-file>");

  for (const raw of readCaseLines(casePath)) {
    const t = splitWs(raw);
    if (t.length === 0) continue;
    const op = t[0]!;
    const i = [1];
    const next = (): string => t[i[0]!++]!;

    if (op === "new") {
      els.set(next(), xml.create(keyOf(next())));
    } else if (op === "fromstr") {
      const eid = next();
      const tag = keyOf(next());
      els.set(eid, xml.from_string(keyOf(next()), tag));
    } else if (op === "fromnum") {
      const eid = next();
      const tag = keyOf(next());
      els.set(eid, xml.from_number(Number(next()), tag));
    } else if (op === "frombool") {
      const eid = next();
      const tag = keyOf(next());
      els.set(eid, xml.from_boolean(next() === "1", tag));
    } else if (op === "fromarr") {
      const eid = next();
      const tag = keyOf(next());
      const v = parseValue(t, i);
      if (!Array.isArray(v)) fail(`fromarr expects an array literal: ${raw}`);
      els.set(eid, xml.from_array(v, tag));
    } else if (op === "fromobj") {
      const eid = next();
      const tag = keyOf(next());
      const v = parseValue(t, i);
      if (typeof v !== "object" || v === null || Array.isArray(v)) {
        fail(`fromobj expects an object literal: ${raw}`);
      }
      els.set(eid, xml.from_object(v, tag));
    } else if (op === "bytag") {
      const parent = el(next());
      const name = keyOf(next());
      const alias = next();
      const child = parent.child_by_tag(name);
      if (child === undefined) fail(`bytag: no child '${name}': ${raw}`);
      els.set(alias, child);
    } else if (op === "bytagi") {
      const parent = el(next());
      const name = keyOf(next());
      const idx = Number(next());
      const alias = next();
      const kids = parent.children_by_tag(name);
      if (idx >= kids.length) fail(`bytagi: index ${idx} out of range: ${raw}`);
      els.set(alias, kids[idx]!);
    } else if (op === "text") {
      el(next()).set_text(keyOf(next()));
    } else if (op === "attr") {
      el(next()).set_attr(keyOf(next()), parseValue(t, i));
    } else if (op === "dattr") {
      el(next()).del_attr(keyOf(next()));
    } else if (op === "sattr") {
      el(next()).set_arr_attr_soft(keyOf(next()), parseValue(t, i));
    } else if (op === "ins") {
      const parent = el(next());
      const child = el(next());
      const idx = next();
      parent.insert(child, idx === "-" ? undefined : Number(idx));
    } else if (op === "rm") {
      const ok = el(next()).remove(el(next()));
      log.push(`rm:${ok}`);
    } else if (op === "rmself") {
      log.push(`rmself:${el(next()).remove_self()}`);
    } else if (op === "rmall") {
      el(next()).remove_all();
    } else if (op === "dv") {
      const name = next();
      data.set(name, i[0]! < t.length ? parseValue(t, i) : undefined);
    } else if (op === "dset") {
      const o = data.get(next()) as Record<string, unknown>;
      o[keyOf(next())] = parseValue(t, i);
    } else if (op === "ddel") {
      const o = data.get(next()) as Record<string, unknown>;
      delete o[keyOf(next())];
    } else if (op === "ddump") {
      const name = next();
      log.push(`ddump|${name}|${renderValue(data.get(name))}`);
    } else if (op === "dvp") {
      const name = next();
      const fn = next();
      const eid = next();
      const hasArg = i[0]! < t.length;
      const arg = hasArg ? next() : "";
      const reader = readers[fn] ?? fail(`unknown reader fn '${fn}'`);
      const v = reader(eid === "-" ? (undefined as never) : el(eid), arg, hasArg);
      data.set(name, v);
      log.push(`dvp|${fn}|${renderValue(v)}`);
    } else if (op === "dvo") {
      const name = next();
      const fn = next();
      const src = data.get(next());
      const v =
        fn === "one_or_arr"
          ? one_or_arr(src as never)
          : fn === "non_empty"
            ? non_empty(src as never)
            : fail(`unknown helper fn '${fn}'`);
      data.set(name, v);
      log.push(`dvo|${fn}|${renderValue(v)}`);
    } else if (op === "dvm") {
      const name = next();
      const parent = el(next());
      const tag = keyOf(next());
      const fn = next();
      const parser = parsers[fn] ?? fail(`unknown parser fn '${fn}'`);
      const hasTarget = i[0]! < t.length;
      const v = hasTarget
        ? merge_by_tag(parent, tag, parser, data.get(next()) as never)
        : merge_by_tag(parent, tag, parser);
      data.set(name, v);
      log.push(`dvm|${fn}|${renderValue(v)}`);
    } else if (op === "wrv") {
      const name = next();
      const fn = next();
      const d = data.get(next());
      const tag = keyOf(next());
      const writer = writers[fn] ?? fail(`unknown writer fn '${fn}'`);
      const e = writer(xml, d, tag);
      if (e) {
        els.set(name, e as ToolXMLElement);
        log.push(`wrv|${name}|${esc(e.stringify())}`);
      } else {
        log.push(`wrv|${name}|u`);
      }
    } else if (op === "wrins") {
      const parentId = next();
      const fn = next();
      const d = data.get(next());
      const tag = keyOf(next());
      if (fn !== "xml_x_t_next_frame") fail("wrins only supports xml_x_t_next_frame");
      const parent = el(parentId);
      const made = xml_x_t_next_frame(xml, d, tag, parent);
      made.forEach((e, j) => els.set(`${parentId}:${j}`, e as ToolXMLElement));
      log.push(`wrins|${parentId}|n=${made.length}|${esc(parent.stringify())}`);
    } else if (op === "dump") {
      dump(el(next()), 0);
    } else if (op === "rd") {
      const e = el(next());
      const what = next();
      let out = `rd|${what}|`;
      if (what === "tag") {
        out += esc(e.tag);
      } else if (what === "text") {
        out += esc(e.text);
      } else if (what === "parent") {
        out += e.parent === undefined ? "u" : esc(e.parent.tag);
      } else if (what === "children") {
        out += `n=${e.children.length}|${e.children.map((c) => esc(c.tag)).join(",")}`;
      } else if (what === "attrs") {
        const attrs = e.attrs.map((a) => `${esc(a.name)}=${esc(a.value)}`).join(",");
        out += `n=${e.attrs.length}|${attrs}`;
      } else if (what === "type") {
        out += e.type === undefined ? "u" : esc(e.type);
      } else if (what === "action") {
        out += esc(e.action_str());
      } else if (what === "strop") {
        out += esc(e.stringify());
      } else if (what === "hasattr") {
        out += e.has_attr(keyOf(next())) ? "true" : "false";
      } else if (what === "attr" || what === "str_attr") {
        const v = e.attr(keyOf(next()));
        out += v === undefined ? "u" : esc(v);
      } else if (what === "num_attr") {
        const v = e.num_attr(keyOf(next()));
        out += v === undefined ? "u" : renderValue(v);
      } else if (what === "bool_attr") {
        const v = e.bool_attr(keyOf(next()));
        out += v === undefined ? "u" : renderValue(v);
      } else if (what === "strs") {
        const v = e.strs_attr(keyOf(next()));
        out += v === undefined ? "u" : renderValue(v);
      } else if (what === "nums") {
        const v = e.nums_attr(keyOf(next()));
        out += v === undefined ? "u" : renderValue(v);
      } else if (what === "strssoft") {
        const v = e.strs_attr_soft(keyOf(next()));
        out += v === undefined ? "u" : renderValue(v);
      } else if (what === "numssoft") {
        const v = e.nums_attr_soft(keyOf(next()));
        out += v === undefined ? "u" : renderValue(v);
      } else if (what === "asstr") {
        const v = e.as_string();
        out += v === undefined ? "u" : esc(v);
      } else if (what === "asnum") {
        const v = e.as_number();
        out += v === undefined ? "u" : renderValue(v);
      } else if (what === "asbool") {
        const v = e.as_boolean();
        out += v === undefined ? "u" : renderValue(v);
      } else if (what === "asval") {
        out += renderValue(e.as_value());
      } else if (what === "asarr") {
        out += renderValue(e.as_array());
      } else if (what === "asobj") {
        out += renderValue(e.as_object());
      } else if (what === "asobj_or") {
        out += renderValue(e.as_object(parseValue(t, i)));
      } else if (what === "asarr_or") {
        out += renderValue(e.as_array(parseValue(t, i)));
      } else if (what === "asstr_or") {
        out += esc(e.as_string(keyOf(next())));
      } else if (what === "asnum_or") {
        out += renderValue(e.as_number(Number(next())));
      } else if (what === "asbool_or") {
        out += renderValue(e.as_boolean(next() === "1"));
      } else if (what === "getstr") {
        const v = e.get_str(keyOf(next()));
        out += v === undefined ? "u" : esc(v);
      } else if (what === "getnum") {
        const v = e.get_num(keyOf(next()));
        out += v === undefined ? "u" : renderValue(v);
      } else if (what === "getbool") {
        const v = e.get_bool(keyOf(next()));
        out += v === undefined ? "u" : renderValue(v);
      } else if (what === "getstr_or") {
        const name = keyOf(next());
        out += esc(e.get_str(name, keyOf(next())));
      } else if (what === "getnum_or") {
        const name = keyOf(next());
        out += renderValue(e.get_num(name, Number(next())));
      } else if (what === "getbool_or") {
        const name = keyOf(next());
        out += renderValue(e.get_bool(name, next() === "1"));
      } else if (what === "gstrarr") {
        const v = e.get_str_arr(keyOf(next()));
        out += v === undefined ? "u" : renderValue(v);
      } else if (what === "gnumarr") {
        const v = e.get_num_arr(keyOf(next()));
        out += v === undefined ? "u" : renderValue(v);
      } else if (what === "getobj") {
        out += renderValue(e.get_obj(keyOf(next())));
      } else if (what === "cbt") {
        const c = e.child_by_tag(keyOf(next()));
        out += c === undefined ? "u" : esc(c.tag);
      } else if (what === "cbtall") {
        const kids = e.children_by_tag(keyOf(next()));
        out += `n=${kids.length}|${kids.map((c) => esc(c.tag)).join(",")}`;
      } else {
        fail(`unknown rd kind '${what}'`);
      }
      log.push(out);
    } else {
      fail(`unknown op '${op}'`);
    }

    if (i[0] !== t.length) fail(`trailing token(s): ${raw}`);
    for (const l of log) process.stdout.write(l + "\n");
    log.length = 0;
  }
}

main();
