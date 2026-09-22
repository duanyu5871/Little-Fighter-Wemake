import type { Entity, LFW } from "../LFW";
import { make_buring_smoke } from "../LFW/dat_translator/make_buring_smoke";
import type { IOpointInfo } from "../LFW/defines";
import { Ditto } from "../LFW/ditto";
import { preprocess_opoint } from "../LFW/loader/preprocess_opoint";
import { round } from "../LFW/utils";
import { MersenneTwister } from "../LFW/utils/math/MersenneTwister";

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

const make_lfw = () => ({ mt: new FakeMt() }) as unknown as LFW;
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
  const ret = preprocess_opoint(
    opoint({
      gen_x: "rand(-w/6, w/6)",
      gen_y: "cy + rand(-h/4, 0)",
      gen_z: "round(w/2)",
    }),
    make_lfw(),
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
  const ret = preprocess_opoint(
    opoint({
      gen_dvx: "pick(-1, 1)",
      gen_dvy: "rand(1, 3)",
      gen_dvz: "round(h/8)",
    }),
    make_lfw(),
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
  const ret = preprocess_opoint(opoint({ __gen_x: hook }), make_lfw());
  expect(ret.__gen_x).toBe(hook);
  expect(ret.__gen_y).toBeUndefined();
  expect(ret.__gen_z).toBeUndefined();
});

test("错误表达式：告警、不生成 getter、保留已有 __gen_*", () => {
  const hook = { get: () => 7 };
  let ret!: IOpointInfo;
  const warn = with_warn(() => {
    ret = preprocess_opoint(
      opoint({ gen_x: "1 +", gen_y: "rand(1)", __gen_z: hook }),
      make_lfw(),
    );
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
  const lfw = { mt: exp_mt } as unknown as LFW;
  const e = { frame, lfw: { mt: exp_mt } } as unknown as Entity;
  const o1 = preprocess_opoint(make_buring_smoke(1), lfw);
  const o2 = preprocess_opoint(make_buring_smoke(2), lfw);
  const ww = round(w / 6);
  for (let i = 0; i < 200; ++i) {
    expect(o1.__gen_y!.get(e)).toBe(round(cy + ref_mt.range(-round(h / 2), 0)));
    expect(o1.__gen_x!.get(e)).toBe(round(ref_mt.range(round(w / 4), round(3 * w / 4))));
    expect(o2.__gen_x!.get(e)).toBe(round(cx + ref_mt.range(-ww, ww)));
    expect(o2.__gen_y!.get(e)).toBe(round(cy + ref_mt.range(-round(3 * h / 4), 0)));
  }
});
