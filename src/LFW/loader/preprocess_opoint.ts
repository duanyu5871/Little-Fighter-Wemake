import { ValExpression } from "../base/ValExpression";
import { Defines } from "../defines/defines";
import type { IOpointInfo } from "../defines/IOpointInfo";
import { StateEnum } from "../defines/StateEnum";
import { Ditto } from "../ditto";
import type { IOpointInfoContext } from "./IEntityDataContext";

const compile_gen = (src: string, tag: string) => {
  const expr = new ValExpression(src, { tag });
  if (expr.err) {
    Ditto.warn(expr.err);
    return void 0;
  }
  return expr;
};

export function preprocess_opoint(ctx: IOpointInfoContext): IOpointInfo {
  const { opoint, frame } = ctx;
  if (opoint.gen_x) opoint.__gen_x = compile_gen(opoint.gen_x, "gen_x") ?? opoint.__gen_x;
  if (opoint.gen_y) opoint.__gen_y = compile_gen(opoint.gen_y, "gen_y") ?? opoint.__gen_y;
  if (opoint.gen_z) opoint.__gen_z = compile_gen(opoint.gen_z, "gen_z") ?? opoint.__gen_z;
  if (opoint.gen_dvx) opoint.__gen_dvx = compile_gen(opoint.gen_dvx, "gen_dvx") ?? opoint.__gen_dvx;
  if (opoint.gen_dvy) opoint.__gen_dvy = compile_gen(opoint.gen_dvy, "gen_dvy") ?? opoint.__gen_dvy;
  if (opoint.gen_dvz) opoint.__gen_dvz = compile_gen(opoint.gen_dvz, "gen_dvz") ?? opoint.__gen_dvz;
  if (opoint.gen_spread_x) opoint.__gen_spread_x = compile_gen(opoint.gen_spread_x, "gen_spread_x") ?? opoint.__gen_spread_x;
  if (opoint.gen_spread_y) opoint.__gen_spread_y = compile_gen(opoint.gen_spread_y, "gen_spread_y") ?? opoint.__gen_spread_y;
  if (opoint.gen_spread_z) opoint.__gen_spread_z = compile_gen(opoint.gen_spread_z, "gen_spread_z") ?? opoint.__gen_spread_z;

  /* TODO: 要想办法尽量摆脱 State 判定，保持数据行为透明 - Gim*/
  switch (frame?.state) {
    case StateEnum.Ball_Flying:
    case StateEnum.Ball_3006:
    case StateEnum.Weapon_Throwing:
    case StateEnum.HeavyWeapon_InTheSky:
      opoint.speedz ??= Defines.DEFAULT_OPOINT_SPEED_Z;
      break;
  }

  return opoint
}
preprocess_opoint.TAG = "preprocess_opoint";