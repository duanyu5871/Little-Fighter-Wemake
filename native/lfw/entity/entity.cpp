#include "lfw/entity/entity.h"

#include <memory>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "lfw/core/value.h"
#include "lfw/defines/defines_data.h"
#include "lfw/defines/entity_enum.h"
#include "lfw/defines/entity_group.h"
#include "lfw/defines/facing_flag.h"
#include "lfw/defines/frame_id.h"
#include "lfw/defines/game_key.h"
#include "lfw/defines/speed_ctrl.h"
#include "lfw/defines/speed_mode.h"
#include "lfw/defines/state_enum.h"
#include "lfw/entity/calc_v.h"
#include "lfw/entity/entity_type_check.h"
#include "lfw/entity/face_helper.h"
#include "lfw/entity/summary_mgr.h"
#include "lfw/ground.h"
#include "lfw/state/entity_states.h"
#include "lfw/utils/container_help/field_or.h"
#include "lfw/utils/math/base.h"
#include "lfw/utils/math/clamp.h"
#include "lfw/utils/math/float_equal.h"
#include "lfw/utils/math/round_float.h"
#include "lfw/utils/type_check.h"

#include <cmath>

namespace lfw {
namespace {

// `Number.MIN_SAFE_INTEGER`, written out because the port has no such constant yet.
constexpr double kMinSafeInteger = -9007199254740991.0;

bool nullish(const Value& v) {
  return std::holds_alternative<std::monostate>(v) || std::holds_alternative<NullTag>(v);
}

// JS `a ?? b`.
Value or_nullish(const Value& v, const Value& fallback) {
  return nullish(v) ? fallback : v;
}

// A stat slot reads as a number; the TS side would hold whatever the dataset holds,
// and the dataset only ever holds numbers.
double num_of(const Value& v) { return nullish(v) ? 0.0 : to_number(v); }

std::optional<double> opt_num(const Value& v) {
  return nullish(v) ? std::nullopt : std::optional<double>(to_number(v));
}

Value base_of(const Value& data) { return field_or(data, u"base"); }

// `obj[key]` with a dynamic key (the `field_or` overloads only take literals).
Value field_kv(const Value& v, const std::u16string& key) {
  const Object* o = as_object(v);
  if (o == nullptr) return Value();
  const Value* p = o->get(key);
  return p != nullptr ? *p : Value();
}

std::u16string frame_id_of(const Entity& e) { return to_string(field_or(e.frame, u"id")); }

Value next_frame_of(const Value& a, const Value& b) { return or_nullish(a, b); }

std::size_t array_length(const Value& v) {
  const Array* a = as_array(v);
  return a != nullptr ? a->size() : 0;
}

}

Entity::Entity(IEntityHost& host, Value data)
    : Entity(host, std::move(data), &state::entity_states()) {}

Entity::Entity(IEntityHost& host, Value data, state::States* states) {
  host_ = &host;
  _data = std::move(data);
  states_ = states;
  _atom_time = num_of(host.world_dataset(u"atom_time"));
  terrain = Ground::horizon();
  {
    Object nf;
    nf.set(u"id", Value(std::u16string()));
    _next_frame_by_id = Value(std::make_shared<Object>(nf));
  }
  reset(_data, states);
}

void Entity::reset(Value data) { reset(std::move(data), &state::entity_states()); }

void Entity::reset(Value data, state::States* states) {
  marks.clear();
  // `for (const buf of this.buffs.values()) buf.del_victim(this)` needs the entity to
  // be a `buff::IBuffEntity`, which lands with the buff wiring slice; the map is still
  // cleared here so a reset never keeps stale entries.
  buffs.clear();
  is_on_ground = false;
  terrain = Ground::horizon();
  _data = std::move(data);
  const Value data_now = _data;
  _origin_data_id = to_string(field_or(data_now, u"id"));
  id = host_->new_id();
  wait = 0;
  _motionless_ticks = 0;
  _lifetime = 0;
  fallinjury = 0;
  _ground_y = 0;
  variant = 0;
  transforms = Value(NullTag{});
  transform_index = 0;
  _reserve = 0;
  _mounted = 0;
  _ghosted = 0;
  prev_position.set(kMinSafeInteger, kMinSafeInteger, kMinSafeInteger);
  position.set(0, 0, 0);
  fuse_bys.clear();
  has_fuse_bys = false;
  dismiss_time = std::nullopt;
  dismiss_data = Value(NullTag{});
  stat_bar = 0;
  _toughness_resting_max = defines::num(u"Defines.DEFAULT_TOUGHNESS_RESTING_MAX");
  _resting_max = opt_num(field_or(base_of(data_now), u"resting_max"));
  _resting = 0;
  _toughness = 0;
  _toughness_max = 0;
  _toughness_resting = 0;
  _fall_value_max = opt_num(field_or(base_of(data_now), u"fall_value_max"));
  _defend_value_max = opt_num(field_or(base_of(data_now), u"defend_value_max"));
  _defend_ratio = opt_num(field_or(base_of(data_now), u"defend_ratio"));
  _catch_time_max = opt_num(field_or(base_of(data_now), u"catch_time_max"));
  throwinjury = 0;
  facing = 1;
  {
    const Value* empty_frame = defines::find(u"EMPTY_FRAME_INFO");
    frame = empty_frame != nullptr ? *empty_frame : Value();
  }
  _prev_frame = frame;
  set_catching(nullptr);
  catcher = nullptr;
  wakeup_invuln = 0;
  name_visible = 0;
  _outline_alpha = 0.8;
  velocity.set(0, 0, 0);
  prev_velocity.set(0, 0, 0);
  callbacks.clear();
  _name = Value(NullTag{});
  _team = host_->new_team();
  _landing_frame = Value(NullTag{});
  bearer = nullptr;
  holding = nullptr;
  emitters.clear();
  _arest = 0;
  vrests.clear();
  blockers.clear();
  superpunchs.clear();
  motionless = 0;
  shaking = 0;
  bounced = false;
  lying_a_count = 0;
  lying_d_count = 0;
  lying_c_count = 0;
  drop_hurted = false;
  dropping = false;
  states_ = states;
  _hp_r_tick.set_max(num_of(dataset(u"hp_r_ticks")));
  _hp_r_tick.set_value(0);
  _mp_r_tick.set_max(num_of(dataset(u"mp_r_ticks")));
  _mp_r_tick.set_value(0);
  _fall_r_tick.set_max(num_of(dataset(u"fall_r_ticks")));
  _fall_r_tick.set_value(0);
  _defend_r_tick.set_max(num_of(dataset(u"defend_r_ticks")));
  _defend_r_tick.set_value(0);
  _toughness_r_value = num_of(dataset(u"toughness_r_value"));
  _defend_r_value = num_of(dataset(u"defend_r_value"));
  _fall_r_value = num_of(dataset(u"fall_r_value"));
  _hp_max = opt_num(dataset(u"hp_max"));
  _mp_max = opt_num(dataset(u"mp_max"));
  _defend_ratio = opt_num(field_or(base_of(data_now), u"defend_ratio"));
  jumping.x = 0;
  jumping.y = 0;
  jumping.z = 0;
  jumping.t = 0;
  if (ctrl_ != nullptr) host_->release_ctrl(ctrl_);
  ctrl_ = host_->acquire_ctrl();
  reset_armor();
  set_fall_value(fall_value_max());
  set_defend_value(defend_value_max());
  _hp_r = hp_max();
  _hp = _hp_r;
  _mp = mp_max();
  set_catch_time(catch_time_max());
  _invisible = 0;
  _invulnerable = 0;
  _blinking = 0;
  _after_blink = std::nullopt;
  state_on_dead = nullptr;
  state_get_gravity = nullptr;
  state_find_frame_by_id = nullptr;
  state_get_auto_frame = nullptr;
  state_get_sudden_death_frame = nullptr;
  state_get_caught_end_frame = nullptr;
  dead_gone = 0;
  dead_join = Value(NullTag{});
  ctrl_visible = 0;
  const Value drink_info = field_or(base_of(data_now), u"drink");
  drink = truthy(drink_info) ? std::make_unique<DrinkInfo>(drink_info) : nullptr;
  opoints.clear();
  _prev_cpoint_a = Value(NullTag{});
  collision_list.clear();
  collided_list.clear();
  lastest_collided = std::nullopt;
  _outline_color.clear();
  _outline_alpha = 0.8;
  _outline_width = 1;
  _outline_enabled = std::nullopt;
  _mix_color.clear();
  _mix_strength = 0;
  _greyscale = 0;
  _render_effect_time = 0;
  auto_key_role();
}

std::u16string Entity::outline_color() const {
  if (!_outline_color.empty()) return _outline_color;
  const Value* map = defines::find(u"Defines.TeamInfoMap");
  const Value info = map != nullptr ? field_or(*map, _team.c_str()) : Value();
  const Value c = field_or(info, u"outline_color");
  return truthy(c) ? to_string(c) : std::u16string();
}

void Entity::set_outline_color(std::u16string v) {
  _outline_color = std::move(v);
  ++_render_effect_time;
}

void Entity::set_outline_alpha(double v) {
  _outline_alpha = v;
  ++_render_effect_time;
}

void Entity::set_outline_width(double v) {
  _outline_width = v;
  ++_render_effect_time;
}

Value Entity::outline_enabled() const {
  return _outline_enabled.has_value() ? Value(*_outline_enabled) : dataset(u"outline_enabled");
}

void Entity::set_outline_enabled(const Value& v) {
  _outline_enabled = opt_num(v);
  ++_render_effect_time;
}

void Entity::set_mix_color(std::u16string v) {
  _mix_color = std::move(v);
  ++_render_effect_time;
}

void Entity::set_mix_strength(double v) {
  _mix_strength = v;
  ++_render_effect_time;
}

void Entity::set_greyscale(double v) {
  _greyscale = v;
  ++_render_effect_time;
}

std::u16string Entity::origin_data_id() const {
  if (!_origin_data_id.empty()) return _origin_data_id;
  return to_string(field_or(_data, u"id"));
}

Value Entity::group() const { return field_or(base_of(_data), u"group"); }

void Entity::set_reserve(double v) {
  v = round_float(v);
  const double o = _reserve;
  if (o == v) return;
  _reserve = v;
  callbacks.call(u"on_reserve_changed", {ref(), Value(v), Value(o)});
}

double Entity::type() const { return num_of(field_or(_data, u"type")); }

Value Entity::itr() const { return field_or(frame, u"itr"); }

Value Entity::bdy() const { return field_or(frame, u"bdy"); }

void Entity::set_toughness_resting_max(double v) {
  v = round_float(v);
  const double o = _toughness_resting_max;
  if (o == v) return;
  _toughness_resting_max = v;
}

double Entity::resting_max() const {
  return _resting_max.has_value() ? *_resting_max : num_of(host_->world_dataset(u"resting_max"));
}

void Entity::set_resting_max(double v) {
  v = round_float(v);
  const double o = resting_max();
  if (o == v) return;
  _resting_max = v;
  callbacks.call(u"on_resting_max_changed", {ref(), Value(v), Value(o)});
}

void Entity::set_resting(double v) {
  v = round_float(v);
  const double o = _resting;
  if (o == v) return;
  _resting = v;
  callbacks.call(u"on_resting_changed", {ref(), Value(v), Value(o)});
}

void Entity::set_fall_value(double v) {
  const double o = _fall_value;
  if (o == v) return;
  _fall_value = round_float(v);
  if (v < o) {
    set_resting(resting_max());
    set_toughness_resting(toughness_resting_max());
  }
  callbacks.call(u"on_fall_value_changed", {ref(), Value(v), Value(o)});
}

void Entity::set_toughness(double v) {
  v = round_float(v);
  if (v < 0) v = 0;
  const double o = _toughness;
  if (o == v) return;
  _toughness = v;
  if (v < o) set_toughness_resting(toughness_resting_max());
  callbacks.call(u"on_toughness_changed", {ref(), Value(v), Value(o)});
}

void Entity::set_toughness_max(double v) {
  v = round_float(v);
  if (v < 0) v = 0;
  const double o = _toughness_max;
  if (o == v) return;
  _toughness_max = v;
  callbacks.call(u"on_toughness_max_changed", {ref(), Value(v), Value(o)});
}

void Entity::set_toughness_resting(double v) {
  v = round_float(v);
  const double o = _toughness_resting;
  if (o == v) return;
  _toughness_resting = v;
}

double Entity::catch_time_max() const {
  return _catch_time_max.has_value() ? *_catch_time_max
                                     : num_of(host_->world_dataset(u"catch_time_max"));
}

void Entity::set_catch_time_max(double v) {
  v = round_float(v);
  const double o = catch_time_max();
  if (o == v) return;
  _catch_time_max = v;
  callbacks.call(u"on_catch_time_max_changed", {ref(), Value(v), Value(o)});
}

double Entity::fall_value_max() const {
  return _fall_value_max.has_value() ? *_fall_value_max
                                     : num_of(host_->world_dataset(u"fall_value_max"));
}

void Entity::set_fall_value_max(double v) {
  v = round_float(v);
  const double o = fall_value_max();
  if (o == v) return;
  _fall_value_max = v;
  callbacks.call(u"on_fall_value_max_changed", {ref(), Value(v), Value(o)});
}

void Entity::set_defend_value(double v) {
  const double o = _defend_value;
  if (o == v) return;
  _defend_value = round_float(v);
  if (v < o) {
    set_resting(resting_max());
    set_toughness_resting(toughness_resting_max());
  }
  callbacks.call(u"on_defend_value_changed", {ref(), Value(v), Value(o)});
}

double Entity::defend_value_max() const {
  return _defend_value_max.has_value() ? *_defend_value_max
                                       : num_of(host_->world_dataset(u"defend_value_max"));
}

void Entity::set_defend_value_max(double v) {
  v = round_float(v);
  const double o = defend_value_max();
  if (o == v) return;
  _defend_value_max = v;
  callbacks.call(u"on_defend_value_max_changed", {ref(), Value(v), Value(o)});
}

double Entity::defend_ratio() const {
  return _defend_ratio.has_value() ? *_defend_ratio
                                   : num_of(host_->world_dataset(u"defend_ratio"));
}

void Entity::set_defend_ratio(double v) {
  v = round_float(v);
  const double o = defend_ratio();
  if (o == v) return;
  _defend_ratio = v;
}

Value Entity::name() const {
  if (!std::holds_alternative<NullTag>(_name)) return _name;
  if (ctrl_ != nullptr && ctrl_->is_human()) {
    const Value pname = field_or(ctrl_->player, u"name");
    if (truthy(pname)) return pname;
    return Value(u"Player " + to_string(field_or(ctrl_->player, u"id")));
  }
  const Value base_name = field_or(base_of(_data), u"name");
  return nullish(base_name) ? Value(std::u16string()) : base_name;
}

void Entity::set_name(const Value& v) {
  if (strict_equals(v, name())) return;
  const Value o = _name;
  _name = v;
  callbacks.call(u"on_name_changed",
                 {ref(), truthy(v) ? v : Value(std::u16string()), o});
}

void Entity::set_mp(double v) {
  const double o = _mp;
  v = max(0.0, v);
  v = round_float(v);
  if (o == v) return;
  _mp = v;
  if (v < o) {
    const std::shared_ptr<Summary> s = summary_mgr().get(id);
    s->set_mp_usage(Value(to_number(s->mp_usage()) + (o - v)));
  }
  if (v < o && !defines::is_independent(_team)) {
    const std::shared_ptr<Summary> s = summary_mgr().get(_team);
    s->set_mp_usage(Value(to_number(s->mp_usage()) + (o - v)));
  }
  callbacks.call(u"on_mp_changed", {ref(), Value(v), Value(o)});
  if (o > 0 && v <= 0) {
    const Value nf = next_frame_of(field_or(frame, u"on_exhaustion"),
                                   field_or(_data, u"on_exhaustion"));
    if (truthy(nf)) host_->enter_frame(nf);
  }
}

void Entity::set_hp_r(double v) {
  const double o = _hp_r;
  v = max(0.0, v);
  v = round_float(v);
  if (o == v) return;
  _hp_r = v;
  callbacks.call(u"on_hp_r_changed", {ref(), Value(_hp_r), Value(o)});
}

void Entity::set_hp(double v) {
  const double o = _hp;
  v = max(0.0, v);
  v = round_float(v);
  if (o == v) return;
  _hp = v;
  if (v < o) {
    const std::shared_ptr<Summary> s = summary_mgr().get(id);
    s->set_hp_lost(Value(to_number(s->hp_lost()) + (o - v)));
  }
  if (v < o && !defines::is_independent(_team)) {
    const std::shared_ptr<Summary> s = summary_mgr().get(_team);
    s->set_hp_lost(Value(to_number(s->hp_lost()) + (o - v)));
  }
  callbacks.call(u"on_hp_changed", {ref(), Value(v), Value(o)});
  if (ctrl_ != nullptr && ctrl_->is_human() && ((o > 0) != (v > 0))) {
    host_->mark_players_alive(v > 0);
  }
  if (o > 0 && v <= 0) {
    callbacks.call(u"on_dead", {ref()});
    if (state_on_dead) state_on_dead();
    const Value brokens = field_or(base_of(_data), u"brokens");
    if (!strict_equals(state(), Value(static_cast<double>(StateEnum::Gone))) &&
        frame_id_of(*this) != std::u16string(frame_id::kGone) &&
        array_length(brokens) > 0) {
      host_->apply_opoints(brokens);
      host_->play_sound(field_or(base_of(_data), u"dead_sounds"));
    }
    const Value nf = next_frame_of(field_or(frame, u"on_dead"), field_or(_data, u"on_dead"));
    if (truthy(nf)) host_->enter_frame(nf);
  }
  if (v > _hp_r) set_hp_r(v);
}

double Entity::mp_max() const {
  return _mp_max.has_value() ? *_mp_max : num_of(host_->world_dataset(u"mp_max"));
}

void Entity::set_mp_max(double v) {
  const double o = mp_max();
  v = max(0.0, v);
  v = round_float(v);
  if (v == o) return;
  _mp_max = v;
  callbacks.call(u"on_mp_max_changed", {ref(), Value(*_mp_max), Value(o)});
}

double Entity::hp_max() const {
  return _hp_max.has_value() ? *_hp_max : num_of(host_->world_dataset(u"hp_max"));
}

void Entity::set_hp_max(double v) {
  const double o = hp_max();
  v = max(0.0, v);
  v = round_float(v);
  if (v == o) return;
  _hp_max = v;
  callbacks.call(u"on_hp_max_changed", {ref(), Value(*_hp_max), Value(o)});
}

void Entity::set_team(std::u16string v) {
  if (equals(Value(v), Value(_team))) return;
  const std::u16string o = _team;
  _team = std::move(v);
  const double n = to_number(Value(_team));
  variant = truthy(Value(n)) ? n : 0;
  callbacks.call(u"on_team_changed", {ref(), Value(_team), Value(o)});
  ++_render_effect_time;
}

const std::u16string* Entity::src_emitter() const {
  return emitters.empty() ? nullptr : &emitters.front();
}

const std::u16string* Entity::emitter() const {
  return emitters.empty() ? nullptr : &emitters.back();
}

void Entity::set_blinking(double v) { _blinking = round_float(max(0.0, v)); }

void Entity::set_invisible(double v) { _invisible = round_float(max(0.0, v)); }

void Entity::set_invulnerable(double v) { _invulnerable = round_float(max(0.0, v)); }

Value Entity::bot_ignore() const {
  return or_nullish(field_or(frame, u"bot_ignore"), field_or(base_of(_data), u"bot_ignore"));
}

void Entity::set_ctrl(controller::BaseController* v) {
  if (v == nullptr) return;
  if (ctrl_ == v) return;
  controller::BaseController* prev = ctrl_;
  ctrl_ = v;
  callbacks.call(u"on_ctrl_changed", {ctrl_ref(v), ctrl_ref(prev), ref()});
  host_->mark_players_alive(ctrl_->is_human() && hp() > 0);
  if (prev != nullptr) host_->release_ctrl(prev);
}

void Entity::as_key_role(const Value& v) {
  name_visible = truthy(v) ? 1 : 0;
  wakeup_invuln = truthy(v) ? 1 : 0;
  dead_gone = truthy(v) ? 0 : 1;
}

void Entity::auto_key_role() {
  bool v = false;
  const Array* group_arr = as_array(group());
  if (group_arr != nullptr) {
    for (std::size_t i = 0; i < group_arr->size(); ++i) {
      const Value& item = group_arr->at(i);
      if (equals(item, Value(std::u16string(entity_group::kRegular))) ||
          equals(item, Value(std::u16string(entity_group::kBoss)))) {
        v = true;
        break;
      }
    }
  }
  as_key_role(Value(v));
}

double Entity::gravity() const {
  const Value g1 = state_get_gravity ? state_get_gravity() : Value();
  const Value g2 = ctrl_ != nullptr && ctrl_->is_end(gk::kDefend) ? dataset(u"gravity")
                                                                  : dataset(u"gravity_d");
  return nullish(g1) ? num_of(g2) : to_number(g1);
}

double Entity::itr_motionless() const {
  if (type() == static_cast<double>(EntityEnum::Ball)) {
    return num_of(dataset(u"ball_itr_motionless"));
  }
  return num_of(dataset(u"itr_motionless"));
}

// --- frame lookup / flags -----------------------------------------------------

// `this._state?.find_frame_by_id?.(this, id)`, then the `FrameId` switch, then the
// `_data.frames` lookup with the `find_auto_frame()` fallback.  The missing-frame
// branch also calls `Ditto.warn(...)`, a console warning with no trace effect.
Value Entity::find_frame_by_id(const Value& id_value) const {
  if (state_find_frame_by_id) {
    const Value r = state_find_frame_by_id(id_value);
    if (truthy(r)) return r;
  }
  // `switch (id)` is strict, so only `undefined` hits `case void 0:` — a `null` id
  // falls through to the `frames[null]` lookup.
  if (std::holds_alternative<std::monostate>(id_value)) return frame;
  const std::u16string* s = std::get_if<std::u16string>(&id_value);
  if (s != nullptr) {
    if (*s == frame_id::kNone || *s == frame_id::kSelf) return frame;
    if (*s == frame_id::kAuto) return find_auto_frame();
    if (*s == frame_id::kGone) {
      const Value* gone = defines::find(u"GONE_FRAME_INFO");
      return gone != nullptr ? *gone : Value();
    }
  }
  const std::u16string key = to_string(id_value);
  const Value found = field_kv(field_or(_data, u"frames"), key);
  if (nullish(found)) return find_auto_frame();
  return found;
}

// `this._state?.get_auto_frame?.(this) ?? this._data.frames["0"] ?? this.frame`
Value Entity::find_auto_frame() const {
  if (state_get_auto_frame) {
    const Value f = state_get_auto_frame();
    if (!nullish(f)) return f;
  }
  const Value f0 = field_or(field_or(_data, u"frames"), u"0");
  if (!nullish(f0)) return f0;
  return frame;
}

// `find_align_frame(frame_id, src, dst)`: the frame after `frame_id` in the `dst`
// list aligned with its position in `src` (`(idx + 1) % len`, so an unknown id lands
// on the first entry), `dst[0]` when only `dst` has entries, else the auto frame.
Value Entity::find_align_frame(const std::u16string& frame_id, const Value& src,
                               const Value& dst) const {
  const Array* d = as_array(dst);
  const Array* s = as_array(src);
  const std::size_t d_len = d != nullptr ? d->size() : 0;
  const std::size_t s_len = s != nullptr ? s->size() : 0;
  if (d_len > 0 && s_len > 0) {
    std::size_t idx = s_len;  // `indexOf` → -1
    for (std::size_t i = 0; i < s_len; ++i) {
      if (strict_equals(s->at(i), Value(frame_id))) {
        idx = i;
        break;
      }
    }
    Object nf;
    nf.set(u"id", d->at((idx + 1) % d_len));
    return Value(std::make_shared<Object>(nf));
  }
  if (d_len > 0) {
    Object nf;
    nf.set(u"id", d->at(0));
    return Value(std::make_shared<Object>(nf));
  }
  return find_auto_frame();
}

// `this._state?.get_sudden_death_frame?.(this) || Defines.NEXT_FRAME_AUTO` — the
// fallback is truthiness-based, so a falsy state answer also falls through.
Value Entity::get_sudden_death_frame() const {
  if (state_get_sudden_death_frame) {
    const Value v = state_get_sudden_death_frame();
    if (truthy(v)) return v;
  }
  const Value* auto_frame = defines::find(u"Defines.NEXT_FRAME_AUTO");
  return auto_frame != nullptr ? *auto_frame : Value();
}

Value Entity::get_caught_end_frame() {
  if (position.y < _ground_y) position.y = _ground_y + 1;
  if (state_get_caught_end_frame) {
    const Value v = state_get_caught_end_frame();
    if (truthy(v)) return v;
  }
  const Value* auto_frame = defines::find(u"Defines.NEXT_FRAME_AUTO");
  return auto_frame != nullptr ? *auto_frame : Value();
}

double Entity::handle_facing_flag(const Value& facing_value) const {
  // `switch (facing)` is a strict comparison, so only exact numbers match a case.
  const double* d = std::get_if<double>(&facing_value);
  if (d == nullptr) return this->facing;
  const double cur = this->facing;
  const int lr = ctrl_ != nullptr ? ctrl_->LR() : 0;
  const Value catcher_facing =
      catcher != nullptr ? Value(catcher->facing) : Value();
  const Value bearer_facing = bearer != nullptr ? Value(bearer->facing) : Value();
  if (*d == static_cast<double>(FacingFlag::Ctrl))
    return lr != 0 ? static_cast<double>(lr) : cur;
  if (*d == static_cast<double>(FacingFlag::AntiCtrl))
    return lr != 0 ? to_number(entity::turn_face(Value(static_cast<double>(lr)))) : cur;
  if (*d == static_cast<double>(FacingFlag::SameAsCatcher))
    return truthy(catcher_facing) ? to_number(catcher_facing) : cur;
  if (*d == static_cast<double>(FacingFlag::OpposingCatcher))
    return truthy(entity::turn_face(catcher_facing))
               ? to_number(entity::turn_face(catcher_facing))
               : cur;
  if (*d == static_cast<double>(FacingFlag::Backward))
    return to_number(entity::turn_face(Value(cur)));
  if (*d == static_cast<double>(FacingFlag::Left) ||
      *d == static_cast<double>(FacingFlag::Right))
    return *d;
  if (*d == static_cast<double>(FacingFlag::VX))
    return velocity.x > 0 ? 1 : velocity.x < 0 ? -1 : cur;
  if (*d == static_cast<double>(FacingFlag::AntiVX))
    return velocity.x > 0 ? -1 : velocity.x < 0 ? 1 : cur;
  if (*d == static_cast<double>(FacingFlag::Trend)) {
    if (lr != 0) return static_cast<double>(lr);
    return velocity.x > 0 ? 1 : velocity.x < 0 ? -1 : cur;
  }
  if (*d == static_cast<double>(FacingFlag::SameAsBearer))
    return truthy(bearer_facing) ? to_number(bearer_facing) : cur;
  if (*d == static_cast<double>(FacingFlag::OpposingBearer))
    return truthy(entity::turn_face(bearer_facing))
               ? to_number(entity::turn_face(bearer_facing))
               : cur;
  return cur;
}

double Entity::handle_wait_flag(const Value& wait_value,
                               const std::optional<Value>& frame_value) const {
  // `frame` is an optional object in TS, so a present-but-falsy frame counts as "no
  // frame" everywhere below.
  const bool has_frame = frame_value.has_value() && truthy(*frame_value);
  if (nullish(wait_value) && has_frame) return get_frame_wait(*frame_value);
  if (is_positive(wait_value)) return to_number(wait_value);
  const std::u16string* s = std::get_if<std::u16string>(&wait_value);
  if ((s != nullptr && *s == u"i") || !has_frame) return this->wait;
  if (s != nullptr && *s == u"d") {
    return max(0.0, to_number(field_or(*frame_value, u"wait")) -
                        to_number(field_or(this->frame, u"wait")) + this->wait);
  }
  return get_frame_wait(*frame_value);
}

double Entity::get_frame_wait(const Value& frame_value) const {
  const double d = to_number(field_or(frame_value, u"wait")) +
                   to_number(host_->world_dataset(u"wait_offset"));
  return _from_wait_block ? d - _atom_time : d;
}

// `get dvx()` / `get dvy()` / `get dvz()`: a falsy frame value is returned untouched
// (`0` / `null` / `undefined`), a truthy one is multiplied by the dataset factor
// (`undefined` dataset → NaN, exactly like the TS `v * undefined`).
Value Entity::dvx() const {
  const Value v = field_or(frame, u"dvx");
  if (!truthy(v)) return v;
  return Value(to_number(v) * to_number(dataset(u"fvx_f")));
}

Value Entity::dvy() const {
  const Value v = field_or(frame, u"dvy");
  if (!truthy(v)) return v;
  return Value(to_number(v) * to_number(dataset(u"fvy_f")));
}

Value Entity::dvz() const {
  const Value v = field_or(frame, u"dvz");
  if (!truthy(v)) return v;
  return Value(to_number(v) * to_number(dataset(u"fvz_f")));
}

void Entity::set_velocity(const Value& x, const Value& y, const Value& z) {
  // TS starts with `if (is_f_num(_x) || is_f_num(_y) || is_f_num(_z)) debugger;` — a
  // dev-only guard with no observable effect.
  if (!nullish(x)) prev_velocity.x = velocity.x = round_float(to_number(x));
  if (!nullish(y)) prev_velocity.y = velocity.y = round_float(to_number(y));
  if (!nullish(z)) prev_velocity.z = velocity.z = round_float(to_number(z));
  if (velocity.y > 0) leave_ground();
}

void Entity::leave_ground() {
  if (eqlt(position.y, _ground_y)) position.y = round_float(_ground_y + 0.1);
  is_on_ground = false;
}

void Entity::handle_ground_velocity_decay(double factor) {
  if (position.y > _ground_y || truthy(Value(shaking)) || truthy(Value(motionless))) return;
  const bool landing = strict_equals(_landing_frame, frame);
  factor *= to_number(dataset(landing ? u"land_friction_factor" : u"friction_factor"));
  const Value fx = dataset(landing ? u"land_friction_x" : u"friction_x");
  const Value fz = dataset(landing ? u"land_friction_z" : u"friction_z");
  // An `undefined` `accz` argument falls back to `accx` (TS default parameter).
  handle_velocity_decay(fx,
                        std::holds_alternative<std::monostate>(fz)
                            ? std::nullopt
                            : std::optional<Value>(fz),
                        factor);
}

void Entity::handle_velocity_decay(const Value& accx, std::optional<Value> accz,
                                   double factor) {
  const double atom_time = to_number(host_->world_dataset(u"atom_time"));
  Value x(round_float(to_number(velocity.x) * std::pow(factor, atom_time)));
  Value z(round_float(to_number(velocity.z) * std::pow(factor, atom_time)));
  const Value accz_value = accz.has_value() ? *accz : accx;
  const double acc_x = round_float(to_number(accx) * atom_time);
  const double acc_z = round_float(to_number(accz_value) * atom_time);

  const Value ctrl_x = field_or(frame, u"ctrl_x");
  const Value ctrl_z = field_or(frame, u"ctrl_z");
  // `let { dvx = 0, dvz = 0 } = this;` — the destructuring default only covers
  // `undefined`, a `null` frame value survives into the comparisons below.
  Value dvx_value = dvx();
  if (std::holds_alternative<std::monostate>(dvx_value)) dvx_value = Value(0.0);
  Value dvz_value = dvz();
  if (std::holds_alternative<std::monostate>(dvz_value)) dvz_value = Value(0.0);
  // `const { UD, LR } = this.ctrl` — TS would throw on a missing controller, so the
  // cases always keep one installed.
  const int lr = ctrl_ != nullptr ? ctrl_->LR() : 0;
  const int ud = ctrl_ != nullptr ? ctrl_->UD() : 0;
  if (truthy(ctrl_x) && lr == 0) dvx_value = Value(0.0);
  if (truthy(ctrl_z) && ud == 0) dvz_value = Value(0.0);

  const double dvx_num = to_number(dvx_value);
  const double dvz_num = to_number(dvz_value);
  if (gt(x, dvx_value)) {
    x = Value(to_number(x) - acc_x);
    if (lt(x, dvx_value)) x = dvx_value;
  } else if (lt(x, Value(-dvx_num))) {
    x = Value(to_number(x) + acc_x);
    if (gt(x, Value(-dvx_num))) x = Value(-dvx_num);
  }
  if (gt(z, dvz_value)) {
    z = Value(to_number(z) - acc_z);
    if (lt(z, dvz_value)) z = dvz_value;
  } else if (lt(z, Value(-dvz_num))) {
    z = Value(to_number(z) + acc_z);
    if (gt(z, Value(-dvz_num))) z = Value(-dvz_num);
  }
  set_velocity(x, Value(NullTag{}), z);
}

void Entity::handle_gravity() {
  if (bearer != nullptr || catcher != nullptr || truthy(Value(shaking)) ||
      truthy(Value(motionless)))
    return;
  // `const { gravity_enabled = true } = this.frame;` — only `undefined` takes the
  // default, `null` reads as falsy.
  const Value gravity_enabled = field_or(frame, u"gravity_enabled");
  const bool enabled = std::holds_alternative<std::monostate>(gravity_enabled)
                           ? true
                           : truthy(gravity_enabled);
  if (position.y <= _ground_y || !enabled) return;
  velocity.y = round_float(velocity.y - gravity() * _atom_time);
}

void Entity::update_velocity(const Value& vinfo) {
  if (bearer != nullptr || catcher != nullptr || truthy(Value(shaking)) ||
      truthy(Value(motionless)))
    return;
  const double atom_time = _atom_time;

  Value dvx = field_or(vinfo, u"dvx");
  Value dvy = field_or(vinfo, u"dvy");
  Value dvz = field_or(vinfo, u"dvz");
  if (truthy(dvx)) dvx = Value(round_float(to_number(dvx) * to_number(dataset(u"fvx_f"))));
  if (truthy(dvy)) dvy = Value(round_float(to_number(dvy) * to_number(dataset(u"fvy_f"))));
  if (truthy(dvz)) dvz = Value(round_float(to_number(dvz) * to_number(dataset(u"fvz_f"))));

  Value vxm = field_or(vinfo, u"vxm");
  if (std::holds_alternative<std::monostate>(vxm))
    vxm = Value(static_cast<double>(SpeedMode::Default));
  Value vym = field_or(vinfo, u"vym");
  if (std::holds_alternative<std::monostate>(vym))
    vym = Value(static_cast<double>(SpeedMode::AccTo));
  Value vzm = field_or(vinfo, u"vzm");
  if (std::holds_alternative<std::monostate>(vzm))
    vzm = Value(static_cast<double>(SpeedMode::Default));
  Value ctrl_x = field_or(vinfo, u"ctrl_x");
  if (std::holds_alternative<std::monostate>(ctrl_x)) ctrl_x = Value(0.0);
  Value ctrl_y = field_or(vinfo, u"ctrl_y");
  if (std::holds_alternative<std::monostate>(ctrl_y)) ctrl_y = Value(0.0);
  Value ctrl_z = field_or(vinfo, u"ctrl_z");
  if (std::holds_alternative<std::monostate>(ctrl_z)) ctrl_z = Value(0.0);

  Value acc_x = field_or(vinfo, u"acc_x");
  Value acc_y = field_or(vinfo, u"acc_y");
  Value acc_z = field_or(vinfo, u"acc_z");
  // `acc_x == void 0` is loose equality: `null` and `undefined` both take the branch.
  if ((equals(vxm, Value(static_cast<double>(SpeedMode::AccTo))) ||
       equals(vxm, Value(static_cast<double>(SpeedMode::FixedAccTo)))) &&
      nullish(acc_x) && truthy(dvx))
    acc_x = dvx;
  if ((equals(vym, Value(static_cast<double>(SpeedMode::AccTo))) ||
       equals(vym, Value(static_cast<double>(SpeedMode::FixedAccTo)))) &&
      nullish(acc_y) && truthy(dvy))
    acc_y = dvy;
  if ((equals(vzm, Value(static_cast<double>(SpeedMode::AccTo))) ||
       equals(vzm, Value(static_cast<double>(SpeedMode::FixedAccTo)))) &&
      nullish(acc_z) && truthy(dvz))
    acc_z = dvz;
  if (truthy(acc_x)) acc_x = Value(round_float(to_number(acc_x) * atom_time));
  if (truthy(acc_y)) acc_y = Value(round_float(to_number(acc_y) * atom_time));
  if (truthy(acc_z)) acc_z = Value(round_float(to_number(acc_z) * atom_time));

  double vx = velocity.x;
  double vy = velocity.y;
  double vz = velocity.z;
  // `const { UD, LR, jd } = this._ctrl` — the cases always keep a controller.
  const int lr = ctrl_ != nullptr ? ctrl_->LR() : 0;
  const int ud = ctrl_ != nullptr ? ctrl_->UD() : 0;
  const int jd = ctrl_ != nullptr ? ctrl_->jd() : 0;
  const Value facing_v = Value(facing);
  const auto apply = [](double current, const Value& value, const Value& mode,
                        const Value& acc, const Value& direction) {
    return entity::calc_v(current, to_number(value), mode, acc, direction);
  };

  if (nullish(dvx)) {
    /* noop */
  } else if (!truthy(ctrl_x)) {
    vx = apply(vx, dvx, vxm, acc_x, facing_v);
  } else if (lr != 0 && equals(ctrl_x, Value(static_cast<double>(SpeedCtrl::Control)))) {
    vx = apply(vx, dvx, vxm, acc_x, Value(static_cast<double>(lr)));
  } else if (lr != 0 && equals(ctrl_x, Value(static_cast<double>(SpeedCtrl::Enable)))) {
    vx = apply(vx, dvx, vxm, acc_x, Value(1.0));
  } else if (lr == 0 && equals(ctrl_x, Value(static_cast<double>(SpeedCtrl::Disable)))) {
    vx = apply(vx, dvx, vxm, acc_x, Value(1.0));
  }

  if (nullish(dvy)) {
    /* noop */
  } else if (!truthy(ctrl_y)) {
    vy = apply(vy, dvy, vym, acc_y, Value(1.0));
  } else if (jd != 0 && equals(ctrl_y, Value(static_cast<double>(SpeedCtrl::Control)))) {
    vy = apply(vy, dvy, vym, acc_y, Value(static_cast<double>(jd)));
  } else if (jd != 0 && equals(ctrl_y, Value(static_cast<double>(SpeedCtrl::Enable)))) {
    vy = apply(vy, dvy, vym, acc_y, Value(1.0));
  } else if (jd == 0 && equals(ctrl_y, Value(static_cast<double>(SpeedCtrl::Disable)))) {
    vy = apply(vy, dvy, vym, acc_y, Value(1.0));
  }

  if (nullish(dvz)) {
    /* noop */
  } else if (!truthy(ctrl_z)) {
    vz = apply(vz, dvz, vzm, acc_z, Value(1.0));
  } else if (ud != 0 && equals(ctrl_z, Value(static_cast<double>(SpeedCtrl::Control)))) {
    vz = apply(vz, dvz, vzm, acc_z, Value(static_cast<double>(ud)));
  } else if (ud != 0 && equals(ctrl_z, Value(static_cast<double>(SpeedCtrl::Enable)))) {
    vz = apply(vz, dvz, vzm, acc_z, Value(1.0));
  } else if (ud == 0 && equals(ctrl_z, Value(static_cast<double>(SpeedCtrl::Disable)))) {
    vz = apply(vz, dvz, vzm, acc_z, Value(1.0));
  }

  velocity.x = round_float(vx);
  velocity.y = round_float(vy);
  velocity.z = round_float(vz);
}

void Entity::set_arest(double v) {
  if (equals(Value(v), Value(_arest))) return;
  _arest = round_float(v);
}

double Entity::weight() const { return num_of(or_nullish(field_or(base_of(_data), u"weight"), Value(1.0))); }

double Entity::base_type() const {
  return num_of(or_nullish(field_or(base_of(_data), u"type"), Value(0.0)));
}

Value Entity::state() const { return field_or(frame, u"state"); }

void Entity::reset_armor() {
  const Value armor_v = field_or(base_of(_data), u"armor");
  armor = truthy(armor_v) ? armor_v : Value(NullTag{});
  const Value toughness = field_or(armor_v, u"toughness");
  set_toughness_max(num_of(toughness));
  set_toughness(num_of(toughness));
  set_toughness_resting(0);
  set_toughness_resting_max(num_of(field_or(armor_v, u"toughness_resting")));
  const Value trv = field_or(armor_v, u"toughness_r_value");
  _toughness_r_value = num_of(or_nullish(trv, dataset(u"toughness_r_value")));
  const Value trt = field_or(armor_v, u"toughness_r_tick");
  _toughness_r_tick.set_max(num_of(or_nullish(trt, dataset(u"toughness_r_tick"))));
  _toughness_r_tick.set_value(0);
}

Entity& Entity::set_catching(Entity* v) {
  if (catching == v) return *this;
  catching = v;
  return *this;
}

Entity& Entity::add_catch_time(double value) {
  if (!truthy(Value(value))) return *this;
  return set_catch_time(_catch_time + value);
}

Entity& Entity::set_catch_time(double value) {
  const double v = round_float(value);
  if (equals(Value(_catch_time), Value(v))) return *this;
  _catch_time = clamp(v, 0.0, catch_time_max());
  return *this;
}

Value Entity::dataset(const std::u16string& name) const {
  Value v = field_or(field_or(frame, u"dataset"), name.c_str());
  if (nullish(v)) v = field_or(base_of(_data), name.c_str());
  if (nullish(v)) v = host_->bg_dataset(name);
  if (nullish(v)) v = host_->world_dataset(name);
  return v;
}

Value Entity::itr_fall(const Value& itr) const {
  const Value fall = field_or(itr, u"fall");
  return nullish(fall) ? dataset(u"itr_fall") : fall;
}

Value Entity::ref() const {
  Object o;
  o.set(u"id", Value(id));
  return Value(std::make_shared<Object>(o));
}

Value Entity::ctrl_ref(controller::BaseController* ctrl) const {
  if (ctrl == nullptr) return Value();
  Object o;
  o.set(u"__is_base_ctrl__", Value(true));
  if (ctrl->is_human()) o.set(u"__is_human_ctrl__", Value(true));
  if (ctrl->is_bot()) o.set(u"__is_bot_ctrl__", Value(true));
  o.set(u"player", ctrl->player);
  o.set(u"player_id", Value(ctrl->player_id));
  return Value(std::make_shared<Object>(o));
}

}
