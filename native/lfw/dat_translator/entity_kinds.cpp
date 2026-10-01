#include "lfw/dat_translator/entity_kinds.h"

#include <cmath>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "lfw/core/value.h"
#include "lfw/dat_translator/helpers.h"
#include "lfw/dat_translator/itr_prefabs.h"
#include "lfw/dat_translator/next_frame.h"
#include "lfw/dat_translator/value_builder.h"
#include "lfw/defines/defines_data.h"
#include "lfw/defines/entity_enum.h"
#include "lfw/defines/frame_behavior.h"
#include "lfw/defines/labels.h"
#include "lfw/defines/speed_mode.h"
#include "lfw/defines/weapon_type.h"
#include "lfw/utils/container_help/traversal.h"
#include "lfw/utils/math/round_float.h"
#include "lfw/utils/type_cast.h"
#include "lfw/utils/type_check.h"

namespace lfw {
namespace dat_translator {
namespace {

bool is_nullish(const Value& v) {
  return std::holds_alternative<std::monostate>(v) || std::holds_alternative<NullTag>(v);
}

bool num_truthy(double n) { return !std::isnan(n) && n != 0; }

std::u16string field_key(const Object* o, const char16_t* key) {
  const Value* v = o != nullptr ? o->get(std::u16string(key)) : nullptr;
  return v != nullptr ? to_string(*v) : std::u16string(u"undefined");
}

std::u16string replace_non_word(const std::u16string& s) {
  std::u16string out;
  for (char16_t c : s) {
    const bool word = (c >= u'a' && c <= u'z') || (c >= u'A' && c <= u'Z') ||
                      (c >= u'0' && c <= u'9') || c == u'_' || c == u'|';
    out.push_back(word ? c : u'-');
  }
  return out;
}

std::u16string strip_obj_suffix(const std::u16string& s) {
  const std::u16string suffix = u"-obj-json5";
  if (s.size() >= suffix.size() &&
      s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0) {
    return s.substr(0, s.size() - suffix.size());
  }
  return s;
}

std::u16string last_segment(const std::u16string& s) {
  const size_t pos = s.rfind(u'/');
  return pos == std::u16string::npos ? s : s.substr(pos + 1);
}

std::u16string kind_name(const Value& index) {
  const Object* idx = as_object(index);
  const Value* hash = idx != nullptr ? idx->get(u"hash") : nullptr;
  if (hash != nullptr && !is_nullish(*hash)) return to_string(*hash);
  const Value* file = idx != nullptr ? idx->get(u"file") : nullptr;
  const std::u16string f = file != nullptr ? to_string(*file) : std::u16string();
  return strip_obj_suffix(replace_non_word(last_segment(f)));
}

std::u16string replace_back_slash(const std::u16string& s) {
  std::u16string out;
  for (char16_t c : s) out.push_back(c == u'\\' ? u'/' : c);
  return out;
}

void cook_kind_frames(Value frames, double acc_z_divisor) {
  traversal(frames, [acc_z_divisor](const std::u16string&, Value& item) {
    Object* f = as_object(item);
    if (f == nullptr) return;
    const Value hit_j = take(*f, u"hit_j");
    if (!strict_equals(hit_j, n(0))) {
      f->set(u"vzm", en(SpeedMode::Extra));
      f->set(u"acc_z", n(round_float((to_num(hit_j, 50) - 50) / acc_z_divisor)));
    }
    const Value hit_a = take(*f, u"hit_a");
    if (truthy(hit_a)) f->set(u"hp", n(round_float(to_number(hit_a) / 2, 10)));
    const Value hit_d = take(*f, u"hit_d");
    const Value* fid = f->get(u"id");
    if (truthy(hit_d) && (fid == nullptr || !strict_equals(hit_d, *fid))) {
      f->set(u"on_dead", get_next_frame_by_raw_id(hit_d, u"frame", u"", nullptr));
    }
    const Value hit_Fa = take(*f, u"hit_Fa");
    if (truthy(hit_Fa)) {
      f->set(u"behavior", hit_Fa);
      f->set(u"behavior_name",
             Value(std::u16string(u"FrameBehavior.") +
                   to_string(defines::js_enum_get(u"FrameBehavior", hit_Fa))));
    }
  });
}

Value weapon_indexes() {
  static const Value table = make_obj({
      {u"0", make_obj({{u"on_ground", s(u"")},
                       {u"just_on_ground", s(u"")},
                       {u"throw_on_ground", s(u"")}})},
      {u"1", make_obj({{u"on_ground", s(u"60")},
                       {u"just_on_ground", s(u"70")},
                       {u"throw_on_ground", s(u"71")}})},
      {u"2", make_obj({{u"on_ground", s(u"20")},
                       {u"just_on_ground", s(u"21")},
                       {u"throw_on_ground", s(u"71")}})},
      {u"3", make_obj({{u"on_ground", s(u"60")},
                       {u"just_on_ground", s(u"70")},
                       {u"throw_on_ground", s(u"71")}})},
      {u"4", make_obj({{u"on_ground", s(u"60")},
                       {u"just_on_ground", s(u"70")},
                       {u"throw_on_ground", s(u"71")}})},
      {u"5", make_obj({{u"on_ground", s(u"60")},
                       {u"just_on_ground", s(u"70")},
                       {u"throw_on_ground", s(u"71")}})},
  });
  return table;
}

void sound_field(Object& info, const char16_t* take_key, const char16_t* set_key) {
  const std::optional<std::u16string> sound = take_str(info, std::u16string(take_key));
  if (sound.has_value() && !sound->empty()) {
    info.set(std::u16string(set_key), make_arr({Value(replace_back_slash(*sound) + u".mp3")}));
  }
}

}

Value make_ball_data(Value& ctx) {
  Object* c = as_object(ctx);
  if (c == nullptr) return Value();
  const Value* base = c->get(u"base");
  const Value* frames = c->get(u"frames");
  const Value* index = c->get(u"index");
  Value info = base != nullptr ? *base : Value();
  const Value frames_value = frames != nullptr ? *frames : Value();
  const Value index_value = index != nullptr ? *index : Value();
  Object* info_obj = as_object(info);
  if (info_obj != nullptr) {
    info_obj->set(u"name", Value(kind_name(index_value)));
    info_obj->set(u"hp_max", n(500));
    sound_field(*info_obj, u"weapon_broken_sound", u"dead_sounds");
    sound_field(*info_obj, u"weapon_hit_sound", u"hit_sounds");
  }
  const Object* idx = as_object(index_value);
  const Value* id = idx != nullptr ? idx->get(u"id") : nullptr;
  Object out;
  out.set(u"id", id != nullptr ? *id : Value());
  out.set(u"type", en(EntityEnum::Ball));
  out.set(u"base", info);
  out.set(u"frames", frames_value);
  out.set(u"processed", Value(false));
  cook_kind_frames(frames_value, 1);
  return Value(std::make_shared<Object>(out));
}

Value make_weapon_data(Value& ctx) {
  Object* c = as_object(ctx);
  if (c == nullptr) return Value();
  const Value* base = c->get(u"base");
  const Value* frames = c->get(u"frames");
  const Value* text = c->get(u"text");
  const Value* index = c->get(u"index");
  Value info = base != nullptr ? *base : Value();
  const Value frames_value = frames != nullptr ? *frames : Value();
  const Value index_value = index != nullptr ? *index : Value();
  const Object* idx = as_object(index_value);
  const Value* type = idx != nullptr ? idx->get(u"type") : nullptr;
  const Value* id = idx != nullptr ? idx->get(u"id") : nullptr;
  const std::u16string type_key = type != nullptr ? to_string(*type) : std::u16string(u"undefined");
  const std::u16string id_key = id != nullptr ? to_string(*id) : std::u16string(u"undefined");
  Object* info_obj = as_object(info);
  if (info_obj != nullptr) {
    info_obj->set(u"name", Value(kind_name(index_value)));
    if (type_key == u"1") {
      const bool knife = id_key == u"120" || id_key == u"124";
      info_obj->set(u"type", en(knife ? WeaponEnum::Knife : WeaponEnum::Stick));
    } else if (type_key == u"2") {
      info_obj->set(u"type", en(WeaponEnum::Heavy));
    } else if (type_key == u"4") {
      info_obj->set(u"type", en(WeaponEnum::Baseball));
    } else if (type_key == u"6") {
      info_obj->set(u"type", en(WeaponEnum::Drink));
    }
  }
  const Value itr_prefabs = text != nullptr ? make_itr_prefabs(*text) : Value();
  const Value table = weapon_indexes();
  const Object* t = as_object(table);
  const Value* found = t != nullptr ? t->get(field_key(info_obj, u"type")) : nullptr;
  const Value* fallback = t != nullptr ? t->get(u"0") : nullptr;
  const Value indexes = found != nullptr ? *found : (fallback != nullptr ? *fallback : Value());
  if (info_obj != nullptr) {
    sound_field(*info_obj, u"weapon_broken_sound", u"dead_sounds");
    sound_field(*info_obj, u"weapon_drop_sound", u"drop_sounds");
    sound_field(*info_obj, u"weapon_hit_sound", u"hit_sounds");
  }
  const Value drop_hurt = info_obj != nullptr ? take(*info_obj, u"weapon_drop_hurt") : Value();
  if (truthy(drop_hurt) && num_truthy(to_number(drop_hurt))) {
    info_obj->set(u"drop_hurt", n(to_number(drop_hurt)));
  }
  const Value weapon_hp = info_obj != nullptr ? take(*info_obj, u"weapon_hp") : Value();
  if (truthy(weapon_hp) && num_truthy(to_number(weapon_hp))) {
    info_obj->set(u"hp_max", n(to_number(weapon_hp)));
  }
  Object out;
  out.set(u"id", id != nullptr ? *id : Value());
  out.set(u"on_dead", defines::find(u"Defines.NEXT_FRAME_GONE") != nullptr
                          ? *defines::find(u"Defines.NEXT_FRAME_GONE")
                          : Value());
  out.set(u"type", en(EntityEnum::Weapon));
  out.set(u"base", info);
  out.set(u"itr_prefabs", itr_prefabs);
  out.set(u"frames", frames_value);
  out.set(u"indexes", indexes);
  out.set(u"processed", Value(false));
  cook_kind_frames(frames_value, 2);
  return Value(std::make_shared<Object>(out));
}

}
}
