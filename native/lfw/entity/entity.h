#pragma once

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
#include "lfw/entity/enter_frame_result.h"
#include "lfw/utils/math/mersenne_twister.h"
#include "lfw/utils/times.h"

namespace lfw {

namespace buff {
class Buff;
}

namespace state {
class States;
class State_Base;
}

class EntityStateView;

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
  // `this.apply_opoints(this._data.base.brokens)`
  virtual void apply_opoints(const Value& opoints) { (void)opoints; }
  // `this.play_sound(this._data.base.dead_sounds)` 等：真实实现（Message 状态的相机
  // 钳制、`lfw.sounds.play`）属于音频/相机切片，宿主只拿到 `(sounds, pos)`。
  virtual void play_sound(const Value& sounds, const Value& pos) {
    (void)sounds;
    (void)pos;
  }
  // `world.restrict(this)` — 返回（可能被夹紧的）坐标点；`terrain` 赋值与越界时
  // `enter_frame(NEXT_FRAME_GONE)` 这些副作用都在 `World` 里，留在宿主侧。
  // （默认实现写在 `entity.cpp`：这里 `Entity` 还是不完整类型。）
  virtual Vector3 world_restrict(Entity& e);
  // `lfw.mt` — 共享的梅森旋转（`mt.pick` / `mt.range`）。
  virtual MersenneTwister& mt() {
    static MersenneTwister fallback(0.0);
    return fallback;
  }
  // `for (const m of v.broadcasts) this.lfw.broadcast(m)`
  virtual void broadcast(const Value& m) { (void)m; }
  // `lfw.factory.create_ctrl(data.id, this.ctrl.player_id, this)`（`transform`）。
  virtual controller::BaseController* create_ctrl(const std::u16string& data_id,
                                                 const std::u16string& player_id) {
    (void)data_id;
    (void)player_id;
    return nullptr;
  }
  // `nf.__judger`：是否存在判定器。TS 的数组分支只**探测**判定器、把运行留给递归调用，
  // 所以「有没有」和「判什么」是两个问题。
  virtual bool has_next_frame_judge(const Value& nf) const {
    (void)nf;
    return false;
  }
  // `nf.__judger?.run(this)`：TS 的 `preprocess_next_frame` 会给带 `expression` 的下帧
  // 挂一个编译好的 `Expression`；端口还没搬那一层（DESIGN §52.4），所以判定结果由宿主
  // 回答 —— 只有 `has_next_frame_judge` 认可的条目才会被问，返回假值即判定失败。
  virtual Value next_frame_judge(const Value& nf) {
    (void)nf;
    return Value();
  }
};

// The inline `jumping` record TS keeps on every entity.
struct Jumping {
  double x = 0;
  double y = 0;
  double z = 0;
  double t = 0;
};

// Mirrors `src/LFW/entity/Entity.ts`.  Ported so far: construction / `reset`, the stat
// accessor & notification layer, the velocity / friction / gravity layer, the frame
// lookup & flag handling, the snapshot pair, the per-tick recovery layer, the
// marks / emitter / opoint helpers and the state wiring (`set_state` + `_state`).
// The collision entry points, the enter-frame chain, `update()` and everything that
// needs a real `World` arrive in later slices.
class Entity {
 public:
  static constexpr const char* TAG = "Entity";
  static constexpr double kMotionlessWaitTicks = 16;

  Entity(IEntityHost& host, Value data);
  Entity(IEntityHost& host, Value data, state::States* states);
  // Out of line because `state_view_` holds an incomplete type here.
  ~Entity();

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
  // `set_state(state_code)`: look the code up in the registry, fall back to the
  // per-type `States.fallback` entry, then leave the old state and enter the new one.
  void set_state(const Value& state_code);
  // `this._state` — `null` until `set_state` runs (and again after every `reset`).
  state::State_Base* state_ptr() const { return _state; }

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
  // `set_position(x?, y?, z?)`: the three axes, then `world.restrict`, the four
  // `on_*_restrict` frame requests plus the `_state.on_restrict` hook (all of them
  // are host/frame requests here), and finally `_ground_y` through `Ground::y`.
  void set_position(const Value& x, const Value& y, const Value& z);
  // `update_position()`: blockers zero the moving axis, then the average-velocity
  // step writes through `set_position`.
  void update_position();

