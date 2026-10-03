#include <cstdio>
#include <fstream>
#include <memory>
#include <string>
#include <variant>
#include <vector>

#include "lfw/buff/buff.h"
#include "lfw/core/value.h"
#include "lfw/state/character_state_burning.h"
#include "lfw/utils/container_help/field_or.h"

#include "trace_util.h"

namespace {

using lfw::Value;
using lfw::buff::Buff;
using lfw::buff::BuffEnv;
using lfw::state::CharacterState_Burning;
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
  Value _indexes;
  Value _wdata;
  Value _facing;
  Value _bounced;
  Value _vx;
  Value _vy;
  Value _vz;
  Value _onlanding;
  bool _catcher = false;

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
  bool has_catcher() const override { return _catcher; }
  void catcher_drop_catching() override {
    g_log.push_back(s_of(_id) + ":catcher_drop_catching");
  }
  Value bounced() const override { return _bounced; }
  void set_bounced(const Value& v) override {
    _bounced = v;
    g_log.push_back(s_of(_id) + ":set_bounced:" + render(v));
  }
  Value facing() const override { return _facing; }
  void set_facing(const Value& v) override {
    _facing = v;
    g_log.push_back(s_of(_id) + ":set_facing:" + render(v));
  }
  Value world_dataset(const std::u16string& key) const override {
    g_log.push_back(s_of(_id) + ":world_dataset:" + s_of(key));
    return field_by(_wdata, key);
  }
  Value frame_on_landing() const override { return _onlanding; }
  Value data_indexes_bouncing() const override {
    return lfw::field_or(_indexes, u"bouncing");
  }
  Value data_indexes_lying() const override { return lfw::field_or(_indexes, u"lying"); }
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
Value g_velocity;
CharacterState_Burning* g_state_obj = nullptr;

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
  return "bounced=" + render(v._bounced) + " facing=" + render(v._facing) + " vel=[" +
         render(v._vx) + ":" + render(v._vy) + ":" + render(v._vz) + "]" + " catcher=" +
         std::string(v._catcher ? "1" : "0");
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
    std::fprintf(stderr, "usage: lfw_trace_character_state_burning <case-file>\n");
    return 2;
  }
  bind();
  g_state = Value(static_cast<double>(0));
  g_velocity = Value();

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
      } else if (sub == "indexes") {
        if (g_victim != nullptr) g_victim->_indexes = parse_value(t, i);
      } else if (sub == "wdata") {
        if (g_victim != nullptr) g_victim->_wdata = parse_value(t, i);
      } else if (sub == "facing") {
        if (g_victim != nullptr) g_victim->_facing = parse_value(t, i);
      } else if (sub == "bounced") {
        if (g_victim != nullptr) g_victim->_bounced = parse_value(t, i);
      } else if (sub == "velx") {
        if (g_victim != nullptr) g_victim->_vx = parse_value(t, i);
      } else if (sub == "vely") {
        if (g_victim != nullptr) g_victim->_vy = parse_value(t, i);
      } else if (sub == "velz") {
        if (g_victim != nullptr) g_victim->_vz = parse_value(t, i);
      } else if (sub == "landingvel") {
        g_velocity = parse_value(t, i);
      } else if (sub == "catcher") {
        if (g_victim != nullptr) g_victim->_catcher = trace::to_double(t[i++]) != 0;
      } else if (sub == "onlanding") {
        if (g_victim != nullptr) g_victim->_onlanding = parse_value(t, i);
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
        g_state_obj = new CharacterState_Burning();
        std::printf("run make || %s | %s\n", join(g_log).c_str(), state_text().c_str());
      } else if (what == "enter") {
        if (g_state_obj->enter) g_state_obj->enter(*g_victim, Value());
        std::printf("run enter || %s | %s\n", join(g_log).c_str(), state_text().c_str());
      } else if (what == "update") {
        g_state_obj->update(*g_victim);
        std::printf("run update || %s | %s\n", join(g_log).c_str(), state_text().c_str());
      } else if (what == "leave") {
        g_state_obj->leave(*g_victim, Value());
        std::printf("run leave || %s | %s\n", join(g_log).c_str(), state_text().c_str());
      } else if (what == "landing") {
        if (g_state_obj->on_landing) g_state_obj->on_landing(*g_victim, g_velocity);
        std::printf("run landing || %s | %s\n", join(g_log).c_str(), state_text().c_str());
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
