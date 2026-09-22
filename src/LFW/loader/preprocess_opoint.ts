import type { LFW } from '../LFW';
import { ValExpression } from "../base/ValExpression";
import { type IOpointInfo, OpointSpreading } from "../defines";
import { Ditto } from "../ditto";
import { Randoming } from "../helper";

const osr_name = (a: number) => (b: string) => `osr_${a}_${b}`

const compile_gen = (src: string, tag: string) => {
  const expr = new ValExpression(src, { tag });
  if (expr.err) {
    Ditto.warn(expr.err);
    return void 0;
  }
  return expr;
};

export function preprocess_opoint(opoint: IOpointInfo, lfw: LFW): IOpointInfo {
  const sp = opoint.spreading;
  if (sp == OpointSpreading.Spreading) {
    const spn = osr_name(sp);
    if (opoint.spreading_x?.length)
      opoint.__spreading_random_x = new Randoming(spn('x'), opoint.spreading_x as [number, ...number[]], lfw.mt)
    if (opoint.spreading_y?.length)
      opoint.__spreading_random_y = new Randoming(spn('y'), opoint.spreading_y as [number, ...number[]], lfw.mt)
    if (opoint.spreading_z?.length)
      opoint.__spreading_random_z = new Randoming(spn('z'), opoint.spreading_z as [number, ...number[]], lfw.mt)
  } else if (sp == OpointSpreading.FloatRange) {
    const spn = osr_name(sp);
    const { spreading_x: xx, spreading_y: yy, spreading_z: zz } = opoint;
    if (xx?.length == 3)
      opoint.__spreading_random_x = {
        get: () => {
          lfw.mt.mark = spn('x')
          return lfw.mt.range(xx[0], xx[1]) / xx[2]
        }
      };
    if (yy?.length == 3)
      opoint.__spreading_random_y = {
        get: () => {
          lfw.mt.mark = spn('y')
          return lfw.mt.range(yy[0], yy[1]) / yy[2]
        }
      };
    if (zz?.length == 3)
      opoint.__spreading_random_z = {
        get: () => {
          lfw.mt.mark = spn('z')
          return lfw.mt.range(zz[0], zz[1]) / zz[2]
        }
      };
  }

  if (opoint.gen_x) opoint.__gen_x = compile_gen(opoint.gen_x, "gen_x") ?? opoint.__gen_x;
  if (opoint.gen_y) opoint.__gen_y = compile_gen(opoint.gen_y, "gen_y") ?? opoint.__gen_y;
  if (opoint.gen_z) opoint.__gen_z = compile_gen(opoint.gen_z, "gen_z") ?? opoint.__gen_z;
  if (opoint.gen_dvx) opoint.__gen_dvx = compile_gen(opoint.gen_dvx, "gen_dvx") ?? opoint.__gen_dvx;
  if (opoint.gen_dvy) opoint.__gen_dvy = compile_gen(opoint.gen_dvy, "gen_dvy") ?? opoint.__gen_dvy;
  if (opoint.gen_dvz) opoint.__gen_dvz = compile_gen(opoint.gen_dvz, "gen_dvz") ?? opoint.__gen_dvz;
  return opoint
}
preprocess_opoint.TAG = "preprocess_opoint";