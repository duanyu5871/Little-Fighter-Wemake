#pragma once

#include <cstddef>
#include <string>
#include <variant>

#include "lfw/controller/base_controller.h"
#include "lfw/core/value.h"
#include "lfw/defines/entity_enum.h"
#include "lfw/defines/entity_group.h"
#include "lfw/utils/container_help/field_or.h"

namespace lfw {
namespace entity {

bool is_fighter_data(const Value& v);
bool is_weapon_data(const Value& v);
bool is_entity_data(const Value& v);
bool is_ball_data(const Value& v);
bool is_fighter(const Value& v);
bool is_ball(const Value& v);
bool is_weapon(const Value& v);
bool is_entity(const Value& v);
bool is_boss(const Value& v);
bool is_object(const Value& v);
bool is_object_data(const Value& v);
bool is_bg_data(const Value& v);
bool is_base_ctrl(const Value& v);
bool is_bot_ctrl(const Value& v);
bool is_human_ctrl(const Value& v);
bool is_ball_ctrl(const Value& v);
// `is_bot_ctrl(v: any): v is BotController`（TS 的 `type_check.ts` 读 `v?.__is_bot_ctrl__`）：
// 端口里控制器是对象 ⇒ 这一条直接收对象（`Value` 形状的那条按 `__is_bot_ctrl__` 字段读）。
bool is_bot_ctrl(const controller::BaseController* ctrl);

}
}
