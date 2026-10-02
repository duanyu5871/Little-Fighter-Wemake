#include "lfw/entity/entity_type_check.h"

#include <cstddef>
#include <string>
#include <variant>

#include "lfw/core/value.h"
#include "lfw/defines/entity_enum.h"
#include "lfw/defines/entity_group.h"
#include "lfw/utils/container_help/field_or.h"

namespace lfw {
namespace entity {
namespace {

bool type_is(const Value& v, EntityEnum want) {
  return strict_equals(field_or(v, u"type"), Value(static_cast<double>(want)));
}

bool flag_is_true(const Value& v, const char16_t* key) {
  const Value flag = field_or(v, key);
  const bool* b = std::get_if<bool>(&flag);
  return b != nullptr && *b;
}

size_t group_length(const Value& group) {
  if (const Array* a = as_array(group)) return a->size();
  if (const std::u16string* s = std::get_if<std::u16string>(&group)) return s->size();
  return 0;
}

}

bool is_fighter_data(const Value& v) { return type_is(v, EntityEnum::Fighter); }

bool is_weapon_data(const Value& v) { return type_is(v, EntityEnum::Weapon); }

bool is_entity_data(const Value& v) { return type_is(v, EntityEnum::Entity); }

bool is_ball_data(const Value& v) { return type_is(v, EntityEnum::Ball); }

bool is_fighter(const Value& v) { return is_fighter_data(field_or(v, u"data")); }

bool is_ball(const Value& v) { return is_ball_data(field_or(v, u"data")); }

bool is_weapon(const Value& v) { return is_weapon_data(field_or(v, u"data")); }

bool is_entity(const Value& v) { return is_entity_data(field_or(v, u"data")); }

bool is_object_data(const Value& v) {
  return is_entity_data(v) || is_fighter_data(v) || is_weapon_data(v) || is_ball_data(v);
}

bool is_object(const Value& v) { return is_object_data(field_or(v, u"data")); }

bool is_bg_data(const Value& v) {
  return strict_equals(field_or(v, u"type"), Value(std::u16string(u"background")));
}

bool is_boss(const Value& v) {
  if (!truthy(v)) return false;
  const Value data = field_or(v, u"data");
  if (!truthy(data)) return false;
  if (!is_object_data(data)) return false;
  const Value group = field_or(field_or(data, u"base"), u"group");
  if (group_length(group) == 0) return false;
  const Array* a = as_array(group);
  if (a == nullptr) return false;
  for (size_t i = 0; i < a->size(); ++i) {
    if (equals(a->at(i), Value(std::u16string(entity_group::kBoss)))) return true;
  }
  return false;
}

bool is_base_ctrl(const Value& v) { return flag_is_true(v, u"__is_base_ctrl__"); }

bool is_bot_ctrl(const Value& v) { return flag_is_true(v, u"__is_bot_ctrl__"); }

bool is_human_ctrl(const Value& v) { return flag_is_true(v, u"__is_human_ctrl__"); }

bool is_ball_ctrl(const Value& v) { return flag_is_true(v, u"__is_ball_ctrl__"); }

}
}
