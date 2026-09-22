import { OID, type IOpointInfo } from "../defines";
import { is_object } from "../entity/type_check";

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
    interval: 3,
    interval_id: `buring_smoke_${foo}`,
    interval_mode: 1,
  };
  if (foo === 1) {
    ret.gen_x = "round(rand(round(w/4), round(3*w/4)))";
    ret.gen_y = "round(cy + rand(-round(h/2), 0))";
  } else {
    ret.gen_x = "round(cx + rand(-round(w/6), round(w/6)))";
    ret.gen_y = "round(cy + rand(-round(3*h/4), 0))";
  }
  return ret;
}
