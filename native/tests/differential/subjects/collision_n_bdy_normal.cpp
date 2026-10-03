#include <cstdio>
#include <fstream>
#include <string>
#include <variant>
#include <vector>

#include "lfw/collision/fall.h"
#include "lfw/collision/handlers.h"
#include "lfw/collision/handlers2.h"
#include "lfw/collision/handlers3.h"
#include "lfw/collision/is_fall.h"
#include "lfw/collision/n_bdy_normal.h"
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
using lfw::collision::Handlers2Env;
using lfw::collision::Handlers3Env;
using lfw::collision::HandlersEnv;
using lfw::collision::IH3Entity;
using lfw::collision::IHandlerEntity;
using lfw::collision::INbdyNormalEntity;
using lfw::collision::ItrVelocity;
using lfw::collision::NbdyNormalEnv;
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

Value g_collision_dataset;

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

struct Fake : INbdyNormalEntity, IH3Entity {
  std::u16string _id;
  Value _hp = Value(50.0);
  Value _hp_r = Value(50.0);
  Value _fall = Value(30.0);
  Value _fall_max = Value(100.0);
  Value _defend = Value(3.0);
  Value _defend_max = Value(50.0);
  Value _resting = Value(0.0);
  Value _catch_time = Value(0.0);
  Value _catch_time_max = Value(30.0);
  Value _src_emitter;
  Value _shaking;
  Value _motionless;
  Value _itr_fall = Value(7.0);
  Value _hit_sounds;
  Value _ice;
  Value _elec_dur = Value(7.0);
  Value _state;
  Value _facing = Value(1.0);
  Value _fire;
  Value _crit;
  Value _holding;
  Value _team = Value(0.0);
  Value _base_type;
  Value _armor;
  Value _in_the_sky;
  Value _dizzy;
  Value _grand_injured;
  Value _injured;
  Value _backhurtact;
  Value _fronthurtact;
  bool _bearer = false;
  bool _catching = false;
  IHandlerEntity* _catcher = nullptr;
  double _pos[3] = {0, 0, 0};
  double _vel[3] = {0, 0, 0};
  double _ground_y = 0;
  double _toughness = 9;
  double _toughness_max = 9;
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
  Value toughness() const override { return Value(_toughness); }
  Value toughness_max() const override { return Value(_toughness_max); }
  Value armor() const override { return _armor; }
  Value data() const override {
    auto o = std::make_shared<lfw::Object>();
    o->set(u"type", lfw::field_or(_data, u"type"));
    auto idx = std::make_shared<lfw::Object>();
    idx->set(u"fire", _fire);
    idx->set(u"critical_hit", _crit);
    idx->set(u"dizzy", _dizzy);
    idx->set(u"grand_injured", _grand_injured);
    idx->set(u"injured", _injured);
    o->set(u"indexes", Value(idx));
    return Value(o);
  }
  Value data_indexes_ice() const override { return _ice; }
  Value data_indexes_fire() const override { return _fire; }
  Value data_indexes_critical_hit() const override { return _crit; }
  Value data_indexes_dizzy() const override { return _dizzy; }
  Value data_indexes_grand_injured() const override { return _grand_injured; }
  Value data_indexes_injured() const override { return _injured; }
  Value data_in_the_skys_first() const override { return lfw::field_or(_in_the_sky, u"0"); }
  Value data_base_hit_sounds() const override { return _hit_sounds; }
  Value dataset(const std::u16string& key) const override {
    if (key == u"electrify_duration") return _elec_dur;
    if (key == u"itr_shaking" || key == u"itr_motionless") {
      const Value own = lfw::field_or(_dataset_values, key.c_str());
      if (!std::holds_alternative<std::monostate>(own)) return own;
      return lfw::field_or(g_collision_dataset, key.c_str());
    }
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
  void set_motionless(const Value& v) override {
    _motionless = v;
    g_log.push_back(s_of(_id) + ":set_motionless:" + render(v));
  }

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
  void velocity(double& x, double& y, double& z) const override {
    x = _vel[0];
    y = _vel[1];
    z = _vel[2];
  }
  void position(double& x, double& y, double& z) const override {
    x = _pos[0];
    y = _pos[1];
    z = _pos[2];
  }
  double position_y() const override { return _pos[1]; }
  double ground_y() const override { return _ground_y; }
  void spark_point(const Cube& a, const Cube& b, double& x, double& y, double& z) override {
    x = a.left;
    y = b.top;
    z = a.near;
    g_log.push_back("sp:" + num(x) + ":" + num(y) + ":" + num(z));
  }
  Value holding_base_type() const override { return lfw::field_or(_holding, u"base_type"); }
  void drop_holding() override {
    _holding = Value();
    g_log.push_back(s_of(_id) + ":drop_holding");
  }
  Value cpoint_backhurtact() const override { return _backhurtact; }
  Value cpoint_fronthurtact() const override { return _fronthurtact; }

  Value team() const override { return _team; }
  void set_team(const Value& v) override {
    _team = v;
    g_log.push_back(s_of(_id) + ":set_team:" + render(v));
  }
  bool has_bearer() const override { return _bearer; }
  Value base_type() const override { return _base_type; }
  Value state() const override { return _state; }
};

Fake g_att(u"A");
Fake g_vic(u"V");
Collision g_c;
HandlersEnv g_henv;
Handlers2Env g_h2;
Handlers3Env g_h3;
FallEnv g_fall;
NbdyNormalEnv g_nb;
Value g_itr_motionless;
bool g_armor_work = false;
ItrVelocity g_iv;

Value is_fall_graph() {
  auto v = std::make_shared<lfw::Object>();
  v->set(u"fall_value", g_vic._fall);
  v->set(u"hp", g_vic._hp);
  v->set(u"is_on_ground", Value(g_vic._pos[1] <= g_vic._ground_y));
  auto fr = std::make_shared<lfw::Object>();
  fr->set(u"state", g_vic._state);
  v->set(u"frame", Value(fr));
  v->set(u"data", g_vic.data());
  auto c = std::make_shared<lfw::Object>();
  c->set(u"victim", Value(v));
  return Value(c);
}

ItrVelocity seam_velocity(Collision&) {
  ItrVelocity r = g_iv;
  const Value effect = lfw::field_or(g_c.itr, u"effect");
  const bool position_based =
      lfw::strict_equals(effect, Value(static_cast<double>(lfw::ItrEffect::FireExplosion))) ||
      lfw::strict_equals(effect, Value(static_cast<double>(lfw::ItrEffect::Explosion))) ||
      lfw::strict_equals(g_att._state,
                         Value(static_cast<double>(lfw::StateEnum::HeavyWeapon_InTheSky)));
  const double direction = position_based ? -1.0 : lfw::to_number(g_att._facing);
  r.x = g_iv.x * direction;
  r.y = lfw::collision::is_fall(is_fall_graph()) ? g_iv.y : 0.0;
  r.x_direction = Value(direction);
  return r;
}

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
  g_h2.hp_recoverability = []() { return lfw::field_or(g_collision_dataset, u"hp_recoverability"); };
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
  g_h2.buff_env = []() { return static_cast<const lfw::buff::BuffEnv*>(nullptr); };
  g_h2.is_fighter = [](const IHandlerEntity& e) {
    return lfw::entity::is_fighter_data(e.data());
  };
  g_h2.calc_velocity = seam_velocity;
  lfw::collision::set_handlers2_env(g_h2);

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
    y = b.top;
    z = a.near;
    g_log.push_back("sp:" + num(x) + ":" + num(y) + ":" + num(z));
  };
  g_h3.spark = [](double x, double y, double z, const Value& type) {
    g_log.push_back("spark:" + num(x) + ":" + num(y) + ":" + num(z) + ":" + render(type));
  };
  g_h3.play_sound_global = [](const Value& sounds, double x, double y, double z) {
    g_log.push_back("snd3:" + render(sounds) + ":" + num(x) + ":" + num(y) + ":" + num(z));
  };
  g_h3.is_armor_work = [](Collision&) { return g_armor_work; };
  lfw::collision::set_handlers3_env(g_h3);

  g_fall.find_entity = [](const std::u16string& id) -> lfw::collision::IFallEntity* {
    if (id == g_att._id) return &g_att;
    if (id == g_vic._id) return &g_vic;
    return nullptr;
  };
  g_fall.is_fighter = [](const lfw::collision::IFallEntity& e) {
    return lfw::entity::is_fighter_data(e.data());
  };
  g_fall.spark = [](const Value& x, const Value& y, const Value& z, const Value& type) {
    g_log.push_back("spark:" + render(x) + ":" + render(y) + ":" + render(z) + ":" + render(type));
  };
  g_fall.calc_velocity = seam_velocity;
  lfw::collision::set_fall_env(g_fall);

  g_nb.find_entity = [](const std::u16string& id) -> INbdyNormalEntity* {
    if (id == g_att._id) return &g_att;
    if (id == g_vic._id) return &g_vic;
    return nullptr;
  };
  g_nb.is_fighter = [](const INbdyNormalEntity& e) {
    return lfw::entity::is_fighter_data(e.data());
  };
  g_nb.is_fall = [](Collision&) { return lfw::collision::is_fall(is_fall_graph()); };
  g_nb.spark = [](const Value& x, const Value& y, const Value& z, const Value& type) {
    g_log.push_back("spark:" + render(x) + ":" + render(y) + ":" + render(z) + ":" + render(type));
  };
  g_nb.calc_velocity = seam_velocity;
  lfw::collision::set_nbdy_normal_env(g_nb);
}

