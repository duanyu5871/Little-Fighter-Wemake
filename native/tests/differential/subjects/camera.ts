// `Camera`（`src/LFW/Camera.ts`）的 TS 侧台面。
//
// 用例：`cases/camera/all.txt`。op：
//   sf     <stage|bg|dataset> <field> <value>   给假世界的某个字段赋值（值字面量：`u` / `z` / `n 5` / `s "x"` / `b 1`）
//   new                                         `new Camera(world)`
//   dump                                        destination / position / velocity / locked / dested
//   reset | undest | unlock
//   jx <n> | jy <n>                             `jump_x` / `jump_y`
//   dest <n> <n> | lock <n> <n>                 `dest` / `lock`
//   pos <n> <n> | dset <n> <n> | vel <n> <n>     直接写 `position` / `destination` / `velocity` 的分量
//   update                                      `camera.update()`
//
// 世界用 `Proxy` 包起来：每次读 `world.stage` / `stage.cam_l` 这类属性都记一条
// `r:<对象>.<字段>=<值>` ⇒ 读的**顺序与次数**都能在日志里看到（端口侧对应 `ICameraWorld` 的
// 三个方法 + `field_or`）。
import { Camera } from "../../../../src/LFW/Camera";
import { Ditto } from "../../../../src/LFW/ditto/Instance";

import { keyOf, numHex, parseValue, readCaseLines, splitWs } from "./trace_util";

const log: string[] = [];

function fail(msg: string): never {
  process.stderr.write(msg + "\n");
  process.exit(2);
}

function pair(v: { x: number; y: number } | null): string {
  if (v === null || v === undefined) return "z";
  return numHex(v.x) + "," + numHex(v.y);
}

type Bag = Record<string, unknown>;

// 只观察 `world.stage` / `world.bg` / `world.dataset` 这几**次**属性读（端口侧的
// `ICameraWorld` 三个方法能记同样的日志）；包内字段的读由 `field_or` 完成、两侧都无法记 ⇒ 不记。
const stage: Bag = {};
const bg: Bag = {};
const dataset: Bag = {};
const world: Bag = { stage, bg, dataset };

function install_world(): void {
  const proxy = new Proxy(world, {
    get(target, key, receiver) {
      if (typeof key !== "string") return Reflect.get(target, key, receiver) as never;
      log.push(`w:${key}`);
      return target[key] as never;
    },
  });
  camera = new Camera(proxy as never);
}

function number_of(t: string[], i: number[], op: string): number {
  const v = parseValue(t, i);
  if (typeof v !== "number") fail(`${op} expects a number literal`);
  return v;
}

let camera: Camera;

function dump(): void {
  log.push(
    `dump|dest=${pair(camera.destination)}` +
      `|pos=${pair(camera.position)}` +
      `|vel=${pair(camera.velocity)}` +
      `|locked=${pair(camera.locked)}` +
      `|dested=${pair(camera.dested)}`,
  );
}

function main(): void {
  const casePath = process.argv[2];
  if (!casePath) fail("usage: lfw_trace_camera.mjs <case-file>");

  // `Ditto.vec2`：浏览器里是 three.js 的 `Vector2`（`constructor(x = 0, y = 0)`）。
  class FakeVector2 {
    x: number;
    y: number;
    constructor(x = 0, y = 0) {
      this.x = x;
      this.y = y;
    }
  }
  Ditto.setup({ Vector2: FakeVector2 } as never);

  install_world();

  for (const raw of readCaseLines(casePath)) {
    const t = splitWs(raw);
    if (t.length === 0) continue;
    const op = t[0]!;
    const i = [1];

    if (op === "sf") {
      const bagName = t[i[0]!++]!;
      const field = keyOf(t[i[0]!++]!);
      const value = parseValue(t, i);
      const bag = bagName === "stage" ? stage : bagName === "bg" ? bg : bagName === "dataset" ? dataset : fail(`bad bag '${bagName}'`);
      bag[field] = value;
    } else if (op === "new") {
      install_world();
    } else if (op === "dump") {
      dump();
    } else if (op === "reset") {
      camera.reset();
    } else if (op === "undest") {
      camera.undest();
    } else if (op === "unlock") {
      camera.unlock();
    } else if (op === "jx") {
      camera.jump_x(number_of(t, i, op));
    } else if (op === "jy") {
      camera.jump_y(number_of(t, i, op));
    } else if (op === "dest") {
      camera.dest(number_of(t, i, op), number_of(t, i, op));
    } else if (op === "lock") {
      camera.lock(number_of(t, i, op), number_of(t, i, op));
    } else if (op === "pos") {
      camera.position.x = number_of(t, i, op);
      camera.position.y = number_of(t, i, op);
    } else if (op === "dset") {
      camera.destination.x = number_of(t, i, op);
      camera.destination.y = number_of(t, i, op);
    } else if (op === "vel") {
      camera.velocity.x = number_of(t, i, op);
      camera.velocity.y = number_of(t, i, op);
    } else if (op === "update") {
      camera.update();
    } else {
      fail(`unknown op '${op}'`);
    }

    if (i[0] !== t.length) fail(`trailing token(s): ${raw}`);
    while (log.length) process.stdout.write(log.shift() + "\n");
  }
}

main();
