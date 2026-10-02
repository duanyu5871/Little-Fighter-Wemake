#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "lfw/collision/stiffness.h"
#include "lfw/core/value.h"
#include "lfw/entity/drink_info.h"

#include "trace_util.h"

namespace {

using trace::parse_value;
using trace::render_value;
using trace::split_ws;
using trace::to_ascii;

std::vector<std::pair<std::u16string, lfw::Value>> g_ent;
std::vector<std::pair<std::u16string, std::shared_ptr<lfw::DrinkInfo>>> g_di;

void emit(const std::string& s) { std::printf("%s\n", s.c_str()); }

std::string render(const lfw::Value& v) { return to_ascii(render_value(v)); }

std::string flag(bool b) { return b ? "b1" : "b0"; }

lfw::Value* find_in(std::vector<std::pair<std::u16string, lfw::Value>>& reg,
                    const std::u16string& name) {
  for (auto& kv : reg) {
    if (kv.first == name) return &kv.second;
  }
  return nullptr;
}

lfw::Value ent_arg(const std::u16string& name, int lineno) {
  lfw::Value* p = find_in(g_ent, name);
  if (p == nullptr) {
    std::fprintf(stderr, "unknown entity '%s' at line %d\n", to_ascii(name).c_str(), lineno);
    std::exit(2);
  }
  return *p;
}

std::shared_ptr<lfw::DrinkInfo> di_arg(const std::u16string& name, int lineno) {
  for (auto& kv : g_di) {
    if (kv.first == name) return kv.second;
  }
  std::fprintf(stderr, "unknown drink '%s' at line %d\n", to_ascii(name).c_str(), lineno);
  std::exit(2);
}

void di_put(const std::u16string& name, std::shared_ptr<lfw::DrinkInfo> v) {
  for (auto& kv : g_di) {
    if (kv.first == name) {
      kv.second = v;
      return;
    }
  }
  g_di.push_back(std::make_pair(name, v));
}

}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: lfw_trace_drink_stiffness <case-file>\n");
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

    if (op == "stf") {
      if (t.size() < 3) {
        std::fprintf(stderr, "too few operands at line %d\n", lineno);
        return 2;
      }
      const std::u16string e_name = trace::parse_js_string_literal(t[1]);
      std::size_t i = 2;
      const lfw::Value itr = parse_value(t, i);
      lfw::Object col;
      col.set(u"itr", itr);
      col.set(u"attacker", ent_arg(e_name, lineno));
      const lfw::collision::Stiffness st = lfw::collision::calc_stiffness(
          lfw::Value(std::make_shared<lfw::Object>(col)));
      emit("stf " + to_ascii(e_name) + " " + render(st.motionless) + " " + render(st.shaking));
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
        lfw::Value* p = find_in(g_ent, name);
        if (p != nullptr) {
          *p = parse_value(t, i);
        } else {
          g_ent.push_back(std::make_pair(name, parse_value(t, i)));
        }
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
    } else if (op == "di") {
      if (sub == "new") {
        di_put(name, std::make_shared<lfw::DrinkInfo>(parse_value(t, i)));
        emit("di " + to_ascii(name) + " " + render(di_arg(name, lineno)->to_snapshot()));
      } else {
        std::shared_ptr<lfw::DrinkInfo> d = di_arg(name, lineno);
        const std::string tag = "di " + to_ascii(name);
        if (sub == "set") {
          const std::u16string field = trace::parse_js_string_literal(t[i++]);
          const lfw::Value v = parse_value(t, i);
          if (field == u"hp_h_value") d->set_hp_h_value(v);
          else if (field == u"hp_h_total") d->set_hp_h_total(v);
          else if (field == u"hp_h") d->set_hp_h(v);
          else if (field == u"hp_r_value") d->set_hp_r_value(v);
          else if (field == u"hp_r_total") d->set_hp_r_total(v);
          else if (field == u"hp_r") d->set_hp_r(v);
          else if (field == u"mp_h_value") d->set_mp_h_value(v);
          else if (field == u"mp_h_total") d->set_mp_h_total(v);
          else if (field == u"mp_h") d->set_mp_h(v);
          else {
            std::fprintf(stderr, "unknown field at line %d\n", lineno);
            return 2;
          }
          emit(tag + " " + render(d->to_snapshot()));
        } else if (sub == "load") {
          d->from_snapshot(parse_value(t, i));
          emit(tag + " " + render(d->to_snapshot()));
        } else if (sub == "snap") {
          emit(tag + " " + render(d->to_snapshot()));
        } else if (sub == "empty") {
          emit(tag + " empty " + flag(d->hp_h_empty()) + " " + flag(d->hp_r_empty()) + " " +
               flag(d->mp_h_empty()));
        } else {
          std::fprintf(stderr, "unknown di sub '%s' at line %d\n", sub.c_str(), lineno);
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
