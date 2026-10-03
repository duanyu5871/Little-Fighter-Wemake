#include <cstdio>
#include <fstream>
#include <functional>
#include <memory>
#include <string>
#include <variant>
#include <vector>

#include "lfw/buff/buff_magic_flute.h"
#include "lfw/core/value.h"
#include "lfw/utils/container_help/field_or.h"

#include "trace_util.h"

namespace {

using lfw::Value;
using lfw::buff::Buff;
using lfw::buff::BuffEnv;
using lfw::buff::IBuffEntity;
using lfw::buff::MagicFluteEnv;
using trace::parse_value;
using trace::render_value;
using trace::split_ws;
using trace::to_ascii;

std::vector<std::string> g_log;
BuffEnv g_benv;

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

struct FakeEnt : IBuffEntity {
  std::u16string _id;
  Value _data;
  Value _indexes;
  Value _state;
  Value _hp;
  Value _hp_r;
  Value _fallinjury;
  Value _toughness;
  Value _team;
  double _vy = 0;

  explicit FakeEnt(std::u16string id) : _id(std::move(id)) {}

  const std::u16string& id() const override { return _id; }
  void position(double& x, double& y, double& z) const override {
    x = 0;
    y = 0;
    z = 0;
  }
  void set_position(double x, double y, double z) override {
    g_log.push_back(s_of(_id) + ":set_position:" + num(x) + ":" + num(y) + ":" + num(z));
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
  Value data() const override { return _data; }
  Value data_type() const override { return lfw::field_or(_data, u"type"); }
  Value state() const override { return _state; }
  Value hp() const override { return _hp; }
  Value hp_r() const override { return _hp_r; }
  void set_hp(const Value& v) override {
    _hp = v;
    g_log.push_back(s_of(_id) + ":set_hp:" + render(v));
  }
  void set_hp_r(const Value& v) override {
    _hp_r = v;
    g_log.push_back(s_of(_id) + ":set_hp_r:" + render(v));
  }
  void set_fallinjury(const Value& v) override {
    _fallinjury = v;
    g_log.push_back(s_of(_id) + ":set_fallinjury:" + render(v));
  }
  void set_toughness(const Value& v) override {
    _toughness = v;
    g_log.push_back(s_of(_id) + ":set_toughness:" + render(v));
  }
  Value team() const override { return _team; }
  void set_team(const Value& v) override {
    _team = v;
    g_log.push_back(s_of(_id) + ":set_team:" + render(v));
  }
  double velocity_y() const override { return _vy; }
  void set_velocity(const Value& x, const Value& y, const Value& z) override {
    g_log.push_back(s_of(_id) + ":set_velocity:" + render(x) + ":" + render(y) + ":" + render(z));
  }
  void handle_velocity_decay(double accx) override {
    g_log.push_back(s_of(_id) + ":handle_velocity_decay:" + num(accx));
  }
  Value data_indexes_falling() const override { return lfw::field_or(_indexes, u"falling"); }
  Value data_indexes_in_the_skys() const override {
    return lfw::field_or(_indexes, u"in_the_skys");
  }
};

std::vector<FakeEnt*> g_ents;
std::vector<FakeEnt*> g_victims;
std::unique_ptr<Buff> g_buff;
FakeEnt* g_effect = nullptr;
std::u16string g_buff_id = u"B1";
std::u16string g_kind = u"MagicFlute";
std::u16string g_cls = u"mf1";
std::u16string g_attacker_id;

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

std::string per_victim(const std::function<std::string(const FakeEnt&)>& fn) {
  std::string out = "[";
  for (size_t i = 0; i < g_victims.size(); ++i) {
    if (i) out += ",";
    out += s_of(g_victims[i]->_id) + ":" + fn(*g_victims[i]);
  }
  return out + "]";
}

std::string state_text() {
  std::string victims = "[";
  for (size_t i = 0; i < g_victims.size(); ++i) {
    if (i) victims += ",";
    victims += s_of(g_victims[i]->_id);
  }
  victims += "]";
  const std::string ticks = g_buff == nullptr ? std::string("-") : num(g_buff->ticks());
  const std::string dur = g_buff == nullptr ? std::string("-") : num(g_buff->duration());
  const std::string life = g_buff == nullptr ? std::string("-") : num(g_buff->lifetime());
  return "id=" + s_of(g_buff_id) + " victims=" + victims + " ticks=" + ticks + " dur=" + dur +
         " life=" + life +
         " hp=" + per_victim([](const FakeEnt& e) { return render(e._hp); }) +
         " hp_r=" + per_victim([](const FakeEnt& e) { return render(e._hp_r); }) +
         " fall=" + per_victim([](const FakeEnt& e) { return render(e._fallinjury); }) +
         " tough=" + per_victim([](const FakeEnt& e) { return render(e._toughness); }) +
         " team=" + per_victim([](const FakeEnt& e) { return render(e._team); }) +
         " vy=" + per_victim([](const FakeEnt& e) { return num(e._vy); }) +
         " type=" + per_victim([](const FakeEnt& e) { return render(e.data_type()); });
}

void bind() {
  g_benv.find_entity = [](const std::u16string& id) -> IBuffEntity* { return find_ent(id); };
  g_benv.create_entity = [](const Value&) -> IBuffEntity* {
    if (g_effect == nullptr) {
      g_effect = new FakeEnt(u"fx1");
      g_ents.push_back(g_effect);
    }
    return g_effect;
  };
  g_benv.find_data = [](const std::u16string& oid) -> Value {
    if (oid.empty()) return Value();
    g_log.push_back("find_data:" + s_of(oid));
    lfw::Object o;
    o.set(u"id", Value(std::u16string(oid)));
    return Value(std::make_shared<lfw::Object>(o));
  };
  g_benv.world_buffs_set = [](const std::u16string& id, Buff*) {
    g_log.push_back("world_buffs_set:" + s_of(id));
  };
  g_benv.world_buffs_get = [](const std::u16string&) -> Buff* { return nullptr; };
  g_benv.create_buff = [](const std::u16string&, const std::u16string&) -> Buff* { return nullptr; };

  MagicFluteEnv menv;
  menv.summary_apply_damage = [](IBuffEntity* a, const Value& injury, IBuffEntity* v,
                                 const Value& prev_hp) {
    g_log.push_back("apply_damage:" + s_of(a->id()) + ":" + render(injury) + ":" +
                    s_of(v->id()) + ":" + render(prev_hp));
  };
  lfw::buff::set_magic_flute_env(menv);
}

std::u16string value_text(const Value& v) {
  const std::u16string* s = std::get_if<std::u16string>(&v);
  return s != nullptr ? *s : std::u16string();
}

void run_make() {
  if (g_cls == u"mf2") {
    g_buff.reset(new lfw::buff::Buff_MagicFlute2(&g_benv, g_buff_id, Value(g_kind)));
  } else {
    g_buff.reset(new lfw::buff::Buff_MagicFlute(&g_benv, g_buff_id, Value(g_kind)));
  }
  for (size_t i = 0; i < g_victims.size(); ++i) {
    if (i == 0) {
      g_buff->set_victim(g_victims[i]);
    } else {
      g_buff->add_victim(g_victims[i]);
    }
  }
  if (!g_attacker_id.empty()) g_buff->set_attacker_by_id(g_attacker_id);
  std::printf("run make || %s | %s\n", join(g_log).c_str(), state_text().c_str());
}

}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: lfw_trace_buff_magic_flute <case-file>\n");
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
    if (op == "env") {
      const std::string& sub = t[i++];
      if (sub == "id") {
        g_buff_id = value_text(parse_value(t, i));
      } else if (sub == "kind") {
        g_kind = value_text(parse_value(t, i));
      } else if (sub == "cls") {
        g_cls = value_text(parse_value(t, i));
      } else if (sub == "attacker") {
        g_attacker_id = value_text(parse_value(t, i));
      } else if (sub == "ateam") {
        FakeEnt& a = ent(g_attacker_id);
        a._team = parse_value(t, i);
      } else if (sub == "victim") {
        FakeEnt* e = &ent(value_text(parse_value(t, i)));
        for (size_t k = 0; k < g_victims.size();) {
          if (g_victims[k] == e) {
            g_victims.erase(g_victims.begin() + static_cast<std::ptrdiff_t>(k));
          } else {
            ++k;
          }
        }
        g_victims.push_back(e);
      } else if (sub == "vtype") {
        if (!g_victims.empty()) {
          lfw::Object o;
          o.set(u"type", parse_value(t, i));
          g_victims.back()->_data = Value(std::make_shared<lfw::Object>(o));
        }
      } else if (sub == "vindexes") {
        if (!g_victims.empty()) g_victims.back()->_indexes = parse_value(t, i);
      } else if (sub == "vstate") {
        if (!g_victims.empty()) g_victims.back()->_state = parse_value(t, i);
      } else if (sub == "vhp") {
        if (!g_victims.empty()) g_victims.back()->_hp = parse_value(t, i);
      } else if (sub == "vhp_r") {
        if (!g_victims.empty()) g_victims.back()->_hp_r = parse_value(t, i);
      } else if (sub == "vfallinjury") {
        if (!g_victims.empty()) g_victims.back()->_fallinjury = parse_value(t, i);
      } else if (sub == "vtough") {
        if (!g_victims.empty()) g_victims.back()->_toughness = parse_value(t, i);
      } else if (sub == "vteam") {
        if (!g_victims.empty()) g_victims.back()->_team = parse_value(t, i);
      } else if (sub == "vvy") {
        if (!g_victims.empty()) g_victims.back()->_vy = trace::to_double(t[i++]);
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
      } else if (what == "init") {
        g_buff->init();
        std::printf("run init || %s | %s\n", join(g_log).c_str(), state_text().c_str());
      } else if (what == "tick") {
        g_buff->update(trace::to_double(t[i++]));
        std::printf("run tick || %s | %s\n", join(g_log).c_str(), state_text().c_str());
      } else {
        std::fprintf(stderr, "unknown run '%s' at line %d\n", what.c_str(), lineno);
        return 2;
      }
      continue;
    }
    std::fprintf(stderr, "unknown op '%s' at line %d\n", op.c_str(), lineno);
    return 2;
  }
  return 0;
}
