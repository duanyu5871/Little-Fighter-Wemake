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

// ─────────────────────── `EntityCollisionView`（`IHandlerEntity` 那一面）───────────────────────

const std::u16string& EntityCollisionView::id() const { return _e.id; }

Value EntityCollisionView::hp() const { return Value(_e.hp()); }

void EntityCollisionView::set_hp(const Value& v) { _e.set_hp(to_number(v)); }

Value EntityCollisionView::hp_r() const { return Value(_e.hp_r()); }

void EntityCollisionView::set_hp_r(const Value& v) { _e.set_hp_r(to_number(v)); }

void EntityCollisionView::set_toughness(const Value& v) { _e.set_toughness(to_number(v)); }

Value EntityCollisionView::data() const { return _e.data(); }

Value EntityCollisionView::data_indexes_ice() const { return indexes_of(_e.data(), u"ice"); }

Value EntityCollisionView::data_base_hit_sounds() const {
  return field_or(field_or(_e.data(), u"base"), u"hit_sounds");
}

Value EntityCollisionView::dataset(const std::u16string& key) const { return _e.dataset(key); }

bool EntityCollisionView::marks_has(const std::u16string& kind) const {
  return _e.marks.find(kind) != _e.marks.end();
}

bool EntityCollisionView::catching() const { return _e.catching != nullptr; }

// `IHandlerEntity*` 是虚基类指针 ⇒ 走宿主的反向表（`-fno-rtti` 下没法 static_cast 回本类）。
void EntityCollisionView::set_catching(collision::IHandlerEntity* v) {
  _e.catching = _host.entity_of_handler(v);
}

Value EntityCollisionView::catch_time_max() const { return Value(_e.catch_time_max()); }

void EntityCollisionView::set_catch_time(const Value& v) { _e.set_catch_time(to_number(v)); }

collision::IHandlerEntity* EntityCollisionView::catcher() const {
  return _e.catcher == nullptr ? nullptr : _host.handler_view(_e.catcher);
}

void EntityCollisionView::set_catcher(collision::IHandlerEntity* v) {
  _e.catcher = _host.entity_of_handler(v);
}

Value EntityCollisionView::resting() const { return Value(_e.resting()); }

void EntityCollisionView::set_resting(const Value& v) { _e.set_resting(to_number(v)); }

Value EntityCollisionView::fall_value() const { return Value(_e.fall_value()); }

void EntityCollisionView::set_fall_value(const Value& v) { _e.set_fall_value(to_number(v)); }

Value EntityCollisionView::fall_value_max() const { return Value(_e.fall_value_max()); }

Value EntityCollisionView::defend_value() const { return Value(_e.defend_value()); }

void EntityCollisionView::set_defend_value(const Value& v) { _e.set_defend_value(to_number(v)); }

Value EntityCollisionView::defend_value_max() const { return Value(_e.defend_value_max()); }

Value EntityCollisionView::itr_fall(const Value& itr) const { return _e.itr_fall(itr); }

Value EntityCollisionView::src_emitter() const { return from_str(_e.src_emitter()); }

Value EntityCollisionView::shaking() const { return Value(_e.shaking); }

void EntityCollisionView::set_shaking(const Value& v) { _e.shaking = to_number(v); }

Value EntityCollisionView::motionless() const { return Value(_e.motionless); }

void EntityCollisionView::set_velocity(const Value& x, const Value& y, const Value& z) {
  _e.set_velocity(x, y, z);
}

void EntityCollisionView::enter_frame(const Value& info) { _e.enter_frame(info); }

void EntityCollisionView::enter_frame_by_id(const Value& id) { _e.enter_frame_by_id(id); }

void EntityCollisionView::play_sound(const Value& sounds) {
  _e.host().play_sound(sounds, position_value(_e.position.x, _e.position.y, _e.position.z));
}

buff::IBuffEntity* EntityCollisionView::buff_entity() { return _host.buff_view(&_e); }

// ─────────────────────────── `IFallEntity` ───────────────────────────

Value EntityCollisionView::facing() const { return Value(_e.facing); }

// 同时重写 `IActionEntity::velocity_x()`（`double`）与 `IFallEntity::velocity_x()`（`double`）。
double EntityCollisionView::velocity_x() const { return _e.velocity.x; }

void EntityCollisionView::spark_point(const collision::Cube& a, const collision::Cube& b, double& x,
                                      double& y, double& z) {
  const Bounding cross = cross_bounding(bounding_of(a), bounding_of(b));
  _host.mt_mark(u"sp_1");
  x = _host.mt_range(cross.left, cross.right);
  y = 2 + round_float((cross.bottom + cross.top) / 2);
  z = max(cross.far, cross.near) + 3;
}

