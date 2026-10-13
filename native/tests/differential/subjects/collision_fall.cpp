#include <cmath>
#include <cstdio>
#include <fstream>
#include <memory>
#include <string>
#include <variant>
#include <vector>

#include "lfw/collision/fall.h"
#include "lfw/core/value.h"
#include "lfw/defines/itr_effect.h"
#include "lfw/defines/state_enum.h"
#include "lfw/entity/entity.h"
#include "lfw/entity/entity_type_check.h"
#include "lfw/utils/container_help/field_or.h"

#include "entity_fixture.h"
#include "trace_util.h"

namespace {

using lfw::Entity;
using lfw::Value;
using lfw::collision::Collision;
using lfw::collision::Cube;
using lfw::collision::FallEnv;
using lfw::collision::ItrVelocity;
using trace::parse_value;
using trace::render_value;
using trace::split_ws;
using trace::to_ascii;

std::vector<std::string> g_log;

std::string render(const Value& v) { return to_ascii(render_value(v)); }
std::string s_of(const std::u16string& s) { return to_ascii(s); }
std::string num(double d) { return render(Value(d)); }

std::string join(const std::vector<std::string>& xs) {
  std::string out;
  for (size_t i = 0; i < xs.size(); ++i) {
    if (i) out += ",";
    out += xs[i];
  }
  return out;
}

bool nullish(const Value& v) {
  return std::holds_alternative<std::monostate>(v) || std::holds_alternative<lfw::NullTag>(v);
}

// 切片 4CE：碰撞族窄接口（`IFallEntity` 等）退役 ⇒ 台面假实体就是真 `lfw::Entity`，
// 只覆写实体面上确实存在的那几条读/写（TS 台面同样只给这几条，见 `collision_fall.ts`）。
//
// 唯一一处「库里有、台面拦不到」的调用是 `drop_holding`（实体上至今非虚），而 TS 台面的假件
// 拦得到、日志就在那儿 ⇒ 那一行由被拿武器假件补：见 `HeldFake`（真 `drop_holding()` 会碰它
// 的虚方法）。

struct Fake : lfw::Entity {
  // 用例面（TS 台面的后备字段）与显示面（`| 状态`）读的都是镜像；库读的那些走 override。
  // `_ice` / `_hit_sounds` / `_src_emitter` / `_motionless` / `_dataset_values` 只挂在用例面
  // 上（窄接口退役后本单元没有读它们的调用），留着是为了不动 `env a|v` 的字段面。
  Value _hp = Value(50.0);
  Value _hp_r = Value(50.0);
  double _toughness = 9;
  Value _fall = Value(0.0);
  Value _fall_max = Value(100.0);
  Value _defend = Value(1.0);
  Value _resting = Value(0.0);
  Value _state;
  Value _facing = Value(1.0);
  Value _type = Value(8.0);
  Value _fire;
  Value _crit;
  Value _src_emitter;
  Value _motionless;
  Value _hit_sounds;
  Value _ice;
  Value _dataset_values;
  double _vel[3] = {0, 0, 0};
  mutable Value _data_cache;

  explicit Fake(std::u16string id) : lfw::Entity(fx::shared_host(), Value()) {
    this->id = std::move(id);
  }

  // `data` 按 TS 台面的样子现搭（`type` 决定 `is_fighter`，`indexes` 供 `data_indexes_of`）。
  const Value& data() const override {
    auto indexes = std::make_shared<lfw::Object>();
    indexes->set(u"fire", _fire);
    indexes->set(u"critical_hit", _crit);
    auto o = std::make_shared<lfw::Object>();
    o->set(u"type", _type);
    o->set(u"indexes", Value(indexes));
    _data_cache = Value(o);
    return _data_cache;
  }

