#pragma once

#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "lfw/base/no_emit_callbacks.h"
#include "lfw/collision/collision.h"
#include "lfw/controller/base_controller.h"
#include "lfw/core/value.h"
#include "lfw/defines/i_terrain_info.h"
#include "lfw/defines/i_vector3.h"
#include "lfw/entity/drink_info.h"
#include "lfw/utils/times.h"

namespace lfw {

namespace buff {
class Buff;
}

namespace state {
class States;
}

// The renderer slot TS keeps in `Entity.renderer`.
struct IEntityRenderer {
  virtual ~IEntityRenderer() = default;
  virtual void render(double dt, double dfactor) = 0;
};

// Host services the entity needs.  In TS these are `this.world` / `this.lfw` reads
// (`world.dataset`, `world.mark_players_alive`, `lfw.new_id`, `lfw.factory`, …); the
// real `World` / `LFW` classes do not exist yet, so the port takes them from a host
// object.  Every method mirrors one TS expression, quoted in the comment.
class Entity;

class IEntityHost {
 public:
  virtual ~IEntityHost() = default;

  // `world.dataset[key]`
  virtual Value world_dataset(const std::u16string& key) const {
    (void)key;
    return Value();
  }
  // `world.bg.data.dataset?.[key]`
  virtual Value bg_dataset(const std::u16string& key) const {
    (void)key;
    return Value();
  }
  // `world.lfw.new_team`
  virtual std::u16string new_team() const { return u"1"; }
  // `world.lfw.new_id`
  virtual std::u16string new_id() { return u""; }
  // `world.lfw.factory.acquire_ctrl(InvalidController, "", this)`
  virtual controller::BaseController* acquire_ctrl() { return nullptr; }
  // `world.lfw.factory.release_ctrl(ctrl)`
  virtual void release_ctrl(controller::BaseController* ctrl) { (void)ctrl; }
  // `world.mark_players_alive(this, alive)`
  virtual void mark_players_alive(bool alive) { (void)alive; }
  // `world.lfw.datas.find(id)`
  virtual Value find_data(const std::u16string& id) const {
    (void)id;
    return Value();
  }
  // `world.entity_map.get(id)` — the TS side writes `?? null`, so a miss is `nullptr`.
  virtual Entity* find_entity(const std::u16string& id) const {
    (void)id;
    return nullptr;
  }
  // `this.enter_frame(nf)`
  virtual void enter_frame(const Value& nf) { (void)nf; }
  // `this.apply_opoints(this._data.base.brokens)`
  virtual void apply_opoints(const Value& opoints) { (void)opoints; }
  // `this.play_sound(this._data.base.dead_sounds)`
  virtual void play_sound(const Value& sounds) { (void)sounds; }
};

// The inline `jumping` record TS keeps on every entity.
struct Jumping {
  double x = 0;
  double y = 0;
  double z = 0;
  double t = 0;
};

// Mirrors `src/LFW/entity/Entity.ts`.  This slice ports the construction / `reset`
// layer and the stat accessor & notification layer; the physics, frame, snapshot and
// collision entry points arrive in later slices.
class Entity {
 public:
  static constexpr const char* TAG = "Entity";
  static constexpr double kMotionlessWaitTicks = 16;

  Entity(IEntityHost& host, Value data);
  Entity(IEntityHost& host, Value data, state::States* states);

  Entity(const Entity&) = delete;
  Entity& operator=(const Entity&) = delete;

