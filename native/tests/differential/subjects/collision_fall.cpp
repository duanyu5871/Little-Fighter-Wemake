#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

#include "lfw/collision/fall.h"
#include "lfw/core/value.h"
#include "lfw/defines/itr_effect.h"
#include "lfw/defines/state_enum.h"
#include "lfw/entity/entity_type_check.h"
#include "lfw/utils/container_help/field_or.h"

#include "trace_util.h"

namespace {

using lfw::Value;
using lfw::collision::Collision;
using lfw::collision::Cube;
using lfw::collision::FallEnv;
using lfw::collision::IFallEntity;
using lfw::collision::IHandlerEntity;
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

struct FakeBuff : lfw::buff::IBuffEntity {
  std::u16string _id;
  explicit FakeBuff(std::u16string id) : _id(std::move(id)) {}
  const std::u16string& id() const override { return _id; }
  void position(double& x, double& y, double& z) const override {
    x = 0;
    y = 0;
    z = 0;
  }
  void set_position(double, double, double) override {}
  double frame_centery() const override { return 0; }
  double frame_height() const override { return 0; }
  double frame_pic_h() const override { return 0; }
  void set_frame(const Value&) override {}
  void buffs_set(const std::u16string&, lfw::buff::Buff*) override {}
  void buffs_delete(const std::u16string&) override {}
  void set_outline_alpha(double) override {}
  void set_outline_width(double) override {}
  void set_outline_color(const std::u16string&) override {}
  void enter_frame_by_id(const std::u16string&) override {}
  void attach(bool) override {}
};

struct Fake : IFallEntity {
  std::u16string _id;
  Value _hp = Value(50.0);
  Value _hp_r = Value(50.0);
  Value _fall = Value(0.0);
  Value _fall_max = Value(100.0);
  Value _defend = Value(1.0);
  Value _resting = Value(0.0);
  Value _catch_time = Value(0.0);
  Value _catch_time_max = Value(30.0);
  Value _src_emitter;
  Value _shaking;
  Value _motionless;
  Value _itr_fall = Value(2.0);
  Value _hit_sounds;
  Value _ice;
  Value _elec_dur = Value(7.0);
  Value _state;
  Value _facing = Value(1.0);
  Value _fire;
  Value _crit;
  Value _holding;
  bool _catching = false;
  IHandlerEntity* _catcher = nullptr;
  double _vel[3] = {0, 0, 0};
  double _toughness = 9;
  FakeBuff _buff;
  Value _dataset_values;
  Value _data;

  explicit Fake(std::u16string id) : _id(std::move(id)), _buff(_id) { set_type(Value(8.0)); }

  void set_type(const Value& t) {
    auto o = std::make_shared<lfw::Object>();
    o->set(u"type", t);
    _data = Value(o);
  }

  const std::u16string& id() const override { return _id; }
  Value hp() const override { return _hp; }
  void set_hp(const Value& v) override {
    _hp = v;
    g_log.push_back(s_of(_id) + ":set_hp:" + render(v));
  }
  Value hp_r() const override { return _hp_r; }
  void set_hp_r(const Value& v) override {
    _hp_r = v;
    g_log.push_back(s_of(_id) + ":set_hp_r:" + render(v));
  }
  void set_toughness(const Value& v) override {
    _toughness = lfw::to_number(v);
    g_log.push_back(s_of(_id) + ":set_toughness:" + render(v));
  }
  Value data() const override {
    auto o = std::make_shared<lfw::Object>();
    o->set(u"type", lfw::field_or(_data, u"type"));
    auto idx = std::make_shared<lfw::Object>();
    idx->set(u"fire", _fire);
    idx->set(u"critical_hit", _crit);
    o->set(u"indexes", Value(idx));
    return Value(o);
  }
  Value data_indexes_ice() const override { return _ice; }
  Value data_base_hit_sounds() const override { return _hit_sounds; }
  Value dataset(const std::u16string& key) const override {
    if (key == u"electrify_duration") return _elec_dur;
    return lfw::field_or(_dataset_values, key.c_str());
  }
  bool marks_has(const std::u16string&) const override { return false; }
  bool catching() const override { return _catching; }
  void set_catching(IHandlerEntity*) override {}
  Value catch_time_max() const override { return _catch_time_max; }
  void set_catch_time(const Value& v) override {
    _catch_time = v;
    g_log.push_back(s_of(_id) + ":set_catch_time:" + render(v));
  }
  IHandlerEntity* catcher() const override { return _catcher; }
  void set_catcher(IHandlerEntity* v) override {
    _catcher = v;
    g_log.push_back(s_of(_id) + ":set_catcher:" + (v == nullptr ? "-" : s_of(v->id())));
  }
  Value resting() const override { return _resting; }
  void set_resting(const Value& v) override {
    _resting = v;
    g_log.push_back(s_of(_id) + ":set_resting:" + render(v));
  }
  Value fall_value() const override { return _fall; }
  void set_fall_value(const Value& v) override {
    _fall = v;
    g_log.push_back(s_of(_id) + ":set_fall_value:" + render(v));
  }
  Value fall_value_max() const override { return _fall_max; }
  Value defend_value() const override { return _defend; }
  void set_defend_value(const Value& v) override {
    _defend = v;
    g_log.push_back(s_of(_id) + ":set_defend_value:" + render(v));
  }
  Value defend_value_max() const override { return _defend; }
  Value itr_fall(const Value&) const override { return _itr_fall; }
  Value src_emitter() const override { return _src_emitter; }
  Value shaking() const override { return _shaking; }
  void set_shaking(const Value& v) override {
    _shaking = v;
    g_log.push_back(s_of(_id) + ":set_shaking:" + render(v));
  }
  Value motionless() const override { return _motionless; }

