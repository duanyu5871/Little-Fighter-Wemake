#include <cstdio>
#include <fstream>
#include <memory>
#include <string>
#include <variant>
#include <vector>

#include "lfw/buff/buff.h"
#include "lfw/core/value.h"
#include "lfw/state/character_state_walking.h"
#include "lfw/utils/container_help/field_or.h"

#include "trace_util.h"

namespace {

using lfw::Value;
using lfw::buff::Buff;
using lfw::buff::BuffEnv;
using lfw::state::CharacterState_Walking;
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

Value marker_frame() {
  lfw::Object o;
  o.set(u"id", Value(std::u16string(u"SD")));
  return Value(std::make_shared<lfw::Object>(o));
}

struct FakeEnt : IStateEntity {
  std::u16string _id;
  Value _hp;
  Value _wait_value;
  Value _frame_info;
  Value _holding;
  Value _indexes;
  Value _ground_y;
  double _px = 0;
  double _py = 0;
  double _pz = 0;
  double _wait_flag = 0;
  bool _ctrl_ud = false;
  bool _ctrl_lr = false;
  bool _holding_is_weapon = false;

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
  void set_outline_color(const std::u16string& v) override {
    g_log.push_back(s_of(_id) + ":outline_color:" + render(Value(v)));
  }
  void enter_frame_by_id(const std::u16string& id) override {
    g_log.push_back(s_of(_id) + ":enter_frame_by_id:" + render(Value(id)) + ":0");
  }
  void enter_frame_by_id_fallback(const std::u16string& id, bool fallback) override {
    g_log.push_back(s_of(_id) + ":enter_frame_by_id:" + render(Value(id)) + ":" +
                    std::string(fallback ? "1" : "0"));
  }
  void attach(bool on) override {
    g_log.push_back(s_of(_id) + ":attach:" + std::string(on ? "1" : "0"));
  }
  Value hp() const override { return _hp; }
  Value wait() const override { return _wait_value; }
  void set_wait(const Value& v) override {
    _wait_value = v;
    g_log.push_back(s_of(_id) + ":set_wait:" + render(v));
  }
  Value holding_base_type() const override { return lfw::field_or(_holding, u"base_type"); }
  bool holding_is_weapon() const override { return _holding_is_weapon; }
  bool ctrl_ud() const override { return _ctrl_ud; }
  bool ctrl_lr() const override { return _ctrl_lr; }
  Value ground_y() const override { return _ground_y; }
  Value frame_info() const override { return _frame_info; }
  double handle_wait_flag(const Value& wait, const Value& frame) override {
    g_log.push_back(s_of(_id) + ":handle_wait_flag:" + render(wait) + ":" + render(frame));
    return _wait_flag;
  }
  Value get_sudden_death_frame() override {
    g_log.push_back(s_of(_id) + ":get_sudden_death_frame");
    return marker_frame();
  }
  Value data_indexes_default() const override { return lfw::field_or(_indexes, u"default"); }
  Value data_indexes_in_the_skys() const override {
    return lfw::field_or(_indexes, u"in_the_skys");
  }
  Value velocity_x() const override { return Value(); }
  Value velocity_z() const override { return Value(); }
  void enter_frame(const Value& frame) override {
    g_log.push_back(s_of(_id) + ":enter_frame:" + render(frame));
  }
  void handle_ground_velocity_decay() override {
    g_log.push_back(s_of(_id) + ":handle_ground_velocity_decay");
  }
};

std::vector<FakeEnt*> g_ents;
FakeEnt* g_victim = nullptr;
Value g_state;
CharacterState_Walking* g_state_obj = nullptr;

FakeEnt* find_ent(const std::u16string& id) {
  for (size_t i = 0; i < g_ents.size(); ++i) {
    if (g_ents[i]->_id == id) return g_ents[i];
  }
  return nullptr;
}

FakeEnt& ent(const std::u16string& id) {
  FakeEnt* e = find_ent(id);
  if (e != nullptr) return *e;
  g_ents.push_back(new FakeEnt(id));
  return *g_ents.back();
}

std::string state_text() {
  const FakeEnt& v = *g_victim;
  return "hp=" + render(v._hp) + " wait=" + render(v._wait_value) +
         " waitflag=" + num(v._wait_flag) + " frame=" + render(v._frame_info) + " pos=[" +
         num(v._px) + ":" + num(v._py) + ":" + num(v._pz) + "]" + " ground=" +
         render(v._ground_y) + " holding=" + render(v.holding_base_type()) + " hweapon=" +
         std::string(v._holding_is_weapon ? "1" : "0") + " ctrl=" +
         std::string(v._ctrl_ud ? "1" : "0") + std::string(v._ctrl_lr ? "1" : "0");
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

std::u16string value_text(const Value& v) {
  const std::u16string* s = std::get_if<std::u16string>(&v);
  return s != nullptr ? *s : std::u16string();
}

}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: lfw_trace_character_state_walking <case-file>\n");
    return 2;
  }
  bind();
  g_state = Value(static_cast<double>(0));

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
      } else if (sub == "victim") {
        g_victim = &ent(value_text(parse_value(t, i)));
      } else if (sub == "hp") {
        if (g_victim != nullptr) g_victim->_hp = parse_value(t, i);
      } else if (sub == "vwait") {
        if (g_victim != nullptr) g_victim->_wait_value = parse_value(t, i);
      } else if (sub == "waitflag") {
        if (g_victim != nullptr) g_victim->_wait_flag = trace::to_double(t[i++]);
      } else if (sub == "frame") {
        if (g_victim != nullptr) g_victim->_frame_info = parse_value(t, i);
      } else if (sub == "ground_y") {
        if (g_victim != nullptr) g_victim->_ground_y = parse_value(t, i);
      } else if (sub == "indexes") {
        if (g_victim != nullptr) g_victim->_indexes = parse_value(t, i);
      } else if (sub == "ctrlud") {
        if (g_victim != nullptr) g_victim->_ctrl_ud = trace::to_double(t[i++]) != 0;
      } else if (sub == "ctrllr") {
        if (g_victim != nullptr) g_victim->_ctrl_lr = trace::to_double(t[i++]) != 0;
      } else if (sub == "hweapon") {
        if (g_victim != nullptr) g_victim->_holding_is_weapon = trace::to_double(t[i++]) != 0;
      } else if (sub == "holding") {
        const Value v = parse_value(t, i);
        if (g_victim != nullptr) {
          lfw::Object o;
          o.set(u"base_type", v);
          g_victim->_holding = Value(std::make_shared<lfw::Object>(o));
        }
      } else if (sub == "pos") {
        const Value v = parse_value(t, i);
        if (g_victim != nullptr) {
          g_victim->_px = lfw::to_number(lfw::field_or(v, u"x"));
          g_victim->_py = lfw::to_number(lfw::field_or(v, u"y"));
          g_victim->_pz = lfw::to_number(lfw::field_or(v, u"z"));
        }
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
        delete g_state_obj;
        g_state_obj = new CharacterState_Walking(g_state);
        std::printf("run make || %s | %s\n", join(g_log).c_str(), state_text().c_str());
      } else if (what == "update") {
        g_state_obj->update(*g_victim);
        std::printf("run update || %s | %s\n", join(g_log).c_str(), state_text().c_str());
      } else {
        std::fprintf(stderr, "unknown run '%s' at line %d\n", what.c_str(), lineno);
        return 2;
      }
      continue;
    }
    std::fprintf(stderr, "unknown op '%s' at line %d\n", op.c_str(), lineno);
    return 2;
  }
  delete g_state_obj;
  return 0;
}
