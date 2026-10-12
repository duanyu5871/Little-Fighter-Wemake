import type { Entity } from "../LFW";
import { make_buring_smoke } from "../LFW/dat_translator/make_buring_smoke";
import { make_fb_bat_chase_start } from "../LFW/dat_translator/frame_behavior/make_fb_bat_chase_start";
import { make_fb_firzen_disater_start } from "../LFW/dat_translator/frame_behavior/make_fb_firzen_disater_start";
import { make_fb_jan_chase_start } from "../LFW/dat_translator/frame_behavior/make_fb_jan_chase_start";
import { make_fb_julian_ball_start } from "../LFW/dat_translator/frame_behavior/make_fb_julian_ball_start";
import type { IOpointInfo } from "../LFW/defines";
import { Defines } from "../LFW/defines/defines";
import { EntityEnum } from "../LFW/defines/EntityEnum";
import { StateEnum } from "../LFW/defines/StateEnum";
import { Ditto } from "../LFW/ditto";
import { Randoming } from "../LFW/helper/Randoming";
import type { IOpointInfoContext } from "../LFW/loader/IEntityDataContext";
import { preprocess_opoint } from "../LFW/loader/preprocess_opoint";
import { round } from "../LFW/utils";
import { MersenneTwister } from "../LFW/utils/math/MersenneTwister";
import { range } from "../LFW/utils/math/range";

class FakeMt {
  mark = "";
  calls: unknown[][] = [];
  range(min: number, max: number): number {
    this.calls.push(["range", this.mark, min, max]);
    return min;
  }
  pick(list: number[]): number {
    this.calls.push(["pick", this.mark, ...list]);
    return list[0];
  }
}

const frame = { width: 96, height: 48, centerx: 50, centery: 30 };

const opoint = (extra: Partial<IOpointInfo>): IOpointInfo =>
  ({ kind: 0, x: 0, y: 0, oid: "test", action: { id: "0" }, ...extra }) as IOpointInfo;

const pre = (o: IOpointInfo, extra: Partial<IOpointInfoContext> = {}): IOpointInfo =>
  preprocess_opoint({ opoint: o, ...extra } as unknown as IOpointInfoContext);

const make_emitter = (mt: FakeMt) => ({ frame, lfw: { mt } }) as unknown as Entity;

const with_warn = (fn: () => void) => {
  const prev = (Ditto as any).warn;
  const warn = jest.fn();
  (Ditto as any).warn = warn;
  try {
    fn();
  } finally {
    (Ditto as any).warn = prev;
  }
  return warn;
};

test("gen_x/gen_y/gen_z 编译为运行时 getter", () => {
  const mt = new FakeMt();
  const ret = pre(
    opoint({
      gen_x: "rand(-w/6, w/6)",
      gen_y: "cy + rand(-h/4, 0)",
      gen_z: "round(w/2)",
    }),
  );
  const e = make_emitter(mt);
  expect(ret.__gen_x?.get(e)).toBe(-16);
  expect(ret.__gen_y?.get(e)).toBe(18);
  expect(ret.__gen_z?.get(e)).toBe(48);
  expect(mt.calls).toEqual([
    ["range", "gen_x", -16, 16],
    ["range", "gen_y", -12, 0],
  ]);
});

test("gen_dvx/gen_dvy/gen_dvz 编译为运行时 getter", () => {
  const mt = new FakeMt();
  const ret = pre(
    opoint({
      gen_dvx: "pick(-1, 1)",
      gen_dvy: "rand(1, 3)",
      gen_dvz: "round(h/8)",
    }),
  );
  const e = make_emitter(mt);
  expect(ret.__gen_dvx?.get(e)).toBe(-1);
  expect(ret.__gen_dvy?.get(e)).toBe(1);
  expect(ret.__gen_dvz?.get(e)).toBe(6);
  expect(mt.calls).toEqual([
    ["pick", "gen_dvx", -1, 1],
    ["range", "gen_dvy", 1, 3],
  ]);
});

test("未声明 gen_* 时不生成 getter", () => {
  const hook = { get: () => 7 };
  const ret = pre(opoint({ __gen_x: hook }));
  expect(ret.__gen_x).toBe(hook);
  expect(ret.__gen_y).toBeUndefined();
  expect(ret.__gen_z).toBeUndefined();
});

test("错误表达式：告警、不生成 getter、保留已有 __gen_*", () => {
  const hook = { get: () => 7 };
  let ret!: IOpointInfo;
  const warn = with_warn(() => {
    ret = pre(opoint({ gen_x: "1 +", gen_y: "rand(1)", __gen_z: hook }));
  });
  expect(warn).toHaveBeenCalledTimes(2);
  expect(warn.mock.calls[0]![0]).toContain("[ValExpression]");
  expect(ret.__gen_x).toBeUndefined();
  expect(ret.__gen_y).toBeUndefined();
  expect(ret.__gen_z).toBe(hook);
});

