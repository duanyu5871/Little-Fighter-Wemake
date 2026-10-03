#include <cstdio>
#include <fstream>
#include <memory>
#include <string>
#include <variant>
#include <vector>

#include "lfw/collision/handlers.h"
#include "lfw/collision/handlers2.h"
#include "lfw/collision/handlers3.h"
#include "lfw/core/value.h"
#include "lfw/entity/entity_type_check.h"
#include "lfw/utils/container_help/field_or.h"

#include "trace_util.h"

namespace {

using lfw::Array;
using lfw::Object;
using lfw::Value;
using lfw::collision::Collision;
using lfw::collision::Cube;
using lfw::collision::Handlers2Env;
using lfw::collision::Handlers3Env;
using lfw::collision::HandlersEnv;
using lfw::collision::IH3Entity;
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

bool absent(const Value& v) {
  return std::holds_alternative<std::monostate>(v) || std::holds_alternative<lfw::NullTag>(v);
}

Value g_dataset;

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

struct Fake : IH3Entity, IHandlerEntity {
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
  Value _in_the_sky;
  bool _catching = false;
  Fake* _catching_entity = nullptr;
  Fake* _catcher = nullptr;
  bool _marks_electrify = false;
  Value _elec_dur = Value(7.0);
  Value _dataset_values;
  Value _type = Value(8.0);
  Value _armor;
  Value _vel[3] = {Value(0.0), Value(0.0), Value(0.0)};
  Value _pos[3] = {Value(0.0), Value(0.0), Value(0.0)};
  Value _team;
  bool _bearer = false;
  Value _base_type;
  Value _state;
  double _toughness = 9;
  Value _tough_max = Value(9.0);
  FakeBuff _buff;

  explicit Fake(std::u16string id) : _id(std::move(id)), _buff(_id) {}

  const std::u16string& id() const override { return _id; }

