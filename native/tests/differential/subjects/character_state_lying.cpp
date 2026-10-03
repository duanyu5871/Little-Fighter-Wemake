#include <cstdio>
#include <fstream>
#include <memory>
#include <string>
#include <variant>
#include <vector>

#include "lfw/buff/buff.h"
#include "lfw/core/value.h"
#include "lfw/state/character_state_lying.h"
#include "lfw/utils/container_help/field_or.h"

#include "trace_util.h"

namespace {

using lfw::Array;
using lfw::Object;
using lfw::Value;
using lfw::buff::Buff;
using lfw::buff::BuffEnv;
using lfw::state::CharacterState_Lying;
using lfw::state::IStateEntity;
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

std::u16string value_text(const Value& v) {
  const std::u16string* s = std::get_if<std::u16string>(&v);
  return s != nullptr ? *s : std::u16string();
}

struct FakeEnt : IStateEntity {
  std::u16string _id;
  double _px = 0;
  double _py = 0;
  double _pz = 0;
  Value _gy;
  Value _frameid;
  Value _hp;
  Value _hpr;
  Value _hpmax;
  bool _holding = false;
  Value _holdtype;
  Value _holdteam;
  Value _team;
  Value _tough;
  Value _tmax;
  Value _trest;
  Value _la;
  Value _ld;
  Value _lc;
  Value _wait;
  std::u16string _held;
  Value _dvals;
  Value _deadjoin;
  Value _deadgone;
  Value _reserve;
  Value _wakeup;
  Value _motionless;
  Value _invul;
  Value _blink;
  std::u16string _outline;
  Value _puppets;
  Value _state;

  explicit FakeEnt(std::u16string id) : _id(std::move(id)) {}

