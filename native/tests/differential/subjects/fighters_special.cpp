#include <cstdio>
#include <fstream>
#include <string>
#include <utility>
#include <vector>

#include "lfw/core/value.h"
#include "lfw/dat_translator/fighters/fighters.h"
#include "lfw/dat_translator/make_fighter_special.h"

#include "trace_util.h"

namespace {

using trace::render_value;
using trace::split_ws;
using trace::to_ascii;

using FighterFn = lfw::Value (*)(lfw::Value&);

FighterFn find_fighter(const std::string& name) {
  static const std::pair<const char*, FighterFn> kTable[] = {
      {"bat", lfw::dat_translator::make_fighter_data_bat},
      {"davis", lfw::dat_translator::make_fighter_data_davis},
      {"deep", lfw::dat_translator::make_fighter_data_deep},
      {"dennis", lfw::dat_translator::make_fighter_data_dennis},
      {"firen", lfw::dat_translator::make_fighter_data_firen},
      {"firzen", lfw::dat_translator::make_fighter_data_firzen},
      {"freeze", lfw::dat_translator::make_fighter_data_freeze},
      {"henry", lfw::dat_translator::make_fighter_data_henry},
      {"henter", lfw::dat_translator::make_fighter_data_henter},
      {"jack", lfw::dat_translator::make_fighter_data_jack},
      {"jan", lfw::dat_translator::make_fighter_data_jan},
      {"john", lfw::dat_translator::make_fighter_data_john},
      {"julian", lfw::dat_translator::make_fighter_data_julian},
      {"justin", lfw::dat_translator::make_fighter_data_justin},
      {"knight", lfw::dat_translator::make_fighter_data_knight},
      {"louis", lfw::dat_translator::make_fighter_data_louis},
      {"louisex", lfw::dat_translator::make_fighter_data_louisex},
      {"mark", lfw::dat_translator::make_fighter_data_mark},
      {"monk", lfw::dat_translator::make_fighter_data_monk},
      {"rudolf", lfw::dat_translator::make_fighter_data_rudolf},
      {"sorcerer", lfw::dat_translator::make_fighter_data_sorcerer},
      {"template", lfw::dat_translator::make_fighter_data_template},
      {"woody", lfw::dat_translator::make_fighter_data_woody},
  };
  for (const std::pair<const char*, FighterFn>& e : kTable) {
    if (name == e.first) return e.second;
  }
  return nullptr;
}

void emit(const std::string& line) { std::printf("%s\n", line.c_str()); }

std::string render(const lfw::Value& v) { return to_ascii(render_value(v)); }

}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: lfw_trace_fighters_special <case-file>\n");
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

    if (op == "fd") {
      if (t.size() < 3) {
        std::fprintf(stderr, "missing fighter name at line %d\n", lineno);
        return 2;
      }
      FighterFn fn = find_fighter(t[1]);
      if (fn == nullptr) {
        std::fprintf(stderr, "unknown fighter '%s' at line %d\n", t[1].c_str(), lineno);
        return 2;
      }
      size_t i = 2;
      lfw::Value data = trace::parse_value(t, i);
      if (i != t.size()) {
        std::fprintf(stderr, "trailing token(s) at line %d\n", lineno);
        return 2;
      }
      emit("fd " + t[1] + " " + render(fn(data)));
      continue;
    }

    if (op == "fs") {
      size_t i = 1;
      lfw::Value data = trace::parse_value(t, i);
      if (i != t.size()) {
        std::fprintf(stderr, "trailing token(s) at line %d\n", lineno);
        return 2;
      }
      emit("fs " + render(lfw::dat_translator::make_fighter_special(data)));
      continue;
    }

    std::fprintf(stderr, "unknown op '%s' at line %d\n", op.c_str(), lineno);
    return 2;
  }
  return 0;
}
