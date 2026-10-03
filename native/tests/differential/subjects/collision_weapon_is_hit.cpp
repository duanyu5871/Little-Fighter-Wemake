#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

#include "lfw/collision/weapon_is_hit.h"
#include "lfw/core/value.h"
#include "lfw/entity/entity_type_check.h"
#include "lfw/utils/container_help/field_or.h"

#include "trace_util.h"

namespace {

using lfw::Value;
using lfw::collision::Collision;
using lfw::collision::Cube;
using lfw::collision::Handlers2Env;
using lfw::collision::HandlersEnv;
using lfw::collision::IHandlerEntity;
using lfw::collision::IWeaponIsHitEntity;
using lfw::collision::ItrVelocity;
using lfw::collision::SparkPoint;
using lfw::collision::WeaponIsHitEnv;
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

struct Fake : IWeaponIsHitEntity {
  std::u16string _id;
  Value _hp = Value(50.0);
  Value _hp_r = Value(50.0);
  Value _fall = Value(5.0);
  Value _fall_max = Value(100.0);
  Value _defend = Value(3.0);
  Value _defend_max = Value(50.0);
  Value _resting = Value(0.0);
  Value _catch_time = Value(0.0);
  Value _catch_time_max = Value(30.0);
  Value _src_emitter;
  Value _shaking;
  Value _motionless;
  Value _itr_fall = Value(2.0);
  Value _hit_sounds;
  Value _ice;
  bool _catching = false;
  IHandlerEntity* _catcher = nullptr;
  Value _elec_dur = Value(7.0);
  Value _state;
  Value _facing = Value(1.0);
  Value _base_type;
  Value _team = Value(0.0);
  Value _data_id;
  Value _throwings;
  Value _in_the_skys;
  bool _bearer = false;
  bool _dropping = false;
  bool _on_ground = true;
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
    o->set(u"id", _data_id);
    auto idx = std::make_shared<lfw::Object>();
    idx->set(u"throwings", _throwings);
    idx->set(u"in_the_skys", _in_the_skys);
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
  Value defend_value_max() const override { return _defend_max; }
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
    if (_vel[1] > 0) leave_ground();
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

