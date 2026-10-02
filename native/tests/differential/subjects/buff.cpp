#include <cstdio>
#include <fstream>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "lfw/buff/buff.h"
#include "lfw/core/value.h"

#include "trace_util.h"

namespace {

using trace::parse_value;
using trace::render_value;
using trace::split_ws;
using trace::to_ascii;

std::vector<std::string> g_log;
std::map<std::u16string, int> g_entity_ok;

std::string render(const lfw::Value& v) { return to_ascii(render_value(v)); }
std::string flag(bool b) { return b ? "b1" : "b0"; }
std::string num(double d) { return render(lfw::Value(d)); }
std::u16string u16(const std::string& s) { return std::u16string(s.begin(), s.end()); }
std::string u8(const std::u16string& s) { return to_ascii(s); }

class FakeEntity : public lfw::buff::IBuffEntity {
 public:
  std::u16string _id;
  double px = 0;
  double py = 0;
  double pz = 0;
  double centery = 0;
  double height = 0;
  double pic_h = 0;
  bool exists = true;
  const std::u16string& id() const override { return _id; }
  void position(double& x, double& y, double& z) const override {
    x = px;
    y = py;
    z = pz;
  }
  void set_position(double x, double y, double z) override {
    px = x;
    py = y;
    pz = z;
    g_log.push_back("set_position:" + u8(_id) + ":" + num(x) + "," + num(y) + "," + num(z));
  }
  double frame_centery() const override { return centery; }
  double frame_height() const override { return height; }
  double frame_pic_h() const override { return pic_h; }
  void set_frame(const lfw::Value& info) override {
    g_log.push_back("set_frame:" + u8(_id) + ":" + render(info));
  }
  void buffs_set(const std::u16string& k, lfw::buff::Buff*) override {
    g_log.push_back("buffs_set:" + u8(_id) + ":" + u8(k));
  }
  void buffs_delete(const std::u16string& k) override {
    g_log.push_back("buffs_delete:" + u8(_id) + ":" + u8(k));
  }
  void set_outline_alpha(double v) override { g_log.push_back("outline_alpha:" + num(v)); }
  void set_outline_width(double v) override { g_log.push_back("outline_width:" + num(v)); }
  void set_outline_color(const std::u16string& v) override {
    g_log.push_back("outline_color:" + render(lfw::Value(v)));
  }
  void enter_frame_by_id(const std::u16string& i) override {
    g_log.push_back("enter_frame:" + u8(i));
  }
  void attach(bool on) override { g_log.push_back("attach:" + flag(on)); }
};

std::map<std::u16string, std::unique_ptr<FakeEntity>> g_entities;
lfw::Value g_data;
lfw::Value g_create_entity_ok = lfw::Value(true);
lfw::Value g_create_buff_ok = lfw::Value(true);
int g_entity_seq = 0;

class TestBuff : public lfw::buff::Buff {
 public:
  using Buff::Buff;
  std::string hooks = "none";
  std::string oid;
  std::string fid = "0";
  bool has_on_update() const override { return hooks == "update"; }
  bool has_on_tick() const override { return hooks == "tick"; }
  bool has_on_end() const override { return hooks == "end"; }
  void on_update(lfw::buff::IBuffEntity* a, lfw::buff::IBuffEntity* v) override {
    g_log.push_back("on_update:" + (a ? u8(a->id()) : std::string("-")) + ":" +
                    (v ? u8(v->id()) : std::string("-")));
  }
  void on_tick(lfw::buff::IBuffEntity* a, lfw::buff::IBuffEntity* v) override {
    g_log.push_back("on_tick:" + (a ? u8(a->id()) : std::string("-")) + ":" +
                    (v ? u8(v->id()) : std::string("-")));
  }
  void on_end(lfw::buff::IBuffEntity* a, lfw::buff::IBuffEntity* v) override {
    g_log.push_back("on_end:" + (a ? u8(a->id()) : std::string("-")) + ":" +
                    (v ? u8(v->id()) : std::string("-")));
  }
  std::u16string effect_oid() const override { return u16(oid); }
  std::u16string effect_frame_id() const override { return u16(fid); }
  void call_place_center(lfw::buff::IBuffEntity* e, lfw::buff::IBuffEntity* v) {
    place_effect_center(e, v);
  }
  void call_show(lfw::buff::IBuffEntity* v) { show_effect(v); }
  void call_clear() { clear_effects(); }
  void call_update_effects() { update_effects(); }
  void call_del_effect(const std::u16string& id) { del_effect(id); }
  bool call_del_id(const std::u16string& id) { return del_id(id); }
};

lfw::buff::BuffEnv g_env;
TestBuff g_buff(&g_env, std::u16string(), lfw::Value(std::u16string(u"k")));
std::map<std::u16string, std::unique_ptr<TestBuff>> g_made;
TestBuff* g_sync = nullptr;

FakeEntity* ensure(const std::u16string& id) {
  auto it = g_entities.find(id);
  if (it != g_entities.end()) return it->second.get();
  auto e = std::make_unique<FakeEntity>();
  e->_id = id;
  FakeEntity* p = e.get();
  g_entities.emplace(id, std::move(e));
  return p;
}

lfw::buff::IBuffEntity* find_entity(const std::u16string& id) {
  auto ok = g_entity_ok.find(id);
  if (ok != g_entity_ok.end() && ok->second == 0) return nullptr;
  auto it = g_entities.find(id);
  if (it == g_entities.end()) return nullptr;
  if (!it->second->exists) return nullptr;
  return it->second.get();
}

void bind() {
  g_env.find_entity = [](const std::u16string& id) { return find_entity(id); };
  g_env.create_entity = [](const lfw::Value&) -> lfw::buff::IBuffEntity* {
    if (!lfw::truthy(g_create_entity_ok)) return nullptr;
    const std::u16string nid = u"E" + u16(std::to_string(++g_entity_seq));
    g_log.push_back("create_entity:" + u8(nid));
    return ensure(nid);
  };
  g_env.find_data = [](const std::u16string& oid) {
    g_log.push_back("find_data:" + u8(oid));
    return g_data;
  };
  g_env.world_buffs_set = [](const std::u16string& id, lfw::buff::Buff*) {
    g_log.push_back("world_buffs_set:" + u8(id));
  };
  g_env.world_buffs_get = [](const std::u16string& id) -> lfw::buff::Buff* {
    auto it = g_made.find(id);
    g_log.push_back("world_buffs_get:" + u8(id) + ":" + flag(it != g_made.end()));
    return it == g_made.end() ? nullptr : it->second.get();
  };
  g_env.create_buff = [](const std::u16string& kind, const std::u16string& id) -> lfw::buff::Buff* {
    const bool ok = lfw::truthy(g_create_buff_ok);
    g_log.push_back("create_buff:" + u8(kind) + ":" + u8(id) + ":" + flag(ok));
    if (!ok) return nullptr;
    auto b = std::make_unique<TestBuff>(&g_env, id, lfw::Value(kind));
    TestBuff* p = b.get();
    g_made.emplace(id, std::move(b));
    return p;
  };
}

std::string log_text() {
  std::string s;
  for (const std::string& e : g_log) s += " " + e;
  return s;
}

std::string buff_text(lfw::buff::Buff& b) {
  std::string s = " id=" + u8(b.id());
  s += " lvl=" + num(b.level());
  s += " mounted=" + flag(b.mounted());
  s += " lifetime=" + num(b.lifetime());
  s += " duration=" + num(b.duration());
  s += " ticks=" + num(b.ticks());
  s += " dead=" + flag(b.dead());
  s += " aid=" + render(lfw::Value(b.attacker_id()));
  s += " atk=" + render(lfw::Value(b.attacter() != nullptr
                                       ? lfw::Value(b.attacter()->id())
                                       : lfw::Value()));
  s += " nfx=" + num(static_cast<double>(b.effects_size()));
  s += " victims=[";
  for (size_t i = 0; i < b.victims().size(); ++i) {
    s += (i ? "," : "") + u8(b.victims()[i]);
  }
  s += "]";
  return s;
}

std::string buff_text() { return buff_text(g_buff); }
std::string buff_text_of(lfw::buff::Buff& b) { return buff_text(b); }

void emit(const std::string& line) { std::printf("%s\n", line.c_str()); }

}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: lfw_trace_buff <case-file>\n");
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
    if (t.size() < 2) {
      std::fprintf(stderr, "too few operands at line %d\n", lineno);
      return 2;
    }
    size_t i = 1;
    if (op == "env") {
      const std::string& sub = t[i++];
      if (sub == "entity") {
        const std::string raw_id = to_ascii(trace::parse_js_string_literal(t[i++]));
        const std::u16string id = raw_id == "@" ? u"E" + u16(std::to_string(g_entity_seq)) : u16(raw_id);
        const bool ok = t[i++] == "1";
        ensure(id)->exists = ok;
        emit("env entity " + u8(id) + " " + flag(ok));
      } else if (sub == "pos") {
        FakeEntity* e = ensure(trace::parse_js_string_literal(t[i++]));
        e->px = trace::to_double(t[i++]);
        e->py = trace::to_double(t[i++]);
        e->pz = trace::to_double(t[i++]);
        emit("env pos");
      } else if (sub == "frame") {
        FakeEntity* e = ensure(trace::parse_js_string_literal(t[i++]));
        e->centery = trace::to_double(t[i++]);
        e->height = trace::to_double(t[i++]);
        e->pic_h = trace::to_double(t[i++]);
        emit("env frame");
      } else if (sub == "data") {
        g_data = parse_value(t, i);
        emit("env data");
      } else if (sub == "create_entity_ok") {
        g_create_entity_ok = lfw::Value(t[i++] == "1");
        emit("env create_entity_ok");
      } else if (sub == "create_buff_ok") {
        g_create_buff_ok = lfw::Value(t[i++] == "1");
        emit("env create_buff_ok");
      } else {
        std::fprintf(stderr, "unknown env '%s' at line %d\n", sub.c_str(), lineno);
        return 2;
      }
      continue;
    }
    if (op == "buff") {
      const std::string& sub = t[i++];
      if (sub == "new") {
        const std::u16string id = trace::parse_js_string_literal(t[i++]);
        const lfw::Value kind = parse_value(t, i);
        g_buff = TestBuff(&g_env, id, kind);
        g_sync = nullptr;
        emit("buff new" + buff_text());
        continue;
      } else if (sub == "use") {
        const std::u16string id = trace::parse_js_string_literal(t[i++]);
        auto it = g_made.find(id);
        if (it != g_made.end()) {
          g_buff = *it->second;
          g_sync = it->second.get();
        }
        emit("buff use " + u8(id));
        continue;
      } else if (sub == "hook") {
        g_buff.hooks = t[i++];
        emit("buff hook " + g_buff.hooks);
        continue;
      } else if (sub == "oid") {
        g_buff.oid = to_ascii(trace::parse_js_string_literal(t[i++]));
        emit("buff oid " + g_buff.oid);
        continue;
      } else if (sub == "fid") {
        g_buff.fid = to_ascii(trace::parse_js_string_literal(t[i++]));
        emit("buff fid " + g_buff.fid);
        continue;
      } else if (sub == "level") {
        g_buff.set_level(trace::to_double(t[i++]));
      } else if (sub == "lifetime") {
        g_buff.set_lifetime(trace::to_double(t[i++]));
      } else if (sub == "duration") {
        g_buff.set_duration(trace::to_double(t[i++]));
      } else if (sub == "ticks") {
        g_buff.set_ticks(trace::to_double(t[i++]));
      } else if (sub == "attacker_id") {
        const std::u16string id = trace::parse_js_string_literal(t[i++]);
        g_buff.set_attacker_by_id(id);
      } else if (sub == "attacker_entity") {
        const std::u16string id = trace::parse_js_string_literal(t[i++]);
        g_buff.set_attacker_entity(find_entity(id));
      } else if (sub == "victim") {
        g_buff.set_victim(find_entity(trace::parse_js_string_literal(t[i++])));
      } else if (sub == "add_victim") {
        g_buff.add_victim(find_entity(trace::parse_js_string_literal(t[i++])));
      } else if (sub == "del_victim") {
        g_buff.del_victim(find_entity(trace::parse_js_string_literal(t[i++])));
      } else if (sub == "del_id") {
        const bool r = g_buff.call_del_id(trace::parse_js_string_literal(t[i++]));
        emit("buff del_id " + flag(r));
        continue;
      } else if (sub == "reset") {
        g_buff.reset(trace::parse_js_string_literal(t[i++]));
      } else if (sub == "mount") {
        g_buff.mount();
      } else if (sub == "unmount") {
        g_buff.unmount();
      } else if (sub == "update") {
        g_buff.update(trace::to_double(t[i++]));
      } else if (sub == "place_center") {
        const std::u16string vid = trace::parse_js_string_literal(t[i++]);
        g_buff.call_place_center(find_entity(vid), find_entity(vid));
      } else if (sub == "show") {
        g_buff.call_show(find_entity(trace::parse_js_string_literal(t[i++])));
      } else if (sub == "clear") {
        g_buff.call_clear();
      } else if (sub == "upd_fx") {
        g_buff.call_update_effects();
      } else if (sub == "del_fx") {
        g_buff.call_del_effect(trace::parse_js_string_literal(t[i++]));

      } else if (sub == "snap") {
        emit("buff snap " + render(g_buff.to_snapshot()));
        continue;
      } else if (sub == "read") {
        g_buff.read_snapshot(parse_value(t, i));
      } else if (sub == "log") {
        emit("buff log" + log_text());
        g_log.clear();
        continue;
      } else {
        std::fprintf(stderr, "unknown buff sub '%s' at line %d\n", sub.c_str(), lineno);
        return 2;
      }
      if (i != t.size()) {
        std::fprintf(stderr, "unexpected trailing token at line %d\n", lineno);
        return 2;
      }
      if (g_sync != nullptr) *g_sync = g_buff;
      emit("buff " + sub + buff_text() + " |" + log_text());
      g_log.clear();
      continue;
    }
    if (op == "grant") {
      const std::u16string kind = trace::parse_js_string_literal(t[i++]);
      const std::u16string aid = trace::parse_js_string_literal(t[i++]);
      const std::u16string vid = trace::parse_js_string_literal(t[i++]);
      const double dur = trace::to_double(t[i++]);
      lfw::buff::Buff* b = lfw::buff::grant_buff(&g_env, kind, find_entity(aid), find_entity(vid), dur);
      std::string s = b == nullptr ? std::string("-") : u8(b->id());
      std::string st = b == nullptr ? std::string() : buff_text_of(*b);
      emit("grant " + u8(kind) + " " + s + st + " |" + log_text());
      g_log.clear();
      continue;
    }
    std::fprintf(stderr, "unknown op '%s' at line %d\n", op.c_str(), lineno);
    return 2;
  }
  return 0;
}
