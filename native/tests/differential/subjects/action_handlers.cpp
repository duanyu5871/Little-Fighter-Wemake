#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

#include "lfw/collision/action_handlers.h"
#include "lfw/core/value.h"
#include "lfw/utils/container_help/field_or.h"
#include "lfw/utils/math/base.h"

#include "trace_util.h"

namespace {

using lfw::Value;
using lfw::collision::ActionEnv;
using lfw::collision::IActionEntity;
using trace::parse_value;
using trace::render_value;
using trace::split_ws;
using trace::to_ascii;

std::vector<std::string> g_log;
lfw::Value g_injury;
lfw::Value g_real_injury;

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

std::string ids_of(const Value& v) {
  const lfw::Array* a = lfw::as_array(v);
  if (a == nullptr) return render(v);
  std::string out;
  for (size_t i = 0; i < a->size(); ++i) {
    if (i) out += ",";
    out += to_ascii(lfw::to_string(a->at(i)));
  }
  return out;
}

struct FakeBuff : lfw::buff::IBuffEntity {
  std::u16string _id;
  bool attach_on = false;
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
  void attach(bool on) override { attach_on = on; }
};

struct Fake : IActionEntity {
  std::u16string _id;
  Value _data;
  Value _data_type;
  Value _velocity_x = Value(3.0);
  Value _facing = Value(1.0);
  Value _team;
  Value _hp = Value(10.0);
  Value _hp_r = Value(10.0);
  Value _hp_max = Value(20.0);
  Value _mp = Value(5.0);
  Value _mp_max = Value(15.0);
  bool _bot = false;
  Value _src_emitter;
  Value _emitter;
  Fake* _bearer = nullptr;
  Value _fuse_bys;
  Value _dismiss_data;
  Value _dismiss_time = Value(lfw::NullTag{});
  double _invisible = 0;
  double _motionless = 0;
  double _invulnerable = 0;
  Value _props;
  Value _position;
  FakeBuff _buff;

  explicit Fake(std::u16string id) : _id(std::move(id)), _buff(_id) {
    auto pos = std::make_shared<lfw::Object>();
    pos->set(u"x", Value(0.0));
    pos->set(u"y", Value(0.0));
    pos->set(u"z", Value(0.0));
    _position = Value(pos);
  }