  // --- TS fields that are plain data -----------------------------------------
  std::u16string id;
  double wait = 0;
  double variant = 0;
  Value transforms = Value(NullTag{});
  double transform_index = 0;
  Vector3 prev_position;
  Vector3 position;
  Vector3 prev_velocity;
  Vector3 velocity;
  std::vector<std::u16string> emitters;
  std::unique_ptr<DrinkInfo> drink;
  std::vector<Entity*> fuse_bys;
  bool has_fuse_bys = false;
  // TS keeps this as an insertion-ordered `Set`; dedup happens on insert.
  std::vector<std::u16string> copies;
  std::optional<double> dismiss_time;
  Value dismiss_data = Value(NullTag{});
  double stat_bar = 0;
  double fallinjury = 0;
  double throwinjury = 0;
  double facing = 1;
  Value frame;
  Entity* catching = nullptr;
  Entity* catcher = nullptr;
  double aabb_min_x = 0;
  double aabb_max_x = 0;
  double aabb_min_z = 0;
  double aabb_max_z = 0;
  double l_len = 0;
  double r_len = 0;
  Entity* bearer = nullptr;
  Entity* holding = nullptr;
  double motionless = 0;
  double shaking = 0;
  bool bounced = false;
  double lying_a_count = 0;
  double lying_d_count = 0;
  double lying_c_count = 0;
  bool drop_hurted = false;
  bool dropping = false;
  double name_visible = 0;
  double wakeup_invuln = 0;
  double dead_gone = 0;
  Value dead_join = Value(NullTag{});
  double ctrl_visible = 0;
  std::vector<std::pair<Value, double>> opoints;
  std::vector<collision::Collision> collision_list;
  std::vector<collision::Collision> collided_list;
  std::optional<collision::Collision> lastest_collided;
  bool is_on_ground = false;
  std::map<std::u16string, buff::Buff*> buffs;
  std::map<std::u16string, std::u16string> marks;
  IEntityRenderer* renderer = nullptr;
  bool puppet = false;
  Jumping jumping;
  ITerrainInfo terrain;
  Value armor = Value(NullTag{});
  Callbacks callbacks;
  std::map<std::u16string, collision::Collision> vrests;
  std::map<std::u16string, collision::Collision> blockers;
  std::map<std::u16string, collision::Collision> superpunchs;

  // --- lifecycle -------------------------------------------------------------
  void reset(Value data);
  void reset(Value data, state::States* states);

  // --- stat accessors (TS `get x` / `set x`) ---------------------------------
  double lifetime() const { return _lifetime; }
  double spawn_time() const { return _spawn_time; }
  double render_effect_time() const { return _render_effect_time; }
  std::u16string outline_color() const;
  void set_outline_color(std::u16string v);
  double outline_alpha() const { return _outline_alpha; }
  void set_outline_alpha(double v);
  double outline_width() const { return _outline_width; }
  void set_outline_width(double v);
  Value outline_enabled() const;
  void set_outline_enabled(const Value& v);
  const std::u16string& mix_color() const { return _mix_color; }
  void set_mix_color(std::u16string v);
  double mix_strength() const { return _mix_strength; }
  void set_mix_strength(double v);
  double greyscale() const { return _greyscale; }
  void set_greyscale(double v);

  double ground_y() const { return _ground_y; }
  const Value& data() const { return _data; }
  std::u16string origin_data_id() const;
  Value group() const;
  double mounted() const { return _mounted; }
  double ghosted() const { return _ghosted; }
  void set_ghosted(double v) { _ghosted = v; }
  double reserve() const { return _reserve; }
  void set_reserve(double v);
  double type() const;
  Value itr() const;
  Value bdy() const;

  double toughness_resting_max() const { return _toughness_resting_max; }
  void set_toughness_resting_max(double v);
  double resting_max() const;
  void set_resting_max(double v);
  double resting() const { return _resting; }
  void set_resting(double v);
  double fall_value() const { return _fall_value; }
  void set_fall_value(double v);
  double toughness() const { return _toughness; }
  void set_toughness(double v);
  double toughness_max() const { return _toughness_max; }
  void set_toughness_max(double v);
  double toughness_resting() const { return _toughness_resting; }
  void set_toughness_resting(double v);
  double catch_time_max() const;
  void set_catch_time_max(double v);
  double fall_value_max() const;
  void set_fall_value_max(double v);
  double defend_value() const { return _defend_value; }
  void set_defend_value(double v);
  double defend_value_max() const;
  void set_defend_value_max(double v);
  double defend_ratio() const;
  void set_defend_ratio(double v);

  Value name() const;
  void set_name(const Value& v);
  double mp() const { return _mp; }
  void set_mp(double v);
  double hp_r() const { return _hp_r; }
  void set_hp_r(double v);
  double hp() const { return _hp; }
  void set_hp(double v);
  double mp_max() const;
  void set_mp_max(double v);
  double hp_max() const;
  void set_hp_max(double v);
  const std::u16string& team() const { return _team; }
  void set_team(std::u16string v);
  const std::u16string* src_emitter() const;
  const std::u16string* emitter() const;
  double blinking() const { return _blinking; }
  void set_blinking(double v);
  double invisible() const { return _invisible; }
  void set_invisible(double v);
  double invulnerable() const { return _invulnerable; }
  void set_invulnerable(double v);
  Value bot_ignore() const;
  controller::BaseController* ctrl() const { return ctrl_; }
  void set_ctrl(controller::BaseController* v);
  void as_key_role(const Value& v);
  void auto_key_role();
  double gravity() const;
  double itr_motionless() const;
  double arest() const { return _arest; }
  void set_arest(double v);
  double weight() const;
  double base_type() const;
  Value state() const;