  // --- enter-frame chain -------------------------------------------------------
  // `set_frame` / `enter_frame` / `get_next_frame` were the last blocked cluster of
  // the Entity block: `__judger` is the host seams `has_next_frame_judge` /
  // `next_frame_judge`, `lfw.mt` the `mt()` seam, and `world.restrict` the
  // `world_restrict` seam.
  void set_frame(const Value& v);
  EnterFrameResult enter_frame(const Value& nf, bool fallback = false);
  EnterFrameResult enter_frame_by_id(const Value& id, bool fallback = false);
  EnterFrameResult handle_next_frame_result(const Value& result, bool fallback = false);
  Value get_next_frame(const Value& which);
  // `holding?.follow_bearer()` / `catching?.follow_catcher()` — the relation half of
  // `set_frame`; `drop_holding` / `pick` are the two ends of the same relation.
  void follow_bearer();
  void follow_catcher();
  void drop_holding();
  void pick(Entity& weapon);
  // `transform(data)` / `transfrom_to_another(data?)` (TS keeps the typo).
  void transform(const Value& data);
  bool transfrom_to_another(const std::optional<Value>& data = std::nullopt);

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

  // --- v_rest / relation cleanup / blink arming --------------------------------
  // TS keeps three `Map<string, Collision>` — `vrests` plus the `itr.kind`-driven
  // `blockers` / `superpunchs` mirrors.  The port stores collisions **by value**
  // while TS stores one shared object in up to three maps; see DESIGN §51.2.
  void add_v_rest(const collision::Collision& c);
  double get_v_rest(const std::u16string& a_id) const;
  void del_v_rest(const std::u16string& a_id);
  // `get_flag(other)`: `Ally` / `Enemy` by team, `| Dead` when the hp is gone, then
  // `| this.type` — a JS bitwise or, so the type goes through `js_to_int32`.
  double get_flag(const Entity& other) const;
  void clean_holding();
  void clean_catching();
  bool drop_catching();
  // `blink_and_gone` / `blink_and_respawn` write `_blinking` **directly** (no
  // `round_float` / `max(0, …)`; those live in the `blinking` setter) and arm
  // `_after_blink` with the private `FrameId` literal.
  void blink_and_gone(double duration);
  void blink_and_respawn(double duration);
  // `update_itr_bdy_hit_ground(itrs)`: asks the host to enter `itr.on_hit_ground`
  // for every itr whose box has reached the ground line.
  void update_itr_bdy_hit_ground(const Value& itrs);

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
  // `_after_blink` is only ever read by `update()` (unported) and written by the
  // `blink_and_*` pair, so the scene reads it back through here.
  const std::optional<std::u16string>& after_blink() const { return _after_blink; }
  // `_motionless_ticks` — `set_frame` zeroes it; the decay lives in `update()`.
  double motionless_ticks() const { return _motionless_ticks; }
  void set_motionless_ticks(double v) { _motionless_ticks = v; }
  // `this._next_frame_by_id` — the record `enter_frame_by_id` reuses.
  const Value& next_frame_by_id() const { return _next_frame_by_id; }

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
  // `this._state?.on_dead?.(this)` / `this._state?.get_gravity?.(this)` / … — the hooks
  // live on the active state object and receive `state_view_`, the adapter that makes
  // them see "the entity".  A missing hook and a state without that optional callback
  // are the same thing in TS.
  state::State_Base* _state = nullptr;
  std::unique_ptr<EntityStateView> state_view_;
  controller::BaseController* ctrl_ = nullptr;
  IEntityHost* host_ = nullptr;

  Value ctrl_ref(controller::BaseController* ctrl) const;
};

}