  const std::u16string& id() const override { return _id; }
  Value data() const override { return _data; }
  Value data_type() const override { return lfw::field_or(_data, u"type"); }
  Value pos_or_self(const Value& pos) const {
    return std::holds_alternative<std::monostate>(pos) ? _position : pos;
  }
  Value velocity_x() const override { return _velocity_x; }
  void set_velocity_x(const Value& v) override {
    _velocity_x = v;
    g_log.push_back("set_velocity:" + render(v));
  }
  Value facing() const override { return _facing; }
  void set_facing(const Value& v) override {
    _facing = v;
    g_log.push_back("set_facing:" + render(v));
  }
  Value team() const override { return _team; }
  void set_team(const Value& v) override {
    _team = v;
    g_log.push_back("set_team:" + render(v));
  }
  Value hp() const override { return _hp; }
  void set_hp(const Value& v) override {
    _hp = v;
    g_log.push_back("set_hp:" + render(v));
  }
  Value hp_r() const override { return _hp_r; }
  void set_hp_r(const Value& v) override {
    _hp_r = v;
    g_log.push_back("set_hp_r:" + render(v));
  }
  Value hp_max() const override { return _hp_max; }
  Value mp() const override { return _mp; }
  void set_mp(const Value& v) override {
    _mp = v;
    g_log.push_back("set_mp:" + render(v));
  }
  Value mp_max() const override { return _mp_max; }
  bool is_bot_ctrl() const override { return _bot; }
  Value src_emitter() const override { return _src_emitter; }
  Value emitter() const override { return _emitter; }
  IActionEntity* bearer() override { return _bearer; }
  Value fuse_bys() const override { return _fuse_bys; }
  void set_fuse_bys(const Value& v) override {
    _fuse_bys = v;
    g_log.push_back("set_fuse_bys:" + ids_of(v));
  }
  void set_dismiss_data(const Value& v) override {
    _dismiss_data = v;
    g_log.push_back("set_dismiss_data:" + render(v));
  }
  void set_dismiss_time(const Value& v) override {
    _dismiss_time = v;
    g_log.push_back("set_dismiss_time:" + render(v));
  }
  void set_invisible(double v) override {
    _invisible = v;
    g_log.push_back("set_invisible:" + num(v));
  }
  void set_motionless(double v) override {
    _motionless = v;
    g_log.push_back("set_motionless:" + num(v));
  }
  void set_invulnerable(double v) override {
    _invulnerable = v;
    g_log.push_back("set_invulnerable:" + num(v));
  }
  void play_sound(const Value& sounds, const Value& pos) override {
    g_log.push_back(s_of(_id) + ":play_sound:" + render(sounds) + ":" + render(pos_or_self(pos)));
  }
  void enter_frame(const Value& info) override {
    g_log.push_back(s_of(_id) + ":enter_frame:" + render(info));
  }
  void transform(const Value& d) override { g_log.push_back(s_of(_id) + ":transform:" + render(d)); }
  void set_prop(const std::u16string& name, const Value& v) override {
    if (name == u"hp") {
      set_hp(v);
      return;
    }
    if (name == u"hp_r") {
      set_hp_r(v);
      return;
    }
    if (name == u"mp") {
      set_mp(v);
      return;
    }
    if (name == u"facing") {
      set_facing(v);
      return;
    }
    if (name == u"team") {
      set_team(v);
      return;
    }
    _props = v;
  }
  lfw::buff::IBuffEntity* buff_entity() override { return &_buff; }
};

Fake g_a(u"A");
Fake g_v(u"V");
Fake g_other(u"E2");
ActionEnv g_env;
lfw::buff::BuffEnv g_benv;
lfw::Value g_data_found;
bool g_has_data = false;
bool g_ally = false;
lfw::buff::Buff* g_granted = nullptr;

bool missing(const Value& v) {
  return std::holds_alternative<std::monostate>(v) || std::holds_alternative<lfw::NullTag>(v);
}

void bind() {
  g_env.mt_int = []() { return 1.0; };
  g_env.mt_set_mark = [](const std::u16string& m) { g_log.push_back("mt_mark:" + s_of(m)); };
  g_env.find_data = [](const std::u16string& oid, Value& out) {
    g_log.push_back("find_data:" + s_of(oid));
    if (!g_has_data) return false;
    out = g_data_found;
    return true;
  };
  g_env.broadcast = [](const std::u16string& m) { g_log.push_back("broadcast:" + s_of(m)); };
  g_env.alert = [](const std::u16string& m) { g_log.push_back("alert:" + s_of(m)); };
  g_env.find_entity = [](const std::u16string& id) -> IActionEntity* {
    g_log.push_back("find_entity:" + s_of(id));
    if (id == g_v._id) return &g_v;
    if (id == g_other._id) return &g_other;
    return nullptr;
  };
  g_env.is_ally = [](const IActionEntity&, const IActionEntity&) {
    g_log.push_back(std::string("is_ally:") + (g_ally ? "1" : "0"));
    return g_ally;
  };
  g_env.buff_env = []() { return &g_benv; };
  g_benv.find_entity = [](const std::u16string& id) -> lfw::buff::IBuffEntity* {
    if (id == g_a._id) return &g_a._buff;
    if (id == g_v._id) return &g_v._buff;
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

std::string state_text() {
  return "a.hp=" + render(g_a._hp) + " a.hp_r=" + render(g_a._hp_r) + " a.mp=" + render(g_a._mp) +
         " a.vel=" + render(g_a._velocity_x) + " a.face=" + render(g_a._facing) +
         " a.team=" + render(g_a._team) + " v.hp=" + render(g_v._hp) + " v.hp_r=" +
         render(g_v._hp_r) + " v.mp=" + render(g_v._mp) + " v.vel=" + render(g_v._velocity_x) +
         " v.face=" + render(g_v._facing) + " v.team=" + render(g_v._team) + " v.inv=" +
         num(g_v._invisible) + "/" + num(g_v._motionless) + "/" + num(g_v._invulnerable) +
         " e2.hp=" +
         render(g_other._hp) + " e2.hp_r=" + render(g_other._hp_r) + " e2.mp=" +
         render(g_other._mp) + " a.fuse=" + ids_of(g_a._fuse_bys) + " a.dismiss=" +
         render(g_a._dismiss_data) + " a.dtime=" + render(g_a._dismiss_time) + " a.bot=" +
         (g_a._bot ? "1" : "0") + " v.bot=" + (g_v._bot ? "1" : "0") + " v.fuse=" +
         ids_of(g_v._fuse_bys) + " v.dismiss=" + render(g_v._dismiss_data) + " v.dtime=" +
         render(g_v._dismiss_time) + " buff=" +
         (g_granted == nullptr
              ? std::string("none")
              : (num(g_granted->lifetime()) + "/" + num(g_granted->duration()) + "/" +
                 num(g_granted->level())));
}

std::string flag_result(const Value& v) {
  return "ret=" + render(v);
}

}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: lfw_trace_action_handlers <case-file>\n");
    return 2;
  }
  bind();

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
      if (sub == "injury") {
        g_injury = parse_value(t, i);
      } else if (sub == "real_injury") {
        g_real_injury = parse_value(t, i);
      } else if (sub == "data_found") {
        g_data_found = parse_value(t, i);
        g_has_data = true;
      } else if (sub == "no_data") {
        g_has_data = false;
      } else if (sub == "ally") {
        g_ally = t[i++] == "1";
      } else if (sub == "mt_int") {
        const double mt = trace::to_double(t[i++]);
        g_env.mt_int = [mt]() { return mt; };
      } else if (sub == "a" || sub == "v" || sub == "e") {
        Fake& f = sub == "a" ? g_a : (sub == "v" ? g_v : g_other);
        const std::string& field = t[i++];
        if (field == "hp") {
          f._hp = parse_value(t, i);
        } else if (field == "hp_r") {
          f._hp_r = parse_value(t, i);
        } else if (field == "hp_max") {
          f._hp_max = parse_value(t, i);
        } else if (field == "mp") {
          f._mp = parse_value(t, i);
        } else if (field == "mp_max") {
          f._mp_max = parse_value(t, i);
        } else if (field == "vel") {
          f._velocity_x = parse_value(t, i);
        } else if (field == "face") {
          f._facing = parse_value(t, i);
        } else if (field == "team") {
          f._team = parse_value(t, i);
        } else if (field == "data") {
          f._data = parse_value(t, i);
        } else if (field == "bot") {
          f._bot = t[i++] == "1";
        } else if (field == "src_emitter") {
          f._src_emitter = parse_value(t, i);
        } else if (field == "emitter") {
          f._emitter = parse_value(t, i);
        } else if (field == "bearer") {
          f._bearer = t[i++] == "1" ? &g_other : nullptr;
        } else if (field == "fuse_bys") {
          f._fuse_bys = parse_value(t, i);
        } else {
          std::fprintf(stderr, "unknown entity field '%s' at line %d\n", field.c_str(), lineno);
          return 2;
        }
      } else {
        std::fprintf(stderr, "unknown env '%s' at line %d\n", sub.c_str(), lineno);
        return 2;
      }
      std::printf("env %s\n", sub.c_str());
      continue;
    }
    if (op == "act") {
      const std::u16string type = trace::parse_js_string_literal(t[i++]);
      const Value action = parse_value(t, i);
      const Value ret = lfw::collision::run_action(g_env, type, action, g_a, g_v, g_injury,
                                                   g_real_injury);
      std::printf("act %s %s || %s | %s\n", s_of(type).c_str(), flag_result(ret).c_str(),
                  join(g_log).c_str(), state_text().c_str());
      continue;
    }
    std::fprintf(stderr, "unknown op '%s' at line %d\n", op.c_str(), lineno);
    return 2;
  }
  return 0;
}
