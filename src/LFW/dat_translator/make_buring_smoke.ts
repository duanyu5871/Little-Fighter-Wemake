import { OID, type IOpointInfo } from "../defines";
import { is_object } from "../entity/type_check";
import { round } from "../utils";

export function make_buring_smoke(foo: 1 | 2): IOpointInfo {
  const ret: IOpointInfo = {
    kind: 0,
    x: 0,
    y: 0,
    oid: OID.BrokenWeapon,
    action: {
      id: "140",
      __gen_facing: {
        get: (e: unknown) => (is_object(e) ? e.lfw.mt.pick([-1, 1]) ?? 0 : 0),
      },
    },
    speedz: 0,
    ghost: true,
    unimportant: 1,
    interval: 1,
    interval_id: `buring_smoke_${foo}`,
    interval_mode: 1,
  };
  if (foo === 1) {
    ret.__gen_x = {
      get: (e: unknown) => {
        if (!is_object(e)) return 0;
        const { frame, lfw: { mt } } = e;
        const { width: w } = frame;
        mt.mark = `buring_smoke_1`;
        return round(mt.range(round(w / 4), round(3 * w / 4)));
      },
    };
    ret.__gen_y = {
      get: (e: unknown) => {
        if (!is_object(e)) return 0;
        const { frame, lfw: { mt } } = e;
        const { height: h } = frame;
        return round(frame.centery + mt.range(-round(h / 2), 0));
      },
    };
  } else {
    ret.__gen_x = {
      get: (e: unknown) => {
        if (!is_object(e)) return 0;
        const { frame, lfw: { mt } } = e;
        const ww = round(frame.width / 6);
        mt.mark = `buring_smoke_2`;
        return round(frame.centerx + mt.range(-ww, ww));
      },
    };
    ret.__gen_y = {
      get: (e: unknown) => {
        if (!is_object(e)) return 0;
        const { frame, lfw: { mt } } = e;
        const { height: h } = frame;
        return round(frame.centery + mt.range(-round(3 * h / 4), 0));
      },
    };
  }
  return ret;
}
