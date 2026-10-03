#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

#include "lfw/collision/handlers2.h"
#include "lfw/core/value.h"
#include "lfw/entity/entity_type_check.h"
#include "lfw/utils/container_help/field_or.h"

#include "trace_util.h"

namespace {

using lfw::Value;
using lfw::collision::Collision;
using lfw::collision::Handlers2Env;
using lfw::collision::HandlersEnv;
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
std::string flag(bool b) { return b ? "b1" : "b0"; }

std::string join(const std::vector<std::string>& xs) {
  std::string out;
  for (size_t i = 0; i < xs.size(); ++i) {
    if (i) out += ",";
    out += xs[i];
  }
  return out;
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

struct Fake : IHandlerEntity {
  std::u16string _id;
  Value _hp = Value(20.0);
  Value _hp_r = Value(20.0);
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
  Fake* _catching_entity = nullptr;
  Fake* _catcher = nullptr;
  bool _marks_electrify = false;
  Value _elec_dur = Value(7.0);
  Value _weight = Value(1.0);
  Value _state;
  Value _facing = Value(1.0);
  bool _on_ground = true;
  Value _velocity[3] = {Value(0.0), Value(0.0), Value(0.0)};
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
  Value data() const override { return _data; }
  Value data_indexes_ice() const override { return _ice; }
  Value data_base_hit_sounds() const override { return _hit_sounds; }
  Value dataset(const std::u16string& key) const override {
    if (key == u"electrify_duration") return _elec_dur;
    return lfw::field_or(_dataset_values, key.c_str());
  }
  bool marks_has(const std::u16string& kind) const override {
    return kind == u"Electrify" && _marks_electrify;
  }
  bool catching() const override { return _catching; }
  void set_catching(IHandlerEntity* v) override {
    _catching_entity = static_cast<Fake*>(v);
    g_log.push_back(s_of(_id) + ":set_catching:" + s_of(v->id()));
  }
  Value catch_time_max() const override { return _catch_time_max; }
  void set_catch_time(const Value& v) override {
    _catch_time = v;
    g_log.push_back(s_of(_id) + ":set_catch_time:" + render(v));
  }
  IHandlerEntity* catcher() const override { return _catcher; }
  void set_catcher(IHandlerEntity* v) override {
    _catcher = static_cast<Fake*>(v);
    g_log.push_back(s_of(_id) + ":set_catcher:" + s_of(v->id()));
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
    _velocity[0] = x;
    _velocity[1] = y;
    _velocity[2] = z;
    g_log.push_back(s_of(_id) + ":set_velocity:" + render(x) + ":" + render(y) + ":" + render(z));
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
};

Fake g_att(u"A");
Fake g_vic(u"V");
Collision g_c;
Handlers2Env g_h2;
HandlersEnv g_henv;
lfw::buff::BuffEnv g_benv;
lfw::buff::Buff* g_granted = nullptr;
Value g_recov = Value(0.25);
Value g_vel = Value(0.0);
Value g_vel_z = Value(0.0);
Value g_itr_motionless;
bool g_is_fighter = true;
Value g_collision_dataset;

void bind() {
  g_h2.warn = [](const std::u16string& m) { g_log.push_back("warn:" + s_of(m)); };
  g_h2.hp_recoverability = []() { return g_recov; };
  g_h2.find_entity = [](const std::u16string& id) -> IHandlerEntity* {
    if (id == g_att._id) return &g_att;
    if (id == g_vic._id) return &g_vic;
    return nullptr;
  };
  g_h2.summary_apply_damage = [](IHandlerEntity* a, const Value& injury, IHandlerEntity* v,
                                 const Value& prev_hp) {
    g_log.push_back("summary:" + s_of(a->id()) + ":" + render(injury) + ":" + s_of(v->id()) + ":" +
                    render(prev_hp));
  };
  g_h2.buff_env = []() { return &g_benv; };
  g_h2.is_fighter = [](const IHandlerEntity& e) {
    return lfw::entity::is_fighter_data(e.data());
  };
  g_h2.calc_velocity = [](Collision&) -> ItrVelocity {
    ItrVelocity v;
    v.x = lfw::to_number(g_vel);
    v.y = 0;
    v.z = lfw::to_number(g_vel_z);
    v.x_direction = Value(1.0);
    return v;
  };
  lfw::collision::set_handlers2_env(g_h2);

  g_henv.attacker_itr_motionless = []() { return g_itr_motionless; };
  g_henv.attacker_set_motionless = [](const Value& v) {
    g_att._motionless = v;
    g_log.push_back("A:set_motionless:" + render(v));
  };
  g_henv.victim_set_shaking = [](const Value& v) {
    g_vic._shaking = v;
    g_log.push_back("V:set_shaking:" + render(v));
  };
  g_henv.attacker_set_arest = [](double v) {
    g_log.push_back("A:set_arest:" + num(v));
  };
  g_henv.victim_add_v_rest = [](Collision& c) {
    g_log.push_back("add_v_rest:" + s_of(c.vid) + ":" + num(c.rest));
  };
  g_henv.buff_get = [](const std::u16string& id) {
    g_log.push_back("buff_get:" + s_of(id));
    return false;
  };
  g_henv.buff_lifetime_zero = [](const std::u16string& id) {
    g_log.push_back("lifetime_zero:" + s_of(id));
  };
  g_henv.buff_create = [](const std::u16string& kind, const std::u16string& id) {
    g_log.push_back("create:" + s_of(kind) + ":" + s_of(id));
    return false;
  };
  g_henv.buff_set_attacker = [](const std::u16string&, const std::u16string&) {};
  g_henv.buff_set_victim = [](const std::u16string&, const std::u16string&) {};
  g_henv.buff_mount = [](const std::u16string&) {};
  g_c.env = &g_henv;

  g_benv.find_entity = [](const std::u16string& id) -> lfw::buff::IBuffEntity* {
    if (id == g_att._id) return &g_att._buff;
    if (id == g_vic._id) return &g_vic._buff;
    return nullptr;
  };
  g_benv.create_entity = [](const Value&) -> lfw::buff::IBuffEntity* { return nullptr; };
  g_benv.find_data = [](const std::u16string&) -> Value { return Value(); };
  g_benv.world_buffs_set = [](const std::u16string&, lfw::buff::Buff*) {};
  g_benv.world_buffs_get = [](const std::u16string&) -> lfw::buff::Buff* { return nullptr; };
  g_benv.create_buff = [](const std::u16string& kind, const std::u16string& id) {
    g_log.push_back("create_buff:" + s_of(kind) + ":" + s_of(id));
    g_granted = new lfw::buff::Buff(&g_benv, id, Value(kind));
    return g_granted;
  };
}

std::string side_text(const Fake& f) {
  return s_of(f._id) + ".hp=" + render(f._hp) + " " + s_of(f._id) + ".hp_r=" + render(f._hp_r) +
         " " + s_of(f._id) + ".fall=" + render(f._fall) + " " + s_of(f._id) +
         ".defend=" + render(f._defend) + " " + s_of(f._id) + ".resting=" + render(f._resting) +
         " " + s_of(f._id) + ".tough=" + num(f._toughness) + " " + s_of(f._id) +
         ".catch_time=" + render(f._catch_time) + " " + s_of(f._id) + ".catching=" +
         (f._catching_entity == nullptr ? std::string("-") : s_of(f._catching_entity->_id)) +
         " " + s_of(f._id) + ".catcher=" +
         (f._catcher == nullptr ? std::string("-") : s_of(f._catcher->_id)) + " " + s_of(f._id) +
         ".shaking=" + render(f._shaking) + " " + s_of(f._id) + ".motionless=" +
         render(f._motionless) + " " + s_of(f._id) + ".vel=" + render(f._velocity[0]) + "/" +
         render(f._velocity[1]) + "/" + render(f._velocity[2]);
}

std::string state_text() {
  std::string s = side_text(g_att) + " " + side_text(g_vic);
  s += " inj=" + render(g_c.injury) + " inj_r=" + render(g_c.injury_r) +
       " rinj=" + render(g_c.real_injury) + " rinj_r=" + render(g_c.real_injury_r);
  s += " buff=" + (g_granted == nullptr
                       ? std::string("none")
                       : num(g_granted->lifetime()) + "/" + num(g_granted->duration()) + "/" +
                             num(g_granted->level()));
  return s;
}

void walk_side(Fake& f, const std::string& field, const std::vector<std::string>& tok, size_t& i) {
  if (field == "hp") {
    f._hp = parse_value(tok, i);
  } else if (field == "hp_r") {
    f._hp_r = parse_value(tok, i);
  } else if (field == "fall") {
    f._fall = parse_value(tok, i);
  } else if (field == "fall_max") {
    f._fall_max = parse_value(tok, i);
  } else if (field == "defend") {
    f._defend = parse_value(tok, i);
  } else if (field == "defend_max") {
    f._defend_max = parse_value(tok, i);
  } else if (field == "resting") {
    f._resting = parse_value(tok, i);
  } else if (field == "catch_max") {
    f._catch_time_max = parse_value(tok, i);
  } else if (field == "catching") {
    f._catching = tok[i++] == "1";
    f._catching_entity = f._catching ? &g_vic : nullptr;
  } else if (field == "catcher") {
    f._catcher = tok[i++] == "1" ? &g_att : nullptr;
  } else if (field == "marks") {
    f._marks_electrify = tok[i++] == "1";
  } else if (field == "elec_dur") {
    f._elec_dur = parse_value(tok, i);
  } else if (field == "itr_fall") {
    f._itr_fall = parse_value(tok, i);
  } else if (field == "hit_sounds") {
    f._hit_sounds = parse_value(tok, i);
  } else if (field == "motionless") {
    f._motionless = parse_value(tok, i);
  } else if (field == "src_emitter") {
    f._src_emitter = parse_value(tok, i);
  } else if (field == "ice") {
    f._ice = parse_value(tok, i);
  } else if (field == "type") {
    f.set_type(parse_value(tok, i));
  } else if (field == "weight") {
    f._weight = parse_value(tok, i);
  } else if (field == "state") {
    f._state = parse_value(tok, i);
  } else if (field == "face") {
    f._facing = parse_value(tok, i);
  } else if (field == "dataset") {
    f._dataset_values = parse_value(tok, i);
  } else {
    std::fprintf(stderr, "unknown side field '%s'\n", field.c_str());
    std::exit(2);
  }
}

void run(const std::string& name, const std::vector<std::string>& tok, size_t& i) {
  if (name == "injury") {
    double scale = 1;
    bool keep = false;
    if (i < tok.size()) scale = trace::to_double(tok[i++]);
    if (i < tok.size()) keep = tok[i++] == "1";
    lfw::collision::handle_injury(g_c, scale, keep);
  } else if (name == "catch") {
    lfw::collision::handle_itr_catch(g_c);
  } else if (name == "freeze") {
    lfw::collision::handle_itr_kind_freeze(g_c);
  } else if (name == "efreeze") {
    lfw::collision::handle_itr_effect_freeze(g_c);
  } else if (name == "shield") {
    lfw::collision::handle_john_shield_hit_other_ball(g_c);
  } else {
    std::fprintf(stderr, "unknown handler '%s'\n", name.c_str());
    std::exit(2);
  }
  std::printf("run %s || %s | %s\n", name.c_str(), join(g_log).c_str(), state_text().c_str());
}

}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: lfw_trace_collision_handlers2 <case-file>\n");
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
    g_granted = nullptr;
    if (op == "env") {
      const std::string& sub = t[i++];
      if (sub == "itr") {
        g_c.itr = parse_value(t, i);
      } else if (sub == "dataset") {
        g_c.dataset = parse_value(t, i);
        g_collision_dataset = g_c.dataset;
      } else if (sub == "rest") {
        g_c.rest = trace::to_double(t[i++]);
      } else if (sub == "recov") {
        g_recov = parse_value(t, i);
      } else if (sub == "fighter") {
        (void)t[i++];
      } else if (sub == "vel") {
        g_vel = parse_value(t, i);
      } else if (sub == "velz") {
        g_vel_z = parse_value(t, i);
      } else if (sub == "itr_motionless") {
        g_itr_motionless = parse_value(t, i);
      } else if (sub == "a" || sub == "v") {
        const std::string& field = t[i++];
        walk_side(sub == "a" ? g_att : g_vic, field, t, i);
      } else {
        std::fprintf(stderr, "unknown env '%s' at line %d\n", sub.c_str(), lineno);
        return 2;
      }
      std::printf("env %s\n", sub.c_str());
      continue;
    }
    if (op == "run") {
      run(t[i++], t, i);
      continue;
    }
    std::fprintf(stderr, "unknown op '%s' at line %d\n", op.c_str(), lineno);
    return 2;
  }
  return 0;
}