  // --- frame lookup / flags ---------------------------------------------------
  // `find_frame_by_id` falls back to `find_auto_frame()` when the id is unknown;
  // `handle_facing_flag` / `handle_wait_flag` / `get_frame_wait` are the pure flag
  // resolvers `handle_next_frame_result` will use.
  Value find_frame_by_id(const Value& id) const;
  Value find_auto_frame() const;
  Value find_align_frame(const std::u16string& frame_id, const Value& src,
                         const Value& dst) const;
  Value get_prev_frame() const { return _prev_frame; }
  Value get_sudden_death_frame() const;
  Value get_caught_end_frame();
  double handle_facing_flag(const Value& facing) const;
  double handle_wait_flag(const Value& wait,
                          const std::optional<Value>& frame = std::nullopt) const;
  double get_frame_wait(const Value& frame) const;

  // --- physics: velocity / friction / gravity --------------------------------
  // `get dvx()` / `get dvy()` / `get dvz()`; a frame without the key yields
  // `undefined`, a falsy one is returned as-is.
  Value dvx() const;
  Value dvy() const;
  Value dvz() const;
  // TS `set_velocity(x?, y?, z?)` — `undefined` / `null` skip the axis.
  void set_velocity(const Value& x, const Value& y, const Value& z);
  void leave_ground();
  void handle_ground_velocity_decay(double factor = 1);
  void handle_velocity_decay(const Value& accx,
                             std::optional<Value> accz = std::nullopt,
                             double factor = 1);
  void handle_gravity();
  void update_velocity(const Value& vinfo);

  // --- stat helpers ----------------------------------------------------------
  void reset_armor();
  Entity& set_catching(Entity* v);
  Entity& add_catch_time(double value);
  Entity& set_catch_time(double value);
  Value dataset(const std::u16string& name) const;
  Value itr_fall(const Value& itr) const;

  // --- per-tick recovery (`update()` calls these) ------------------------------
  // Every one of them is a guarded `clamp_add` on a single stat plus its `Times`
  // gate; `get hp_r()` reads the private `_hp_r` the hp branch clamps against.
  void toughness_recovering();
  void fall_value_recovering();
  void defend_value_recovering();
  void stat_recovering();
  void hp_recovering();
  void mp_recovering();

  // --- marks / emitter helpers -------------------------------------------------
  // `set_mark` / `del_mark` are the `Map<string, string>` pair the buff & collision
  // layers write; `prev` / `value` use JS `==` (so a `null` guard behaves like the
  // missing one) and the stored string is compared loosely too.
  bool set_mark(const std::u16string& key, const std::u16string& value,
                const std::optional<Value>& prev = std::nullopt);
  bool del_mark(const std::u16string& key,
                const std::optional<Value>& value = std::nullopt);
  bool is_ally(const Entity& other) const;
  // `this.emitters[idx]` → `world.entity_map.get(...)`; a fractional / negative /
  // out-of-range index behaves like a hole in the JS array (`undefined`).
  Entity* get_emitter(double idx) const;
  // `get_opoint_speed_z(emitter, opoint)`: `speedz !== void 0` wins (a `null` is
  // passed through), otherwise only fighters in a ball / throwing state get the
  // default z speed.  A missing emitter (`undefined` upstream) is not a fighter.
  Value get_opoint_speed_z(const Entity* emitter, const Value& opoint) const;

  // --- snapshot (`to_snapshot` / `read_snapshot`) -----------------------------
  // TS hands over `number[]` / `string[]`; the port keeps `Value` entries so a slot
  // whose TS type allows `null` stays distinguishable from `NaN` in both directions.
  // Both vectors are indexed by `NSlot` / `SSlot` and sized by the caller.
  void to_snapshot(std::vector<Value>& nums, std::vector<std::u16string>& strs) const;
  void read_snapshot(const std::vector<Value>& nums, const std::vector<std::u16string>& strs);
  // `this.copies.add(id)` — `Set.add` keeps the first insertion order and ignores a
  // duplicate, so the ported vector search-and-appends.  Returns whether it inserted.
  bool add_copy(const std::u16string& copy_id);

