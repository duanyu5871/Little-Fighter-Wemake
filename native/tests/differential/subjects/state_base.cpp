#include <cstdio>
#include <fstream>
#include <memory>
#include <string>
#include <variant>
#include <vector>

#include "lfw/buff/buff_healing.h"
#include "lfw/core/value.h"
#include "lfw/state/state_base.h"
#include "lfw/utils/container_help/field_or.h"

#include "entity_fixture.h"
#include "trace_util.h"

namespace {

using lfw::Value;
using lfw::buff::Buff;
using lfw::buff::BuffEnv;
using lfw::state::IStateEntity;
using lfw::state::State_Base;
using lfw::state::StateEnv;
using trace::parse_value;
using trace::render_value;
using trace::split_ws;
using trace::to_ascii;

std::vector<std::string> g_log;
BuffEnv g_benv;
Buff* g_created = nullptr;

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

struct FakeEnt;

// 切片 4BY：`buff` 以 `lfw::Entity` 为准入 ⇒ `entity_self()` 要交出一个真 `Entity`
// （`State_Base::leave` 的 HealSelf 分支会 grant 一个 `Buff_Healing`，它会在受害者
// 身上落 `buffs_set` / `set_mark` / `dataset`）。这就是那个实体：同样是台面假件，
// 只是观测点从窄接口换到实体面。
struct BuffStub : lfw::Entity {
  FakeEnt* _owner = nullptr;
  std::vector<std::pair<std::u16string, std::u16string>> _marks;

  explicit BuffStub(std::u16string id) : Entity(fx::shared_host(), Value()) { this->id = std::move(id); }

  Value dataset(const std::u16string& key) const override;
  void buffs_set(const std::u16string& key, Buff* b) override {
    g_log.push_back(s_of(id) + ":buffs_set:" + s_of(key));
    Entity::buffs_set(key, b);
  }
  void buffs_delete(const std::u16string& key) override {
    g_log.push_back(s_of(id) + ":buffs_delete:" + s_of(key));
    Entity::buffs_delete(key);
  }
  Value marks_get(const std::u16string& key) const override {
    for (size_t i = 0; i < _marks.size(); ++i) {
      if (_marks[i].first == key) return Value(std::u16string(_marks[i].second));
    }
    return Value();
  }
  void marks_set(const std::u16string& key, const std::u16string& value) override {
    g_log.push_back(s_of(id) + ":set_mark:" + s_of(key) + ":" + render(Value(value)));
    for (size_t i = 0; i < _marks.size(); ++i) {
      if (_marks[i].first == key) {
        _marks[i].second = value;
        return;
      }
    }
    _marks.emplace_back(key, value);
  }
  bool marks_delete(const std::u16string& key) override {
    for (size_t i = 0; i < _marks.size(); ++i) {
      if (_marks[i].first != key) continue;
      _marks.erase(_marks.begin() + static_cast<std::ptrdiff_t>(i));
      g_log.push_back(s_of(id) + ":del_mark:" + s_of(key));
      return true;
    }
    g_log.push_back(s_of(id) + ":del_mark_miss:" + s_of(key));
    return false;
  }
};

struct FakeEnt : IStateEntity {
  std::u16string _id;
  Value _dataset;
  Value _vx;
  Value _vz;
  double _px = 0;
  double _py = 0;
  double _pz = 0;
  BuffStub _stub;

