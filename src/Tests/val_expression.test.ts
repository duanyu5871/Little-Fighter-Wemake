import { ValExpression, type IValExpressionOptions } from "../LFW/base/ValExpression";

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

type GetArg = Parameters<ValExpression["get"]>[0];

const frame = { width: 96, height: 48, centerx: 50, centery: 30 };

const make_entity = (mt: FakeMt): GetArg =>
  ({ frame, lfw: { mt } }) as unknown as GetArg;

const eval_src = (
  src: string,
  mt = new FakeMt(),
  options?: IValExpressionOptions,
): number => new ValExpression(src, options).get(make_entity(mt));

test("字面量、优先级与结合性", () => {
  expect(eval_src("1+2*3")).toBe(7);
  expect(eval_src("(1+2)*(3+4)")).toBe(21);
  expect(eval_src("-(2+3)*4")).toBe(-20);
  expect(eval_src("8/2/2")).toBe(2);
  expect(eval_src("2+3*4-6/3")).toBe(12);
  expect(eval_src(".5*2")).toBe(1);
});

test("变量与空白", () => {
  expect(eval_src("w/2+cx")).toBe(98);
  expect(eval_src(" h * 2 ")).toBe(96);
  expect(eval_src("\nw\n+\ncy")).toBe(126);
});

test("vars 可注入/覆盖", () => {
  expect(eval_src("w", new FakeMt(), { vars: { w: () => 5 } })).toBe(5);
});

test("rand: 参数顺序与 mark", () => {
  const mt = new FakeMt();
  eval_src("rand(-w/6, w/6)", mt, { tag: "gen_x" });
  expect(mt.calls).toEqual([["range", "gen_x", -16, 16]]);

  const mt2 = new FakeMt();
  expect(eval_src("rand(0, 4) + rand(10, 20)", mt2, { tag: "g" })).toBe(10);
  expect(mt2.calls).toEqual([
    ["range", "g", 0, 4],
    ["range", "g", 10, 20],
  ]);
});

test("pick: 列表原样传递", () => {
  const mt = new FakeMt();
  expect(eval_src("pick(1,2,2,3)", mt, { tag: "gen_dvx" })).toBe(1);
  expect(mt.calls).toEqual([["pick", "gen_dvx", 1, 2, 2, 3]]);
});

test("flip: 走 pick([-1,1])", () => {
  const mt = new FakeMt();
  expect(eval_src("flip()", mt, { tag: "gen_facing" })).toBe(-1);
  expect(mt.calls).toEqual([["pick", "gen_facing", -1, 1]]);
});

test("round", () => {
  expect(eval_src("round(2.6)")).toBe(3);
  expect(eval_src("round(-2.6)")).toBe(-3);
  expect(eval_src("round(w/4)")).toBe(24);
});

test("解析错误：err 有值、get 返回 0", () => {
  const bad = [
    "w +",
    "ww",
    "foo(1)",
    "rand(1)",
    "pick()",
    "flip(1)",
    "round()",
    "(w",
    "w)",
    "w$",
    "",
    "1e3",
  ];
  for (const src of bad) {
    const expr = new ValExpression(src, { tag: "t" });
    expect(expr.err).toBeTruthy();
    expect(expr.get(make_entity(new FakeMt()))).toBe(0);
  }
});

test("错误信息带 tag 与原因", () => {
  const expr = new ValExpression("ww", { tag: "gen_y" });
  expect(expr.err).toContain("[ValExpression] gen_y:");
  expect(expr.err).toContain("unknown identifier");
});