  // --- harness / platform peek -------------------------------------------------
  // TS keeps these slots private (`_catch_time`, `_toughness_r_value`, the recovery
  // ticks and their ranges); the differential harness reads them through here, and so
  // will the host once it renders stat bars.  The physics peek adds `atom_time` and
  // the `landing_frame` / `_ground_y` writers, because `set_position` / `set_frame`
  // (which would own those two) arrive with later slices.
  double catch_time() const { return _catch_time; }
  double toughness_r_value() const { return _toughness_r_value; }
  double fall_r_value() const { return _fall_r_value; }
  double defend_r_value() const { return _defend_r_value; }
  double toughness_r_tick_max() const { return _toughness_r_tick.max(); }
  double hp_r_tick_max() const { return _hp_r_tick.max(); }
  double mp_r_tick_max() const { return _mp_r_tick.max(); }
  double fall_r_tick_max() const { return _fall_r_tick.max(); }
  double defend_r_tick_max() const { return _defend_r_tick.max(); }
  double atom_time() const { return _atom_time; }
  void set_atom_time(double v) { _atom_time = v; }
  Value landing_frame() const { return _landing_frame; }
  void set_landing_frame(const Value& v) { _landing_frame = v; }
  void set_ground_y(double v) { _ground_y = v; }
  bool from_wait_block() const { return _from_wait_block; }
  void set_from_wait_block(bool v) { _from_wait_block = v; }
  void set_prev_frame(const Value& v) { _prev_frame = v; }

  // `_state` is a `state::State_Base*` in the full port; the ports so far only reach
  // the optional hooks below, so they are injected until the state wiring slice lands
  // (`_state?.on_dead?.(this)` / `_state?.get_gravity?.(this)` / …).  A missing hook
  // and a state without that optional callback are the same thing in TS.
  std::function<void()> state_on_dead;
  std::function<Value()> state_get_gravity;
  std::function<Value(const Value&)> state_find_frame_by_id;
  std::function<Value()> state_get_auto_frame;
  std::function<Value()> state_get_sudden_death_frame;
  std::function<Value()> state_get_caught_end_frame;

  // The `this` argument every callback receives.  A listener sees the real object in
  // TS; the port hands over a `Value` view, which is what the ported type checks
  // (`is_self_ref` / `is_ally_ref`) compare against.
  Value ref() const;

 private:
  double _lifetime = 0;
  double _spawn_time = 0;
  double _render_effect_time = 0;
  std::u16string _outline_color;
  double _outline_alpha = 0.8;
  double _outline_width = 1;
  std::optional<double> _outline_enabled;
  std::u16string _mix_color;
  double _mix_strength = 0;
  double _greyscale = 0;
  Value _data;
  std::u16string _origin_data_id;
  double _reserve = 0;
  double _mounted = 0;
  double _ghosted = 0;
  Times _hp_r_tick;
  Times _mp_r_tick;
  double _resting = 0;
  std::optional<double> _resting_max;
  Times _resting_tick;
  double _toughness = 0;
  double _toughness_max = 0;
  Times _toughness_r_tick;
  double _toughness_r_value = 0;
  double _toughness_resting = 0;
  double _toughness_resting_max = 0;
  double _fall_value = 0;
  std::optional<double> _fall_value_max;
  Times _fall_r_tick;
  double _fall_r_value = 0;
  double _defend_value = 0;
  std::optional<double> _defend_value_max;
  Times _defend_r_tick;
  double _defend_r_value = 0;
  std::optional<double> _defend_ratio;
  Value _name = Value(NullTag{});
  double _mp = 0;
  std::optional<double> _mp_max;
  double _hp = 0;
  double _hp_r = 0;
  std::optional<double> _hp_max;
  double _arest = 0;
  double _motionless_ticks = 0;
  double _catch_time = 0;
  std::optional<double> _catch_time_max;
  double _invisible = 0;
  double _invulnerable = 0;
  double _blinking = 0;
  Value _landing_frame = Value(NullTag{});
  std::optional<std::u16string> _after_blink;
  double _ground_y = 0;
  double _prev_ground_y = 0;
  double _atom_time = 0;
  bool _from_wait_block = false;
  Value _prev_frame;
  Value _next_frame_by_id;
  Value _prev_cpoint_a = Value(NullTag{});
  std::u16string _team;
  state::States* states_ = nullptr;
  controller::BaseController* ctrl_ = nullptr;
  IEntityHost* host_ = nullptr;

  Value ctrl_ref(controller::BaseController* ctrl) const;
};

}