  double hp() const override { return lfw::to_number(_hp); }
  void set_hp(double v) override { _hp = Value(v); log_set("set_hp", v); }
  double hp_r() const override { return lfw::to_number(_hp_r); }
  void set_hp_r(double v) override { _hp_r = Value(v); log_set("set_hp_r", v); }
  double toughness() const override { return _toughness; }
  void set_toughness(double v) override { _toughness = v; log_set("set_toughness", v); }
  double fall_value() const override { return lfw::to_number(_fall); }
  void set_fall_value(double v) override { _fall = Value(v); log_set("set_fall_value", v); }
  double fall_value_max() const override { return lfw::to_number(_fall_max); }
  double defend_value() const override { return lfw::to_number(_defend); }
  void set_defend_value(double v) override { _defend = Value(v); log_set("set_defend_value", v); }
  double defend_value_max() const override { return lfw::to_number(_defend); }
  double resting() const override { return lfw::to_number(_resting); }
  void set_resting(double v) override { _resting = Value(v); log_set("set_resting", v); }

  // `set_velocity(x?, y?, z?)`：`undefined` / `null` 的分量跳过（TS 台面同式）；真速度也
  // 写进实体字段 —— `handle_fall` 的 `normal_fall_act` 直接读 `velocity.x`。
  void set_velocity(const Value& x, const Value& y, const Value& z) override {
    std::string s = s_of(id) + ":set_velocity";
    if (!nullish(x)) {
      _vel[0] = lfw::to_number(x);
      velocity.x = _vel[0];
      s += ":" + render(x);
    }
    if (!nullish(y)) {
      _vel[1] = lfw::to_number(y);
      velocity.y = _vel[1];
      s += ":" + render(y);
    }
    if (!nullish(z)) {
      _vel[2] = lfw::to_number(z);
      velocity.z = _vel[2];
      s += ":" + render(z);
    }
    g_log.push_back(s);
  }

  // TS 台面的假 `spark_point`：x/y/z 从两个 cube 取，并把 `sp:` 一行打进日志（真
  // `handle_fall` 拿这三个出参去喂 `spark` 缝 ⇒ `spark:` 那一行也随之一致）。
  void spark_point(const Cube& a, const Cube& b, double& x, double& y, double& z) const override {
    x = a.left;
    y = b.top;
    z = a.near;
    g_log.push_back("sp:" + num(x) + ":" + num(y) + ":" + num(z));
  }

  // 台面的假件只打这一行（真 `enter_frame` 会去翻帧表，这里没有帧表可翻）。
  lfw::EnterFrameResult enter_frame(const Value& nf, bool fallback = false) override {
    (void)fallback;
    g_log.push_back(s_of(id) + ":enter_frame:" + render(nf));
    return lfw::EnterFrameResult::NotFound;
  }

  // 同上：台面的假件只打这一行。
  lfw::EnterFrameResult enter_frame_by_id(const Value& id_value,
                                          bool fallback = false) override {
    (void)fallback;
    g_log.push_back(s_of(id) + ":enter_frame_by_id:" + render(id_value));
    return lfw::EnterFrameResult::NotFound;
  }

 private:
  void log_set(const char* name, double v) {
    g_log.push_back(s_of(id) + ":" + name + ":" + num(v));
  }
};

Fake g_att(u"A");
Fake g_vic(u"V");

// 受害者手里那把武器：真 `Entity::drop_holding()` 会读它的 `data`、摆它的位置并让它
// `enter_frame` —— 台面借这几条虚方法观测「真的走了掉落这条链」，同时把真 `drop_holding`
// 顺带的副作用挡掉（TS 台面的假 `drop_holding` 也只打一行日志）。
struct HeldFake : lfw::Entity {
  double _base_type = 0;
  mutable Value _data_empty;

  HeldFake() : lfw::Entity(fx::shared_host(), Value()) {
    this->id = u"H";
    // TS 台面的假件没有帧表（它的 `frame` 是 `undefined`）：这里也让真 `enter_frame` 拿不到
    // 帧，免得它顺手改 `arest` / `state` / `facing` 这些 TS 台面并没有的东西。
    frame = Value();
  }