std::string side_text(const Fake& f) {
  return s_of(f._id) + ".hp=" + render(f._hp) + " " + s_of(f._id) + ".hp_r=" + render(f._hp_r) +
         " " + s_of(f._id) + ".tough=" + num(f._toughness) + "/" + num(f._toughness_max) + " " +
         s_of(f._id) + ".armor=" + render(f._armor) + " " + s_of(f._id) +
         ".state=" + render(f._state) + " " + s_of(f._id) + ".face=" + render(f._facing) + " " +
         s_of(f._id) + ".fall=" + render(f._fall) + " " + s_of(f._id) +
         ".fall_max=" + render(f._fall_max) + " " + s_of(f._id) + ".defend=" + render(f._defend) +
         " " + s_of(f._id) + ".resting=" + render(f._resting) + " " + s_of(f._id) +
         ".vel=" + num(f._vel[0]) + "/" + num(f._vel[1]) + "/" + num(f._vel[2]);
}

std::string state_text() {
  std::string s = side_text(g_att) + " " + side_text(g_vic);
  s += " inj=" + render(g_c.injury) + " inj_r=" + render(g_c.injury_r) +
       " rinj=" + render(g_c.real_injury);
  return s;
}

void walk_side(Fake& f, const std::string& field, const std::vector<std::string>& tok, size_t& i) {
  if (field == "hp") {
    f._hp = parse_value(tok, i);
  } else if (field == "hp_r") {
    f._hp_r = parse_value(tok, i);
  } else if (field == "tough") {
    f._toughness = trace::to_double(tok[i++]);
  } else if (field == "tough_max") {
    f._toughness_max = trace::to_double(tok[i++]);
  } else if (field == "armor") {
    f._armor = parse_value(tok, i);
  } else if (field == "state") {
    f._state = parse_value(tok, i);
  } else if (field == "face") {
    f._facing = parse_value(tok, i);
  } else if (field == "team") {
    f._team = parse_value(tok, i);
  } else if (field == "base_type") {
    f._base_type = parse_value(tok, i);
  } else if (field == "bearer") {
    f._bearer = tok[i++] == "1";
  } else if (field == "type") {
    f.set_type(parse_value(tok, i));
  } else if (field == "velx") {
    f._vel[0] = trace::to_double(tok[i++]);
  } else if (field == "vely") {
    f._vel[1] = trace::to_double(tok[i++]);
  } else if (field == "velz") {
    f._vel[2] = trace::to_double(tok[i++]);
  } else if (field == "posx") {
    f._pos[0] = trace::to_double(tok[i++]);
  } else if (field == "posy") {
    f._pos[1] = trace::to_double(tok[i++]);
  } else if (field == "posz") {
    f._pos[2] = trace::to_double(tok[i++]);
  } else if (field == "ground_y") {
    f._ground_y = trace::to_double(tok[i++]);
  } else if (field == "fall") {
    f._fall = parse_value(tok, i);
  } else if (field == "fall_max") {
    f._fall_max = parse_value(tok, i);
  } else if (field == "defend") {
    f._defend = parse_value(tok, i);
  } else if (field == "itr_fall") {
    f._itr_fall = parse_value(tok, i);
  } else if (field == "resting") {
    f._resting = parse_value(tok, i);
  } else if (field == "fire") {
    f._fire = parse_value(tok, i);
  } else if (field == "crit") {
    f._crit = parse_value(tok, i);
  } else if (field == "dizzy") {
    f._dizzy = parse_value(tok, i);
  } else if (field == "grand_injured") {
    f._grand_injured = parse_value(tok, i);
  } else if (field == "injured") {
    f._injured = parse_value(tok, i);
  } else if (field == "backhurtact") {
    f._backhurtact = parse_value(tok, i);
  } else if (field == "fronthurtact") {
    f._fronthurtact = parse_value(tok, i);
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
  } else if (field == "in_the_sky") {
    f._in_the_sky = parse_value(tok, i);
  } else if (field == "dataset") {
    f._dataset_values = parse_value(tok, i);
  } else {
    std::fprintf(stderr, "unknown side field '%s'\n", field.c_str());
    std::exit(2);
  }
}

void run() {
  lfw::collision::handle_itr_normal_bdy_normal(g_c);
  std::printf("run hit || %s | %s\n", join(g_log).c_str(), state_text().c_str());
}

}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: lfw_trace_collision_n_bdy_normal <case-file>\n");
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
        g_collision_dataset = g_c.dataset;
      } else if (sub == "rest") {
        g_c.rest = trace::to_double(t[i++]);
      } else if (sub == "itr_motionless") {
        g_itr_motionless = parse_value(t, i);
      } else if (sub == "bframe") {
        g_c.bframe = parse_value(t, i);
      } else if (sub == "aframe") {
        g_c.aframe = parse_value(t, i);
      } else if (sub == "armorwork") {
        g_armor_work = t[i++] == "1";
      } else if (sub == "velx") {
        g_iv.x = trace::to_double(t[i++]);
      } else if (sub == "vely") {
        g_iv.y = trace::to_double(t[i++]);
      } else if (sub == "velz") {
        g_iv.z = trace::to_double(t[i++]);
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
