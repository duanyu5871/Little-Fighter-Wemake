#pragma once

#include <string>

#include "lfw/core/value.h"
#include "lfw/entity/entity.h"

namespace lfw {
namespace ui {

// TS `ui/component/ISumInfo.ts` / `ITeamSumInfo.ts` / `IFighterSumInfo.ts` 的端口形状。
// 四个 maker 定义在 `summary_logic.cpp`（唯一的消费方是 `SummaryLogic`）。

struct SumInfo {
  double wins = 0;
  double loses = 0;
  double kills = 0;
  double damages = 0;
  double pickings = 0;
  double spawns = 0;
  double deads = 0;
  std::u16string team;
  double latest_dead_time = -1;
  double hp_lost = 0;
  double mp_usage = 0;
  bool lost = false;
};

struct TeamSumInfo : SumInfo {
  double lives = 0;
  double hp = 0;
  double reserve = 0;
};

struct FighterSumInfo : SumInfo {
  Value data;
};

struct PlayerSumInfo : FighterSumInfo {
  std::u16string player_id;
};

SumInfo make_sum_info(const std::u16string& team);
FighterSumInfo make_fighter_sum(const Value& data);
PlayerSumInfo make_player_sum(Entity& e);
TeamSumInfo make_team_sum_info(const std::u16string& team);

}
}