  double base_type() const override { return _base_type; }
  const Value& data() const override { return _data_empty; }
  void set_position(const Value& x, const Value& y, const Value& z) override {
    (void)x;
    (void)y;
    (void)z;
    g_log.push_back(s_of(g_vic.id) + ":drop_holding");
  }
};

HeldFake g_held;
Collision g_c;
FallEnv g_env;
ItrVelocity g_iv;
Value g_itr_motionless;

void bind() {
  g_env.find_entity = [](const std::u16string& id) -> Entity* {
    if (id == g_att.id) return &g_att;
    if (id == g_vic.id) return &g_vic;
    return nullptr;
  };
  g_env.is_fighter = [](const Entity& e) { return lfw::entity::is_fighter_data(e.data()); };
  g_env.spark = [](const Value& x, const Value& y, const Value& z, const Value& type) {
    // 三个数就是假 `spark_point` 交给缝的读数（TS 台面的 `world.spark` 记的就是它们）。
    g_log.push_back("spark:" + render(x) + ":" + render(y) + ":" + render(z) + ":" +
                    render(type));
  };
  g_env.calc_velocity = [](Collision&) {
    ItrVelocity r = g_iv;
    const Value effect = lfw::field_or(g_c.itr, u"effect");
    const bool position_based =
        lfw::strict_equals(effect, Value(static_cast<double>(lfw::ItrEffect::FireExplosion))) ||
        lfw::strict_equals(effect, Value(static_cast<double>(lfw::ItrEffect::Explosion))) ||
        lfw::strict_equals(g_att._state,
                           Value(static_cast<double>(lfw::StateEnum::HeavyWeapon_InTheSky)));
    const double direction =
        position_based ? -1.0 : lfw::to_number(g_att._facing);
    r.x = g_iv.x * direction;
    r.x_direction = Value(direction);
    return r;
  };
  lfw::collision::set_fall_env(g_env);
}

std::string side_text(const Fake& f) {
  return s_of(f.id) + ".hp=" + render(f._hp) + " " + s_of(f.id) + ".hp_r=" + render(f._hp_r) +
         " " + s_of(f.id) + ".tough=" + num(f._toughness) + " " + s_of(f.id) +
         ".fall=" + render(f._fall) + " " + s_of(f.id) + ".fall_max=" + render(f._fall_max) +
         " " + s_of(f.id) + ".defend=" + render(f._defend) + " " + s_of(f.id) +
         ".resting=" + render(f._resting) + " " + s_of(f.id) + ".state=" + render(f._state) +
         " " + s_of(f.id) + ".face=" + render(f._facing) + " " + s_of(f.id) +
         ".vel=" + num(f._vel[0]) + "/" + num(f._vel[1]) + "/" + num(f._vel[2]);
}

std::string state_text() { return side_text(g_att) + " " + side_text(g_vic); }

void walk_side(Fake& f, const std::string& field, const std::vector<std::string>& tok, size_t& i) {
  if (field == "hp") {
    f._hp = parse_value(tok, i);
  } else if (field == "hp_r") {
    f._hp_r = parse_value(tok, i);
  } else if (field == "tough") {
    f._toughness = trace::to_double(tok[i++]);
  } else if (field == "fall") {
    f._fall = parse_value(tok, i);
  } else if (field == "fall_max") {
    f._fall_max = parse_value(tok, i);
  } else if (field == "defend") {
    f._defend = parse_value(tok, i);
  } else if (field == "resting") {
    f._resting = parse_value(tok, i);
  } else if (field == "state") {
    f._state = parse_value(tok, i);
  } else if (field == "face") {
    const Value v = parse_value(tok, i);
    f._facing = v;
    f.facing = lfw::to_number(v);  // `handle_fall` 直接读实体字段
  } else if (field == "type") {
    f._type = parse_value(tok, i);
  } else if (field == "velx") {
    f._vel[0] = trace::to_double(tok[i++]);
  } else if (field == "vely") {
    f._vel[1] = trace::to_double(tok[i++]);
  } else if (field == "velz") {
    f._vel[2] = trace::to_double(tok[i++]);
  } else if (field == "fire") {
    f._fire = parse_value(tok, i);
  } else if (field == "crit") {
    f._crit = parse_value(tok, i);
  } else if (field == "holding") {
    // `holding_base_type()` 是实体自己的读法（`holding->base_type()`）⇒ 台面摆一把假武器。
    const Value v = parse_value(tok, i);
    if (nullish(v)) {
      g_vic.holding = nullptr;
    } else {
      g_held._base_type = lfw::to_number(lfw::field_or(v, u"base_type"));
      g_vic.holding = &g_held;
    }
  } else if (field == "ice") {
    f._ice = parse_value(tok, i);
  } else if (field == "hit_sounds") {
    f._hit_sounds = parse_value(tok, i);
  } else if (field == "src_emitter") {
    f._src_emitter = parse_value(tok, i);
  } else if (field == "motionless") {
    f._motionless = parse_value(tok, i);
  } else if (field == "dataset") {
    f._dataset_values = parse_value(tok, i);
  } else {
    std::fprintf(stderr, "unknown side field '%s'\n", field.c_str());
    std::exit(2);
  }
}

void run() {
  lfw::collision::handle_fall(g_c);
  std::printf("run fall || %s | %s\n", join(g_log).c_str(), state_text().c_str());
}

}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: lfw_trace_collision_fall <case-file>\n");
    return 2;
  }
  bind();
  g_c.aid = u"A";
  g_c.vid = u"V";

  std::ifstream in(argv[1]);
  if (!in) {
    std::fprintf(stderr, "cannot open case file: %s\n", argv[1]);
    return 2;
  }

  std::string raw;
  int lineno = 0;
  while (std::getline(in, raw)) {
    ++lineno;
    const std::vector<std::string> t = split_ws(trace::strip_comment(raw));
    if (t.empty()) continue;
    const std::string& op = t[0];
    size_t i = 1;
    g_log.clear();
    if (op == "env") {
      const std::string& sub = t[i++];
      if (sub == "itr") {
        g_c.itr = parse_value(t, i);
      } else if (sub == "dataset") {
        g_c.dataset = parse_value(t, i);
      } else if (sub == "velx") {
        g_iv.x = trace::to_double(t[i++]);
      } else if (sub == "vely") {
        g_iv.y = trace::to_double(t[i++]);
      } else if (sub == "velz") {
        g_iv.z = trace::to_double(t[i++]);
      } else if (sub == "rest") {
        g_c.rest = trace::to_double(t[i++]);
      } else if (sub == "itr_motionless") {
        g_itr_motionless = parse_value(t, i);
      } else if (sub == "acube") {
        const Value v = parse_value(t, i);
        g_c.a_cube.left = lfw::to_number(lfw::field_or(v, u"left"));
        g_c.a_cube.right = lfw::to_number(lfw::field_or(v, u"right"));
        g_c.a_cube.bottom = lfw::to_number(lfw::field_or(v, u"bottom"));
        g_c.a_cube.top = lfw::to_number(lfw::field_or(v, u"top"));
        g_c.a_cube.near = lfw::to_number(lfw::field_or(v, u"near"));
        g_c.a_cube.far = lfw::to_number(lfw::field_or(v, u"far"));
      } else if (sub == "bcube") {
        const Value v = parse_value(t, i);
        g_c.b_cube.left = lfw::to_number(lfw::field_or(v, u"left"));
        g_c.b_cube.right = lfw::to_number(lfw::field_or(v, u"right"));
        g_c.b_cube.bottom = lfw::to_number(lfw::field_or(v, u"bottom"));
        g_c.b_cube.top = lfw::to_number(lfw::field_or(v, u"top"));
        g_c.b_cube.near = lfw::to_number(lfw::field_or(v, u"near"));
        g_c.b_cube.far = lfw::to_number(lfw::field_or(v, u"far"));
      } else if (sub == "a" || sub == "v") {
        const std::string& field = t[i++];
        walk_side(sub == "a" ? g_att : g_vic, field, t, i);
        if (i != t.size()) {
          std::fprintf(stderr, "trailing tokens after side field '%s' at line %d\n", field.c_str(),
                       lineno);
          return 2;
        }
      } else {
        std::fprintf(stderr, "unknown env '%s' at line %d\n", sub.c_str(), lineno);
        return 2;
      }
      std::printf("env %s\n", sub.c_str());
      continue;
    }
    if (op == "run") {
      run();
      continue;
    }
    std::fprintf(stderr, "unknown op '%s' at line %d\n", op.c_str(), lineno);
    return 2;
  }
  return 0;
}
