#include "lfw/world_dataset.h"

#include <algorithm>
#include <memory>
#include <utility>

#include "lfw/defines/fields_gen.h"

namespace lfw {

WorldDataset& WorldDataset::default_instance() {
  static WorldDataset instance;
  return instance;
}

WorldDataset::WorldDataset(bool pure) : _pure(pure) {
  install_defaults();
  if (pure) return;
  const Value& table = world_dataset_fields();
  const Object* fields = as_object(table);
  if (fields != nullptr) {
    for (const std::u16string& key : _keys) {
      if (fields->has(key)) _tracked.insert(key);
    }
  }
  // The accessor install runs first, so the `Object.assign(this, wdataset)` record
  // stays a plain own property (and is never tracked).
  _keys.emplace_back(u"__is_world_dataset__");
  _values.emplace(u"__is_world_dataset__", Value(true));
}

void WorldDataset::add(const char16_t* key, double value) {
  _keys.emplace_back(key);
  _values.emplace(std::u16string(key), Value(value));
}

void WorldDataset::install_defaults() {
  add(u"itr_fall", 40);
  add(u"itr_shaking", 8);
  add(u"itr_motionless", 8);
  add(u"ball_itr_motionless", 8);
  add(u"fvx_f", 0.5);
  add(u"fvy_f", -0.5);
  add(u"fvz_f", 1);
  add(u"ivx_f", 0.5);
  add(u"ivy_f", -0.54);
  add(u"ivz_f", 1);
  add(u"ivy_d", -7);
  add(u"ivx_d", 0);
  add(u"cvy_d", 3);
  add(u"cvx_d", 2);
  add(u"tvx_f", 0.5);
  add(u"tvy_f", -0.625);
  add(u"tvz_f", 0.5);
  add(u"begin_blink_time", 144);
  add(u"lying_blink_time", 32);
  add(u"gone_blink_time", 56);
  add(u"vrest_offset", -6);
  add(u"itr_arest", 20);
  add(u"min_arest", 2);
  add(u"min_vrest", 2);
  add(u"arest_offset", 0);
  add(u"wait_offset", 0);
  add(u"cha_bc_spd", 2);
  add(u"cha_bc_tst_spd_x", 5);
  add(u"cha_bc_tst_spd_y", -2.6);
  add(u"hp_recoverability", 0.66);
  add(u"hp_r_ticks", 24);
  add(u"hp_r_value", 1);
  add(u"hp_healing_ticks", 16);
  add(u"hp_healing_value", 8);
  add(u"mp_healing_ticks", 16);
  add(u"mp_healing_value", 8);
  add(u"electrify_duration", 200);
  add(u"mp_r_ticks", 12);
  add(u"mp_r_ratio", 1);
  add(u"double_click_interval", 30);
  add(u"key_hit_duration", 10);
  add(u"friction_factor", 1);
  add(u"friction_x", 0.25);
  add(u"friction_z", 0.25);
  add(u"land_friction_factor", 1);
  add(u"land_friction_x", 1);
  add(u"land_friction_z", 0.5);
  add(u"screen_w", 794);
  add(u"screen_h", 450);
  add(u"gravity", 0.4375);
  add(u"gravity_d", 0.4375);
  add(u"weapon_throwing_gravity", 0.21875);
  add(u"sync_render", 3);
  add(u"difficulty", 3);
  add(u"infinity_mp", 0);
  add(u"fall_r_ticks", 1);
  add(u"fall_r_value", 1);
  add(u"defend_r_ticks", 1);
  add(u"defend_r_value", 1);
  add(u"fall_value_max", 140);
  add(u"catch_time_max", 680);
  add(u"defend_value_max", 90);
  add(u"defend_ratio", 0.1);
  add(u"mp_max", 500);
  add(u"hp_max", 500);
  add(u"resting_max", 40);
  add(u"vrest_after_shaking", 1);
  add(u"arest_after_motionless", 1);
  add(u"invisible_blinking", 120);
  add(u"jump_x_f", 0.5);
  add(u"jump_z_f", 1);
  add(u"jump_h_f", -0.5);
  add(u"dash_x_f", 0.5);
  add(u"dash_z_f", 1);
  add(u"dash_h_f", -0.5);
  add(u"bfall_x_f", 0.5);
  add(u"bfall_h_f", -0.5);
  add(u"jump_height", -16.299999);
  add(u"jump_distance", 8);
  add(u"jump_distancez", 3);
  add(u"dash_height", -11);
  add(u"dash_distance", 15);
  add(u"dash_distancez", 3.750000);
  add(u"rowing_height", -2.000000);
  add(u"rowing_distance", 5);
  add(u"wvx_f", 0.5);
  add(u"wvy_f", -0.5);
  add(u"wvz_f", 1);
  add(u"whirlwind_vy_max", 4);
  add(u"whirlwind_acc_y", 1);
  add(u"whirlwind_acc_x", 0.5);
  add(u"whirlwind_acc_z", 0.5);
  add(u"outline_enabled", 1);
  add(u"entity_flags", 0);
  add(u"bg_flags", 0);
  add(u"UPS", 60);
  add(u"playrate", 1);
  add(u"atom_time", 1);
  add(u"toughness_r_tick", 1);
  add(u"toughness_r_value", 1);
  add(u"GIM_INK", 0);
  add(u"HERO_FT", 0);
  add(u"LF2_NET", 0);
}

bool WorldDataset::has(const std::u16string& key) const {
  return _values.find(key) != _values.end();
}

bool WorldDataset::tracked(const std::u16string& key) const {
  return !_pure && _tracked.find(key) != _tracked.end();
}

std::vector<std::u16string> WorldDataset::enumerable_keys() const {
  std::vector<std::u16string> out;
  out.reserve(_keys.size());
  for (const std::u16string& key : _keys) {
    if (tracked(key)) continue;
    out.push_back(key);
  }
  return out;
}

Value WorldDataset::get(const std::u16string& key) const {
  const auto it = _values.find(key);
  return it == _values.end() ? Value() : it->second;
}

void WorldDataset::set(const std::u16string& key, Value v) {
  auto it = _values.find(key);
  if (it == _values.end()) {
    // A brand new property never goes through an accessor.
    _keys.push_back(key);
    _values.emplace(key, std::move(v));
    return;
  }
  if (strict_equals(it->second, v)) return;
  const Value prev = it->second;
  it->second = std::move(v);
  if (!tracked(key)) return;
  const auto hook = _hooks.find(key);
  if (hook != _hooks.end() && hook->second) hook->second(it->second, prev);
  if (on_dataset_change) on_dataset_change(key, it->second, prev);
}

void WorldDataset::set_field_hook(const std::u16string& key, FieldHook hook) {
  _hooks[key] = std::move(hook);
}

Value WorldDataset::dump_dataset() const {
  std::vector<std::u16string> keys;
  const Value& table = world_dataset_fields();
  const Object* fields = as_object(table);
  if (fields != nullptr) keys = fields->keys();
  std::sort(keys.begin(), keys.end());
  Object ret;
  for (const std::u16string& key : keys) {
    ret.set(key, get(key));
  }
  return Value(std::make_shared<Object>(std::move(ret)));
}

}