  bool has_bearer() const override { return _bearer; }
  void set_dropping(bool v) override {
    _dropping = v;
    g_log.push_back(s_of(_id) + ":set_dropping:" + (v ? "1" : "0"));
  }
  Value base_type() const override { return _base_type; }
  Value facing() const override { return _facing; }
  Value team() const override { return _team; }
  void set_team(const Value& v) override {
    _team = v;
    g_log.push_back(s_of(_id) + ":set_team:" + render(v));
  }
  Value data_id() const override { return _data_id; }
  Value data_indexes_throwings() const override { return _throwings; }
  Value data_indexes_in_the_skys() const override { return _in_the_skys; }
  void leave_ground() override {
    _on_ground = false;
    g_log.push_back(s_of(_id) + ":leave_ground");
  }
};

Fake g_att(u"A");
Fake g_vic(u"V");
Collision g_c;
HandlersEnv g_henv;
Handlers2Env g_h2;
WeaponIsHitEnv g_env;
Cube g_acube;
Cube g_bcube;
Value g_itr_motionless;
Value g_recov = Value(0.25);
ItrVelocity g_iv;
Value g_emitter_target;
Fake* g_emitter = nullptr;

void bind() {
  g_henv.attacker_itr_motionless = []() { return g_itr_motionless; };
  g_henv.attacker_set_motionless = [](const Value& v) {
    g_att._motionless = v;
    g_log.push_back("A:set_motionless:" + render(v));
  };
  g_henv.victim_set_shaking = [](const Value& v) {
    g_vic._shaking = v;
    g_log.push_back("V:set_shaking:" + render(v));
  };
  g_henv.attacker_set_arest = [](double v) { g_log.push_back("A:set_arest:" + num(v)); };
  g_henv.victim_add_v_rest = [](Collision& c) {
    g_log.push_back("add_v_rest:" + s_of(c.vid) + ":" + num(c.rest));
  };
  g_henv.attacker_pick_victim = [](Collision&) {};
  g_henv.buff_get = [](const std::u16string&) { return false; };
  g_henv.buff_lifetime_zero = [](const std::u16string&) {};
  g_henv.buff_create = [](const std::u16string&, const std::u16string&) { return false; };
  g_henv.buff_set_attacker = [](const std::u16string&, const std::u16string&) {};
  g_henv.buff_set_victim = [](const std::u16string&, const std::u16string&) {};
  g_henv.buff_mount = [](const std::u16string&) {};
  g_c.env = &g_henv;

  g_h2.warn = [](const std::u16string& m) { g_log.push_back("warn:" + s_of(m)); };
  g_h2.hp_recoverability = []() { return g_recov; };
  g_h2.find_entity = [](const std::u16string& id) -> IHandlerEntity* {
    if (id == g_att._id) return &g_att;
    if (id == g_vic._id) return &g_vic;
    return nullptr;
  };
  g_h2.summary_apply_damage = [](IHandlerEntity* a, const Value& injury, IHandlerEntity* v,
                                 const Value& prev_hp) {
    g_log.push_back("summary:" + s_of(a->id()) + ":" + render(injury) + ":" + s_of(v->id()) +
                    ":" + render(prev_hp));
  };
  g_h2.buff_env = []() { return static_cast<const lfw::buff::BuffEnv*>(nullptr); };
  g_h2.is_fighter = [](const IHandlerEntity& e) {
    return lfw::entity::is_fighter_data(e.data());
  };
  g_h2.calc_velocity = [](Collision&) {
    ItrVelocity r = g_iv;
    r.x = g_iv.x * lfw::to_number(g_att._facing);
    return r;
  };
  lfw::collision::set_handlers2_env(g_h2);

  g_env.find_entity = [](const std::u16string& id) -> IWeaponIsHitEntity* {
    if (id == g_att._id) return &g_att;
    if (id == g_vic._id) return &g_vic;
    return nullptr;
  };
  g_env.spark_point = [](const Cube& a, const Cube& b) {
    SparkPoint p;
    p.x = Value(a.left);
    p.y = Value(b.top);
    p.z = Value(a.near);
    g_log.push_back("sp:" + render(p.x) + ":" + render(p.y) + ":" + render(p.z));
    return p;
  };
  g_env.spark = [](const Value& x, const Value& y, const Value& z, const std::u16string& kind) {
    g_log.push_back("spark:" + render(x) + ":" + render(y) + ":" + render(z) + ":" +
                    s_of(kind));
  };
  g_env.mt_mark = [](const std::u16string& m) { g_log.push_back("mark:" + s_of(m)); };
  g_env.mt_pick = [](const Value& indexes) {
    g_log.push_back("pick:" + render(indexes));
    return Value(u"picked");
  };
  g_env.calc_velocity = [](Collision&) {
    ItrVelocity r = g_iv;
    r.x = g_iv.x * lfw::to_number(g_att._facing);
    return r;
  };
  lfw::collision::set_weapon_is_hit_env(g_env);
}

std::string side_text(const Fake& f) {
  return s_of(f._id) + ".hp=" + render(f._hp) + " " + s_of(f._id) + ".hp_r=" + render(f._hp_r) +
         " " + s_of(f._id) + ".tough=" + num(f._toughness) + " " + s_of(f._id) +
         ".state=" + render(f._state) + " " + s_of(f._id) + ".face=" + render(f._facing) + " " +
         s_of(f._id) + ".base_type=" + render(f._base_type) + " " + s_of(f._id) +
         ".team=" + render(f._team) + " " + s_of(f._id) +
         ".bearer=" + (f._bearer ? "1" : "0") + " " + s_of(f._id) +
         ".dropping=" + (f._dropping ? "1" : "0") + " " + s_of(f._id) +
         ".on_ground=" + (f._on_ground ? "1" : "0") + " " + s_of(f._id) +
         ".data_id=" + render(f._data_id) + " " + s_of(f._id) + ".vel=" + num(f._vel[0]) + "/" +
         num(f._vel[1]) + "/" + num(f._vel[2]);
}

std::string state_text() {
  std::string s = side_text(g_att) + " " + side_text(g_vic);
  s += " inj=" + render(g_c.injury) + " inj_r=" + render(g_c.injury_r) +
       " rinj=" + render(g_c.real_injury) + " rinj_r=" + render(g_c.real_injury_r);
  return s;
}

void walk_side(Fake& f, const std::string& field, const std::vector<std::string>& tok, size_t& i) {
  if (field == "hp") {
    f._hp = parse_value(tok, i);
  } else if (field == "hp_r") {
    f._hp_r = parse_value(tok, i);
  } else if (field == "tough") {
    f._toughness = trace::to_double(tok[i++]);
  } else if (field == "state") {
    f._state = parse_value(tok, i);
  } else if (field == "face") {
    f._facing = parse_value(tok, i);
  } else if (field == "base_type") {
    f._base_type = parse_value(tok, i);
  } else if (field == "team") {
    f._team = parse_value(tok, i);
  } else if (field == "bearer") {
    f._bearer = tok[i++] == "1";
  } else if (field == "drop") {
    f._dropping = tok[i++] == "1";
  } else if (field == "data_id") {
    f._data_id = parse_value(tok, i);
  } else if (field == "throwings") {
    f._throwings = parse_value(tok, i);
  } else if (field == "in_the_sky") {
    f._in_the_skys = parse_value(tok, i);
  } else if (field == "velx") {
    f._vel[0] = trace::to_double(tok[i++]);
  } else if (field == "vely") {
    f._vel[1] = trace::to_double(tok[i++]);
  } else if (field == "velz") {
    f._vel[2] = trace::to_double(tok[i++]);
  } else if (field == "on_ground") {
    f._on_ground = tok[i++] == "1";
  } else if (field == "fall") {
    f._fall = parse_value(tok, i);
  } else if (field == "fall_max") {
    f._fall_max = parse_value(tok, i);
  } else if (field == "defend") {
    f._defend = parse_value(tok, i);
  } else if (field == "resting") {
    f._resting = parse_value(tok, i);
  } else if (field == "type") {
    f.set_type(parse_value(tok, i));
  } else if (field == "src_emitter") {
    f._src_emitter = parse_value(tok, i);
  } else if (field == "ice") {
    f._ice = parse_value(tok, i);
  } else if (field == "hit_sounds") {
    f._hit_sounds = parse_value(tok, i);
  } else if (field == "motionless") {
    f._motionless = parse_value(tok, i);
  } else if (field == "dataset") {
    f._dataset_values = parse_value(tok, i);
  } else {
    std::fprintf(stderr, "unknown side field '%s'\n", field.c_str());
    std::exit(2);
  }
}

void run(const std::string& name) {
  if (name == "hit") {
    lfw::collision::handle_weapon_is_hit(g_c);
  } else {
    std::fprintf(stderr, "unknown handler '%s'\n", name.c_str());
    std::exit(2);
  }
  std::printf("run %s || %s | %s\n", name.c_str(), join(g_log).c_str(), state_text().c_str());
}

}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: lfw_trace_collision_weapon_is_hit <case-file>\n");
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
      } else if (sub == "rest") {
        g_c.rest = trace::to_double(t[i++]);
      } else if (sub == "recov") {
        g_recov = Value(trace::to_double(t[i++]));
      } else if (sub == "itr_motionless") {
        g_itr_motionless = parse_value(t, i);
      } else if (sub == "velx") {
        g_iv.x = trace::to_double(t[i++]);
      } else if (sub == "vely") {
        g_iv.y = trace::to_double(t[i++]);
      } else if (sub == "velz") {
        g_iv.z = trace::to_double(t[i++]);
      } else if (sub == "acube") {
        const Value v = parse_value(t, i);
        g_acube.left = lfw::to_number(lfw::field_or(v, u"left"));
        g_acube.right = lfw::to_number(lfw::field_or(v, u"right"));
        g_acube.bottom = lfw::to_number(lfw::field_or(v, u"bottom"));
        g_acube.top = lfw::to_number(lfw::field_or(v, u"top"));
        g_acube.near = lfw::to_number(lfw::field_or(v, u"near"));
        g_acube.far = lfw::to_number(lfw::field_or(v, u"far"));
        g_c.a_cube = g_acube;
      } else if (sub == "bcube") {
        const Value v = parse_value(t, i);
        g_bcube.left = lfw::to_number(lfw::field_or(v, u"left"));
        g_bcube.right = lfw::to_number(lfw::field_or(v, u"right"));
        g_bcube.bottom = lfw::to_number(lfw::field_or(v, u"bottom"));
        g_bcube.top = lfw::to_number(lfw::field_or(v, u"top"));
        g_bcube.near = lfw::to_number(lfw::field_or(v, u"near"));
        g_bcube.far = lfw::to_number(lfw::field_or(v, u"far"));
        g_c.b_cube = g_bcube;
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
      run(t[i++]);
      continue;
    }
    std::fprintf(stderr, "unknown op '%s' at line %d\n", op.c_str(), lineno);
    return 2;
  }
  return 0;
}
