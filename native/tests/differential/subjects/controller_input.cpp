#include <cstdio>
#include <fstream>
#include <map>
#include <string>
#include <variant>
#include <vector>

#include "lfw/controller/controller_key_status.h"
#include "lfw/controller/controller_result.h"
#include "lfw/core/value.h"
#include "lfw/defines/game_key.h"

#include "trace_util.h"

namespace {

using trace::parse_value;
using trace::render_value;
using trace::split_ws;
using trace::to_ascii;

std::map<std::u16string, lfw::controller::ControllerKeyStatus> g_ks;
std::map<std::u16string, lfw::controller::ControllerResult> g_res;

void emit(const std::string& line) { std::printf("%s\n", line.c_str()); }

std::string render(const lfw::Value& v) { return to_ascii(render_value(v)); }

std::string flag(bool b) { return b ? "b1" : "b0"; }

std::u16string str_of(const std::string& s) {
  return std::u16string(s.begin(), s.end());
}

std::string result_text(const lfw::Value& v) {
  return std::holds_alternative<std::monostate>(v) ? std::string("-") : render(v);
}

lfw::controller::ControllerKeyStatus* find_ks(const std::u16string& name) {
  auto it = g_ks.find(name);
  return it == g_ks.end() ? nullptr : &it->second;
}

lfw::controller::ControllerResult* find_res(const std::u16string& name) {
  auto it = g_res.find(name);
  return it == g_res.end() ? nullptr : &it->second;
}

lfw::controller::KeyStatus* need_slot(lfw::controller::ControllerKeyStatus* inst,
                                      const std::string& key, int lineno) {
  lfw::controller::KeyStatus* p = inst->slot(str_of(key));
  if (p == nullptr) {
    std::fprintf(stderr, "unknown key '%s' at line %d\n", key.c_str(), lineno);
  }
  return p;
}

std::string fields_text(const lfw::controller::ControllerResult& r) {
  return render(lfw::Value(r.time())) + " " + render(lfw::Value(r.keys())) + " " +
         render(lfw::Value(r.kind())) + " " + result_text(r.result());
}

const char16_t* const kOrder[7] = {u"L", u"R", u"U", u"D", u"d", u"j", u"a"};

}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: lfw_trace_controller_input <case-file>\n");
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
    const std::vector<std::string> t = split_ws(trace::strip_comment(raw));
    if (t.empty()) continue;
    const std::string& op = t[0];
    if (t.size() < 2) {
      std::fprintf(stderr, "too few operands at line %d\n", lineno);
      return 2;
    }
    size_t i = 1;

    if (op == "ks") {
      const std::string& sub = t[i++];
      if (sub == "new") {
        const std::u16string name = trace::parse_js_string_literal(t[i++]);
        g_ks.insert_or_assign(name, lfw::controller::ControllerKeyStatus());
        emit("ks " + to_ascii(name) + " " + render(g_ks.at(name).to_snapshot()));
      } else {
        const std::u16string name = trace::parse_js_string_literal(t[i++]);
        lfw::controller::ControllerKeyStatus* inst = find_ks(name);
        if (inst == nullptr) {
          std::fprintf(stderr, "unknown ks '%s' at line %d\n", to_ascii(name).c_str(), lineno);
          return 2;
        }
        if (sub == "hit") {
          const std::string key = to_ascii(trace::parse_js_string_literal(t[i++]));
          lfw::Value tv;
          if (t[i] == "-") {
            ++i;
          } else {
            tv = parse_value(t, i);
          }
          const double time = trace::to_double(t[i++]);
          need_slot(inst, key, lineno)->hit(tv, time);
        } else if (sub == "end") {
          const std::string key = to_ascii(trace::parse_js_string_literal(t[i++]));
          const double time = trace::to_double(t[i++]);
          need_slot(inst, key, lineno)->end(time);
        } else if (sub == "reset") {
          inst->reset();
        } else if (sub == "load") {
          inst->from_snapshot(parse_value(t, i));
        } else if (sub == "slot") {
          const std::u16string key = trace::parse_js_string_literal(t[i++]);
          const lfw::controller::ControllerKeyStatus& cref = *inst;
          const lfw::controller::KeyStatus* p = cref.slot(key);
          emit("ks " + to_ascii(name) + " slot " + to_ascii(key) + " " + flag(p != nullptr));
          continue;
        } else if (sub == "use") {
          const std::string key = to_ascii(trace::parse_js_string_literal(t[i++]));
          const double ret = need_slot(inst, key, lineno)->use();
          emit("ks " + to_ascii(name) + " use " + render(lfw::Value(ret)) + " " +
               render(inst->to_snapshot()));
          continue;
        } else if (sub == "flags") {
          const std::string key = to_ascii(trace::parse_js_string_literal(t[i++]));
          const double time = trace::to_double(t[i++]);
          const double dur = trace::to_double(t[i++]);
          const lfw::controller::KeyStatus* p = need_slot(inst, key, lineno);
          emit("ks " + to_ascii(name) + " flags " + flag(p->is_start(time)) + " " +
               flag(p->is_hit(time, dur)) + " " + flag(p->is_hld(time, dur)) + " " +
               flag(p->is_end()));
          continue;
        } else if (sub == "raw") {
          std::string s;
          for (int k = 0; k < 7; ++k) {
            const lfw::controller::KeyStatus* p = inst->slot(std::u16string(kOrder[k]));
            s += " ";
            if (p == nullptr) {
              s += "-";
            } else {
              s += render(p->key()) + "|" + render(lfw::Value(p->time())) + "/" +
                   render(lfw::Value(p->u_time())) + "/" + render(lfw::Value(p->used()));
            }
          }
          emit("ks " + to_ascii(name) + " raw" + s);
          continue;
        } else if (sub != "snap") {
          std::fprintf(stderr, "unknown ks sub '%s' at line %d\n", sub.c_str(), lineno);
          return 2;
        }
        emit("ks " + to_ascii(name) + " " + render(inst->to_snapshot()));
      }
    } else if (op == "gktable") {
      const std::string& what = t[i++];
      if (what == "labels") {
        std::string s;
        for (const auto& e : lfw::gk_label_table()) {
          s += " ";
          s += to_ascii(std::u16string(e.first)) + "=" + to_ascii(std::u16string(e.second));
        }
        emit("gktable labels" + s);
      } else if (what == "label") {
        const std::u16string key = trace::parse_js_string_literal(t[i++]);
        const std::u16string label = lfw::gk_label_of(key);
        emit("gktable label " + to_ascii(key) + " " +
             (label.empty() ? std::string("-") : to_ascii(label)));
      } else if (what == "agk") {
        std::string s;
        for (const char16_t* k : lfw::all_game_keys()) {
          s += " " + to_ascii(std::u16string(k));
        }
        emit("gktable agk" + s);
      } else if (what == "conflicts") {
        std::string s;
        for (const auto& e : lfw::conflicts_key_table()) {
          s += " ";
          s += to_ascii(std::u16string(e.first)) + "=" +
               (e.second == nullptr ? std::string("-") : to_ascii(std::u16string(e.second)));
        }
        emit("gktable conflicts" + s);
      } else if (what == "conflict") {
        const std::u16string key = trace::parse_js_string_literal(t[i++]);
        const char16_t* c = lfw::conflicts_key_of(key);
        emit("gktable conflict " + to_ascii(key) + " " +
             (c == nullptr ? std::string("-") : to_ascii(std::u16string(c))));
      } else {
        std::fprintf(stderr, "unknown gktable '%s' at line %d\n", what.c_str(), lineno);
        return 2;
      }
    } else if (op == "res") {
      const std::string& sub = t[i++];
      if (sub == "new") {
        const std::u16string name = trace::parse_js_string_literal(t[i++]);
        lfw::controller::ControllerResult r;
        r.set_resolver([](const lfw::Value& nf) {
          return lfw::to_string(nf) == u"none" ? lfw::Value() : nf;
        });
        g_res.insert_or_assign(name, r);
        emit("res " + to_ascii(name) + " snap " + fields_text(g_res.at(name)));
      } else {
        const std::u16string name = trace::parse_js_string_literal(t[i++]);
        lfw::controller::ControllerResult* r = find_res(name);
        if (r == nullptr) {
          std::fprintf(stderr, "unknown res '%s' at line %d\n", to_ascii(name).c_str(), lineno);
          return 2;
        }
        if (sub == "fire") {
          const lfw::Value nf = parse_value(t, i);
          const double time = trace::to_double(t[i++]);
          const std::u16string keys = trace::parse_js_string_literal(t[i++]);
          const std::u16string kind = trace::parse_js_string_literal(t[i++]);
          const bool ok = r->fire(nf, time, keys, kind);
          emit("res " + to_ascii(name) + " " + flag(ok) + " " + fields_text(*r));
          continue;
        } else if (sub == "fire2") {
          const lfw::Value val = parse_value(t, i);
          const double time = trace::to_double(t[i++]);
          const std::u16string keys = trace::parse_js_string_literal(t[i++]);
          const std::u16string kind = trace::parse_js_string_literal(t[i++]);
          const bool ok = r->fire2(val, time, keys, kind);
          emit("res " + to_ascii(name) + " " + flag(ok) + " " + fields_text(*r));
          continue;
        } else if (sub == "clear") {
          r->clear();
        } else if (sub != "snap") {
          std::fprintf(stderr, "unknown res sub '%s' at line %d\n", sub.c_str(), lineno);
          return 2;
        }
        emit("res " + to_ascii(name) + " snap " + fields_text(*r));
      }
    } else {
      std::fprintf(stderr, "unknown op '%s' at line %d\n", op.c_str(), lineno);
      return 2;
    }

    if (i != t.size()) {
      std::fprintf(stderr, "unexpected trailing token '%s' at line %d\n", t[i].c_str(), lineno);
      return 2;
    }
  }
  return 0;
}
