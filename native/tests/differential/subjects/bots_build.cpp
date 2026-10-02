#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

#include "lfw/core/value.h"
#include "lfw/dat_translator/bots/bot_actions.h"
#include "lfw/dat_translator/bots/bots_types.h"
#include "lfw/dat_translator/bots/frames.h"
#include "lfw/dat_translator/cond_maker.h"

#include "trace_util.h"

namespace {

using trace::parse_value;
using trace::render_value;
using trace::split_ws;
using trace::to_ascii;

namespace bots = lfw::dat_translator::bots;

lfw::Value sv(const char16_t* s) { return lfw::Value(std::u16string(s)); }

std::string render(const lfw::Value& v) { return to_ascii(render_value(v)); }

void emit(const std::string& line) { std::printf("%s\n", line.c_str()); }

const lfw::Value& at(const std::vector<lfw::Value>& a, size_t n) {
  static const lfw::Value kUndefined;
  return n < a.size() ? a[n] : kUndefined;
}

lfw::Value build(const std::string& name, const std::vector<lfw::Value>& a,
                 const bots::EditBotAction* edit) {
  auto S = [&a](size_t n) { return lfw::to_string(at(a, n)); };
  auto V = [&a](size_t n) -> const lfw::Value& { return at(a, n); };

  if (name == "bot_ball_cancelling") return bots::bot_ball_cancelling(S(0), V(1), V(2))(edit);
  if (name == "bot_ball_continuation") {
    return bots::bot_ball_continuation(S(0), V(1), V(2), V(3))(edit);
  }
  if (name == "bot_chasing_action") return bots::bot_chasing_action(S(0), V(1), V(2), V(3))(edit);
  if (name == "bot_chasing_skill_action") {
    return bots::bot_chasing_skill_action(S(0), V(1), V(2), V(3))(edit);
  }
  if (name == "bot_front_test") {
    return bots::bot_front_test(S(0), V(1), V(2), V(3), V(4), V(5), V(6))(edit);
  }
  if (name == "bot_ball_dfa") return bots::bot_ball_dfa(V(0), V(1), V(2), V(3), V(4))(edit);
  if (name == "bot_ball_dfj") return bots::bot_ball_dfj(V(0), V(1), V(2), V(3), V(4))(edit);
  if (name == "bot_explosion_dua") {
    return bots::bot_explosion_dua(V(0), V(1), V(2), V(3), V(4))(edit);
  }
  if (name == "bot_explosion_duj") {
    return bots::bot_explosion_duj(V(0), V(1), V(2), V(3), V(4))(edit);
  }
  if (name == "bot_idle_action") return bots::bot_idle_action(S(0), V(1), V(2), V(3))(edit);
  if (name == "bot_uppercut_dua") return bots::bot_uppercut_dua(V(0), V(1), V(2), V(3), V(4));
  if (name == "bot_uppercut_duj") return bots::bot_uppercut_duj(V(0), V(1), V(2), V(3));
  if (name == "bot_uppercut_dva") {
    return bots::bot_uppercut_dva(V(0), V(1), V(2), V(3), V(4))(edit);
  }
  emit("unknown " + name);
  return lfw::Value();
}

const bots::EditBotAction kEdit = [](lfw::Value& action, lfw::CondMaker& cond) -> lfw::Value {
  lfw::Object* o = lfw::as_object(action);
  if (o == nullptr) return action;
  o->set(u"action_id", sv(u"Z"));
  o->set(u"expression", lfw::Value(cond.done()));
  return action;
};

}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: lfw_trace_bots_build <case-file>\n");
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
    size_t i = 1;

    if (op == "fr") {
      emit("fr " + render(bots::frames_object()));
    } else if (op == "ba" || op == "bae") {
      const std::string name = trace::to_ascii(trace::to_u16(t[i++]));
      std::vector<lfw::Value> args;
      while (i < t.size()) args.push_back(parse_value(t, i));
      const bots::EditBotAction* edit = op == "bae" ? &kEdit : nullptr;
      emit(op + " " + name + " " + render(build(name, args, edit)));
    } else {
      std::fprintf(stderr, "unknown op '%s' at line %d\n", op.c_str(), lineno);
      return 2;
    }
  }
  return 0;
}
