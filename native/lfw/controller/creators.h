#pragma once

#include "lfw/factory.h"

namespace lfw {
namespace controller {

// TS `Factory.register_ctrl(data.id, BallController)` 里的「类」在端口是 `ICtrlCreator`。
// 控制器看不见 `Entity`（见 `base_controller.h`）：创建时只落 `player_id`，env（player /
// lfw / world）在 `Entity::set_ctrl` 挂上后刷新 —— 与 TS 构造函数「当场拿 entity」的错位
// 与 4R 的 bot 一致（记 README 偏差表）。
//
// `DatMgr._cook_data` 是 TS 里唯一的注册点（ball / weapon ⇒ `BallController`，
// fighter ⇒ `BotController`）；一个类一个单例（注册表的池键就是指针身份）。
const ICtrlCreator* bot_controller_creator();
const ICtrlCreator* ball_controller_creator();

}
}
