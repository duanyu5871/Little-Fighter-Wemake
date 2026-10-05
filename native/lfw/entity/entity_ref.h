#pragma once

#include "lfw/core/value.h"

namespace lfw {

class Entity;

// 「实体引用」：`{ id, position:{x,y,z}, frame, team, hp, type, ghosted }`。
//
// TS 里 `BallController` / `bot/*` 直接吃 `Entity`；端口这些模块看不见 `Entity`
// （同 `CtrlEnv`），一律用 `Value` 引用 —— 这一份是唯一生成处，别各处手搭。
// `frame` 是**同一个对象**（`Value` 共享 `shared_ptr`），所以 `frame.id` 之类的读法
// 与 `core/same_ref.h` 的同一性判断都跟实体自己一致。
Value ref_of(const Entity& e);

}
