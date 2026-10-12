#include "lfw/entity/entity_view_base.h"

#include <variant>

#include "lfw/entity/entity.h"
#include "lfw/utils/container_help/field_or.h"

namespace lfw {
namespace {

// `null` / `undefined`（`??` 只在这两种情况下穿透）。
bool is_missing(const Value& v) {
  return std::holds_alternative<std::monostate>(v) || std::holds_alternative<NullTag>(v);
}

// `data.indexes?.[key]`
Value indexes_of(const Value& data, const char16_t* key) {
  return field_or(field_or(data, u"indexes"), key);
}

}  // namespace

// ─────────────────────── `buff::IBuffEntity` 纯虚 ───────────────────────

const std::u16string& EntityViewBase::id() const { return _e.id; }

void EntityViewBase::position(double& x, double& y, double& z) const {
  x = _e.position.x;
  y = _e.position.y;
  z = _e.position.z;
}

void EntityViewBase::set_position(double x, double y, double z) {
  _e.set_position(Value(x), Value(y), Value(z));
}

// 帧几何（`frame_centery` / `frame_height` / `frame_pic_h`）：`Buff.ts` 的读法是
// `const { centery = 0, height = 0, pic } = victim.frame` 与 `pic?.h ?? 0` ⇒ 缺省 0、
// `NaN` 保留（`||` 链在 `Buff::place_effect_center` 里）。
double EntityViewBase::frame_centery() const {
  const Value v = field_or(_e.frame, u"centery");
  return is_missing(v) ? 0.0 : to_number(v);
}

double EntityViewBase::frame_height() const {
  const Value v = field_or(_e.frame, u"height");
  return is_missing(v) ? 0.0 : to_number(v);
}

double EntityViewBase::frame_pic_h() const {
  const Value v = field_or(field_or(_e.frame, u"pic"), u"h");
  return is_missing(v) ? 0.0 : to_number(v);
}

void EntityViewBase::set_frame(const Value& info) { _e.set_frame(info); }

void EntityViewBase::buffs_set(const std::u16string& key, buff::Buff* b) { _e.buffs[key] = b; }

void EntityViewBase::buffs_delete(const std::u16string& key) { _e.buffs.erase(key); }

void EntityViewBase::set_outline_alpha(double v) { _e.set_outline_alpha(v); }

void EntityViewBase::set_outline_width(double v) { _e.set_outline_width(v); }

void EntityViewBase::set_outline_color(const std::u16string& v) { _e.set_outline_color(v); }

void EntityViewBase::enter_frame_by_id(const std::u16string& id) {
  _e.enter_frame_by_id(Value(id));
}

// `this.attach(on)` —— `Entity::attach` 本体已实现（含 `host->add_entities`）。
void EntityViewBase::attach(bool on) { _e.attach(Value(on)); }

// ─────────────────────── `buff::IBuffEntity` 默认虚 ───────────────────────

Value EntityViewBase::data() const { return _e.data(); }

Value EntityViewBase::state() const { return _e.state(); }

Value EntityViewBase::mp() const { return Value(_e.mp()); }

void EntityViewBase::set_mp(const Value& v) { _e.set_mp(to_number(v)); }

Value EntityViewBase::mp_max() const { return Value(_e.mp_max()); }

Value EntityViewBase::dataset(const std::u16string& key) const { return _e.dataset(key); }

Value EntityViewBase::hp() const { return Value(_e.hp()); }

Value EntityViewBase::hp_r() const { return Value(_e.hp_r()); }

void EntityViewBase::set_hp(const Value& v) { _e.set_hp(to_number(v)); }

void EntityViewBase::set_hp_r(const Value& v) { _e.set_hp_r(to_number(v)); }

void EntityViewBase::set_toughness(const Value& v) { _e.set_toughness(to_number(v)); }

Value EntityViewBase::data_type() const { return field_or(_e.data(), u"type"); }

double EntityViewBase::velocity_y() const { return _e.velocity.y; }

void EntityViewBase::set_velocity(const Value& x, const Value& y, const Value& z) {
  _e.set_velocity(x, y, z);
}

Value EntityViewBase::team() const { return Value(_e.team()); }

void EntityViewBase::set_team(const Value& v) { _e.set_team(to_string(v)); }

Value EntityViewBase::data_indexes_in_the_skys() const {
  return indexes_of(_e.data(), u"in_the_skys");
}

Value EntityViewBase::marks_get(const std::u16string& key) const {
  const auto it = _e.marks.find(key);
  return it == _e.marks.end() ? Value() : Value(it->second);
}

void EntityViewBase::marks_set(const std::u16string& key, const std::u16string& value) {
  _e.marks[key] = value;
}

bool EntityViewBase::marks_delete(const std::u16string& key) {
  return _e.marks.erase(key) > 0;
}

}
