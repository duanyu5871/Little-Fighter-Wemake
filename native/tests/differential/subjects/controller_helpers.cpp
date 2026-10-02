#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <map>
#include <string>
#include <vector>

#include "lfw/controller/controller_double_clicks.h"
#include "lfw/controller/double_click.h"
#include "lfw/controller/key_status.h"
#include "lfw/controller/seq_keys.h"
#include "lfw/core/value.h"

#include "trace_util.h"

namespace {

using trace::parse_value;
using trace::render_value;
using trace::split_ws;
using trace::to_ascii;

std::map<std::u16string, lfw::controller::DoubleClick> g_dc;
std::map<std::u16string, lfw::controller::SeqKeys> g_sk;
std::map<std::u16string, lfw::controller::KeyStatus> g_ks;
std::map<std::u16string, lfw::controller::ControllerDoubleClicks> g_cdc;

void emit(const std::string& line) { std::printf("%s\n", line.c_str()); }

std::string render(const lfw::Value& v) { return to_ascii(render_value(v)); }

std::string flag(bool b) { return b ? "b1" : "b0"; }

lfw::controller::DoubleClick* find_dc(const std::u16string& name) {
  auto it = g_dc.find(name);
  return it == g_dc.end() ? nullptr : &it->second;
}

lfw::controller::SeqKeys* find_sk(const std::u16string& name) {
  auto it = g_sk.find(name);
  return it == g_sk.end() ? nullptr : &it->second;
}

lfw::controller::KeyStatus* find_ks(const std::u16string& name) {
  auto it = g_ks.find(name);
  return it == g_ks.end() ? nullptr : &it->second;
}

lfw::controller::ControllerDoubleClicks* find_cdc(const std::u16string& name) {
  auto it = g_cdc.find(name);
  return it == g_cdc.end() ? nullptr : &it->second;
}

}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: lfw_trace_controller_helpers <case-file>\n");
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
    if (t.size() < 3) {
      std::fprintf(stderr, "too few operands at line %d\n", lineno);
      return 2;
    }
    const std::string& sub = t[1];
    const std::u16string name = trace::parse_js_string_literal(t[2]);
    size_t i = 3;

    if (op == "dc") {
      if (sub == "new") {
        const lfw::Value ctor = parse_value(t, i);
        g_dc.insert_or_assign(name, lfw::controller::DoubleClick(ctor));
      } else {
        lfw::controller::DoubleClick* inst = find_dc(name);
        if (inst == nullptr) {
          std::fprintf(stderr, "unknown dc '%s' at line %d\n", to_ascii(name).c_str(), lineno);
          return 2;
        }
        if (sub == "press") {
          const double time = trace::to_double(t[i++]);
          const double interval = trace::to_double(t[i++]);
          const lfw::Value data = parse_value(t, i);
          inst->press(time, data, interval);
        } else if (sub == "step") {
          inst->step();
        } else if (sub == "reset") {
          inst->reset();
        } else if (sub == "load") {
          inst->from_snapshot(parse_value(t, i));
        } else if (sub != "snap") {
          std::fprintf(stderr, "unknown dc sub '%s' at line %d\n", sub.c_str(), lineno);
          return 2;
        }
      }
      emit("dc " + to_ascii(name) + " " + render(find_dc(name)->to_snapshot()));
    } else if (op == "sk") {
      if (sub == "new") {
        const std::u16string keys = lfw::to_string(parse_value(t, i));
        const lfw::Value data = parse_value(t, i);
        g_sk.insert_or_assign(name, lfw::controller::SeqKeys(keys, data));
      } else {
        lfw::controller::SeqKeys* inst = find_sk(name);
        if (inst == nullptr) {
          std::fprintf(stderr, "unknown sk '%s' at line %d\n", to_ascii(name).c_str(), lineno);
          return 2;
        }
        if (sub == "press") {
          inst->press(lfw::to_string(parse_value(t, i)));
        } else if (sub == "reset") {
          inst->reset();
        } else if (sub == "load") {
          inst->from_snapshot(parse_value(t, i));
        } else if (sub != "snap") {
          std::fprintf(stderr, "unknown sk sub '%s' at line %d\n", sub.c_str(), lineno);
          return 2;
        }
      }
      emit("sk " + to_ascii(name) + " " + render(find_sk(name)->to_snapshot()));
    } else if (op == "ks") {
      if (sub == "new") {
        g_ks.insert_or_assign(name, lfw::controller::KeyStatus(parse_value(t, i)));
      } else {
        lfw::controller::KeyStatus* inst = find_ks(name);
        if (inst == nullptr) {
          std::fprintf(stderr, "unknown ks '%s' at line %d\n", to_ascii(name).c_str(), lineno);
          return 2;
        }
        if (sub == "hit") {
          const double time = trace::to_double(t[i++]);
          const lfw::Value t_arg = i < t.size() ? parse_value(t, i) : lfw::Value();
          inst->hit(t_arg, time);
        } else if (sub == "end") {
          inst->end(trace::to_double(t[i++]));
        } else if (sub == "reset") {
          inst->reset();
        } else if (sub == "load") {
          inst->from_snapshot(parse_value(t, i));
        } else if (sub == "use") {
          const double back = inst->use();
          emit("ks " + to_ascii(name) + " use " + render(lfw::Value(back)) + " " +
               render(inst->to_snapshot()));
          continue;
        } else if (sub == "query") {
          const double time = trace::to_double(t[i++]);
          const double duration = trace::to_double(t[i++]);
          emit("ks " + to_ascii(name) + " query " + flag(inst->is_start(time)) + " " +
               flag(inst->is_hit(time, duration)) + " " + flag(inst->is_hld(time, duration)) +
               " " + flag(inst->is_end()));
          continue;
        } else if (sub != "snap") {
          std::fprintf(stderr, "unknown ks sub '%s' at line %d\n", sub.c_str(), lineno);
          return 2;
        }
      }
      emit("ks " + to_ascii(name) + " " + render(find_ks(name)->to_snapshot()));
    } else if (op == "cdc") {
      if (sub == "new") {
        g_cdc.insert_or_assign(name, lfw::controller::ControllerDoubleClicks());
      } else {
        lfw::controller::ControllerDoubleClicks* inst = find_cdc(name);
        if (inst == nullptr) {
          std::fprintf(stderr, "unknown cdc '%s' at line %d\n", to_ascii(name).c_str(), lineno);
          return 2;
        }
        if (sub == "reset") {
          inst->reset();
        } else if (sub == "load") {
          inst->from_snapshot(parse_value(t, i));
        } else if (sub == "press") {
          lfw::controller::DoubleClick* slot = inst->slot(lfw::to_string(parse_value(t, i)));
          if (slot == nullptr) {
            std::fprintf(stderr, "unknown cdc slot at line %d\n", lineno);
            return 2;
          }
          const double time = trace::to_double(t[i++]);
          const double interval = trace::to_double(t[i++]);
          slot->press(time, parse_value(t, i), interval);
        } else if (sub != "snap") {
          std::fprintf(stderr, "unknown cdc sub '%s' at line %d\n", sub.c_str(), lineno);
          return 2;
        }
      }
      emit("cdc " + to_ascii(name) + " " + render(find_cdc(name)->to_snapshot()));
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
