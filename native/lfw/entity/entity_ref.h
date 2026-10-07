#pragma once

#include "lfw/core/value.h"

namespace lfw {

class Entity;

// 「实体引用」：`{ id, position, velocity, frame, team, hp, type, ghosted, data, facing,
// mounted, invisible, invulnerable, toughness, arest, wakeup_invuln, resting, ground_y,
// is_on_ground, holding, catching, bearer, catcher }`。
//
// TS 里 `BallController` / `bot/*` 直接吃 `Entity`；端口这些模块看不见 `Entity`
// （同 `CtrlEnv`），一律用 `Value` 引用 —— 这一份是唯一生成处，别各处手搭。
// `frame` 是**同一个对象**（`Value` 共享 `shared_ptr`），所以 `frame.id` 之类的读法
// 与 `core/same_ref.h` 的同一性判断都跟实体自己一致。
// 四个关系槽（`holding` / `catching` / `bearer` / `catcher`）里放的是**浅引用**
// （不再带它们自己的关系），与 TS 读 `e.bearer?.frame.wpoint` 的深度一致。
Value ref_of(const Entity& e);

}