  void set_velocity(const Value& x, const Value& y, const Value& z) override {
    std::string s = s_of(_id) + ":set_velocity";
    if (!nullish(x)) {
      _vel[0] = lfw::to_number(x);
      s += ":" + render(x);
    }
    if (!nullish(y)) {
      _vel[1] = lfw::to_number(y);
      s += ":" + render(y);
    }
    if (!nullish(z)) {
      _vel[2] = lfw::to_number(z);
      s += ":" + render(z);
    }
    g_log.push_back(s);
  }

  void enter_frame(const Value& info) override {
    g_log.push_back(s_of(_id) + ":enter_frame:" + render(info));
  }
  void enter_frame_by_id(const Value& id) override {
    g_log.push_back(s_of(_id) + ":enter_frame_by_id:" + render(id));
  }
  void play_sound(const Value& sounds) override {
    g_log.push_back(s_of(_id) + ":play_sound:" + render(sounds));
  }
  lfw::buff::IBuffEntity* buff_entity() override { return &_buff; }

  Value facing() const override { return _facing; }
  double velocity_x() const override { return _vel[0]; }
  void spark_point(const Cube& a, const Cube& b, double& x, double& y, double& z) override {
    x = a.left;
    y = b.top;
    z = a.near;
    g_log.push_back("sp:" + num(x) + ":" + num(y) + ":" + num(z));
  }
  Value data_indexes_fire() const override { return _fire; }
  Value data_indexes_critical_hit() const override { return _crit; }
  Value holding_base_type() const override { return lfw::field_or(_holding, u"base_type"); }
  void drop_holding() override {
    _holding = Value();
    g_log.push_back(s_of(_id) + ":drop_holding");
  }
};

Fake g_att(u"A");
Fake g_vic(u"V");
Collision g_c;
FallEnv g_env;
ItrVelocity g_iv;
Value g_itr_motionless;

void bind() {
  g_env.find_entity = [](const std::u16string& id) -> IFallEntity* {
    if (id == g_att._id) return &g_att;
    if (id == g_vic._id) return &g_vic;
    return nullptr;
  };
  g_env.is_fighter = [](const IFallEntity& e) {
    return lfw::entity::is_fighter_data(e.data());
  };
  g_env.spark = [](const Value& x, const Value& y, const Value& z, const Value& type) {
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
  return s_of(f._id) + ".hp=" + render(f._hp) + " " + s_of(f._id) + ".hp_r=" + render(f._hp_r) +
         " " + s_of(f._id) + ".tough=" + num(f._toughness) + " " + s_of(f._id) +
         ".fall=" + render(f._fall) + " " + s_of(f._id) + ".fall_max=" + render(f._fall_max) +
         " " + s_of(f._id) + ".defend=" + render(f._defend) + " " + s_of(f._id) +
         ".resting=" + render(f._resting) + " " + s_of(f._id) + ".state=" + render(f._state) +
         " " + s_of(f._id) + ".face=" + render(f._facing) + " " + s_of(f._id) +
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
    f._facing = parse_value(tok, i);
  } else if (field == "type") {
    f.set_type(parse_value(tok, i));
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
    f._holding = parse_value(tok, i);
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
