#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "lfw/base/clock.h"
#include "lfw/cases.h"
#include "lfw/core/value.h"
#include "lfw/helper/randoming.h"
#include "lfw/state/spawn_ice_piece.h"
#include "lfw/utils/math/mersenne_twister.h"

#include "trace_util.h"

namespace {

using trace::parse_value;
using trace::render_value;
using trace::split_ws;
using trace::to_ascii;

struct TestClock : lfw::IClock {
  double value = 0.0;
  double now() const override { return value; }
  int add(std::function<void()> handler) override {
    (void)handler;
    return 0;
  }
  void del(int handle) override { (void)handle; }
  bool hidden() const override { return false; }
};

TestClock g_clock;

std::vector<std::pair<std::u16string, std::shared_ptr<lfw::MersenneTwister>>> g_mts;
std::vector<std::pair<std::u16string, std::shared_ptr<lfw::Randoming>>> g_rqs;
std::vector<std::pair<std::u16string, lfw::Value>> g_ents;

void emit(const std::string& s) { std::printf("%s\n", s.c_str()); }

std::string render(const lfw::Value& v) { return to_ascii(render_value(v)); }

void fatal(const char* what, const std::string& detail, int lineno) {
  std::fprintf(stderr, "%s '%s' at line %d\n", what, detail.c_str(), lineno);
  std::exit(2);
}

template <typename T>
T* find_in(std::vector<std::pair<std::u16string, T>>& reg, const std::u16string& name) {
  for (auto& kv : reg) {
    if (kv.first == name) return &kv.second;
  }
  return nullptr;
}

lfw::Value* ent_arg(const std::u16string& name, int lineno) {
  lfw::Value* p = find_in(g_ents, name);
  if (p == nullptr) fatal("unknown entity", to_ascii(name), lineno);
  return p;
}

std::shared_ptr<lfw::Randoming> rq_arg(const std::u16string& name, int lineno) {
  auto* p = find_in(g_rqs, name);
  if (p == nullptr) fatal("unknown randoming", to_ascii(name), lineno);
  return *p;
}

std::shared_ptr<lfw::MersenneTwister> mt_arg(const std::u16string& name, int lineno) {
  auto* p = find_in(g_mts, name);
  if (p == nullptr) fatal("unknown mt", to_ascii(name), lineno);
  return *p;
}

void put_mt(const std::u16string& name, std::shared_ptr<lfw::MersenneTwister> v) {
  auto* p = find_in(g_mts, name);
  if (p != nullptr) {
    *p = std::move(v);
    return;
  }
  g_mts.push_back(std::make_pair(name, std::move(v)));
}

void put_rq(const std::u16string& name, std::shared_ptr<lfw::Randoming> v) {
  auto* p = find_in(g_rqs, name);
  if (p != nullptr) {
    *p = std::move(v);
    return;
  }
  g_rqs.push_back(std::make_pair(name, std::move(v)));
}

std::string mt_label(lfw::MersenneTwister* mt) {
  for (auto& kv : g_mts) {
    if (kv.second.get() == mt) return to_ascii(kv.first);
  }
  if (mt == &lfw::Randoming::default_mt()) return "default";
  return "?";
}

lfw::MersenneTwister* mt_token(const std::string& tok, int lineno) {
  if (tok == "-") return nullptr;
  return mt_arg(trace::parse_js_string_literal(tok), lineno).get();
}

std::vector<lfw::Value> as_list(const lfw::Value& v, int lineno) {
  const lfw::Array* arr = lfw::as_array(v);
  if (arr == nullptr) fatal("source literal is not an array", "", lineno);
  return arr->items();
}

std::string rq_dump(const std::u16string& name, const lfw::Randoming& r) {
  return "rn " + to_ascii(name) + " dump name=" + to_ascii(r.name()) +
         " mt=" + mt_label(r.mt()) + " dup=" + render(r.duplicate()) +
         " taken=" + render(r.taken()) + " src=" + render(lfw::Value(std::make_shared<lfw::Array>(r.src()))) +
         " cur=" + render(lfw::Value(std::make_shared<lfw::Array>(r.cur())));
}

}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: lfw_trace_mt_random <case-file>\n");
    return 2;
  }
  g_clock.value = 1700000000000.0;
  lfw::set_clock(&g_clock);

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

    if (op == "ip") {
      if (t.size() < 2) fatal("too few operands", line, lineno);
      const std::string& sub = t[1];
      if (sub == "table") {
        emit("ip table " + render(lfw::Value(std::make_shared<lfw::Array>(lfw::state::ice_piece_opoints()))));
        if (t.size() != 2) fatal("trailing token(s)", line, lineno);
        continue;
      }
      if (sub == "spawn") {
        if (t.size() < 4) fatal("too few operands", line, lineno);
        const std::u16string name = trace::parse_js_string_literal(t[2]);
        std::size_t i = 3;
        const lfw::Value id = parse_value(t, i);
        if (i != t.size()) fatal("trailing token(s)", line, lineno);
        const std::u16string* sid = std::get_if<std::u16string>(&id);
        if (sid == nullptr) fatal("opoint id is not a string", line, lineno);
        emit("ip " + to_ascii(name) + " " + render(lfw::state::spawn_ice_piece(*sid)));
        continue;
      }
      if (sub == "dvx" || sub == "dvy" || sub == "x" || sub == "y") {
        if (t.size() < 4) fatal("too few operands", line, lineno);
        lfw::MersenneTwister* mt = mt_token(t[2], lineno);
        lfw::MersenneTwister& ref = mt != nullptr ? *mt : lfw::Randoming::default_mt();
        const lfw::Value* e = ent_arg(trace::parse_js_string_literal(t[3]), lineno);
        lfw::Value out;
        if (sub == "dvx") out = lfw::state::ice_piece_dvx(*e, ref);
        else if (sub == "dvy") out = lfw::state::ice_piece_dvy(*e, ref);
        else if (sub == "x") out = lfw::state::ice_piece_x(*e, ref);
        else out = lfw::state::ice_piece_y(*e, ref);
        if (t.size() != 4) fatal("trailing token(s)", line, lineno);
        emit("ip " + sub + " " + render(out));
        continue;
      }
      fatal("unknown ip sub", sub, lineno);
    }

    if (op == "cases") {
      if (t.size() != 1) fatal("trailing token(s)", line, lineno);
      const std::u16string text = lfw::mt_cases().submit();
      emit("cases " + render(lfw::Value(text)) + " " +
           render(lfw::Value(static_cast<double>(lfw::mt_cases().cases().size()))));
      continue;
    }

    if (t.size() < 3) {
      std::fprintf(stderr, "too few operands at line %d\n", lineno);
      return 2;
    }
    const std::string& sub = t[1];
    const std::u16string name = trace::parse_js_string_literal(t[2]);
    std::size_t i = 3;

    if (op == "mt") {
      if (sub == "new") {
        const lfw::Value seed = parse_value(t, i);
        if (i != t.size()) fatal("trailing token(s)", line, lineno);
        put_mt(name, std::make_shared<lfw::MersenneTwister>(lfw::to_number(seed)));
        emit("mt " + to_ascii(name));
      } else if (sub == "dbg") {
        lfw::MersenneTwister& ref = *mt_arg(name, lineno);
        ref.debugging = lfw::truthy(parse_value(t, i));
        emit("mt " + to_ascii(name) + " dbg " + render(lfw::Value(ref.debugging)) +
             " mark=" + render(lfw::Value(ref.mark)));
      } else if (sub == "mark") {
        emit("mt " + to_ascii(name) + " mark " +
             render(lfw::Value(mt_arg(name, lineno)->mark)));
      } else {
        fatal("unknown mt sub", sub, lineno);
      }
    } else if (op == "ent") {
      if (sub == "put") {
        lfw::Value* p = find_in(g_ents, name);
        if (p != nullptr) {
          *p = parse_value(t, i);
        } else {
          g_ents.push_back(std::make_pair(name, parse_value(t, i)));
        }
      } else if (sub == "del") {
        for (std::size_t k = 0; k < g_ents.size(); ++k) {
          if (g_ents[k].first == name) {
            g_ents.erase(g_ents.begin() + static_cast<std::ptrdiff_t>(k));
            break;
          }
        }
      } else {
        fatal("unknown ent sub", sub, lineno);
      }
      emit("ent " + to_ascii(name));
    } else if (op == "rn") {
      if (sub == "new" || sub == "create") {
        if (i >= t.size()) fatal("missing mt token", line, lineno);
        lfw::MersenneTwister* mt = mt_token(t[i++], lineno);
        const lfw::Value src = parse_value(t, i);
        std::vector<lfw::Value> list = as_list(src, lineno);
        lfw::Value dup = lfw::Value(false);
        if (sub == "create" || i < t.size()) dup = parse_value(t, i);
        std::shared_ptr<lfw::Randoming> r;
        if (sub == "new") {
          r = std::make_shared<lfw::Randoming>(name, std::move(list), mt, dup);
        } else {
          r = lfw::Randoming::create(name, std::move(list), mt, dup);
        }
        put_rq(name, r);
        emit(rq_dump(name, *r));
      } else {
        std::shared_ptr<lfw::Randoming> r = rq_arg(name, lineno);
        if (sub == "src") {
          r->set_src(as_list(parse_value(t, i), lineno));
          emit("rn " + to_ascii(name) + " set_src " +
               render(lfw::Value(std::make_shared<lfw::Array>(r->cur()))));
        } else if (sub == "get") {
          emit("rn " + to_ascii(name) + " get " + render(r->get()));
        } else if (sub == "dump") {
          emit(rq_dump(name, *r));
        } else {
          fatal("unknown rn sub", sub, lineno);
        }
      }
    } else {
      fatal("unknown op", op, lineno);
    }

    if (i != t.size()) fatal("trailing token(s)", line, lineno);
  }
  return 0;
}
