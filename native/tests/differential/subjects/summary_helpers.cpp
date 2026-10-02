#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "lfw/core/value.h"
#include "lfw/entity/summary.h"
#include "lfw/entity/summary_mgr.h"

#include "trace_util.h"

namespace {

using trace::parse_value;
using trace::render_value;
using trace::split_ws;
using trace::to_ascii;

std::vector<std::pair<std::u16string, lfw::Value>> g_ent;
std::vector<std::pair<std::u16string, std::shared_ptr<lfw::SummaryMgr>>> g_mgr;

void emit(const std::string& s) { std::printf("%s\n", s.c_str()); }

std::string render(const lfw::Value& v) { return to_ascii(render_value(v)); }

lfw::Value* find_in(std::vector<std::pair<std::u16string, lfw::Value>>& reg,
                    const std::u16string& name) {
  for (auto& kv : reg) {
    if (kv.first == name) return &kv.second;
  }
  return nullptr;
}

void put_ent(const std::u16string& name, lfw::Value v) {
  lfw::Value* p = find_in(g_ent, name);
  if (p != nullptr) {
    *p = std::move(v);
    return;
  }
  g_ent.push_back(std::make_pair(name, std::move(v)));
}

lfw::Value ent_arg(const std::u16string& name, int lineno) {
  lfw::Value* p = find_in(g_ent, name);
  if (p == nullptr) {
    std::fprintf(stderr, "unknown entity '%s' at line %d\n", to_ascii(name).c_str(), lineno);
    std::exit(2);
  }
  return *p;
}

std::shared_ptr<lfw::SummaryMgr> find_mgr(const std::u16string& name) {
  for (auto& kv : g_mgr) {
    if (kv.first == name) return kv.second;
  }
  return nullptr;
}

std::shared_ptr<lfw::SummaryMgr> mgr_arg(const std::u16string& name, int lineno) {
  std::shared_ptr<lfw::SummaryMgr> m = find_mgr(name);
  if (m == nullptr) {
    std::fprintf(stderr, "unknown manager '%s' at line %d\n", to_ascii(name).c_str(), lineno);
    std::exit(2);
  }
  return m;
}

std::string snap(const lfw::Summary& s) {
  return "id=" + trace::esc(s.id()) + " dmg=" + render(s.damage_sum()) +
         " kill=" + render(s.kill_sum()) + " pick=" + render(s.picking_sum()) +
         " hp=" + render(s.hp_lost()) + " mp=" + render(s.mp_usage());
}

std::string items_of(const std::string& prefix, const lfw::SummaryMgr& m) {
  std::string s = prefix + " items";
  for (const auto& kv : m.items()) s += " " + to_ascii(kv.first);
  s += " graves=" + std::to_string(m.grave_count());
  return s;
}

}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: lfw_trace_summary_helpers <case-file>\n");
    return 2;
  }

  std::ifstream in(argv[1]);
  if (!in) {
    std::fprintf(stderr, "cannot open case file: %s\n", argv[1]);
    return 2;
  }

  std::string raw;
  int lineno = 0;
  while (std::getline(in, raw)) {
    ++lineno;
    const std::string line = trace::strip_comment(raw);
    const std::vector<std::string> t = split_ws(line);
    if (t.empty()) continue;
    const std::string& op = t[0];

    if (op == "sg") {
      if (t.size() == 2 && t[1] == "items") {
        emit(items_of("sg", lfw::summary_mgr()));
        continue;
      }
      if (t.size() == 3 && t[1] == "get") {
        const std::u16string id = trace::parse_js_string_literal(t[2]);
        emit("sg get " + to_ascii(id) + " " + snap(*lfw::summary_mgr().get(id)));
        continue;
      }
      std::fprintf(stderr, "unknown sg operands at line %d\n", lineno);
      return 2;
    }

    if (op == "su") {
      if (t.size() < 4) {
        std::fprintf(stderr, "too few operands at line %d\n", lineno);
        return 2;
      }
      const std::string& sub = t[1];
      const std::u16string mgr_name = trace::parse_js_string_literal(t[2]);
      const std::u16string id = trace::parse_js_string_literal(t[3]);
      std::size_t i = 4;
      std::shared_ptr<lfw::SummaryMgr> mgr = mgr_arg(mgr_name, lineno);
      const std::string mgr_s = to_ascii(mgr_name);
      const std::string id_s = to_ascii(id);
      if (sub == "on") {
        const std::u16string event = trace::parse_js_string_literal(t[i++]);
        std::shared_ptr<lfw::Summary> s = mgr->get(id);
        const std::u16string ev = event;
        s->callbacks.on(event, [mgr_s, id_s, ev](const std::vector<lfw::Value>& args) {
          std::string out = "cb " + mgr_s + " " + id_s + " " + to_ascii(ev);
          out += " v=" + (args.size() > 0 ? render(args[0]) : std::string("u"));
          out += " o=" + (args.size() > 1 ? render(args[1]) : std::string("u"));
          emit(out);
        });
        emit("su " + mgr_s + " " + id_s + " on " + to_ascii(event));
      } else {
        std::shared_ptr<lfw::Summary> s = mgr->get(id);
        if (sub == "set") {
          const std::u16string field = trace::parse_js_string_literal(t[i++]);
          const lfw::Value v = parse_value(t, i);
          if (field == u"damage_sum") s->set_damage_sum(v);
          else if (field == u"kill_sum") s->set_kill_sum(v);
          else if (field == u"picking_sum") s->set_picking_sum(v);
          else if (field == u"hp_lost") s->set_hp_lost(v);
          else if (field == u"mp_usage") s->set_mp_usage(v);
          else {
            std::fprintf(stderr, "unknown field '%s' at line %d\n", to_ascii(field).c_str(),
                         lineno);
            return 2;
          }
        } else if (sub == "reset") {
          s->reset(id);
        } else if (sub == "rel") {
          s->release();
        } else if (sub != "snap") {
          std::fprintf(stderr, "unknown su sub '%s' at line %d\n", sub.c_str(), lineno);
          return 2;
        }
        emit("su " + mgr_s + " " + id_s + " " + snap(*s));
      }
      if (i != t.size()) {
        std::fprintf(stderr, "trailing token(s) at line %d\n", lineno);
        return 2;
      }
      continue;
    }

    if (t.size() < 3) {
      std::fprintf(stderr, "too few operands at line %d\n", lineno);
      return 2;
    }
    const std::string& sub = t[1];
    const std::u16string name = trace::parse_js_string_literal(t[2]);
    std::size_t i = 3;

    if (op == "ent") {
      if (sub == "put") {
        put_ent(name, parse_value(t, i));
      } else if (sub == "del") {
        for (std::size_t k = 0; k < g_ent.size(); ++k) {
          if (g_ent[k].first == name) {
            g_ent.erase(g_ent.begin() + static_cast<std::ptrdiff_t>(k));
            break;
          }
        }
      } else {
        std::fprintf(stderr, "unknown ent sub '%s' at line %d\n", sub.c_str(), lineno);
        return 2;
      }
      emit("ent " + to_ascii(name));
    } else if (op == "sm") {
      if (sub == "new") {
        std::shared_ptr<lfw::SummaryMgr> made = std::make_shared<lfw::SummaryMgr>();
        bool replaced = false;
        for (auto& kv : g_mgr) {
          if (kv.first == name) {
            kv.second = made;
            replaced = true;
            break;
          }
        }
        if (!replaced) g_mgr.push_back(std::make_pair(name, made));
      } else {
        std::shared_ptr<lfw::SummaryMgr> mgr = mgr_arg(name, lineno);
        const std::string mgr_s = to_ascii(name);
        if (sub == "get") {
          const std::u16string id = trace::parse_js_string_literal(t[i++]);
          emit("sm " + mgr_s + " get " + to_ascii(id) + " " + snap(*mgr->get(id)));
          if (i != t.size()) {
            std::fprintf(stderr, "trailing token(s) at line %d\n", lineno);
            return 2;
          }
          continue;
        } else if (sub == "items") {
          emit(items_of("sm " + mgr_s, *mgr));
        } else if (sub == "release") {
          mgr->release(trace::parse_js_string_literal(t[i++]));
          emit(items_of("sm " + mgr_s, *mgr));
        } else if (sub == "clear") {
          mgr->clear();
          emit(items_of("sm " + mgr_s, *mgr));
        } else if (sub == "dmg") {
          const std::u16string a_name = trace::parse_js_string_literal(t[i++]);
          const lfw::Value value = parse_value(t, i);
          mgr->add_damage_sum(ent_arg(a_name, lineno), value);
          emit(items_of("sm " + mgr_s, *mgr));
        } else if (sub == "kill") {
          const std::u16string a_name = trace::parse_js_string_literal(t[i++]);
          const lfw::Value value = i < t.size() ? parse_value(t, i) : lfw::Value();
          mgr->add_kill_sum(ent_arg(a_name, lineno), value);
          emit(items_of("sm " + mgr_s, *mgr));
        } else if (sub == "apply") {
          const std::u16string a_name = trace::parse_js_string_literal(t[i++]);
          const lfw::Value injury = parse_value(t, i);
          const std::u16string v_name = trace::parse_js_string_literal(t[i++]);
          const lfw::Value prev_hp = parse_value(t, i);
          mgr->apply_damage(ent_arg(a_name, lineno), injury, ent_arg(v_name, lineno), prev_hp);
          emit(items_of("sm " + mgr_s, *mgr));
        } else {
          std::fprintf(stderr, "unknown sm sub '%s' at line %d\n", sub.c_str(), lineno);
          return 2;
        }
      }
    } else {
      std::fprintf(stderr, "unknown op '%s' at line %d\n", op.c_str(), lineno);
      return 2;
    }

    if (i != t.size()) {
      std::fprintf(stderr, "trailing token(s) at line %d\n", lineno);
      return 2;
    }
  }
  return 0;
}
