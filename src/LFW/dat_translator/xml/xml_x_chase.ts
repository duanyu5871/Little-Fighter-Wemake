import { type IChaseInfo, type IChaseOvershoot, chase_info_new } from "../../defines";
import type { IXML } from "../../ditto";
import type { IXMLElement } from "../../ditto/xml/IXMLElement";
import { delete_undefined } from "./delete_undefined";

export function xml_2_chase(el: IXMLElement): IChaseInfo {
  const ret    /**/ = chase_info_new();
  ret.stratedy /**/ = el.get_num("stratedy", ret.stratedy)
  ret.flag     /**/ = el.get_num("flag", ret.flag)
  ret.lost     /**/ = el.get_num("lost", ret.lost)
  ret.oy       /**/ = el.get_num("oy", ret.oy)
  ret.overshoot /**/ = xml_2_overshoot(el)
  return delete_undefined(ret);
}
export function xml_x_chase(xml: IXML, c: IChaseInfo | undefined, tag: string): IXMLElement | undefined {
  if (!c) return void 0;
  const ret = xml.create(tag);
  ret.set_attr('stratedy', c.stratedy);
  ret.set_attr('flag', c.flag);
  ret.set_attr('lost', c.lost);
  ret.set_attr('oy', c.oy);
  xml_x_overshoot(ret, c.overshoot);
  return ret;
}

function xml_2_overshoot(el: IXMLElement): IChaseOvershoot | undefined {
  const over = el.nums_attr_soft("overshoot");
  if (!over?.some(v => v != null)) return void 0;
  if (over.length === 1) {
    const v = over[0]!;
    return { x: v, y: v, z: v };
  }
  const ret: IChaseOvershoot = {};
  if (over[0] != null) ret.x = over[0];
  if (over[1] != null) ret.y = over[1];
  if (over[2] != null) ret.z = over[2];
  return ret;
}

function xml_x_overshoot(el: IXMLElement, o: IChaseOvershoot | undefined): void {
  if (!o) return;
  const { x, y, z } = o;
  if (x === void 0 && y === void 0 && z === void 0) return;
  if (x !== void 0 && x === y && y === z) {
    el.set_attr("overshoot", x);
    return;
  }
  el.set_attr("overshoot", [x ?? "", y ?? "", z ?? ""].join(","));
}