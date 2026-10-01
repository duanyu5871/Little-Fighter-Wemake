#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

#include "lfw/core/value.h"
#include "lfw/loader/preprocess_action.h"
#include "lfw/loader/preprocess_bot_data.h"
#include "lfw/loader/preprocess_next_frame.h"

#include "trace_util.h"

namespace {

using trace::parse_value;
using trace::render_value;
using trace::split_ws;
using trace::to_ascii;

std::string render(const lfw::Value& v) { return to_ascii(render_value(v)); }

void emit(const std::string& line) { std::printf("%s\n", line.c_str()); }

}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: lfw_trace_loader_actions <case-file>\n");
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

    if (op == "bd") {
      lfw::Value data = parse_value(t, i);
      const bool ok = lfw::loader::preprocess_bot_data(data);
      emit((ok ? "bd ok " : "bd throw ") + render(data));
    } else if (op == "pa") {
      lfw::Value action = parse_value(t, i);
      const bool ok = lfw::loader::preprocess_action(action);
      emit((ok ? "pa ok " : "pa throw ") + render(action));
    } else if (op == "pnf") {
      lfw::Value nf = parse_value(t, i);
      const bool ok = lfw::loader::preprocess_next_frame(nf);
      emit((ok ? "pnf ok " : "pnf throw ") + render(nf));
    } else {
      std::fprintf(stderr, "unknown op '%s' at line %d\n", op.c_str(), lineno);
      return 2;
    }
  }
  return 0;
}
