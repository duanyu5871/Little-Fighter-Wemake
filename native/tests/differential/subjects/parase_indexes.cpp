#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

#include "lfw/core/value.h"
#include "lfw/dat_translator/parase_indexes.h"

#include "trace_util.h"

namespace {

using trace::parse_value;
using trace::render_value;
using trace::split_ws;
using trace::to_ascii;

void emit(const std::string& line) { std::printf("%s\n", line.c_str()); }

}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: lfw_trace_parase_indexes <case-file>\n");
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

    if (op == "parse") {
      const lfw::Value suffix = parse_value(t, i);
      const lfw::Value text = parse_value(t, i);
      const std::u16string suffix_str =
          std::holds_alternative<std::u16string>(suffix)
              ? std::get<std::u16string>(suffix)
              : std::u16string();
      const lfw::dat_translator::ParseIndexesResult r =
          lfw::dat_translator::parase_indexes(text, suffix_str);
      if (!r.ok) emit("parse err " + to_ascii(r.error));
      else emit("parse ok " + to_ascii(render_value(r.lists)));
    } else {
      std::fprintf(stderr, "line %d: unknown op '%s'\n", lineno, op.c_str());
      return 2;
    }
  }

  return 0;
}
