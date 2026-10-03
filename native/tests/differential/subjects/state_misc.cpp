#include <cstdio>
#include <fstream>
#include <memory>
#include <string>
#include <variant>
#include <vector>

#include "lfw/buff/buff.h"
#include "lfw/core/value.h"
#include "lfw/state/state_misc.h"
#include "lfw/utils/container_help/field_or.h"

#include "trace_util.h"

namespace {

using lfw::Value;
using lfw::buff::Buff;
using lfw::buff::BuffEnv;
using lfw::state::BallState_Base;
using lfw::state::CharacterState_TransformToLouisEX;
using lfw::state::IStateEntity;
using lfw::state::State_Base;
using lfw::state::State_TransformTo8XXX;
using lfw::state::State_TransformToCatching;
using lfw::state::State_WeaponBroken;
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
  o.set(u"id", Value(std::u16string(u"AUTO")));
  return Value(std::make_shared<lfw::Object>(o));
}

struct FakeEnt : IStateEntity {
  std::u16string _id;
  Value _state;
  Value _data;
  Value _find_data;
  Value _find_fighter;
  Value _transform_type;
  Value _shaking;
  Value _motionless;
  Value _vx;
  Value _vy;
  Value _vz;

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
  Value data() const override { return _data; }
  Value data_type() const override { return lfw::field_or(_data, u"type"); }
  Value datas_find(const std::u16string& oid) override {
    g_log.push_back(s_of(_id) + ":datas_find:" + s_of(oid));
    return _find_data;
  }
  Value datas_find_fighter(const std::u16string& oid) override {
    g_log.push_back(s_of(_id) + ":datas_find_fighter:" + s_of(oid));
    return _find_fighter;
  }
  void transform(const Value& data) override {
    g_log.push_back(s_of(_id) + ":transform:" + render(data));
    lfw::Object o;
    o.set(u"type", _transform_type);
    _data = Value(std::make_shared<lfw::Object>(o));
  }
  Value find_auto_frame() override {
    g_log.push_back(s_of(_id) + ":find_auto_frame");
    return marker_frame();
  }
  void transfrom_to_another() override {
    g_log.push_back(s_of(_id) + ":transfrom_to_another");
  }
  void set_shaking(const Value& v) override {
    _shaking = v;
    g_log.push_back(s_of(_id) + ":set_shaking:" + render(v));
  }
  void set_motionless(const Value& v) override {
    _motionless = v;
    g_log.push_back(s_of(_id) + ":set_motionless:" + render(v));
  }
  void world_callbacks_call(const std::u16string& name) override {
    g_log.push_back(s_of(_id) + ":world_callbacks_call:" + s_of(name));
  }
  void enter_frame(const Value& frame) override {
    g_log.push_back(s_of(_id) + ":enter_frame:" + render(frame));
  }
  Value velocity_x() const override { return _vx; }
  Value velocity_z() const override { return _vz; }
  void set_velocity(const Value& x, const Value& y, const Value& z) override {
    g_log.push_back(s_of(_id) + ":set_velocity:" + render(x) + ":" + render(y) + ":" + render(z));
    if (!std::holds_alternative<std::monostate>(x) && !std::holds_alternative<lfw::NullTag>(x)) {
      _vx = x;
    }
    if (!std::holds_alternative<std::monostate>(y) && !std::holds_alternative<lfw::NullTag>(y)) {
      _vy = y;
    }
    if (!std::holds_alternative<std::monostate>(z) && !std::holds_alternative<lfw::NullTag>(z)) {
      _vz = z;
    }
  }
};

std::vector<FakeEnt*> g_ents;
FakeEnt* g_victim = nullptr;
Value g_state;
std::u16string g_cls = u"weapon_broken";
State_Base* g_state_obj = nullptr;

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
  return "state=" + render(g_state) + " vstate=" + render(v._state) +
         " type=" + render(v.data_type()) + " shaking=" + render(v._shaking) +
         " motionless=" + render(v._motionless) + " vel=[" + render(v._vx) + ":" +
         render(v._vy) + ":" + render(v._vz) + "]";
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

void run_make() {
  delete g_state_obj;
  if (g_cls == u"to_catching") {
    g_state_obj = new State_TransformToCatching(g_state);
  } else if (g_cls == u"to_louisex") {
    g_state_obj = new CharacterState_TransformToLouisEX(g_state);
  } else if (g_cls == u"to_8xxx") {
    g_state_obj = new State_TransformTo8XXX(g_state);
  } else if (g_cls == u"ball") {
    g_state_obj = new BallState_Base(g_state);
  } else {
    g_state_obj = new State_WeaponBroken(g_state);
  }
  std::printf("run make || %s | %s\n", join(g_log).c_str(), state_text().c_str());
}

}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: lfw_trace_state_misc <case-file>\n");
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
      if (sub == "cls") {
        g_cls = value_text(parse_value(t, i));
      } else if (sub == "state") {
        g_state = parse_value(t, i);
      } else if (sub == "victim") {
        g_victim = &ent(value_text(parse_value(t, i)));
      } else if (sub == "vstate") {
        if (g_victim != nullptr) g_victim->_state = parse_value(t, i);
      } else if (sub == "vtype") {
        const Value v = parse_value(t, i);
        if (g_victim != nullptr) {
          lfw::Object o;
          o.set(u"type", v);
          g_victim->_data = Value(std::make_shared<lfw::Object>(o));
        }
      } else if (sub == "finddata") {
        if (g_victim != nullptr) g_victim->_find_data = parse_value(t, i);
      } else if (sub == "findfighter") {
        if (g_victim != nullptr) g_victim->_find_fighter = parse_value(t, i);
      } else if (sub == "transformtype") {
        if (g_victim != nullptr) g_victim->_transform_type = parse_value(t, i);
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
        run_make();
      } else if (what == "landing") {
        if (g_state_obj->on_landing) g_state_obj->on_landing(*g_victim, Value());
        std::printf("run landing || %s | %s\n", join(g_log).c_str(), state_text().c_str());
      } else if (what == "update") {
        g_state_obj->update(*g_victim);
        std::printf("run update || %s | %s\n", join(g_log).c_str(), state_text().c_str());
      } else if (what == "enter") {
        if (g_state_obj->enter) g_state_obj->enter(*g_victim, Value());
        std::printf("run enter || %s | %s\n", join(g_log).c_str(), state_text().c_str());
      } else if (what == "leave") {
        g_state_obj->leave(*g_victim, Value());
        std::printf("run leave || %s | %s\n", join(g_log).c_str(), state_text().c_str());
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
