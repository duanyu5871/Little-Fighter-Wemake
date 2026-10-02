#include <cstdio>
#include <fstream>
#include <optional>
#include <string>
#include <vector>

#include "lfw/core/value.h"
#include "lfw/dat_translator/bots/bot_maker.h"
#include "lfw/dat_translator/bots/make_bot_data.h"

#include "trace_util.h"

namespace {

using trace::render_value;
using trace::split_ws;
using trace::to_ascii;

namespace bots = lfw::dat_translator::bots;

std::string render(const lfw::Value& v) { return to_ascii(render_value(v)); }

void emit(const std::string& line) { std::printf("%s\n", line.c_str()); }

std::optional<bots::BotMaker> make_bot(const std::string& name) {
  if (name == "bat") return bots::make_bot_data_bat();
  if (name == "davis") return bots::make_bot_data_davis();
  if (name == "firen") return bots::make_bot_data_firen();
  if (name == "firzen") return bots::make_bot_data_firzen();
  if (name == "henry") return bots::make_bot_data_henry();
  if (name == "hunter") return bots::make_bot_data_hunter();
  if (name == "jack") return bots::make_bot_data_jack();
  if (name == "jan") return bots::make_bot_data_jan();
  if (name == "julian") return bots::make_bot_data_julian();
  if (name == "justin") return bots::make_bot_data_justin();
  if (name == "knight") return bots::make_bot_data_knight();
  if (name == "louis") return bots::make_bot_data_louis();
  if (name == "louisex") return bots::make_bot_data_louisex();
  if (name == "mark") return bots::make_bot_data_mark();
  if (name == "monk") return bots::make_bot_data_monk();
  if (name == "sorcerer") return bots::make_bot_data_sorcerer();
  if (name == "woody") return bots::make_bot_data_woody();
  return std::nullopt;
}

lfw::Value field_of(const lfw::Value& v, const char16_t* key) {
  const lfw::Object* o = lfw::as_object(v);
  const lfw::Value* p = o != nullptr ? o->get(std::u16string(key)) : nullptr;
  return p != nullptr ? *p : lfw::Value();
}

}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: lfw_trace_bots_data <case-file>\n");
    return 2;
  }

  bots::register_all_bots();

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

    if (op == "reg") {
      const std::vector<std::pair<std::u16string, bots::BotMakerFactory>>& makers =
          bots::BotMaker::makers();
      std::string order;
      for (const std::pair<std::u16string, bots::BotMakerFactory>& e : makers) {
        bool listed = false;
        for (size_t k = 1; k < t.size(); ++k) {
          if (to_ascii(e.first) == t[k]) listed = true;
        }
        if (!listed) continue;
        if (!order.empty()) order += ",";
        order += to_ascii(e.first);
      }
      std::string line = "reg order=" + order;
      for (size_t k = 1; k < t.size(); ++k) {
        const std::string oid = t[k];
        bots::BotMakerFactory fn = nullptr;
        for (const std::pair<std::u16string, bots::BotMakerFactory>& e : makers) {
          if (to_ascii(e.first) == oid) fn = e.second;
        }
        if (fn == nullptr) {
          line += " " + oid + "=missing";
        } else {
          line += " " + oid + "=" + render(field_of(fn().bot(), u"id"));
        }
      }
      emit(line);
      continue;
    }

    if (t.size() < 2) {
      std::fprintf(stderr, "missing bot name at line %d\n", lineno);
      return 2;
    }
    const std::string name = t[1];
    std::optional<bots::BotMaker> m = make_bot(name);
    if (!m.has_value()) {
      std::fprintf(stderr, "unknown bot '%s' at line %d\n", name.c_str(), lineno);
      return 2;
    }

    if (op == "mb") {
      emit("mb " + name + " " + render(m->bot()));
    } else if (op == "mbf") {
      m->frames();
      emit("mbf " + name + " " + render(m->bot()));
    } else if (op == "mbs") {
      m->states();
      emit("mbs " + name + " " + render(m->bot()));
    } else if (op == "mbd") {
      emit("mbd " + name + " " + render(field_of(m->bot(), u"dataset")));
    } else if (op == "mba") {
      emit("mba " + name + " " + render(field_of(m->bot(), u"actions")));
    } else {
      std::fprintf(stderr, "unknown op '%s' at line %d\n", op.c_str(), lineno);
      return 2;
    }
  }
  return 0;
}