Value EntityCollisionView::data_indexes_fire() const { return indexes_of(_e.data(), u"fire"); }

Value EntityCollisionView::data_indexes_critical_hit() const {
  return indexes_of(_e.data(), u"critical_hit");
}

Value EntityCollisionView::holding_base_type() const {
  if (_e.holding == nullptr) return Value();
  return Value(_e.holding->base_type());
}

void EntityCollisionView::drop_holding() { _e.drop_holding(); }

// ─────────────────────── `INbdyNormalEntity` ───────────────────────

Value EntityCollisionView::state() const { return _e.state(); }

double EntityCollisionView::position_y() const { return _e.position.y; }

double EntityCollisionView::ground_y() const { return _e.ground_y(); }

Value EntityCollisionView::data_indexes_dizzy() const { return indexes_of(_e.data(), u"dizzy"); }

Value EntityCollisionView::data_indexes_grand_injured() const {
  return indexes_of(_e.data(), u"grand_injured");
}

Value EntityCollisionView::data_indexes_injured() const {
  return indexes_of(_e.data(), u"injured");
}

Value EntityCollisionView::cpoint_backhurtact() const {
  return cpoint_of(_e.frame, u"backhurtact");
}

Value EntityCollisionView::cpoint_fronthurtact() const {
  return cpoint_of(_e.frame, u"fronthurtact");
}

// ─────────────────────── `INdbdyDefendEntity` ───────────────────────

Value EntityCollisionView::defend_ratio() const { return Value(_e.defend_ratio()); }

// ─────────────────────── `IWeaponIsHitEntity` ───────────────────────

bool EntityCollisionView::has_bearer() const { return _e.bearer != nullptr; }

void EntityCollisionView::set_dropping(bool v) { _e.dropping = v; }

Value EntityCollisionView::base_type() const { return Value(_e.base_type()); }

Value EntityCollisionView::team() const { return Value(_e.team()); }

void EntityCollisionView::set_team(const Value& v) { _e.set_team(to_string(v)); }

Value EntityCollisionView::data_id() const { return field_or(_e.data(), u"id"); }

Value EntityCollisionView::data_indexes_throwings() const {
  return indexes_of(_e.data(), u"throwings");
}

Value EntityCollisionView::data_indexes_in_the_skys() const {
  return indexes_of(_e.data(), u"in_the_skys");
}

void EntityCollisionView::leave_ground() { _e.leave_ground(); }

// ─────────────────────── `buff::IBuffEntity` ───────────────────────

void EntityCollisionView::position(double& x, double& y, double& z) const {
  x = _e.position.x;
  y = _e.position.y;
  z = _e.position.z;
}

void EntityCollisionView::set_position(double x, double y, double z) {
  _e.set_position(Value(x), Value(y), Value(z));
}

// 帧几何（`frame_centery` / `frame_height` / `frame_pic_h`）属于帧信息那一层，还没搬
// ——与 `EntityStateView` 同一处置（buff 的特效摆放拿到 0）。
double EntityCollisionView::frame_centery() const { return 0; }

double EntityCollisionView::frame_height() const { return 0; }

double EntityCollisionView::frame_pic_h() const { return 0; }

void EntityCollisionView::set_frame(const Value& info) { _e.set_frame(info); }

void EntityCollisionView::buffs_set(const std::u16string& key, buff::Buff* b) {
  _e.buffs[key] = b;
}

void EntityCollisionView::buffs_delete(const std::u16string& key) { _e.buffs.erase(key); }

void EntityCollisionView::set_outline_alpha(double v) { _e.set_outline_alpha(v); }

void EntityCollisionView::set_outline_width(double v) { _e.set_outline_width(v); }

void EntityCollisionView::set_outline_color(const std::u16string& v) {
  _e.set_outline_color(v);
}

void EntityCollisionView::enter_frame_by_id(const std::u16string& id) {
  _e.enter_frame_by_id(Value(id));
}

// `this.attach(on)` 要 `world.add_entities` / 渲染层，属于宿主那一刀（与 `EntityStateView`
// 一样先空着）。
void EntityCollisionView::attach(bool on) { (void)on; }

Value EntityCollisionView::data_type() const { return field_or(_e.data(), u"type"); }

Value EntityCollisionView::marks_get(const std::u16string& key) const {
  const auto it = _e.marks.find(key);
  return it == _e.marks.end() ? Value() : Value(it->second);
}

