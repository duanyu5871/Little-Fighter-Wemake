#pragma once

#include "lfw/defines/i_vector2.h"

namespace lfw {
namespace ditto {

// Mirrors `src/LFW/ditto/Instance.ts`：`Ditto` 是宿主装进来的平台包（浏览器里是 three.js 的类，
// 由 `Ditto.setup(pack)` 挂上）。本刀只用到 `vec2`：`vec2(x?, y?)` 就是 `new Vector2(x, y)`，
// 而默认参只吃 `undefined`（`null` 会原样写进 `x`）⇒ 端口只收数字，`null` / `undefined` 由
// 调用点自己处理（`Camera` 只用无参形式与数字形式，记在 README 偏差表）。
inline Vector2 vec2() { return Vector2(); }
inline Vector2 vec2(double x, double y) { return Vector2(x, y); }

}
}
