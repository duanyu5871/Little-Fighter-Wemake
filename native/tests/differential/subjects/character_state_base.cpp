#include <cstdio>
#include <fstream>
#include <memory>
#include <string>
#include <variant>
#include <vector>

#include "lfw/buff/buff.h"
#include "lfw/core/value.h"
#include "lfw/state/character_state_base.h"
#include "lfw/utils/container_help/field_or.h"

#include "trace_util.h"

namespace {

using lfw::Value;
using lfw::buff::Buff;
using lfw::buff::BuffEnv;
using lfw::state::CharacterState_Base;
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

Value field_by(const Value& holder, const std::u16string& key) {
  const lfw::Object* o = lfw::as_object(holder);
  if (o == nullptr) return Value();
  const Value* p = o->get(key);
  return p != nullptr ? *p : Value();
}

struct FakeEnt : IStateEntity {
  std::u16string _id;
  Value _state;
  Value _hp;
  Value _facing;
  Value _holding;
  Value _dataset;
  Value _indexes;
  Value _frames;
  Value _onlanding;
  Value _vx;
  Value _vz;
  bool _on_ground = false;

  explicit FakeEnt(std::u16string id) : _id(std::move(id)) {}

  const std::u16string& id() const override { return _id; }
  void position(double& x, double& y, double& z) const override {
    x = 0;
    y = 0;
    z = 0;
  }
  void set_position(double x, double y, double z) override {
    (void)x;
    (void)y;
    (void)z;
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
    g_log.push_back(s_of(_id) + ":enter_frame_by_id:" + render(Value(id)));
  }
  void attach(bool on) override {
    g_log.push_back(s_of(_id) + ":attach:" + std::string(on ? "1" : "0"));
  }
  Value state() const override { return _state; }
  Value hp() const override { return _hp; }
  Value facing() const override { return _facing; }
  Value holding_base_type() const override {
    return lfw::field_or(_holding, u"base_type");
  }
  bool is_on_ground() const override { return _on_ground; }
  Value frame_on_landing() const override { return _onlanding; }
  void enter_frame(const Value& frame) override {
    g_log.push_back(s_of(_id) + ":enter_frame:" + render(frame));
  }
  void drop_holding() override { g_log.push_back(s_of(_id) + ":drop_holding"); }
  void handle_ground_velocity_decay() override {
    g_log.push_back(s_of(_id) + ":handle_ground_velocity_decay");
  }
  Value data_indexes_default() const override {
    return lfw::field_or(_indexes, u"default");
  }
  Value data_indexes_landing_2() const override {
    return lfw::field_or(_indexes, u"landing_2");
  }
  Value data_indexes_heavy_obj_walk() const override {
    return lfw::field_or(_indexes, u"heavy_obj_walk");
  }
  Value data_indexes_in_the_skys() const override {
    return lfw::field_or(_indexes, u"in_the_skys");
  }
  Value data_indexes_falling() const override {
    return lfw::field_or(_indexes, u"falling");
  }
  Value data_frames() const override { return _frames; }
  Value dataset(const std::u16string& key) const override {
    g_log.push_back(s_of(_id) + ":dataset:" + s_of(key));
    return field_by(_dataset, key);
  }
  Value velocity_x() const override { return _vx; }
  Value velocity_z() const override { return _vz; }
  void set_velocity(const Value& x, const Value& y, const Value& z) override {
    g_log.push_back(s_of(_id) + ":set_velocity:" + render(x) + ":" + render(y) + ":" + render(z));
    if (!std::holds_alternative<std::monostate>(x) && !std::holds_alternative<lfw::NullTag>(x)) {
      _vx = x;
    }
    if (!std::holds_alternative<std::monostate>(z) && !std::holds_alternative<lfw::NullTag>(z)) {
      _vz = z;
    }
  }
};

std::vector<FakeEnt*> g_ents;
FakeEnt* g_victim = nullptr;
Value g_state;
CharacterState_Base* g_state_obj = nullptr;

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
  const std::string vel = g_victim == nullptr
                              ? std::string("-")
                              : "[" + render(g_victim->_vx) + ":" + render(g_victim->_vz) + "]";
  return "state=" + render(g_state) + " estate=" + render(g_victim->_state) +
         " hp=" + render(g_victim->_hp) +
         " face=" + render(g_victim->_facing) + " onground=" +
         std::string(g_victim->_on_ground ? "1" : "0") + " holding=" +
         render(g_victim->holding_base_type()) + " vel=" + vel;
}

