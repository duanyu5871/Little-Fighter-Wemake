#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "lfw/collision/calc_itr_velocity.h"
#include "lfw/collision/is_armor_work.h"
#include "lfw/collision/is_fall.h"
#include "lfw/core/value.h"
#include "lfw/entity/entity_dataset.h"

#include "trace_util.h"

namespace {

using trace::parse_value;
using trace::render_value;
using trace::split_ws;
using trace::to_ascii;

std::vector<std::pair<std::u16string, lfw::Value>> g_ent;
std::vector<std::pair<std::u16string, lfw::Value>> g_col;

void emit(const std::string& s) { std::printf("%s\n", s.c_str()); }

std::string render(const lfw::Value& v) { return to_ascii(render_value(v)); }

std::string render_num(double d) { return render(lfw::Value(d)); }

std::string flag(bool b) { return b ? "b1" : "b0"; }

lfw::Value* find_in(std::vector<std::pair<std::u16string, lfw::Value>>& reg,
                    const std::u16string& name) {
  for (auto& kv : reg) {
    if (kv.first == name) return &kv.second;
  }
  return nullptr;
}

void put_in(std::vector<std::pair<std::u16string, lfw::Value>>& reg, const std::u16string& name,
            lfw::Value v) {
  lfw::Value* p = find_in(reg, name);
  if (p != nullptr) {
    *p = std::move(v);
    return;
  }
  reg.push_back(std::make_pair(name, std::move(v)));
}

void del_in(std::vector<std::pair<std::u16string, lfw::Value>>& reg, const std::u16string& name) {
  for (std::size_t i = 0; i < reg.size(); ++i) {
    if (reg[i].first == name) {
      reg.erase(reg.begin() + static_cast<std::ptrdiff_t>(i));
      return;
    }
  }
}

lfw::Value arg_in(std::vector<std::pair<std::u16string, lfw::Value>>& reg,
                  const std::u16string& name, const char* kind, int lineno) {
  lfw::Value* p = find_in(reg, name);
  if (p == nullptr) {
    std::fprintf(stderr, "unknown %s '%s' at line %d\n", kind, to_ascii(name).c_str(), lineno);
    std::exit(2);
  }
  return *p;
}

lfw::Value make_collision(const lfw::Value& attacker, const lfw::Value& victim,
                          const lfw::Value& itr, const lfw::Value& bframe,
                          const lfw::Value& aframe) {
  lfw::Object o;
  o.set(u"attacker", attacker);
  o.set(u"victim", victim);
  o.set(u"itr", itr);
  o.set(u"bframe", bframe);
  o.set(u"aframe", aframe);
  return lfw::Value(std::make_shared<lfw::Object>(o));
}

}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: lfw_trace_collision_helpers <case-file>\n");
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

    if (op == "ds") {
      if (t.size() != 3) {
        std::fprintf(stderr, "ds needs <entity> <key> at line %d\n", lineno);
        return 2;
      }
      const std::u16string e_name = trace::parse_js_string_literal(t[1]);
      const std::u16string key = trace::parse_js_string_literal(t[2]);
      emit("ds " + to_ascii(e_name) + " " + to_ascii(key) + " " +
           render(lfw::entity::entity_dataset(arg_in(g_ent, e_name, "entity", lineno), key)));
      continue;
    }

    if (op == "ifall" || op == "armor" || op == "civ") {
      if (t.size() != 2) {
        std::fprintf(stderr, "%s needs <collision> at line %d\n", op.c_str(), lineno);
        return 2;
      }
      const std::u16string c_name = trace::parse_js_string_literal(t[1]);
      const lfw::Value c = arg_in(g_col, c_name, "collision", lineno);
      if (op == "ifall") {
        emit("ifall " + to_ascii(c_name) + " " + flag(lfw::collision::is_fall(c)));
      } else if (op == "armor") {
        emit("armor " + to_ascii(c_name) + " " + flag(lfw::collision::is_armor_work(c)));
      } else {
        const lfw::collision::ItrVelocity r = lfw::collision::calc_itr_velocity(c);
        emit("civ " + to_ascii(c_name) + " " + render_num(r.x) + " " + render_num(r.y) + " " +
             render_num(r.z) + " " + render(r.x_direction));
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
        put_in(g_ent, name, parse_value(t, i));
      } else if (sub == "del") {
        del_in(g_ent, name);
      } else {
        std::fprintf(stderr, "unknown ent sub '%s' at line %d\n", sub.c_str(), lineno);
        return 2;
      }
      emit("ent " + to_ascii(name));
    } else if (op == "col") {
      if (sub == "mk") {
        const std::u16string a_name = trace::parse_js_string_literal(t[i++]);
        const std::u16string v_name = trace::parse_js_string_literal(t[i++]);
        const lfw::Value itr = parse_value(t, i);
        const lfw::Value bframe = parse_value(t, i);
        const lfw::Value aframe = parse_value(t, i);
        put_in(g_col, name,
               make_collision(arg_in(g_ent, a_name, "entity", lineno),
                              arg_in(g_ent, v_name, "entity", lineno), itr, bframe, aframe));
      } else if (sub == "del") {
        del_in(g_col, name);
      } else {
        std::fprintf(stderr, "unknown col sub '%s' at line %d\n", sub.c_str(), lineno);
        return 2;
      }
      emit("col " + to_ascii(name));
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