  const std::u16string& id() const override { return _id; }
  void position(double& x, double& y, double& z) const override {
    x = _px;
    y = _py;
    z = _pz;
  }
  void set_position(double x, double y, double z) override {
    _px = x;
    _py = y;
    _pz = z;
  }
  double frame_centery() const override { return 0; }
  double frame_height() const override { return 0; }
  double frame_pic_h() const override { return 0; }
  void set_frame(const Value& info) override {
    g_log.push_back(s_of(_id) + ":set_frame:" + render(lfw::field_or(info, u"id")));
  }
  void buffs_set(const std::u16string& key, Buff*) override {
    g_log.push_back(s_of(_id) + ":buffs_set:" + s_of(key));
  }
  void buffs_delete(const std::u16string& key) override {
    g_log.push_back(s_of(_id) + ":buffs_delete:" + s_of(key));
  }
  void set_outline_alpha(double v) override {
    g_log.push_back(s_of(_id) + ":outline_alpha:" + num(v));
  }
  void set_outline_width(double v) override {
    g_log.push_back(s_of(_id) + ":outline_width:" + num(v));
  }
  void set_outline_color(const std::u16string& v) override { _outline = v; }
  void enter_frame_by_id(const std::u16string& id) override {
    g_log.push_back(s_of(_id) + ":enter_frame_by_id:" + render(Value(id)));
  }
  void attach(bool on) override {
    g_log.push_back(s_of(_id) + ":attach:" + std::string(on ? "1" : "0"));
  }
  Value velocity_x() const override { return Value(); }
  Value velocity_z() const override { return Value(); }
  Value ground_y() const override { return _gy; }
  Value frame_info() const override {
    Object o;
    o.set(u"id", _frameid);
    return Value(std::make_shared<Object>(o));
  }
  Value hp() const override { return _hp; }
  Value hp_r() const override { return _hpr; }
  Value hp_max() const override { return _hpmax; }
  void set_hp(const Value& v) override { _hp = v; }
  void set_hp_r(const Value& v) override { _hpr = v; }
  void set_hp_max(const Value& v) override { _hpmax = v; }
  Value team() const override { return _team; }
  void set_team(const Value& v) override { _team = v; }
  Value state() const override { return _state; }
  Value wait() const override { return _wait; }
  void set_wait(const Value& v) override { _wait = v; }
  bool has_holding() const override { return _holding; }
  Value holding_base_type() const override { return _holdtype; }
  void holding_set_team(const Value& v) override {
    g_log.push_back(s_of(_id) + ":holding_set_team:" + render(v));
    _holdteam = v;
  }
  void drop_holding() override { g_log.push_back(s_of(_id) + ":drop_holding"); }
  Value toughness_max() const override { return _tmax; }
  void set_toughness(const Value& v) override { _tough = v; }
  void set_toughness_resting(const Value& v) override { _trest = v; }
  Value lying_a_count() const override { return _la; }
  void set_lying_a_count(const Value& v) override { _la = v; }
  Value lying_d_count() const override { return _ld; }
  void set_lying_d_count(const Value& v) override { _ld = v; }
  Value lying_c_count() const override { return _lc; }
  void set_lying_c_count(const Value& v) override { _lc = v; }
  void ctrl_reset_key_list() override { g_log.push_back(s_of(_id) + ":ctrl_reset_key_list"); }
  bool ctrl_is_end(const std::u16string& key) const override {
    g_log.push_back(s_of(_id) + ":ctrl_is_end:" + s_of(key));
    return _held.find(key[0]) == std::u16string::npos;
  }
  Value dead_join() const override { return _deadjoin; }
  void set_dead_join(const Value& v) override { _deadjoin = v; }
  Value dead_gone() const override { return _deadgone; }
  Value reserve() const override { return _reserve; }
  void set_reserve(const Value& v) override { _reserve = v; }
  Value wakeup_invuln() const override { return _wakeup; }
  void set_wakeup_invuln(const Value& v) override { _wakeup = v; }
  void set_invulnerable(const Value& v) override { _invul = v; }
  void set_blinking(const Value& v) override { _blink = v; }
  void set_motionless(const Value& v) override { _motionless = v; }
  void blink_and_respawn(const Value& duration) override {
    g_log.push_back(s_of(_id) + ":blink_and_respawn:" + render(duration));
  }
  void blink_and_gone(const Value& duration) override {
    g_log.push_back(s_of(_id) + ":blink_and_gone:" + render(duration));
  }
  Value world_puppets() const override {
    const Array* arr = as_array(_puppets);
    if (arr == nullptr) return Value();
    Array out;
    for (size_t i = 0; i < arr->size(); ++i) {
      Object entry;
      entry.set(u"team", arr->at(i));
      out.push_back(Value(std::make_shared<Object>(entry)));
    }
    return Value(std::make_shared<Array>(std::move(out)));
  }
  void world_etc(double x, double y, double z, const std::u16string& kind) override {
    g_log.push_back(s_of(_id) + ":world_etc:" + num(x) + ":" + num(y) + ":" + num(z) + ":" +
                    render(Value(kind)));
  }
  Value world_dataset(const std::u16string& key) const override {
    g_log.push_back(s_of(_id) + ":world_dataset:" + s_of(key));
    return lfw::field_or(_dvals, key.c_str());
  }
  void handle_ground_velocity_decay() override {
    g_log.push_back(s_of(_id) + ":handle_ground_velocity_decay:" + num(1.0));
  }
  void handle_ground_velocity_decay(double factor) override {
    g_log.push_back(s_of(_id) + ":handle_ground_velocity_decay:" + num(factor));
  }
};

FakeEnt* g_ent = nullptr;
Value g_state;
CharacterState_Lying* g_obj = nullptr;

std::string state_text() {
  const FakeEnt& e = *g_ent;
  return "pos=[" + num(e._px) + "," + num(e._py) + "," + num(e._pz) + "] hp=" + render(e._hp) +
         "/" + render(e._hpr) + "/" + render(e._hpmax) + " team=" + render(e._team) +
         " tough=" + render(e._tough) + "/" + render(e._tmax) + " trest=" + render(e._trest) +
         " la=" + render(e._la) + " ld=" + render(e._ld) + " lc=" + render(e._lc) +
         " wait=" + render(e._wait) + " holding=" + render(Value(e._holding)) +
         " holdteam=" + render(e._holdteam) + " deadjoin=" + render(e._deadjoin) +
         " deadgone=" + render(e._deadgone) + " reserve=" + render(e._reserve) +
         " wakeup=" + render(e._wakeup) + " motionless=" + render(e._motionless) +
         " invul=" + render(e._invul) + " blink=" + render(e._blink) +
         " outline=" + render(Value(e._outline)) + " frameid=" + render(e._frameid) +
         " gy=" + render(e._gy);
}

void bind() {
  static BuffEnv keep;
  keep.find_entity = [](const std::u16string&) -> lfw::buff::IBuffEntity* { return nullptr; };
  keep.create_entity = [](const Value&) -> lfw::buff::IBuffEntity* { return nullptr; };
  keep.find_data = [](const std::u16string&) -> Value { return Value(); };
  keep.world_buffs_set = [](const std::u16string&, Buff*) {};
  keep.world_buffs_get = [](const std::u16string&) -> Buff* { return nullptr; };
  keep.create_buff = [](const std::u16string&, const std::u16string&) -> Buff* { return nullptr; };
  lfw::state::StateEnv senv;
  senv.buff_env = &keep;
  lfw::state::set_state_env(senv);
}

}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: lfw_trace_character_state_lying <case-file>\n");
    return 2;
  }
  bind();
  g_ent = new FakeEnt(u"E1");
  g_state = Value(0.0);

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
      if (sub == "state") {
        g_state = parse_value(t, i);
      } else if (sub == "estate") {
        g_ent->_state = parse_value(t, i);
      } else if (sub == "pos") {
        const Value v = parse_value(t, i);
        g_ent->_px = lfw::to_number(lfw::field_or(v, u"x"));
        g_ent->_py = lfw::to_number(lfw::field_or(v, u"y"));
        g_ent->_pz = lfw::to_number(lfw::field_or(v, u"z"));
      } else if (sub == "gy") {
        g_ent->_gy = parse_value(t, i);
      } else if (sub == "frameid") {
        g_ent->_frameid = parse_value(t, i);
      } else if (sub == "hp") {
        g_ent->_hp = parse_value(t, i);
      } else if (sub == "hpr") {
        g_ent->_hpr = parse_value(t, i);
      } else if (sub == "hpmax") {
        g_ent->_hpmax = parse_value(t, i);
      } else if (sub == "holding") {
        g_ent->_holding = lfw::truthy(parse_value(t, i));
      } else if (sub == "holdtype") {
        g_ent->_holdtype = parse_value(t, i);
      } else if (sub == "holdteam") {
        g_ent->_holdteam = parse_value(t, i);
      } else if (sub == "team") {
        g_ent->_team = parse_value(t, i);
      } else if (sub == "tough") {
        g_ent->_tough = parse_value(t, i);
      } else if (sub == "tmax") {
        g_ent->_tmax = parse_value(t, i);
      } else if (sub == "trest") {
        g_ent->_trest = parse_value(t, i);
      } else if (sub == "la") {
        g_ent->_la = parse_value(t, i);
      } else if (sub == "ld") {
        g_ent->_ld = parse_value(t, i);
      } else if (sub == "lc") {
        g_ent->_lc = parse_value(t, i);
      } else if (sub == "wait") {
        g_ent->_wait = parse_value(t, i);
      } else if (sub == "held") {
        g_ent->_held = value_text(parse_value(t, i));
      } else if (sub == "dvals") {
        g_ent->_dvals = parse_value(t, i);
      } else if (sub == "deadjoin") {
        g_ent->_deadjoin = parse_value(t, i);
      } else if (sub == "deadgone") {
        g_ent->_deadgone = parse_value(t, i);
      } else if (sub == "reserve") {
        g_ent->_reserve = parse_value(t, i);
      } else if (sub == "wakeup") {
        g_ent->_wakeup = parse_value(t, i);
      } else if (sub == "motionless") {
        g_ent->_motionless = parse_value(t, i);
      } else if (sub == "invul") {
        g_ent->_invul = parse_value(t, i);
      } else if (sub == "blink") {
        g_ent->_blink = parse_value(t, i);
      } else if (sub == "outline") {
        g_ent->_outline = value_text(parse_value(t, i));
      } else if (sub == "puppets") {
        g_ent->_puppets = parse_value(t, i);
      } else {
        std::fprintf(stderr, "unknown env '%s' at line %d\n", sub.c_str(), lineno);
        return 2;
      }
      std::printf("env %s\n", sub.c_str());
      continue;
    }
    if (op == "run") {
      const std::string& what = t[i++];
      if (what == "make") {
        delete g_obj;
        g_obj = new CharacterState_Lying(g_state);
        std::printf("run make || %s | s=%s | %s\n", join(g_log).c_str(),
                    render(g_obj->state()).c_str(), state_text().c_str());
      } else if (what == "default") {
        delete g_obj;
        g_obj = new CharacterState_Lying();
        std::printf("run default || %s | s=%s | %s\n", join(g_log).c_str(),
                    render(g_obj->state()).c_str(), state_text().c_str());
      } else if (what == "enter") {
        if (g_obj->enter) g_obj->enter(*g_ent, Value());
        std::printf("run enter || %s | %s\n", join(g_log).c_str(), state_text().c_str());
      } else if (what == "update") {
        g_obj->update(*g_ent);
        std::printf("run update || %s | %s\n", join(g_log).c_str(), state_text().c_str());
      } else if (what == "leave") {
        g_obj->leave(*g_ent, Value());
        std::printf("run leave || %s | %s\n", join(g_log).c_str(), state_text().c_str());
      } else if (what == "dead") {
        if (g_obj->on_dead) g_obj->on_dead(*g_ent);
        std::printf("run dead || %s | %s\n", join(g_log).c_str(), state_text().c_str());
      } else if (what == "findframe") {
        Value r;
        if (g_obj->find_frame_by_id) r = g_obj->find_frame_by_id(*g_ent, Value());
        std::printf("run findframe || %s | r=%s | %s\n", join(g_log).c_str(),
                    render(r).c_str(), state_text().c_str());
      } else {
        std::fprintf(stderr, "unknown run '%s' at line %d\n", what.c_str(), lineno);
        return 2;
      }
      continue;
    }
    std::fprintf(stderr, "unknown op '%s' at line %d\n", op.c_str(), lineno);
    return 2;
  }
  delete g_obj;
  return 0;
}
