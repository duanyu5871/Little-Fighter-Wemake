import { EntityGroup, type IEntityData } from "../../defines";
import { ensure } from "../../utils";
import { frames } from "../bots";

export function make_fighter_data_firzen(data: IEntityData) {
  data.base.group = ensure(data.base.group, EntityGroup.Boss);
  data.base.mp_r_ratio = 2;
  data.base.ce = 2;
  data.base.bg_face ??= "sprite/MENU_BACK6.png";
  [
    ...frames.walkings,
    ...frames.defends,
    "213",
  ].forEach(v => {
    const frame = data.frames[v]
    if (!frame) return;
    frame.seqs = frame.seqs || {}
    const [sp] = frames.super_punch;
    frame.seqs[`Da`] = frame.seqs[`Da`] ?? {
      id: '' + sp
    }
  });

  [
    "216",
    "217"
  ].forEach(v => {
    const frame = data.frames[v]
    if (!frame) return;
    frame.hit = frame.hit || {}
    const [sp] = frames.super_punch;
    frame.hit[`a`] = frame.hit[`a`] ?? {
      id: '' + sp
    }
  });
  
  return data;
}