test("燃烧烟雾: gen_x/gen_y 与旧公式抽取序列一致", () => {
  const { width: w, height: h, centerx: cx, centery: cy } = frame;
  const exp_mt = new MersenneTwister(1234);
  const ref_mt = new MersenneTwister(1234);
  const e = { frame, lfw: { mt: exp_mt } } as unknown as Entity;
  const o1 = pre(make_buring_smoke(1));
  const o2 = pre(make_buring_smoke(2));
  const ww = round(w / 6);
  for (let i = 0; i < 200; ++i) {
    expect(o1.__gen_y!.get(e)).toBe(round(cy + ref_mt.range(-round(h / 2), 0)));
    expect(o1.__gen_x!.get(e)).toBe(round(ref_mt.range(round(w / 4), round(3 * w / 4))));
    expect(o2.__gen_x!.get(e)).toBe(round(cx + ref_mt.range(-ww, ww)));
    expect(o2.__gen_y!.get(e)).toBe(round(cy + ref_mt.range(-round(3 * h / 4), 0)));
  }
});

test("gen_spread_* 编译并携带发射者调用", () => {
  const mt = new FakeMt();
  const ret = pre(
    opoint({ spreading: 1, gen_spread_x: "bag(-1, 1)", gen_spread_y: "rand(-2, 2)" }),
  );
  const e = make_emitter(mt);
  expect(ret.__gen_spread_x?.get(e)).toBe(-1);
  expect(ret.__gen_spread_y?.get(e)).toBe(-2);
  expect(mt.calls).toEqual([
    ["range", "gen_spread_x", 0, 2],
    ["range", "gen_spread_y", -2, 2],
  ]);
});

test("扩散迁移: 各工厂与旧 Randoming 机制抽取序列一致", () => {
  const exp_mt = new MersenneTwister(1234);
  const ref_mt = new MersenneTwister(1234);
  const e = { frame, lfw: { mt: exp_mt } } as unknown as Entity;
  const spawn = (make: (f: any) => void): IOpointInfo => {
    const f: any = { centerx: 50, centery: 30 };
    make(f);
    return pre(f.opoint[0]!);
  };

  const bat = spawn(make_fb_bat_chase_start);
  const bat_x = new Randoming("bat_x", range(-6, 6, 1), ref_mt);
  const bat_z = new Randoming("bat_z", range(-2, 2, 1), ref_mt);
  for (let i = 0; i < 40; ++i) {
    expect(bat.__gen_spread_x!.get(e)).toBe(bat_x.get());
    expect(bat.__gen_spread_z!.get(e)).toBe(bat_z.get());
  }

  const disater = spawn(make_fb_firzen_disater_start);
  const disater_x = new Randoming("disater_x", range(-5, 5, 1), ref_mt);
  const disater_y = new Randoming("disater_y", range(2, 8, 0.5), ref_mt);
  for (let i = 0; i < 40; ++i) {
    expect(disater.__gen_spread_x!.get(e)).toBe(disater_x.get());
    expect(disater.__gen_spread_y!.get(e)).toBe(disater_y.get());
  }

  const jan = spawn(make_fb_jan_chase_start);
  const jan_x = new Randoming("jan_x", range(-5, 5, 1), ref_mt);
  const jan_y = new Randoming("jan_y", range(2, 8, 0.5), ref_mt);
  for (let i = 0; i < 40; ++i) {
    expect(jan.__gen_spread_x!.get(e)).toBe(jan_x.get());
    expect(jan.__gen_spread_y!.get(e)).toBe(jan_y.get());
  }

  const julian = spawn(make_fb_julian_ball_start);
  for (let i = 0; i < 40; ++i) {
    expect(julian.__gen_spread_y!.get(e)).toBe(ref_mt.range(-5, 5) / 10);
    expect(julian.__gen_spread_z!.get(e)).toBe(ref_mt.range(-15, 15) / 10);
  }
});

const fighter = { type: EntityEnum.Fighter } as any;

const ball_states = [
  StateEnum.Ball_Flying,
  StateEnum.Ball_3006,
  StateEnum.Weapon_Throwing,
  StateEnum.HeavyWeapon_InTheSky,
];

test("斗士数据 + 弹道类状态：speedz 默认落 3.5", () => {
  for (const state of ball_states) {
    const ret = pre(opoint({}), { data: fighter, frame: { state } as any });
    expect(ret.speedz).toBe(Defines.DEFAULT_OPOINT_SPEED_Z);
  }
});

test("非弹道状态 / 无 frame：不落 speedz", () => {
  expect(pre(opoint({}), { data: fighter, frame: { state: StateEnum.Standing } as any }).speedz).toBeUndefined();
  expect(pre(opoint({}), { data: fighter }).speedz).toBeUndefined();
  expect(pre(opoint({})).speedz).toBeUndefined();
});

test("不看 data：非斗士数据也落 speedz", () => {
  const not_fighter = { type: 0 } as any;
  expect(pre(opoint({}), { data: not_fighter, frame: { state: StateEnum.Ball_Flying } as any }).speedz).toBe(Defines.DEFAULT_OPOINT_SPEED_Z);
  expect(pre(opoint({}), { frame: { state: StateEnum.Ball_Flying } as any }).speedz).toBe(Defines.DEFAULT_OPOINT_SPEED_Z);
});

test("已声明 speedz：??= 保留原值", () => {
  const ret = pre(opoint({ speedz: 7 }), { data: fighter, frame: { state: StateEnum.Ball_Flying } as any });
  expect(ret.speedz).toBe(7);
});