  explicit FakeEnt(std::u16string id) : _id(id), _stub(id) { _stub._owner = this; }

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
  void set_outline_color(const std::u16string& v) override {
    g_log.push_back(s_of(_id) + ":outline_color:" + render(Value(v)));
  }
  void enter_frame_by_id(const std::u16string& id) override {
    g_log.push_back(s_of(_id) + ":enter_frame_by_id:" + render(Value(id)));
  }
  double velocity_x() const override { return lfw::to_number(_vx); }
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
  Value dataset(const std::u16string& key) const override {
    g_log.push_back(s_of(_id) + ":dataset:" + s_of(key));
    const lfw::Object* o = lfw::as_object(_dataset);
    if (o == nullptr) return Value();
    const Value* p = o->get(key);
    return p != nullptr ? *p : Value();
  }
  lfw::Entity* entity_self() override { return &_stub; }
};

Value BuffStub::dataset(const std::u16string& key) const {
  g_log.push_back(s_of(id) + ":dataset:" + s_of(key));
  const lfw::Object* o = _owner == nullptr ? nullptr : lfw::as_object(_owner->_dataset);
  if (o == nullptr) return Value();
  const Value* p = o->get(key);
  return p != nullptr ? *p : Value();
}

std::vector<FakeEnt*> g_ents;
FakeEnt* g_victim = nullptr;
Value g_state;
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
  std::string granted = "-";
  if (g_created != nullptr) {
    granted = s_of(g_created->id()) + ":" + num(g_created->duration());
  }
  std::string marks = "[";
  if (g_victim != nullptr) {
    for (size_t i = 0; i < g_victim->_stub._marks.size(); ++i) {
      if (i) marks += ",";
      marks += s_of(g_victim->_stub._marks[i].first) + "=" +
               render(Value(g_victim->_stub._marks[i].second));
    }
  }
  marks += "]";
  const std::string pos =
      g_victim == nullptr
          ? std::string("-")
          : "[" + num(g_victim->_px) + ":" + num(g_victim->_py) + ":" + num(g_victim->_pz) + "]";
  const std::string vel = g_victim == nullptr
                              ? std::string("-")
                              : "[" + render(g_victim->_vx) + ":" + render(g_victim->_vz) + "]";
  return "state=" + render(g_state) + " pos=" + pos + " vel=" + vel + " granted=" + granted +
         " marks=" + marks;
}

void bind() {
  g_benv.find_entity = [](const std::u16string& id) -> lfw::Entity* {
    FakeEnt* e = find_ent(id);
    return e == nullptr ? nullptr : &e->_stub;
  };
  g_benv.create_entity = [](const Value&) -> lfw::Entity* { return nullptr; };
  g_benv.find_data = [](const std::u16string&) -> Value { return Value(); };
  g_benv.world_buffs_set = [](const std::u16string& id, Buff*) {
    g_log.push_back("world_buffs_set:" + s_of(id));
  };
  g_benv.world_buffs_get = [](const std::u16string&) -> Buff* { return nullptr; };
  g_benv.create_buff = [](const std::u16string& kind, const std::u16string& id) -> Buff* {
    g_log.push_back("create_buff:" + s_of(kind) + ":" + s_of(id));
    g_created = new lfw::buff::Buff_Healing(&g_benv, id, Value(kind));
    return g_created;
  };
  StateEnv senv;
  senv.buff_env = &g_benv;
  lfw::state::set_state_env(senv);
}

std::u16string value_text(const Value& v) {
  const std::u16string* s = std::get_if<std::u16string>(&v);
  return s != nullptr ? *s : std::u16string();
}

}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: lfw_trace_state_base <case-file>\n");
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
      } else if (sub == "dataset") {
        if (g_victim != nullptr) g_victim->_dataset = parse_value(t, i);
      } else if (sub == "pos") {
        const Value v = parse_value(t, i);
        if (g_victim != nullptr) {
          g_victim->_px = lfw::to_number(lfw::field_or(v, u"x"));
          g_victim->_py = lfw::to_number(lfw::field_or(v, u"y"));
          g_victim->_pz = lfw::to_number(lfw::field_or(v, u"z"));
        }
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
        g_state_obj = new State_Base(g_state);
        g_created = nullptr;
        std::printf("run make || %s | %s\n", join(g_log).c_str(), state_text().c_str());
      } else if (what == "leave") {
        g_state_obj->leave(*g_victim, Value());
        std::printf("run leave || %s | %s\n", join(g_log).c_str(), state_text().c_str());
      } else if (what == "restrict") {
        const double x = trace::to_double(t[i++]);
        const double y = trace::to_double(t[i++]);
        const double z = trace::to_double(t[i++]);
        g_state_obj->on_restrict(*g_victim, x, y, z);
        std::printf("run restrict || %s | %s\n", join(g_log).c_str(), state_text().c_str());
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
