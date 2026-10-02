#include "lfw/entity/entity_snapshot.h"

#include <cmath>
#include <variant>
#include <vector>

#include "lfw/core/value.h"
#include "lfw/defines/enum_entries.h"

namespace lfw {
namespace entity {

const std::vector<EnumNumberEntry>& nslot_entries() {
  static const std::vector<EnumNumberEntry> e = {
      {u"WAIT", static_cast<double>(NSlot::WAIT)},
      {u"VARIANT", static_cast<double>(NSlot::VARIANT)},
      {u"TRANSFORM_INDEX", static_cast<double>(NSlot::TRANSFORM_INDEX)},
      {u"LIFETIME", static_cast<double>(NSlot::LIFETIME)},
      {u"SPAWN_TIME", static_cast<double>(NSlot::SPAWN_TIME)},
      {u"RESERVE", static_cast<double>(NSlot::RESERVE)},
      {u"MOUNTED", static_cast<double>(NSlot::MOUNTED)},
      {u"GHOSTED", static_cast<double>(NSlot::GHOSTED)},
      {u"RESTING", static_cast<double>(NSlot::RESTING)},
      {u"RESTING_MAX", static_cast<double>(NSlot::RESTING_MAX)},
      {u"TOUGHNESS", static_cast<double>(NSlot::TOUGHNESS)},
      {u"TOUGHNESS_MAX", static_cast<double>(NSlot::TOUGHNESS_MAX)},
      {u"TOUGHNESS_R_VALUE", static_cast<double>(NSlot::TOUGHNESS_R_VALUE)},
      {u"TOUGHNESS_RESTING", static_cast<double>(NSlot::TOUGHNESS_RESTING)},
      {u"TOUGHNESS_RESTING_MAX", static_cast<double>(NSlot::TOUGHNESS_RESTING_MAX)},
      {u"FALL_VALUE", static_cast<double>(NSlot::FALL_VALUE)},
      {u"FALL_VALUE_MAX", static_cast<double>(NSlot::FALL_VALUE_MAX)},
      {u"FALL_R_VALUE", static_cast<double>(NSlot::FALL_R_VALUE)},
      {u"DEFEND_VALUE", static_cast<double>(NSlot::DEFEND_VALUE)},
      {u"DEFEND_VALUE_MAX", static_cast<double>(NSlot::DEFEND_VALUE_MAX)},
      {u"DEFEND_R_VALUE", static_cast<double>(NSlot::DEFEND_R_VALUE)},
      {u"DEFEND_RATIO", static_cast<double>(NSlot::DEFEND_RATIO)},
      {u"FALLINJURY", static_cast<double>(NSlot::FALLINJURY)},
      {u"THROWINJURY", static_cast<double>(NSlot::THROWINJURY)},
      {u"FACING", static_cast<double>(NSlot::FACING)},
      {u"POS_X", static_cast<double>(NSlot::POS_X)},
      {u"POS_Y", static_cast<double>(NSlot::POS_Y)},
      {u"POS_Z", static_cast<double>(NSlot::POS_Z)},
      {u"PREV_POS_X", static_cast<double>(NSlot::PREV_POS_X)},
      {u"PREV_POS_Y", static_cast<double>(NSlot::PREV_POS_Y)},
      {u"PREV_POS_Z", static_cast<double>(NSlot::PREV_POS_Z)},
      {u"VEL_X", static_cast<double>(NSlot::VEL_X)},
      {u"VEL_Y", static_cast<double>(NSlot::VEL_Y)},
      {u"VEL_Z", static_cast<double>(NSlot::VEL_Z)},
      {u"PREV_VEL_X", static_cast<double>(NSlot::PREV_VEL_X)},
      {u"PREV_VEL_Y", static_cast<double>(NSlot::PREV_VEL_Y)},
      {u"PREV_VEL_Z", static_cast<double>(NSlot::PREV_VEL_Z)},
      {u"MP", static_cast<double>(NSlot::MP)},
      {u"MP_MAX", static_cast<double>(NSlot::MP_MAX)},
      {u"HP", static_cast<double>(NSlot::HP)},
      {u"HP_R", static_cast<double>(NSlot::HP_R)},
      {u"HP_MAX", static_cast<double>(NSlot::HP_MAX)},
      {u"AREST", static_cast<double>(NSlot::AREST)},
      {u"MOTIONLESS", static_cast<double>(NSlot::MOTIONLESS)},
      {u"SHAKING", static_cast<double>(NSlot::SHAKING)},
      {u"CATCH_TIME", static_cast<double>(NSlot::CATCH_TIME)},
      {u"CATCH_TIME_MAX", static_cast<double>(NSlot::CATCH_TIME_MAX)},
      {u"DISMISS_TIME", static_cast<double>(NSlot::DISMISS_TIME)},
      {u"INVISIBLE_DURATION", static_cast<double>(NSlot::INVISIBLE_DURATION)},
      {u"INVULNERABLE_DURATION", static_cast<double>(NSlot::INVULNERABLE_DURATION)},
      {u"BLINKING_DURATION", static_cast<double>(NSlot::BLINKING_DURATION)},
      {u"JUMP_X", static_cast<double>(NSlot::JUMP_X)},
      {u"JUMP_Y", static_cast<double>(NSlot::JUMP_Y)},
      {u"JUMP_Z", static_cast<double>(NSlot::JUMP_Z)},
      {u"JUMP_T", static_cast<double>(NSlot::JUMP_T)},
      {u"GROUND_Y", static_cast<double>(NSlot::GROUND_Y)},
      {u"PREV_GROUND_Y", static_cast<double>(NSlot::PREV_GROUND_Y)},
      {u"AABB_MIN_X", static_cast<double>(NSlot::AABB_MIN_X)},
      {u"AABB_MAX_X", static_cast<double>(NSlot::AABB_MAX_X)},
      {u"AABB_MIN_Z", static_cast<double>(NSlot::AABB_MIN_Z)},
      {u"AABB_MAX_Z", static_cast<double>(NSlot::AABB_MAX_Z)},
      {u"L_LEN", static_cast<double>(NSlot::L_LEN)},
      {u"R_LEN", static_cast<double>(NSlot::R_LEN)},
      {u"STAT_BAR_TYPE", static_cast<double>(NSlot::STAT_BAR_TYPE)},
      {u"HP_R_TICK_VALUE", static_cast<double>(NSlot::HP_R_TICK_VALUE)},
      {u"HP_R_TICK_MIN", static_cast<double>(NSlot::HP_R_TICK_MIN)},
      {u"HP_R_TICK_MAX", static_cast<double>(NSlot::HP_R_TICK_MAX)},
      {u"HP_R_TICK_LIFES", static_cast<double>(NSlot::HP_R_TICK_LIFES)},
      {u"HP_R_TICK_REMAINS", static_cast<double>(NSlot::HP_R_TICK_REMAINS)},
      {u"MP_R_TICK_VALUE", static_cast<double>(NSlot::MP_R_TICK_VALUE)},
      {u"MP_R_TICK_MIN", static_cast<double>(NSlot::MP_R_TICK_MIN)},
      {u"MP_R_TICK_MAX", static_cast<double>(NSlot::MP_R_TICK_MAX)},
      {u"MP_R_TICK_LIFES", static_cast<double>(NSlot::MP_R_TICK_LIFES)},
      {u"MP_R_TICK_REMAINS", static_cast<double>(NSlot::MP_R_TICK_REMAINS)},
      {u"RESTING_TICK_VALUE", static_cast<double>(NSlot::RESTING_TICK_VALUE)},
      {u"RESTING_TICK_MIN", static_cast<double>(NSlot::RESTING_TICK_MIN)},
      {u"RESTING_TICK_MAX", static_cast<double>(NSlot::RESTING_TICK_MAX)},
      {u"RESTING_TICK_LIFES", static_cast<double>(NSlot::RESTING_TICK_LIFES)},
      {u"RESTING_TICK_REMAINS", static_cast<double>(NSlot::RESTING_TICK_REMAINS)},
      {u"TOUGHNESS_R_TICK_VALUE", static_cast<double>(NSlot::TOUGHNESS_R_TICK_VALUE)},
      {u"TOUGHNESS_R_TICK_MIN", static_cast<double>(NSlot::TOUGHNESS_R_TICK_MIN)},
      {u"TOUGHNESS_R_TICK_MAX", static_cast<double>(NSlot::TOUGHNESS_R_TICK_MAX)},
      {u"TOUGHNESS_R_TICK_LIFES", static_cast<double>(NSlot::TOUGHNESS_R_TICK_LIFES)},
      {u"TOUGHNESS_R_TICK_REMAINS", static_cast<double>(NSlot::TOUGHNESS_R_TICK_REMAINS)},
      {u"FALL_R_TICK_VALUE", static_cast<double>(NSlot::FALL_R_TICK_VALUE)},
      {u"FALL_R_TICK_MIN", static_cast<double>(NSlot::FALL_R_TICK_MIN)},
      {u"FALL_R_TICK_MAX", static_cast<double>(NSlot::FALL_R_TICK_MAX)},
      {u"FALL_R_TICK_LIFES", static_cast<double>(NSlot::FALL_R_TICK_LIFES)},
      {u"FALL_R_TICK_REMAINS", static_cast<double>(NSlot::FALL_R_TICK_REMAINS)},
      {u"DEFEND_R_TICK_VALUE", static_cast<double>(NSlot::DEFEND_R_TICK_VALUE)},
      {u"DEFEND_R_TICK_MIN", static_cast<double>(NSlot::DEFEND_R_TICK_MIN)},
      {u"DEFEND_R_TICK_MAX", static_cast<double>(NSlot::DEFEND_R_TICK_MAX)},
      {u"DEFEND_R_TICK_LIFES", static_cast<double>(NSlot::DEFEND_R_TICK_LIFES)},
      {u"DEFEND_R_TICK_REMAINS", static_cast<double>(NSlot::DEFEND_R_TICK_REMAINS)},
      {u"BOUNCED", static_cast<double>(NSlot::BOUNCED)},
      {u"LYING_A_COUNT", static_cast<double>(NSlot::LYING_A_COUNT)},
      {u"LYING_D_COUNT", static_cast<double>(NSlot::LYING_D_COUNT)},
      {u"LYING_C_COUNT", static_cast<double>(NSlot::LYING_C_COUNT)},
      {u"DROP_HURTED", static_cast<double>(NSlot::DROP_HURTED)},
      {u"IS_ON_GROUND", static_cast<double>(NSlot::IS_ON_GROUND)},
      {u"NAME_VISIBLE", static_cast<double>(NSlot::NAME_VISIBLE)},
      {u"WAKEUP_INVULN", static_cast<double>(NSlot::WAKEUP_INVULN)},
      {u"DEAD_GONE", static_cast<double>(NSlot::DEAD_GONE)},
      {u"CTRL_VISIBLE", static_cast<double>(NSlot::CTRL_VISIBLE)},
      {u"DROPPING", static_cast<double>(NSlot::DROPPING)},
      {u"COUNT", static_cast<double>(NSlot::COUNT)},
  };
  return e;
}

const std::vector<EnumNumberEntry>& sslot_entries() {
  static const std::vector<EnumNumberEntry> e = {
      {u"ID", static_cast<double>(SSlot::ID)},
      {u"DATA_ID", static_cast<double>(SSlot::DATA_ID)},
      {u"FRAME_ID", static_cast<double>(SSlot::FRAME_ID)},
      {u"PREV_FRAME_ID", static_cast<double>(SSlot::PREV_FRAME_ID)},
      {u"LANDING_FRAME_ID", static_cast<double>(SSlot::LANDING_FRAME_ID)},
      {u"CATCHING_ID", static_cast<double>(SSlot::CATCHING_ID)},
      {u"CATCHER_ID", static_cast<double>(SSlot::CATCHER_ID)},
      {u"BEARER_ID", static_cast<double>(SSlot::BEARER_ID)},
      {u"HOLDING_ID", static_cast<double>(SSlot::HOLDING_ID)},
      {u"TEAM", static_cast<double>(SSlot::TEAM)},
      {u"NAME", static_cast<double>(SSlot::NAME)},
      {u"AFTER_BLINK", static_cast<double>(SSlot::AFTER_BLINK)},
      {u"DISMISS_DATA_ID", static_cast<double>(SSlot::DISMISS_DATA_ID)},
      {u"TRANSFORM_0_ID", static_cast<double>(SSlot::TRANSFORM_0_ID)},
      {u"TRANSFORM_1_ID", static_cast<double>(SSlot::TRANSFORM_1_ID)},
      {u"COPIES", static_cast<double>(SSlot::COPIES)},
      {u"DEAD_JOIN", static_cast<double>(SSlot::DEAD_JOIN)},
      {u"COUNT", static_cast<double>(SSlot::COUNT)},
  };
  return e;
}

double num_slots() { return static_cast<double>(NSlot::COUNT); }

double str_slots() { return static_cast<double>(SSlot::COUNT); }

Value num_or_null(const Value& v) {
  const double* d = std::get_if<double>(&v);
  if (d != nullptr && std::isnan(*d)) return Value(NullTag{});
  return v;
}

double to_tri(const Value& v) {
  if (std::holds_alternative<std::monostate>(v) || std::holds_alternative<NullTag>(v)) return -1.0;
  return truthy(v) ? 1.0 : 0.0;
}

Value from_tri(const Value& v) {
  if (lt(v, Value(0.0))) return Value(NullTag{});
  return Value(!strict_equals(v, Value(0.0)));
}

}
}