void bind() {
  BuffEnv benv;
  benv.find_entity = [](const std::u16string&) -> lfw::buff::IBuffEntity* { return nullptr; };
  benv.create_entity = [](const Value&) -> lfw::buff::IBuffEntity* { return nullptr; };
  benv.find_data = [](const std::u16string&) -> Value { return Value(); };
  benv.world_buffs_set = [](const std::u16string&, Buff*) {};
  benv.world_buffs_get = [](const std::u16string&) -> Buff* { return nullptr; };
  benv.create_buff = [](const std::u16string&, const std::u16string&) -> Buff* { return nullptr; };
  static BuffEnv keep = benv;
  lfw::state::StateEnv senv;
  senv.buff_env = &keep;
  lfw::state::set_state_env(senv);
}

std::u16string value_text(const Value& v) {
  const std::u16string* s = std::get_if<std::u16string>(&v);
  return s != nullptr ? *s : std::u16string();
}

void print_run(const char* what, const Value* ret) {
  const std::string ret_s =
      (ret == nullptr || std::holds_alternative<std::monostate>(*ret)) ? "-" : render(*ret);
  std::printf("run %s || %s | ret=%s | %s\n", what, join(g_log).c_str(), ret_s.c_str(),
              state_text().c_str());
}

}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: lfw_trace_character_state_base <case-file>\n");
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
      } else if (sub == "vstate") {
        if (g_victim != nullptr) g_victim->_state = parse_value(t, i);
      } else if (sub == "victim") {
        g_victim = &ent(value_text(parse_value(t, i)));
      } else if (sub == "hp") {
        if (g_victim != nullptr) g_victim->_hp = parse_value(t, i);
      } else if (sub == "facing") {
        if (g_victim != nullptr) g_victim->_facing = parse_value(t, i);
      } else if (sub == "onground") {
        if (g_victim != nullptr) g_victim->_on_ground = trace::to_double(t[i++]) != 0;
      } else if (sub == "holding") {
        const Value v = parse_value(t, i);
        if (g_victim != nullptr) {
          lfw::Object o;
          o.set(u"base_type", v);
          g_victim->_holding = Value(std::make_shared<lfw::Object>(o));
        }
      } else if (sub == "onlanding") {
        if (g_victim != nullptr) g_victim->_onlanding = parse_value(t, i);
      } else if (sub == "dataset") {
        if (g_victim != nullptr) g_victim->_dataset = parse_value(t, i);
      } else if (sub == "indexes") {
        if (g_victim != nullptr) g_victim->_indexes = parse_value(t, i);
      } else if (sub == "frames") {
        if (g_victim != nullptr) g_victim->_frames = parse_value(t, i);
      } else if (sub == "velx") {
        if (g_victim != nullptr) g_victim->_vx = parse_value(t, i);
      } else if (sub == "velz") {
        if (g_victim != nullptr) g_victim->_vz = parse_value(t, i);
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
        g_state_obj = new CharacterState_Base(g_state);
        print_run("make", nullptr);
      } else if (what == "update") {
        g_state_obj->update(*g_victim);
        print_run("update", nullptr);
      } else if (what == "landing") {
        g_state_obj->on_landing(*g_victim, Value());
        print_run("landing", nullptr);
      } else if (what == "up") {
        g_state_obj->on_leave_ground(*g_victim);
        print_run("up", nullptr);
      } else if (what == "auto") {
        const Value r =
            g_state_obj->get_auto_frame ? g_state_obj->get_auto_frame(*g_victim) : Value();
        print_run("auto", &r);
      } else if (what == "sudden") {
        const Value r = g_state_obj->get_sudden_death_frame
                            ? g_state_obj->get_sudden_death_frame(*g_victim)
                            : Value();
        print_run("sudden", &r);
      } else if (what == "caught") {
        const Value r =
            g_state_obj->get_caught_end_frame ? g_state_obj->get_caught_end_frame(*g_victim)
                                              : Value();
        print_run("caught", &r);
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