  Value data() const override {
    auto indexes = std::make_shared<Object>();
    indexes->set(u"ice", _ice);
    indexes->set(u"in_the_skys",
                 Value(std::make_shared<Array>(std::vector<Value>{_in_the_sky})));
    auto base = std::make_shared<Object>();
    base->set(u"hit_sounds", _hit_sounds);
    auto o = std::make_shared<Object>();
    o->set(u"type", _type);
    o->set(u"indexes", Value(indexes));
    o->set(u"base", Value(base));
    return Value(o);
  }

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
  Value data_indexes_ice() const override { return _ice; }
  Value data_base_hit_sounds() const override { return _hit_sounds; }
  Value data_in_the_skys_first() const override { return _in_the_sky; }
  Value dataset(const std::u16string& key) const override {
    if (key == u"electrify_duration") return _elec_dur;
    const Value own = lfw::field_or(_dataset_values, key.c_str());
    if (!absent(own)) return own;
    return lfw::field_or(g_dataset, key.c_str());
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
  void set_motionless(const Value& v) override {
    _motionless = v;
    g_log.push_back(s_of(_id) + ":set_motionless:" + render(v));
  }

  void velocity(double& x, double& y, double& z) const override {
    x = lfw::to_number(_vel[0]);
    y = lfw::to_number(_vel[1]);
    z = lfw::to_number(_vel[2]);
  }
  void set_velocity(const Value& x, const Value& y, const Value& z) override {
    _vel[0] = x;
    _vel[1] = y;
    _vel[2] = z;
    g_log.push_back(s_of(_id) + ":set_velocity:" + render(x) + ":" + render(y) + ":" + render(z));
  }
  void position(double& x, double& y, double& z) const override {
    x = lfw::to_number(_pos[0]);
    y = lfw::to_number(_pos[1]);
    z = lfw::to_number(_pos[2]);
  }
  Value team() const override { return _team; }
  void set_team(const Value& v) override {
    _team = v;
    g_log.push_back(s_of(_id) + ":set_team:" + render(v));
  }
  bool has_bearer() const override { return _bearer; }
  Value base_type() const override { return _base_type; }
  Value state() const override { return _state; }
  Value armor() const override { return _armor; }
  Value toughness() const override { return Value(_toughness); }
  Value toughness_max() const override { return _tough_max; }
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
HandlersEnv g_henv;
Handlers2Env g_h2;
Handlers3Env g_h3;
lfw::buff::BuffEnv g_benv;
lfw::buff::Buff* g_granted = nullptr;
Value g_recov = Value(0.25);
Value g_itr_motionless;
bool g_armorwork = true;

Cube cube_of(const Value& v) {
  Cube c;
  c.left = lfw::to_number(lfw::field_or(v, u"left"));
  c.right = lfw::to_number(lfw::field_or(v, u"right"));
  c.bottom = lfw::to_number(lfw::field_or(v, u"bottom"));
  c.top = lfw::to_number(lfw::field_or(v, u"top"));
  c.near = lfw::to_number(lfw::field_or(v, u"near"));
  c.far = lfw::to_number(lfw::field_or(v, u"far"));
  return c;
}

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
    v.x = 0;
    v.y = 0;
    v.z = 0;
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
  g_henv.attacker_set_arest = [](double v) { g_log.push_back("A:set_arest:" + num(v)); };
  g_henv.victim_add_v_rest = [](Collision& c) {
    g_log.push_back("add_v_rest:" + s_of(c.vid) + ":" + num(c.rest));
  };
  g_henv.attacker_pick_victim = [](Collision&) {};
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

  g_h3.find_entity = [](const std::u16string& id) -> IH3Entity* {
    if (id == g_att._id) return &g_att;
    if (id == g_vic._id) return &g_vic;
    return nullptr;
  };
  g_h3.is_ball = [](const IH3Entity& e) {
    return lfw::entity::is_ball_data(static_cast<const Fake&>(e).data());
  };
  g_h3.is_weapon = [](const IH3Entity& e) {
    return lfw::entity::is_weapon_data(static_cast<const Fake&>(e).data());
  };
  g_h3.spark_point = [](const Cube& a, const Cube& b, double& x, double& y, double& z) {
    x = a.left;
    y = b.right;
    z = a.top;
  };
  g_h3.spark = [](double x, double y, double z, const Value& type) {
    g_log.push_back("spark:" + num(x) + ":" + num(y) + ":" + num(z) + ":" + render(type));
  };
  g_h3.play_sound_global = [](const Value& s, double x, double y, double z) {
    g_log.push_back("snd:" + render(s) + ":" + num(x) + ":" + num(y) + ":" + num(z));
  };
  g_h3.is_armor_work = [](Collision&) { return g_armorwork; };
  lfw::collision::set_handlers3_env(g_h3);

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
         " " + s_of(f._id) + ".tough=" + num(f._toughness) + " " + s_of(f._id) +
         ".tough_max=" + render(f._tough_max) + " " + s_of(f._id) + ".vel=" + render(f._vel[0]) +
         "/" + render(f._vel[1]) + "/" + render(f._vel[2]) + " " + s_of(f._id) +
         ".team=" + render(f._team) + " " + s_of(f._id) + ".state=" + render(f._state) + " " +
         s_of(f._id) + ".motionless=" + render(f._motionless) + " " + s_of(f._id) +
         ".shaking=" + render(f._shaking);
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
  } else if (field == "in_the_sky") {
    f._in_the_sky = parse_value(tok, i);
  } else if (field == "type") {
    f._type = parse_value(tok, i);
  } else if (field == "state") {
    f._state = parse_value(tok, i);
  } else if (field == "base_type") {
    f._base_type = parse_value(tok, i);
  } else if (field == "bearer") {
    f._bearer = tok[i++] == "1";
  } else if (field == "team") {
    f._team = parse_value(tok, i);
  } else if (field == "armor") {
    f._armor = parse_value(tok, i);
  } else if (field == "tough") {
    f._toughness = lfw::to_number(parse_value(tok, i));
  } else if (field == "tough_max") {
    f._tough_max = parse_value(tok, i);
  } else if (field == "velx") {
    f._vel[0] = parse_value(tok, i);
  } else if (field == "vely") {
    f._vel[1] = parse_value(tok, i);
  } else if (field == "velz") {
    f._vel[2] = parse_value(tok, i);
  } else if (field == "posx") {
    f._pos[0] = parse_value(tok, i);
  } else if (field == "posy") {
    f._pos[1] = parse_value(tok, i);
  } else if (field == "posz") {
    f._pos[2] = parse_value(tok, i);
  } else if (field == "dataset") {
    f._dataset_values = parse_value(tok, i);
  } else {
    std::fprintf(stderr, "unknown side field '%s'\n", field.c_str());
    std::exit(2);
  }
}

void run(const std::string& name, const std::vector<std::string>& tok, size_t& i) {
  if (name == "whirlwind") {
    lfw::collision::handle_itr_kind_whirlwind(g_c);
  } else if (name == "ballhit_a") {
    lfw::collision::handle_ball_is_hit_a(g_c);
  } else if (name == "ballhit_b") {
    lfw::collision::handle_ball_is_hit_b(g_c);
  } else if (name == "armor") {
    const bool r = lfw::collision::handle_armor(g_c);
    g_log.push_back(std::string("ret:") + (r ? "1" : "0"));
  } else {
    std::fprintf(stderr, "unknown handler '%s'\n", name.c_str());
    std::exit(2);
  }
  std::printf("run %s || %s | %s\n", name.c_str(), join(g_log).c_str(), state_text().c_str());
}

}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: lfw_trace_collision_handlers3 <case-file>\n");
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
        g_dataset = parse_value(t, i);
        g_c.dataset = g_dataset;
      } else if (sub == "rest") {
        g_c.rest = trace::to_double(t[i++]);
      } else if (sub == "recov") {
        g_recov = parse_value(t, i);
      } else if (sub == "itr_motionless") {
        g_itr_motionless = parse_value(t, i);
      } else if (sub == "armorwork") {
        g_armorwork = t[i++] == "1";
      } else if (sub == "acube") {
        g_c.a_cube = cube_of(parse_value(t, i));
      } else if (sub == "bcube") {
        g_c.b_cube = cube_of(parse_value(t, i));
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
      run(t[i++], t, i);
      continue;
    }
    std::fprintf(stderr, "unknown op '%s' at line %d\n", op.c_str(), lineno);
    return 2;
  }
  return 0;
}
