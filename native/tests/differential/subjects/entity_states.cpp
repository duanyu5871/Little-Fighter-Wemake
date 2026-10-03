#include <cstdio>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include "lfw/core/value.h"
#include "lfw/state/character_state_basic.h"
#include "lfw/state/entity_states.h"
#include "lfw/state/state_names.h"

#include "trace_util.h"

namespace {

using lfw::Value;
using lfw::state::State_Base;
using lfw::state::States;
using trace::parse_value;
using trace::render_value;
using trace::split_ws;
using trace::to_ascii;

std::string render(const Value& v) { return to_ascii(render_value(v)); }

std::string join(const std::vector<std::string>& xs, const std::string& sep) {
  std::string out;
  for (size_t i = 0; i < xs.size(); ++i) {
    if (i) out += sep;
    out += xs[i];
  }
  return out;
}

std::string name_of(const State_Base* s) {
  if (s == nullptr) return "none";
  for (const States::Entry& e : lfw::state::entity_states().entries()) {
    if (e.state.get() == s) return to_ascii(e.class_name);
  }
  return "unknown";
}

std::string state_of(const State_Base* s) {
  return s == nullptr ? std::string("none") : render(s->state());
}

States& reg() { return lfw::state::entity_states(); }

}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: lfw_trace_entity_states <case-file>\n");
    return 2;
  }

  std::ifstream in(argv[1]);
  if (!in) {
    std::fprintf(stderr, "cannot open case file: %s\n", argv[1]);
    return 2;
  }

  Value type;
  double code = 0;

  std::string raw;
  int lineno = 0;
  while (std::getline(in, raw)) {
    ++lineno;
    const std::vector<std::string> t = split_ws(trace::strip_comment(raw));
    if (t.empty()) continue;
    const std::string& op = t[0];
    size_t i = 1;
    if (op == "env") {
      const std::string& sub = t[i++];
      if (sub == "type") {
        type = parse_value(t, i);
      } else if (sub == "code") {
        code = trace::to_double(t[i++]);
      } else {
        std::fprintf(stderr, "unknown env '%s' at line %d\n", sub.c_str(), lineno);
        return 2;
      }
      std::printf("env %s\n", sub.c_str());
      continue;
    }
    if (op == "run") {
      const std::string& what = t[i++];
      if (what == "size") {
        std::printf("run size || n=%zu\n", reg().size());
      } else if (what == "dump") {
        std::vector<std::string> rows;
        rows.reserve(reg().size());
        for (const States::Entry& e : reg().entries()) {
          rows.push_back("k=" + render(e.key) + " cls=" + to_ascii(e.class_name) +
                         " s=" + render(e.state->state()));
        }
        std::printf("run dump ||\n%s\n", join(rows, "\n").c_str());
      } else if (what == "head") {
        std::vector<std::string> rows;
        size_t n = 0;
        for (const States::Entry& e : reg().entries()) {
          if (++n > 2) break;
          rows.push_back("k=" + render(e.key) + " cls=" + to_ascii(e.class_name) +
                         " s=" + render(e.state->state()));
        }
        std::printf("run head ||\n%s\n", join(rows, "\n").c_str());
      } else if (what == "tail") {
        std::vector<std::string> rows;
        const std::vector<States::Entry>& all = reg().entries();
        for (size_t idx = all.size() > 2 ? all.size() - 2 : 0; idx < all.size(); ++idx) {
          rows.push_back("k=" + render(all[idx].key) + " cls=" + to_ascii(all[idx].class_name) +
                         " s=" + render(all[idx].state->state()));
        }
        std::printf("run tail ||\n%s\n", join(rows, "\n").c_str());
      } else if (what == "has") {
        const Value key = parse_value(t, i);
        std::printf("run has %s || has=%s\n", render(key).c_str(),
                    reg().has(key) ? "b1" : "b0");
      } else if (what == "get") {
        const Value key = parse_value(t, i);
        State_Base* hit = reg().get(key);
        std::printf("run get %s || r=%s s=%s\n", render(key).c_str(), name_of(hit).c_str(),
                    state_of(hit).c_str());
      } else if (what == "fallback") {
        State_Base& hit = reg().fallback(type, code);
        std::printf("run fallback || r=%s s=%s\n", name_of(&hit).c_str(),
                    state_of(&hit).c_str());
      } else if (what == "fallback2") {
        State_Base* a = &reg().fallback(type, code);
        State_Base* b = &reg().fallback(type, code);
        std::printf("run fallback2 || same=%s r=%s s=%s\n", a == b ? "b1" : "b0",
                    name_of(b).c_str(), state_of(b).c_str());
      } else if (what == "setdup") {
        reg().set(Value(0.0),
                  std::make_unique<lfw::state::CharacterState_Running>(),
                  lfw::state::state_name_u16<lfw::state::CharacterState_Running>());
        std::printf("run setdup ||\n");
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
