#include "lfw/entity/entity_collision_view.h"

#include <memory>
#include <vector>

#include "lfw/controller/base_controller.h"
#include "lfw/defines/i_vector3.h"
#include "lfw/entity/entity.h"
#include "lfw/utils/container_help/field_or.h"
#include "lfw/utils/cross_bounding.h"
#include "lfw/utils/math/base.h"
#include "lfw/utils/math/round_float.h"
#include "lfw/utils/type_cast.h"

namespace lfw {
namespace {

// TS 的 `map.get(k)`：命中给字符串，未命中给 `undefined`（不是空串）。
Value from_str(const std::u16string* p) { return p == nullptr ? Value() : Value(*p); }

// `null` / `undefined`（`??` 只在这两种情况下穿透）。
bool is_missing(const Value& v) {
  return std::holds_alternative<std::monostate>(v) || std::holds_alternative<NullTag>(v);
}

// `data.indexes?.[key]`
Value indexes_of(const Value& data, const char16_t* key) {
  return field_or(field_or(data, u"indexes"), key);
}

// `frame.cpoint?.[key]`
Value cpoint_of(const Value& frame, const char16_t* key) {
  return field_or(field_or(frame, u"cpoint"), key);
}

Value position_value(double x, double y, double z) {
  Object o;
  o.set(u"x", Value(x));
  o.set(u"y", Value(y));
  o.set(u"z", Value(z));
  return Value(std::make_shared<Object>(o));
}

Bounding bounding_of(const collision::Cube& c) {
  Bounding b{};
  b.left = c.left;
  b.right = c.right;
  b.top = c.top;
  b.bottom = c.bottom;
  b.near = c.near;
  b.far = c.far;
  return b;
}

// 窄接口 → `Entity`：视图之外没有别的实现者，故静态向下转换即可（`-fno-rtti`）。
Entity* entity_of_handler(collision::IHandlerEntity* v) {
  if (v == nullptr) return nullptr;
  return &static_cast<EntityHandlerView*>(v)->entity();
}

// `A_SET_PROP` / `V_SET_PROP` 在 TS 里是 `entity[name] = value`（任意属性写入）。端口没有
// 动态字段表，这里给一份**在白名单内**的写入；名单外的名字是 no-op（DESIGN §78 记偏差）。
// 名单按 `src/LFW/entity/collision_action_handlers.ts` 的用法与实体上可写字段取交集。
void set_prop_of(Entity& e, const std::u16string& name, const Value& v) {
  if (name == u"hp") {
    e.set_hp(to_number(v));
  } else if (name == u"hp_r") {
    e.set_hp_r(to_number(v));
  } else if (name == u"hp_max") {
    e.set_hp_max(to_number(v));
  } else if (name == u"mp") {
    e.set_mp(to_number(v));
  } else if (name == u"mp_max") {
    e.set_mp_max(to_number(v));
  } else if (name == u"invisible") {
    e.set_invisible(to_number(v));
  } else if (name == u"invulnerable") {
    e.set_invulnerable(to_number(v));
  } else if (name == u"toughness") {
    e.set_toughness(to_number(v));
  } else if (name == u"fallinjury") {
    e.fallinjury = to_number(v);
  } else if (name == u"throwinjury") {
    e.throwinjury = to_number(v);
  } else if (name == u"motionless") {
    e.motionless = to_number(v);
  } else if (name == u"shaking") {
    e.shaking = to_number(v);
  } else if (name == u"facing") {
    e.facing = to_number(v);
  } else if (name == u"arest") {
    e.set_arest(to_number(v));
  } else if (name == u"dropping") {
    e.dropping = truthy(v);
  } else if (name == u"reserve") {
    e.set_reserve(to_number(v));
  }
}

}

// ───────────────────────── `EntityHandlerView` ─────────────────────────

const std::u16string& EntityHandlerView::id() const { return _e.id; }

Value EntityHandlerView::hp() const { return Value(_e.hp()); }

void EntityHandlerView::set_hp(const Value& v) { _e.set_hp(to_number(v)); }

Value EntityHandlerView::hp_r() const { return Value(_e.hp_r()); }

void EntityHandlerView::set_hp_r(const Value& v) { _e.set_hp_r(to_number(v)); }

void EntityHandlerView::set_toughness(const Value& v) { _e.set_toughness(to_number(v)); }

Value EntityHandlerView::data() const { return _e.data(); }

Value EntityHandlerView::data_indexes_ice() const { return indexes_of(_e.data(), u"ice"); }

Value EntityHandlerView::data_base_hit_sounds() const {
  return field_or(field_or(_e.data(), u"base"), u"hit_sounds");
}

Value EntityHandlerView::dataset(const std::u16string& key) const { return _e.dataset(key); }

bool EntityHandlerView::marks_has(const std::u16string& kind) const {
  return _e.marks.find(kind) != _e.marks.end();
}

bool EntityHandlerView::catching() const { return _e.catching != nullptr; }

void EntityHandlerView::set_catching(collision::IHandlerEntity* v) {
  _e.catching = entity_of_handler(v);
}

Value EntityHandlerView::catch_time_max() const { return Value(_e.catch_time_max()); }

void EntityHandlerView::set_catch_time(const Value& v) { _e.set_catch_time(to_number(v)); }

collision::IHandlerEntity* EntityHandlerView::catcher() const {
  return _e.catcher == nullptr ? nullptr : _host.handler_view(_e.catcher);
}

void EntityHandlerView::set_catcher(collision::IHandlerEntity* v) {
  _e.catcher = entity_of_handler(v);
}

Value EntityHandlerView::resting() const { return Value(_e.resting()); }

void EntityHandlerView::set_resting(const Value& v) { _e.set_resting(to_number(v)); }

Value EntityHandlerView::fall_value() const { return Value(_e.fall_value()); }

void EntityHandlerView::set_fall_value(const Value& v) { _e.set_fall_value(to_number(v)); }

Value EntityHandlerView::fall_value_max() const { return Value(_e.fall_value_max()); }

Value EntityHandlerView::defend_value() const { return Value(_e.defend_value()); }

void EntityHandlerView::set_defend_value(const Value& v) { _e.set_defend_value(to_number(v)); }

Value EntityHandlerView::defend_value_max() const { return Value(_e.defend_value_max()); }

Value EntityHandlerView::itr_fall(const Value& itr) const { return _e.itr_fall(itr); }

Value EntityHandlerView::src_emitter() const { return from_str(_e.src_emitter()); }

Value EntityHandlerView::shaking() const { return Value(_e.shaking); }

void EntityHandlerView::set_shaking(const Value& v) { _e.shaking = to_number(v); }

Value EntityHandlerView::motionless() const { return Value(_e.motionless); }

void EntityHandlerView::set_velocity(const Value& x, const Value& y, const Value& z) {
  _e.set_velocity(x, y, z);
}

void EntityHandlerView::enter_frame(const Value& info) { _e.enter_frame(info); }

void EntityHandlerView::enter_frame_by_id(const Value& id) { _e.enter_frame_by_id(id); }

void EntityHandlerView::play_sound(const Value& sounds) {
  _e.host().play_sound(sounds, position_value(_e.position.x, _e.position.y, _e.position.z));
}

buff::IBuffEntity* EntityHandlerView::buff_entity() { return _host.buff_view(&_e); }

Value EntityHandlerView::facing() const { return Value(_e.facing); }

double EntityHandlerView::velocity_x() const { return _e.velocity.x; }

void EntityHandlerView::spark_point(const collision::Cube& a, const collision::Cube& b, double& x,
                                    double& y, double& z) {
  const Bounding cross = cross_bounding(bounding_of(a), bounding_of(b));
  _host.mt_mark(u"sp_1");
  x = _host.mt_range(cross.left, cross.right);
  y = 2 + round_float((cross.bottom + cross.top) / 2);
  z = max(cross.far, cross.near) + 3;
}

Value EntityHandlerView::data_indexes_fire() const { return indexes_of(_e.data(), u"fire"); }

Value EntityHandlerView::data_indexes_critical_hit() const {
  return indexes_of(_e.data(), u"critical_hit");
}

Value EntityHandlerView::holding_base_type() const {
  if (_e.holding == nullptr) return Value();
  return Value(_e.holding->base_type());
}

void EntityHandlerView::drop_holding() { _e.drop_holding(); }

Value EntityHandlerView::state() const { return _e.state(); }

double EntityHandlerView::position_y() const { return _e.position.y; }

double EntityHandlerView::ground_y() const { return _e.ground_y(); }

Value EntityHandlerView::data_indexes_dizzy() const { return indexes_of(_e.data(), u"dizzy"); }

Value EntityHandlerView::data_indexes_grand_injured() const {
  return indexes_of(_e.data(), u"grand_injured");
}

Value EntityHandlerView::data_indexes_injured() const {
  return indexes_of(_e.data(), u"injured");
}

Value EntityHandlerView::cpoint_backhurtact() const {
  return cpoint_of(_e.frame, u"backhurtact");
}

Value EntityHandlerView::cpoint_fronthurtact() const {
  return cpoint_of(_e.frame, u"fronthurtact");
}

Value EntityHandlerView::defend_ratio() const { return Value(_e.defend_ratio()); }

// ───────────────────────── `EntityWeaponView` ─────────────────────────

const std::u16string& EntityWeaponView::id() const { return _e.id; }

Value EntityWeaponView::hp() const { return Value(_e.hp()); }

void EntityWeaponView::set_hp(const Value& v) { _e.set_hp(to_number(v)); }

Value EntityWeaponView::hp_r() const { return Value(_e.hp_r()); }

void EntityWeaponView::set_hp_r(const Value& v) { _e.set_hp_r(to_number(v)); }

void EntityWeaponView::set_toughness(const Value& v) { _e.set_toughness(to_number(v)); }

Value EntityWeaponView::data() const { return _e.data(); }

Value EntityWeaponView::data_indexes_ice() const { return indexes_of(_e.data(), u"ice"); }

Value EntityWeaponView::data_base_hit_sounds() const {
  return field_or(field_or(_e.data(), u"base"), u"hit_sounds");
}

Value EntityWeaponView::dataset(const std::u16string& key) const { return _e.dataset(key); }

bool EntityWeaponView::marks_has(const std::u16string& kind) const {
  return _e.marks.find(kind) != _e.marks.end();
}

bool EntityWeaponView::catching() const { return _e.catching != nullptr; }

void EntityWeaponView::set_catching(collision::IHandlerEntity* v) {
  _e.catching = entity_of_handler(v);
}

Value EntityWeaponView::catch_time_max() const { return Value(_e.catch_time_max()); }

void EntityWeaponView::set_catch_time(const Value& v) { _e.set_catch_time(to_number(v)); }

collision::IHandlerEntity* EntityWeaponView::catcher() const {
  return _e.catcher == nullptr ? nullptr : _host.handler_view(_e.catcher);
}

void EntityWeaponView::set_catcher(collision::IHandlerEntity* v) {
  _e.catcher = entity_of_handler(v);
}

Value EntityWeaponView::resting() const { return Value(_e.resting()); }

void EntityWeaponView::set_resting(const Value& v) { _e.set_resting(to_number(v)); }

Value EntityWeaponView::fall_value() const { return Value(_e.fall_value()); }

void EntityWeaponView::set_fall_value(const Value& v) { _e.set_fall_value(to_number(v)); }

Value EntityWeaponView::fall_value_max() const { return Value(_e.fall_value_max()); }

Value EntityWeaponView::defend_value() const { return Value(_e.defend_value()); }

void EntityWeaponView::set_defend_value(const Value& v) { _e.set_defend_value(to_number(v)); }

Value EntityWeaponView::defend_value_max() const { return Value(_e.defend_value_max()); }

Value EntityWeaponView::itr_fall(const Value& itr) const { return _e.itr_fall(itr); }

Value EntityWeaponView::src_emitter() const { return from_str(_e.src_emitter()); }

Value EntityWeaponView::shaking() const { return Value(_e.shaking); }

void EntityWeaponView::set_shaking(const Value& v) { _e.shaking = to_number(v); }

Value EntityWeaponView::motionless() const { return Value(_e.motionless); }

void EntityWeaponView::set_velocity(const Value& x, const Value& y, const Value& z) {
  _e.set_velocity(x, y, z);
}

void EntityWeaponView::enter_frame(const Value& info) { _e.enter_frame(info); }

void EntityWeaponView::enter_frame_by_id(const Value& id) { _e.enter_frame_by_id(id); }

void EntityWeaponView::play_sound(const Value& sounds) {
  _e.host().play_sound(sounds, position_value(_e.position.x, _e.position.y, _e.position.z));
}

buff::IBuffEntity* EntityWeaponView::buff_entity() { return _host.buff_view(&_e); }

bool EntityWeaponView::has_bearer() const { return _e.bearer != nullptr; }

void EntityWeaponView::set_dropping(bool v) { _e.dropping = v; }

Value EntityWeaponView::base_type() const { return Value(_e.base_type()); }

Value EntityWeaponView::facing() const { return Value(_e.facing); }

Value EntityWeaponView::team() const { return Value(_e.team()); }

void EntityWeaponView::set_team(const Value& v) { _e.set_team(to_string(v)); }

Value EntityWeaponView::data_id() const { return field_or(_e.data(), u"id"); }

Value EntityWeaponView::data_indexes_throwings() const {
  return indexes_of(_e.data(), u"throwings");
}

Value EntityWeaponView::data_indexes_in_the_skys() const {
  return indexes_of(_e.data(), u"in_the_skys");
}

void EntityWeaponView::leave_ground() { _e.leave_ground(); }

// ───────────────────────── `EntityActionView` ─────────────────────────

const std::u16string& EntityActionView::id() const { return _e.id; }

void EntityActionView::position(double& x, double& y, double& z) const {
  x = _e.position.x;
  y = _e.position.y;
  z = _e.position.z;
}

void EntityActionView::set_position(double x, double y, double z) {
  _e.set_position(Value(x), Value(y), Value(z));
}

// 帧几何（`frame_centery` / `frame_height` / `frame_pic_h`）属于帧信息那一层，还没搬
// ——与 `EntityStateView` 同一处置（buff 的特效摆放拿到 0）。
double EntityActionView::frame_centery() const { return 0; }

double EntityActionView::frame_height() const { return 0; }

double EntityActionView::frame_pic_h() const { return 0; }

void EntityActionView::set_frame(const Value& info) { _e.set_frame(info); }

void EntityActionView::buffs_set(const std::u16string& key, buff::Buff* b) {
  _e.buffs[key] = b;
}

void EntityActionView::buffs_delete(const std::u16string& key) { _e.buffs.erase(key); }

void EntityActionView::set_outline_alpha(double v) { _e.set_outline_alpha(v); }

void EntityActionView::set_outline_width(double v) { _e.set_outline_width(v); }

void EntityActionView::set_outline_color(const std::u16string& v) {
  _e.set_outline_color(v);
}

void EntityActionView::enter_frame_by_id(const std::u16string& id) {
  _e.enter_frame_by_id(Value(id));
}

// `this.attach(on)` 要 `world.add_entities` / 渲染层，属于宿主那一刀（与 `EntityStateView`
// 一样先空着）。
void EntityActionView::attach(bool on) { (void)on; }

Value EntityActionView::data_type() const { return field_or(_e.data(), u"type"); }

Value EntityActionView::data_indexes_in_the_skys() const {
  return indexes_of(_e.data(), u"in_the_skys");
}

Value EntityActionView::marks_get(const std::u16string& key) const {
  const auto it = _e.marks.find(key);
  return it == _e.marks.end() ? Value() : Value(it->second);
}

void EntityActionView::marks_set(const std::u16string& key, const std::u16string& value) {
  _e.marks[key] = value;
}

bool EntityActionView::marks_delete(const std::u16string& key) {
  return _e.marks.erase(key) > 0;
}

Value EntityActionView::data() const { return _e.data(); }

Value EntityActionView::velocity_x() const { return Value(_e.velocity.x); }

void EntityActionView::set_velocity_x(const Value& v) { _e.velocity.x = to_number(v); }

Value EntityActionView::facing() const { return Value(_e.facing); }

void EntityActionView::set_facing(const Value& v) { _e.facing = to_number(v); }

Value EntityActionView::team() const { return Value(_e.team()); }

void EntityActionView::set_team(const Value& v) { _e.set_team(to_string(v)); }

Value EntityActionView::hp() const { return Value(_e.hp()); }

void EntityActionView::set_hp(const Value& v) { _e.set_hp(to_number(v)); }

Value EntityActionView::hp_r() const { return Value(_e.hp_r()); }

void EntityActionView::set_hp_r(const Value& v) { _e.set_hp_r(to_number(v)); }

Value EntityActionView::hp_max() const { return Value(_e.hp_max()); }

Value EntityActionView::mp() const { return Value(_e.mp()); }

void EntityActionView::set_mp(const Value& v) { _e.set_mp(to_number(v)); }

Value EntityActionView::mp_max() const { return Value(_e.mp_max()); }

// TS 的 `is_bot_ctrl(e.ctrl)` 读的是控制器上的 `__is_bot_ctrl__`；端口落在
// `BaseController::is_bot()` 上（`ctrl` 为 `nullptr` ⇒ false，与 TS 的 `undefined` 同）。
bool EntityActionView::is_bot_ctrl() const {
  return _e.ctrl() != nullptr && _e.ctrl()->is_bot();
}

Value EntityActionView::src_emitter() const { return from_str(_e.src_emitter()); }

Value EntityActionView::emitter() const { return from_str(_e.emitter()); }

collision::IActionEntity* EntityActionView::bearer() {
  return _e.bearer == nullptr ? nullptr : _host.action_view(_e.bearer);
}

Value EntityActionView::fuse_bys() const {
  if (_e.fuse_bys.empty()) return Value();
  std::vector<Value> ids;
  ids.reserve(_e.fuse_bys.size());
  for (Entity* e : _e.fuse_bys) ids.push_back(Value(e->id));
  return Value(std::make_shared<Array>(std::move(ids)));
}

void EntityActionView::set_fuse_bys(const Value& v) {
  _e.fuse_bys.clear();
  const Array* arr = as_array(v);
  if (arr == nullptr) {
    _e.has_fuse_bys = false;
    return;
  }
  for (size_t i = 0; i < arr->size(); ++i) {
    Entity* e = _host.entity_by_id(to_string(arr->at(i)));
    if (e != nullptr) _e.fuse_bys.push_back(e);
  }
  _e.has_fuse_bys = !_e.fuse_bys.empty();
}

void EntityActionView::set_dismiss_data(const Value& v) { _e.dismiss_data = v; }

void EntityActionView::set_dismiss_time(const Value& v) {
  if (is_missing(v)) {
    _e.dismiss_time = std::nullopt;
    return;
  }
  _e.dismiss_time = to_number(v);
}

void EntityActionView::set_invisible(double v) { _e.set_invisible(v); }

void EntityActionView::set_motionless(double v) { _e.motionless = v; }

void EntityActionView::set_invulnerable(double v) { _e.set_invulnerable(v); }

void EntityActionView::play_sound(const Value& sounds, const Value& pos) {
  _e.host().play_sound(sounds, pos);
}

void EntityActionView::enter_frame(const Value& info) { _e.enter_frame(info); }

void EntityActionView::transform(const Value& data) { _e.transform(data); }

void EntityActionView::set_prop(const std::u16string& name, const Value& v) {
  set_prop_of(_e, name, v);
}

buff::IBuffEntity* EntityActionView::buff_entity() { return _host.buff_view(&_e); }

void EntityActionView::velocity(double& x, double& y, double& z) const {
  x = _e.velocity.x;
  y = _e.velocity.y;
  z = _e.velocity.z;
}

void EntityActionView::set_velocity(const Value& x, const Value& y, const Value& z) {
  _e.set_velocity(x, y, z);
}

bool EntityActionView::has_bearer() const { return _e.bearer != nullptr; }

Value EntityActionView::base_type() const { return Value(_e.base_type()); }

Value EntityActionView::state() const { return _e.state(); }

Value EntityActionView::data_in_the_skys_first() const {
  const Value list = indexes_of(_e.data(), u"in_the_skys");
  if (const Array* a = as_array(list)) return a->empty() ? Value() : a->at(0);
  return field_or(list, u"0");
}

Value EntityActionView::data_base_hit_sounds() const {
  return field_or(field_or(_e.data(), u"base"), u"hit_sounds");
}

Value EntityActionView::armor() const { return _e.armor; }

Value EntityActionView::toughness() const { return Value(_e.toughness()); }

void EntityActionView::set_toughness(const Value& v) { _e.set_toughness(to_number(v)); }

Value EntityActionView::toughness_max() const { return Value(_e.toughness_max()); }

Value EntityActionView::itr_fall(const Value& itr) const { return _e.itr_fall(itr); }

Value EntityActionView::dataset(const std::u16string& key) const { return _e.dataset(key); }

void EntityActionView::set_motionless(const Value& v) { _e.motionless = to_number(v); }

void EntityActionView::set_shaking(const Value& v) { _e.shaking = to_number(v); }

void EntityActionView::enter_frame_by_id(const Value& id) { _e.enter_frame_by_id(id); }

void EntityActionView::play_sound(const Value& sounds) {
  _e.host().play_sound(sounds, position_value(_e.position.x, _e.position.y, _e.position.z));
}

Value EntityActionView::frame_id() const {
  const Value f = _e.frame;
  return field_or(f, u"id");
}

Value EntityActionView::data_indexes_throwings() const {
  return indexes_of(_e.data(), u"throwings");
}

Value EntityActionView::arest() const { return Value(_e.arest()); }

void EntityActionView::set_arest(const Value& v) { _e.set_arest(to_number(v)); }

void EntityActionView::set_dropping(bool v) { _e.dropping = v; }

Value EntityActionView::group() const { return _e.group(); }

Value EntityActionView::frame() const { return _e.frame; }

double EntityActionView::position_x() const { return _e.position.x; }

double EntityActionView::position_y() const { return _e.position.y; }

double EntityActionView::position_z() const { return _e.position.z; }

bool EntityActionView::spawn(const Value& opoint, const Value& face) {
  Vector3 zero{};
  return _e.spawn(opoint, zero, to_number(face)) != nullptr;
}

}
