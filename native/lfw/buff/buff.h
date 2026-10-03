#pragma once

#include <functional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "lfw/core/value.h"
#include "lfw/utils/times.h"

namespace lfw {
namespace buff {

class Buff;

class IBuffEntity {
 public:
  virtual ~IBuffEntity() = default;
  virtual const std::u16string& id() const = 0;
  virtual void position(double& x, double& y, double& z) const = 0;
  virtual void set_position(double x, double y, double z) = 0;
  virtual double frame_centery() const = 0;
  virtual double frame_height() const = 0;
  virtual double frame_pic_h() const = 0;
  virtual void set_frame(const Value& info) = 0;
  virtual void buffs_set(const std::u16string& key, Buff* b) = 0;
  virtual void buffs_delete(const std::u16string& key) = 0;
  virtual void set_outline_alpha(double v) = 0;
  virtual void set_outline_width(double v) = 0;
  virtual void set_outline_color(const std::u16string& v) = 0;
  virtual void enter_frame_by_id(const std::u16string& id) = 0;
  virtual void attach(bool on) = 0;
  virtual Value data() const { return Value(); }
  virtual Value state() const { return Value(); }
  virtual Value wait() const { return Value(); }
  virtual void set_wait(const Value& v) { (void)v; }
  virtual Value mp() const { return Value(); }
  virtual void set_mp(const Value& v) { (void)v; }
  virtual Value mp_max() const { return Value(); }
  virtual Value dataset(const std::u16string& key) const {
    (void)key;
    return Value();
  }
  virtual Value hp() const { return Value(); }
  virtual Value hp_r() const { return Value(); }
  virtual void set_hp(const Value& v) { (void)v; }
  virtual Value marks_get(const std::u16string& key) const {
    (void)key;
    return Value();
  }
  virtual void marks_set(const std::u16string& key, const std::u16string& value) {
    (void)key;
    (void)value;
  }
  virtual bool marks_delete(const std::u16string& key) {
    (void)key;
    return false;
  }
  virtual Value data_type() const { return Value(); }
  virtual double velocity_y() const { return 0; }
  virtual void set_velocity(const Value& x, const Value& y, const Value& z) {
    (void)x;
    (void)y;
    (void)z;
  }
  virtual void handle_velocity_decay(double accx) { (void)accx; }
  virtual void set_hp_r(const Value& v) { (void)v; }
  virtual void set_fallinjury(const Value& v) { (void)v; }
  virtual void set_toughness(const Value& v) { (void)v; }
  virtual Value team() const { return Value(); }
  virtual void set_team(const Value& v) { (void)v; }
  virtual Value data_indexes_falling() const { return Value(); }
  virtual Value data_indexes_in_the_skys() const { return Value(); }
};

inline bool set_mark(IBuffEntity& e, const std::u16string& key, const std::u16string& value,
                     const Value& prev) {
  if (std::holds_alternative<std::monostate>(prev) || equals(e.marks_get(key), prev)) {
    e.marks_set(key, value);
    return true;
  }
  return false;
}

inline bool del_mark(IBuffEntity& e, const std::u16string& key, const Value& value) {
  if (std::holds_alternative<std::monostate>(value) || equals(e.marks_get(key), value)) {
    return e.marks_delete(key);
  }
  return false;
}

struct BuffEnv {
  std::function<IBuffEntity*(const std::u16string& id)> find_entity;
  std::function<IBuffEntity*(const Value& data)> create_entity;
  std::function<Value(const std::u16string& oid)> find_data;
  std::function<void(const std::u16string& id, Buff* b)> world_buffs_set;
  std::function<Buff*(const std::u16string& id)> world_buffs_get;
  std::function<Buff*(const std::u16string& kind, const std::u16string& id)> create_buff;
};

class Buff {
 public:
  Buff(const BuffEnv* env, const std::u16string& id, const Value& kind);

  const std::u16string& id() const { return _id; }
  const std::vector<std::u16string>& victims() const { return _victims; }
  bool dead() const { return _lifetime.remains() == 0; }
  double lifetime() const { return _lifetime.value(); }
  void set_lifetime(double v) { _lifetime.set_value(v); }
  double duration() const { return _lifetime.max(); }
  void set_duration(double v) { _lifetime.set_max(v); }
  double ticks() const { return _ticker.max(); }
  void set_ticks(double v) { _ticker.set_max(v); }
  const Value& kind() const { return _kind; }
  double level() const { return _level; }
  void set_level(double v) { _level = v; }
  bool mounted() const { return _mounted; }
  const std::u16string& attacker_id() const { return _attacker_id; }
  size_t effects_size() const { return _effects.size(); }
  IBuffEntity* attacter();

  void set_env(const BuffEnv* env) { _env = env; }
  const BuffEnv* env() const { return _env; }
  virtual void init() {}
  void reset(const std::u16string& id);
  void set_attacker_by_id(const std::u16string& id);
  void set_attacker_entity(IBuffEntity* e);
  void set_victim(IBuffEntity* victim);
  void add_victim(IBuffEntity* victim);
  void del_victim(IBuffEntity* victim);
  void update(double d);
  Value to_snapshot() const;
  void read_snapshot(const Value& s);
  virtual void mount();
  virtual void unmount();

 protected:
  virtual std::u16string effect_oid() const { return std::u16string(); }
  virtual std::u16string effect_frame_id() const { return std::u16string(u"0"); }
  virtual bool has_on_update() const { return false; }
  virtual bool has_on_tick() const { return false; }
  virtual bool has_on_end() const { return false; }
  virtual void on_update(IBuffEntity* attacker, IBuffEntity* victim) {
    (void)attacker;
    (void)victim;
  }
  virtual void on_tick(IBuffEntity* attacker, IBuffEntity* victim) {
    (void)attacker;
    (void)victim;
  }
  virtual void on_end(IBuffEntity* attacker, IBuffEntity* victim) {
    (void)attacker;
    (void)victim;
  }
  Value effect_data();
  virtual void place_effect(IBuffEntity* effect, IBuffEntity* victim);
  void place_effect_center(IBuffEntity* effect, IBuffEntity* victim);
  void del_effect(const std::u16string& vid);
  void clear_effects();
  void show_effect(IBuffEntity* victim);
  void update_effects();
  bool del_id(const std::u16string& id);

  const BuffEnv* _env = nullptr;
  std::u16string _id;
  Value _kind;
  IBuffEntity* _attacker = nullptr;
  std::u16string _attacker_id;
  double _level = 0;
  bool _mounted = false;
  std::vector<std::u16string> _victims;
  Times _ticker;
  Times _lifetime{0.0, 1.0};
  std::vector<std::pair<std::u16string, IBuffEntity*>> _effects;
  Value _effect_data;
  bool _effect_data_loaded = false;
};

Buff* grant_buff(const BuffEnv* env, const std::u16string& kind, IBuffEntity* attacker,
                 IBuffEntity* victim, double duration);

}
}