void EntityCollisionView::marks_set(const std::u16string& key, const std::u16string& value) {
  _e.marks[key] = value;
}

bool EntityCollisionView::marks_delete(const std::u16string& key) {
  return _e.marks.erase(key) > 0;
}

// ─────────────────────── `IActionEntity` ───────────────────────

void EntityCollisionView::set_velocity_x(const Value& v) { _e.velocity.x = to_number(v); }

void EntityCollisionView::set_facing(const Value& v) { _e.facing = to_number(v); }

Value EntityCollisionView::hp_max() const { return Value(_e.hp_max()); }

Value EntityCollisionView::mp() const { return Value(_e.mp()); }

void EntityCollisionView::set_mp(const Value& v) { _e.set_mp(to_number(v)); }

Value EntityCollisionView::mp_max() const { return Value(_e.mp_max()); }

// TS 的 `is_bot_ctrl(e.ctrl)` 读的是控制器上的 `__is_bot_ctrl__`；端口落在
// `BaseController::is_bot()` 上（`ctrl` 为 `nullptr` ⇒ false，与 TS 的 `undefined` 同）。
bool EntityCollisionView::is_bot_ctrl() const {
  return _e.ctrl() != nullptr && _e.ctrl()->is_bot();
}

Value EntityCollisionView::emitter() const { return from_str(_e.emitter()); }

collision::IActionEntity* EntityCollisionView::bearer() {
  return _e.bearer == nullptr ? nullptr : _host.action_view(_e.bearer);
}

Value EntityCollisionView::fuse_bys() const {
  if (_e.fuse_bys.empty()) return Value();
  std::vector<Value> ids;
  ids.reserve(_e.fuse_bys.size());
  for (Entity* e : _e.fuse_bys) ids.push_back(Value(e->id));
  return Value(std::make_shared<Array>(std::move(ids)));
}

void EntityCollisionView::set_fuse_bys(const Value& v) {
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

void EntityCollisionView::set_dismiss_data(const Value& v) { _e.dismiss_data = v; }

void EntityCollisionView::set_dismiss_time(const Value& v) {
  if (is_missing(v)) {
    _e.dismiss_time = std::nullopt;
    return;
  }
  _e.dismiss_time = to_number(v);
}

void EntityCollisionView::set_invisible(double v) { _e.set_invisible(v); }

void EntityCollisionView::set_motionless(double v) { _e.motionless = v; }

void EntityCollisionView::set_invulnerable(double v) { _e.set_invulnerable(v); }

void EntityCollisionView::play_sound(const Value& sounds, const Value& pos) {
  _e.host().play_sound(sounds, pos);
}

void EntityCollisionView::transform(const Value& data) { _e.transform(data); }

void EntityCollisionView::set_prop(const std::u16string& name, const Value& v) {
  set_prop_of(_e, name, v);
}

// ─────────────────────── `IH3Entity` ───────────────────────

void EntityCollisionView::velocity(double& x, double& y, double& z) const {
  x = _e.velocity.x;
  y = _e.velocity.y;
  z = _e.velocity.z;
}

Value EntityCollisionView::data_in_the_skys_first() const {
  const Value list = indexes_of(_e.data(), u"in_the_skys");
  if (const Array* a = as_array(list)) return a->empty() ? Value() : a->at(0);
  return field_or(list, u"0");
}

Value EntityCollisionView::armor() const { return _e.armor; }

Value EntityCollisionView::toughness() const { return Value(_e.toughness()); }

Value EntityCollisionView::toughness_max() const { return Value(_e.toughness_max()); }

void EntityCollisionView::set_motionless(const Value& v) { _e.motionless = to_number(v); }

// ─────────────────────── `IH4Entity` ───────────────────────

Value EntityCollisionView::frame_id() const {
  const Value f = _e.frame;
  return field_or(f, u"id");
}

Value EntityCollisionView::arest() const { return Value(_e.arest()); }

void EntityCollisionView::set_arest(const Value& v) { _e.set_arest(to_number(v)); }

// ─────────────────────── `IFrozenEntity` ───────────────────────

Value EntityCollisionView::group() const { return _e.group(); }

Value EntityCollisionView::frame() const { return _e.frame; }

double EntityCollisionView::position_x() const { return _e.position.x; }

double EntityCollisionView::position_z() const { return _e.position.z; }

bool EntityCollisionView::spawn(const Value& opoint, const Value& face) {
  Vector3 zero{};
  return _e.spawn(opoint, zero, to_number(face)) != nullptr;
}

}
