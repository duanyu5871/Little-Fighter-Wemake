#include <cstdio>
#include <fstream>
#include <string>
#include <variant>
#include <vector>

#include "lfw/buff/buff.h"
#include "lfw/collision/collision.h"
#include "lfw/collision/healing.h"
#include "lfw/core/value.h"
#include "lfw/utils/container_help/field_or.h"

#include "trace_util.h"

namespace {

using lfw::Value;
using lfw::collision::Collision;
using lfw::collision::HealingEnv;
using lfw::collision::IHealingEntity;
using trace::parse_value;
using trace::render_value;
using trace::split_ws;
using trace::to_ascii;

std::vector<std::string> g_log;
lfw::buff::BuffEnv g_benv;
lfw::buff::Buff* g_granted = nullptr;
std::string g_granted_attacker;

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

struct Fake;

struct FakeBuff : lfw::buff::IBuffEntity {
  std::u16string _id;
  Fake* _owner;
  FakeBuff(std::u16string id, Fake* owner) : _id(std::move(id)), _owner(owner) {}
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
  Value dataset(const std::u16string& key) const override;
};

struct Fake : IHealingEntity {
  std::u16string _id;
  Value _dataset;
  FakeBuff _buff;

  explicit Fake(std::u16string id) : _id(std::move(id)), _buff(_id, this) {}

  const std::u16string& id() const override { return _id; }

  Value dataset(const std::u16string& key) const override {
    return lfw::field_or(_dataset, key.c_str());
  }

  lfw::buff::IBuffEntity* buff_entity() override { return &_buff; }
};

Value FakeBuff::dataset(const std::u16string& key) const { return _owner->dataset(key); }

Fake g_att(u"A");
Fake g_vic(u"V");
Collision g_c;

void bind() {
  HealingEnv env;
  env.find_entity = [](const std::u16string& id) -> IHealingEntity* {
    if (id == g_att._id) return &g_att;
    if (id == g_vic._id) return &g_vic;
    return nullptr;
  };
  env.buff_env = []() -> const lfw::buff::BuffEnv* { return &g_benv; };
  lfw::collision::set_healing_env(env);

  g_benv.find_entity = [](const std::u16string& id) -> lfw::buff::IBuffEntity* {
    if (id == g_att._buff.id()) return &g_att._buff;
    if (id == g_vic._buff.id()) return &g_vic._buff;
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

std::string side_text(const Fake& f) { return s_of(f._id) + ".dataset=" + render(f._dataset); }

std::string state_text() {
  std::string s = side_text(g_att) + " " + side_text(g_vic);
  s += " buff=";
  if (g_granted == nullptr) {
    s += "none";
  } else {
    s += s_of(g_granted->id()) + "/" + num(g_granted->lifetime()) + "/" +
         num(g_granted->duration()) + "/" + num(g_granted->level()) + "/" +
         s_of(g_granted->attacker_id());
  }
  return s;
}

void walk_side(Fake& f, const std::string& field, const std::vector<std::string>& t, size_t& i) {
  if (field == "dataset") {
    f._dataset = parse_value(t, i);
  } else {
    std::fprintf(stderr, "unknown side field '%s'\n", field.c_str());
    std::exit(2);
  }
}

void run() {
  lfw::collision::handle_healing(g_c);
  std::printf("run heal || %s | %s\n", join(g_log).c_str(), state_text().c_str());
}

}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: lfw_trace_collision_healing <case-file>\n");
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
    g_granted = nullptr;
    if (op == "env") {
      const std::string& sub = t[i++];
      if (sub == "itr") {
        g_c.itr = parse_value(t, i);
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
