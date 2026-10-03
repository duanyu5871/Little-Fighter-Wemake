#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

#include "lfw/collision/handlers4.h"
#include "lfw/core/value.h"
#include "lfw/entity/entity_type_check.h"
#include "lfw/utils/container_help/field_or.h"

#include "trace_util.h"

namespace {

using lfw::Value;
using lfw::collision::Collision;
using lfw::collision::Handlers4Env;
using lfw::collision::HandlersEnv;
using lfw::collision::IH4Entity;
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

struct Fake : IH4Entity {
  std::u16string _id;
  Value _hp = Value(20.0);
  Value _hp_r = Value(30.0);
  Value _state;
  Value _facing = Value(1.0);
  Value _base_type;
  double _vel[3] = {0, 0, 0};
  Value _frame_id = Value(u"f1");
  Value _throwings;
  Value _in_the_skys;
  Value _arest;
  bool _dropping = false;
  Value _hit_sounds;
  Value _type = Value(8.0);

  explicit Fake(std::u16string id) : _id(std::move(id)) {}

  const std::u16string& id() const override { return _id; }

  Value data() const override {
    auto base = std::make_shared<lfw::Object>();
    base->set(u"hit_sounds", _hit_sounds);
    auto o = std::make_shared<lfw::Object>();
    o->set(u"type", _type);
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
  Value state() const override { return _state; }
  Value facing() const override { return _facing; }
  Value base_type() const override { return _base_type; }

  void velocity(double& x, double& y, double& z) const override {
    x = _vel[0];
    y = _vel[1];
    z = _vel[2];
  }
  void set_velocity(const Value& x, const Value& y, const Value& z) override {
    _vel[0] = lfw::to_number(x);
    _vel[1] = lfw::to_number(y);
    _vel[2] = lfw::to_number(z);
    g_log.push_back(s_of(_id) + ":set_velocity:" + render(x) + ":" + render(y) + ":" + render(z));
  }

  Value frame_id() const override { return _frame_id; }
  Value data_indexes_throwings() const override { return _throwings; }
  Value data_indexes_in_the_skys() const override { return _in_the_skys; }
  Value arest() const override { return _arest; }
  void set_arest(const Value& v) override {
    _arest = v;
    g_log.push_back(s_of(_id) + ":set_arest:" + render(v));
  }
  void enter_frame(const Value& info) override {
    g_log.push_back(s_of(_id) + ":enter_frame:" + render(info));
  }
  void set_dropping(bool v) override {
    _dropping = v;
    g_log.push_back(s_of(_id) + ":set_dropping:" + (v ? "1" : "0"));
  }
  Value data_base_hit_sounds() const override { return _hit_sounds; }
  void play_sound(const Value& sounds) override {
    g_log.push_back(s_of(_id) + ":play_sound:" + render(sounds));
  }
};

Fake g_att(u"A");
Fake g_vic(u"V");
Collision g_c;
HandlersEnv g_henv;
Handlers4Env g_h4;
Value g_dataset;
Value g_itr_motionless;

void bind() {
  g_henv.attacker_itr_motionless = []() { return g_itr_motionless; };
  g_henv.attacker_set_motionless = [](const Value& v) {
    g_log.push_back("A:set_motionless:" + render(v));
  };
  g_henv.victim_set_shaking = [](const Value& v) { g_log.push_back("V:set_shaking:" + render(v)); };
  g_henv.attacker_set_arest = [](double v) {
    g_att._arest = Value(v);
    g_log.push_back("A:set_arest:" + num(v));
  };
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

  g_h4.find_entity = [](const std::u16string& id) -> IH4Entity* {
    if (id == g_att._id) return &g_att;
    if (id == g_vic._id) return &g_vic;
    return nullptr;
  };
  g_h4.is_fighter = [](const IH4Entity& e) {
    return lfw::entity::is_fighter_data(e.data());
  };
  g_h4.find_align_frame = [](const Value& fid, const Value& thr, const Value& sky) {
    g_log.push_back("faf:" + render(fid) + ":" + render(thr) + ":" + render(sky));
    return Value(u"faf_result");
  };
  lfw::collision::set_handlers4_env(g_h4);
}

std::string side_text(const Fake& f) {
  return s_of(f._id) + ".hp=" + render(f._hp) + " " + s_of(f._id) + ".hp_r=" + render(f._hp_r) +
         " " + s_of(f._id) + ".state=" + render(f._state) + " " + s_of(f._id) +
         ".facing=" + render(f._facing) + " " + s_of(f._id) +
         ".base_type=" + render(f._base_type) + " " + s_of(f._id) + ".vel=" + num(f._vel[0]) +
         "/" + num(f._vel[1]) + "/" + num(f._vel[2]) + " " + s_of(f._id) +
         ".arest=" + render(f._arest) + " " + s_of(f._id) +
         ".dropping=" + (f._dropping ? "1" : "0") + " " + s_of(f._id) +
         ".frame_id=" + render(f._frame_id);
}

std::string state_text() { return side_text(g_att) + " " + side_text(g_vic); }

void walk_side(Fake& f, const std::string& field, const std::vector<std::string>& tok, size_t& i) {
  if (field == "hp") {
    f._hp = parse_value(tok, i);
  } else if (field == "hp_r") {
    f._hp_r = parse_value(tok, i);
  } else if (field == "state") {
    f._state = parse_value(tok, i);
  } else if (field == "facing") {
    f._facing = parse_value(tok, i);
  } else if (field == "base_type") {
    f._base_type = parse_value(tok, i);
  } else if (field == "velx") {
    f._vel[0] = lfw::to_number(parse_value(tok, i));
  } else if (field == "vely") {
    f._vel[1] = lfw::to_number(parse_value(tok, i));
  } else if (field == "velz") {
    f._vel[2] = lfw::to_number(parse_value(tok, i));
  } else if (field == "frame_id") {
    f._frame_id = parse_value(tok, i);
  } else if (field == "throwings") {
    f._throwings = parse_value(tok, i);
  } else if (field == "in_the_sky") {
    f._in_the_skys = parse_value(tok, i);
  } else if (field == "arest") {
    f._arest = parse_value(tok, i);
  } else if (field == "dropping") {
    f._dropping = tok[i++] == "1";
  } else if (field == "hit_sounds") {
    f._hit_sounds = parse_value(tok, i);
  } else if (field == "type") {
    f._type = parse_value(tok, i);
  } else {
    std::fprintf(stderr, "unknown side field '%s'\n", field.c_str());
    std::exit(2);
  }
}

void run(const std::string& name) {
  if (name == "ballhitother") {
    lfw::collision::handle_ball_hit_other(g_c);
  } else if (name == "weaponhitother") {
    lfw::collision::handle_weapon_hit_other(g_c);
  } else {
    std::fprintf(stderr, "unknown handler '%s'\n", name.c_str());
    std::exit(2);
  }
  std::printf("run %s || %s | %s\n", name.c_str(), join(g_log).c_str(), state_text().c_str());
}

}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: lfw_trace_collision_handlers4 <case-file>\n");
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
      } else if (sub == "bdy") {
        g_c.bdy = parse_value(t, i);
      } else if (sub == "aframe") {
        g_c.aframe = parse_value(t, i);
      } else if (sub == "dataset") {
        g_dataset = parse_value(t, i);
        g_c.dataset = g_dataset;
      } else if (sub == "rest") {
        g_c.rest = trace::to_double(t[i++]);
      } else if (sub == "itr_motionless") {
        g_itr_motionless = parse_value(t, i);
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
